/* Stage111 — two ACTUAL Linux processes racing to stage the same target.
 *
 * The first child stops inside the real pending fwrite after flushing half
 * its record while the persistent per-target flock is held. The second
 * child attempts a distinct generation using the SAME pending path.
 * While the first remains stopped, writer two may not finish or disturb
 * its half-written pending file. After either SIGCONT or SIGKILL of the
 * first child, the second must commit a complete valid gen4 record.
 *
 * Uses unchanged production RKS1/RKC2/RKL3 encoders and pair LOADERS.
 * Linux host only. This is not a physical Switch/FAT concurrency proof.
 */
#define _POSIX_C_SOURCE 200809L
#include "rikitu_flower.h"
#include "rikitu_bridge.h"
#include "rikitu_save.h"
#include "rikitu_campaign_save.h"
#include "rikitu_level_save.h"
#include "rikitu_staged_slot.h"
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

enum { MAP=0,CAMPAIGN=1,LEVEL=2,COMPLETE=0,KILL_FIRST=1 };
static volatile sig_atomic_t pause_first_writer=0;
extern size_t __real_fwrite(const void *,size_t,size_t,FILE *);

size_t __wrap_fwrite(const void *data,size_t size,size_t count,FILE *f){
  size_t half;
  if(!pause_first_writer)return __real_fwrite(data,size,count,f);
  assert(size==1 && count>=32);
  pause_first_writer=0;
  half=count/2;
  assert(__real_fwrite(data,1,half,f)==half);
  assert(fflush(f)==0);
  assert(raise(SIGSTOP)==0);
  assert(__real_fwrite((const uint8_t *)data+half,1,count-half,f)==count-half);
  return count;
}
static size_t record_size(int family){
  return family==MAP?RIKITU_SAVE_BYTES:
         family==CAMPAIGN?RIKITU_CAMPAIGN_SAVE_BYTES:
         (size_t)RIKITU_LEVEL_SAVE_BYTES;
}
static void scene_state(int family,int generation,RikituScene *s,RikituLoop *l){
  int phase=generation==1?0:generation==2?1:2,i;
  if(family==MAP)rikitu_flower_init(s);
  else rikitu_bridge_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
  if(phase){
    s->reached_checkpoints=phase;
    s->active_checkpoint=s->checkpoints[phase-1];
    s->hearts=generation==4?2:3-phase;
    l->active_seconds=(float)(generation*20);
    if(family==LEVEL){
      s->stage.coin_count=phase;
      for(i=0;i<phase;i++)s->stage.coins[i]=1;
    }
  }
}
static void encode(int family,int generation,uint8_t out[RIKITU_LEVEL_SAVE_BYTES]){
  RikituScene s;
  RikituLoop l;
  RikituCampaign c;
  int phase=generation==1?0:generation==2?1:2;
  assert(rikitu_campaign_init(&c,4));
  if(family==CAMPAIGN){
    c.wallet=157+phase+(generation==4?1:0);
    assert(rikitu_campaign_save_encode(out,&c,(uint32_t)generation));
  }else{
    scene_state(family,generation,&s,&l);
    c.wallet=157;
    if(family==MAP)
      assert(rikitu_save_encode(out,&s,&l,(uint32_t)generation));
    else
      assert(rikitu_level_save_encode(out,&s,&l,&c,(uint32_t)generation));
  }
}
static void seed(const char *path,const uint8_t *bytes,size_t n){
  FILE *f=fopen(path,"wb");
  assert(f);
  assert(fwrite(bytes,1,n,f)==n && fflush(f)==0);
  assert(fclose(f)==0);
}
static void assert_file(const char *path,const uint8_t *bytes,size_t n){
  uint8_t data[RIKITU_LEVEL_SAVE_BYTES];
  FILE *f=fopen(path,"rb");
  assert(f && n<=sizeof(data));
  assert(fread(data,1,n,f)==n && fgetc(f)==EOF);
  assert(fclose(f)==0);
  assert(memcmp(bytes,data,n)==0);
}
static void assert_partial(const char *path,const uint8_t *bytes,size_t n){
  uint8_t data[RIKITU_LEVEL_SAVE_BYTES];
  struct stat st;
  FILE *f=fopen(path,"rb");
  assert(stat(path,&st)==0 && st.st_size==(off_t)(n/2));
  assert(f);
  assert(fread(data,1,n/2,f)==n/2 && fgetc(f)==EOF);
  assert(fclose(f)==0);
  assert(memcmp(bytes,data,n/2)==0);
}
static void assert_loader(int family,const char *a,const char *b){
  RikituScene s;
  RikituLoop l;
  RikituCampaign c;
  if(family==CAMPAIGN){
    memset(&c,0,sizeof(c));
    assert(rikitu_campaign_save_load_pair(a,b,&c));
    assert(c.level==4 && c.wallet==160);
  }else{
    scene_state(family,1,&s,&l);
    if(family==MAP){
      assert(rikitu_save_load_pair(a,b,&s,&l));
    }else{
      assert(rikitu_campaign_init(&c,4));
      c.wallet=157;
      assert(rikitu_level_save_load_pair(a,b,&s,&l,&c));
      assert(s.stage.coin_count==2 && s.stage.coins[1]==1);
    }
    assert(s.reached_checkpoints==2 && s.hearts==2);
  }
}
static void wait_stopped(pid_t pid){
  int status=0;
  assert(waitpid(pid,&status,WUNTRACED)==pid);
  assert(WIFSTOPPED(status) && WSTOPSIG(status)==SIGSTOP);
}
static void wait_exit(pid_t pid,int must_die){
  int status=0;
  assert(waitpid(pid,&status,0)==pid);
  if(must_die){
    assert(WIFSIGNALED(status) && WTERMSIG(status)==SIGKILL);
  }else{
    assert(WIFEXITED(status) && WEXITSTATUS(status)==0);
  }
}
static void run_case(int family,int backup_b,int interrupt){
  const char *ext=family==MAP?"rks":family==CAMPAIGN?"rkc":"rkl";
  char a[240],b[240],pending[260],lock[260];
  const char *backup,*target;
  uint8_t gen1[RIKITU_LEVEL_SAVE_BYTES],gen2[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t gen3[RIKITU_LEVEL_SAVE_BYTES],gen4[RIKITU_LEVEL_SAVE_BYTES];
  size_t n=record_size(family);
  pid_t first,second;
  struct timespec delay={0,150000000L};
  struct stat st;
  int pipefd[2],status=0;
  char marker='X';
  assert(snprintf(a,sizeof(a),"/tmp/rikitu-stage111-%s-a.%s",ext,ext)>0);
  assert(snprintf(b,sizeof(b),"/tmp/rikitu-stage111-%s-b.%s",ext,ext)>0);
  backup=backup_b?b:a;
  target=backup_b?a:b;
  assert(snprintf(pending,sizeof(pending),"%s.pending",target)>0);
  assert(snprintf(lock,sizeof(lock),"%s.pending.lock",target)>0);
  (void)remove(a);(void)remove(b);
  (void)remove(pending);(void)remove(lock);
  encode(family,1,gen1);encode(family,2,gen2);
  encode(family,3,gen3);encode(family,4,gen4);
  seed(target,gen1,n);
  seed(backup,gen2,n);
  assert(pipe(pipefd)==0);
  first=fork();
  assert(first>=0);
  if(first==0){
    (void)close(pipefd[0]);(void)close(pipefd[1]);
    pause_first_writer=1;
    _exit(rikitu_staged_slot_commit(target,gen3,n)?0:21);
  }
  wait_stopped(first);
  assert_partial(pending,gen3,n);
  second=fork();
  assert(second>=0);
  if(second==0){
    (void)close(pipefd[0]);
    assert(write(pipefd[1],&marker,1)==1);
    (void)close(pipefd[1]);
    _exit(rikitu_staged_slot_commit(target,gen4,n)?0:22);
  }
  (void)close(pipefd[1]);
  assert(read(pipefd[0],&marker,1)==1 && marker=='X');
  (void)close(pipefd[0]);
  assert(nanosleep(&delay,NULL)==0);
  /* Second process must be blocked on the live target's flock. */
  assert(waitpid(second,&status,WNOHANG)==0);
  assert_partial(pending,gen3,n);
  assert_file(backup,gen2,n);
  assert_file(target,gen1,n);
  if(interrupt){
    assert(kill(first,SIGKILL)==0);
    wait_exit(first,1);
  }else{
    assert(kill(first,SIGCONT)==0);
    wait_exit(first,0);
  }
  wait_exit(second,0);
  assert(stat(pending,&st)!=0 && errno==ENOENT);
  assert(stat(lock,&st)==0 && S_ISREG(st.st_mode));
  assert_file(backup,gen2,n);
  assert_file(target,gen4,n);
  assert_loader(family,a,b);
  (void)remove(a);(void)remove(b);
  (void)remove(lock);
  printf("PASS Stage111 %s backup=%c first=%s: gen4 valid, gen2 preserved\n",
         ext,backup_b?'B':'A',interrupt?"SIGKILL":"resumed");
}
int main(void){
  int family,b,k;
  for(family=MAP;family<=LEVEL;family++)
    for(b=0;b<2;b++)
      for(k=COMPLETE;k<=KILL_FIRST;k++)
        run_case(family,b,k);
  puts("PASS Stage111: 12 two-process pending-lock contention cases");
  return 0;
}
