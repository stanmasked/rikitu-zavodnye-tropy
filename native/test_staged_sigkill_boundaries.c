/* Stage108: actual child-process kill at two staging boundaries.
 *
 * Linux-host only. Real C99 save codecs and pair LOADERS verify recovery.
 * Forked child is stopped via SIGSTOP at (1) flushed half-written pending
 * bytes, or (2) after a successful rename() but before it returns to caller.
 * Parent SIGKILLs the stopped process, then verifies both live slots.
 *
 * This is a process-abort boundary test, NOT a physical SDMC power failure.
 * Do not link wrappers or this fixture into shipping Switch NRO.
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
#include <unistd.h>

enum { MAP=0, CAMPAIGN=1, LEVEL=2, MID_WRITE=1, AFTER_RENAME=2 };
static volatile sig_atomic_t child_hook=0;
extern size_t __real_fwrite(const void *,size_t,size_t,FILE *);
extern int __real_fflush(FILE *);
extern int __real_rename(const char *,const char *);

size_t __wrap_fwrite(const void *src,size_t size,size_t count,FILE *out){
  size_t half;
  if(child_hook!=MID_WRITE)return __real_fwrite(src,size,count,out);
  assert(size==1 && count>=32);
  half=count/2;
  assert(__real_fwrite(src,1,half,out)==half);
  assert(__real_fflush(out)==0); /* Ensure partial pending reaches host FS. */
  assert(raise(SIGSTOP)==0);     /* Parent sends SIGKILL before resumption. */
  _exit(80);                    /* Should never execute. */
}
int __wrap_rename(const char *from,const char *to){
  int rc=__real_rename(from,to);
  if(child_hook==AFTER_RENAME){
    assert(rc==0);
    assert(raise(SIGSTOP)==0);
    _exit(81);
  }
  return rc;
}
static void flower(RikituScene *s,RikituLoop *l,int phase){
  rikitu_flower_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
  if(phase){
    s->reached_checkpoints=phase;
    s->active_checkpoint=s->checkpoints[phase-1];
    s->hearts=3-phase;
    l->active_seconds=(float)(phase*20);
  }
}
static void bridge(RikituScene *s,RikituLoop *l,int phase){
  int i;
  rikitu_bridge_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
  if(phase){
    s->reached_checkpoints=phase;
    s->active_checkpoint=s->checkpoints[phase-1];
    s->hearts=3-phase;
    s->stage.coin_count=phase;
    for(i=0;i<phase;i++)s->stage.coins[i]=1;
    l->active_seconds=(float)(phase*20);
  }
}
static size_t save_size(int family){
  return family==MAP?RIKITU_SAVE_BYTES:
         family==CAMPAIGN?RIKITU_CAMPAIGN_SAVE_BYTES:
         (size_t)RIKITU_LEVEL_SAVE_BYTES;
}
static void encode_real(int family,int generation,uint8_t out[RIKITU_LEVEL_SAVE_BYTES]){
  int phase=generation==1?0:generation==2?1:2;
  RikituScene scene;
  RikituLoop loop;
  RikituCampaign c;
  switch(family){
  case MAP:
    flower(&scene,&loop,phase);
    if(generation==4)scene.hearts=2; /* Distinguish gen4 from gen3. */
    assert(rikitu_save_encode(out,&scene,&loop,(uint32_t)generation));
    break;
  case CAMPAIGN:
    assert(rikitu_campaign_init(&c,4));
    c.wallet=157+phase+(generation==4?1:0);
    assert(rikitu_campaign_save_encode(out,&c,(uint32_t)generation));
    break;
  default:
    assert(rikitu_campaign_init(&c,4));
    c.wallet=157;
    bridge(&scene,&loop,phase);
    if(generation==4)scene.hearts=2;
    assert(rikitu_level_save_encode(out,&scene,&loop,&c,(uint32_t)generation));
    break;
  }
}
static void assert_load(int family,const char *a,const char *b,int gen){
  const int phase=gen==1?0:gen==2?1:2;
  const int expected_hearts=gen==4?2:3-phase;
  RikituScene s;
  RikituLoop l;
  RikituCampaign c;
  if(family==MAP){
    flower(&s,&l,0);
    assert(rikitu_save_load_pair(a,b,&s,&l));
    assert(s.reached_checkpoints==phase && s.hearts==expected_hearts);
  }else if(family==CAMPAIGN){
    memset(&c,0,sizeof(c));
    assert(rikitu_campaign_save_load_pair(a,b,&c));
    assert(c.level==4 && c.wallet==157+phase+(gen==4?1:0));
  }else{
    assert(rikitu_campaign_init(&c,4));
    c.wallet=157;
    bridge(&s,&l,0);
    assert(rikitu_level_save_load_pair(a,b,&s,&l,&c));
    assert(s.reached_checkpoints==phase && s.hearts==expected_hearts);
    assert(s.stage.coin_count==phase);
    if(phase>0)assert(s.stage.coins[phase-1]==1);
  }
}
static void exact_file(const char *path,const uint8_t *data,size_t size){
  uint8_t got[RIKITU_LEVEL_SAVE_BYTES];
  FILE *f=fopen(path,"rb");
  assert(f && size<=sizeof(got));
  assert(fread(got,1,size,f)==size && fgetc(f)==EOF && fclose(f)==0);
  assert(memcmp(got,data,size)==0);
}
static void write_seed(const char *path,const uint8_t *data,size_t size){
  FILE *f=fopen(path,"wb");
  assert(f);
  assert(fwrite(data,1,size,f)==size && fflush(f)==0 && fclose(f)==0);
}
static void pending_path(char out[240],const char *target){
  int n=snprintf(out,240,"%s.pending",target);
  assert(n>0 && n<240);
}
static void run_case(int family,int first_b,int boundary){
  const char *extension=family==MAP?"rks":family==CAMPAIGN?"rkc":"rkl";
  char a[240],b[240],tmp[240];
  const char *target,*backup;
  uint8_t v1[RIKITU_LEVEL_SAVE_BYTES],v2[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t v3[RIKITU_LEVEL_SAVE_BYTES],v4[RIKITU_LEVEL_SAVE_BYTES];
  size_t n=save_size(family);
  pid_t pid;
  int status=0;
  struct stat st;
  assert(snprintf(a,sizeof(a),"/tmp/rikitu-stage108-%s-a.%s",
                  extension,extension)>0);
  assert(snprintf(b,sizeof(b),"/tmp/rikitu-stage108-%s-b.%s",
                  extension,extension)>0);
  backup=first_b?b:a;
  target=first_b?a:b;
  pending_path(tmp,target);
  (void)remove(a);(void)remove(b);(void)remove(tmp);
  encode_real(family,1,v1);
  encode_real(family,2,v2);
  encode_real(family,3,v3);
  encode_real(family,4,v4);
  /* Both live slots already exist. Target starts with the older gen1. */
  write_seed(target,v1,n);
  write_seed(backup,v2,n);
  assert_load(family,a,b,2);
  pid=fork();
  assert(pid>=0);
  if(pid==0){
    child_hook=(sig_atomic_t)boundary;
    /* Must hit wrapper and stop. Return to parent is NEVER expected. */
    (void)rikitu_staged_slot_commit(target,v3,n);
    _exit(82);
  }
  assert(waitpid(pid,&status,WUNTRACED)==pid);
  assert(WIFSTOPPED(status) && WSTOPSIG(status)==SIGSTOP);
  assert(kill(pid,SIGKILL)==0);
  assert(waitpid(pid,&status,0)==pid);
  assert(WIFSIGNALED(status) && WTERMSIG(status)==SIGKILL);
  exact_file(backup,v2,n);
  if(boundary==MID_WRITE){
    exact_file(target,v1,n);
    assert_load(family,a,b,2);
    assert(stat(tmp,&st)==0 && S_ISREG(st.st_mode));
    assert(st.st_size==(off_t)(n/2));
    /* Restart ignores an abandoned half-record, then overwrites it. */
    assert(rikitu_staged_slot_commit(target,v3,n));
    exact_file(target,v3,n);
    exact_file(backup,v2,n);
    assert_load(family,a,b,3);
  }else{
    /* rename has already succeeded, although the process died before
     * observing its return value. Both live slots must be loadable. */
    assert(stat(tmp,&st)!=0 && errno==ENOENT);
    exact_file(target,v3,n);
    assert_load(family,a,b,3);
    assert(rikitu_staged_slot_commit(backup,v4,n));
    exact_file(target,v3,n);
    exact_file(backup,v4,n);
    assert_load(family,a,b,4);
  }
  assert(stat(tmp,&st)!=0 && errno==ENOENT);
  (void)remove(a);(void)remove(b);
  printf("PASS Stage108 %s old=%c crash=%s: killed child, both live saves survive\n",
         extension,first_b?'B':'A',
         boundary==MID_WRITE?"pending-half":"post-rename");
}
int main(void){
  int family,b;
  for(family=MAP;family<=LEVEL;family++)
    for(b=0;b<2;b++){
      run_case(family,b,MID_WRITE);
      run_case(family,b,AFTER_RENAME);
    }
  puts("PASS Stage108 twelve SIGKILL staged-save process boundary cases");
  return 0;
}
