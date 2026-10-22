/* Stage112: real two-process A/B pair election under a single Linux flock.
 *
 * Starting from two CRC-valid saves (gen1 and gen2), writer one pauses in
 * its encoder AFTER pair lock acquisition but BEFORE the staged commit.
 * Writer two starts with reversed A/B path arguments and must not enter
 * its own encoder or change EITHER slot while writer one is stopped.
 *
 * When first resumes: commits gen3; second re-reads and commits gen4
 * in the OTHER slot, preserving gen3 as backup.
 * When first is SIGKILLed: second commits gen3, preserving original gen2.
 *
 * Both cases for three real save codecs and both initial A/B orientations.
 * This checks cooperative Linux processes only; NOT physical Switch/FAT.
 */
#define _POSIX_C_SOURCE 200809L
#include "rikitu_staged_pair.h"
#include "rikitu_save.h"
#include "rikitu_campaign_save.h"
#include "rikitu_level_save.h"
#include "rikitu_flower.h"
#include "rikitu_bridge.h"
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

enum { MAP=0, CAMPAIGN=1, LEVEL=2, RESUME=0, KILL=1 };
typedef struct {
  int family;
  int writer;  /* 0 initial, 1 first, 2 second. */
  int pause;
  const char *entered_marker;
  RikituCampaign campaign;
} Context;
static size_t bytes_of(int family){
  return family==MAP?RIKITU_SAVE_BYTES:
    family==CAMPAIGN?RIKITU_CAMPAIGN_SAVE_BYTES:RIKITU_LEVEL_SAVE_BYTES;
}
static void scene_for(int family,int kind,RikituScene *s,RikituLoop *l){
  int i,phase=kind==0?0:kind==1?1:2;
  if(family==MAP)rikitu_flower_init(s);
  else rikitu_bridge_init(s);
  memset(l,0,sizeof(*l));l->player=s->player;
  if(phase){
    s->reached_checkpoints=phase;
    s->active_checkpoint=s->checkpoints[phase-1];
    s->hearts=kind==4?2:3-phase;
    l->active_seconds=(float)(kind*20);
    if(family==LEVEL){
      s->stage.coin_count=phase;
      for(i=0;i<phase;i++)s->stage.coins[i]=1;
    }
  }
}
static int codec_encode(uint8_t *out,size_t n,uint32_t gen,void *opaque){
  Context *ctx=(Context*)opaque;
  RikituScene s;
  RikituLoop l;
  RikituCampaign c;
  int kind=ctx->writer==1?3:ctx->writer==2?4:ctx->writer;
  int phase=kind==0?0:kind==1?1:2;
  assert(n==bytes_of(ctx->family));
  if(ctx->entered_marker){
    FILE *f=fopen(ctx->entered_marker,"wb");
    assert(f&&fclose(f)==0);
  }
  if(ctx->pause){
    assert(raise(SIGSTOP)==0);
    ctx->pause=0;
  }
  c=ctx->campaign;
  if(ctx->family==CAMPAIGN){
    c.wallet=157+phase+(kind==4?1:0);
    return rikitu_campaign_save_encode(out,&c,gen);
  }
  scene_for(ctx->family,kind,&s,&l);
  if(ctx->family==MAP)return rikitu_save_encode(out,&s,&l,gen);
  c.wallet=157;
  return rikitu_level_save_encode(out,&s,&l,&c,gen);
}
static int codec_valid(const uint8_t *data,size_t n,void *opaque,uint32_t *gen){
  Context *ctx=(Context*)opaque;
  RikituScene s;
  RikituLoop l;
  RikituCampaign c=ctx->campaign;
  if(ctx->family==MAP){
    scene_for(MAP,0,&s,&l);
    return rikitu_save_decode(&s,&l,data,n,gen);
  }
  if(ctx->family==CAMPAIGN)
    return rikitu_campaign_save_decode(&c,data,n,gen);
  scene_for(LEVEL,0,&s,&l);
  return rikitu_level_save_decode(&s,&l,&c,data,n,gen);
}
static void write_record(const char *path,const uint8_t *bytes,size_t n){
  FILE *f=fopen(path,"wb");
  assert(f);
  assert(fwrite(bytes,1,n,f)==n&&fflush(f)==0);
  assert(fclose(f)==0);
}
static void assert_file(const char *path,const uint8_t *bytes,size_t n){
  uint8_t got[RIKITU_LEVEL_SAVE_BYTES];
  FILE *f=fopen(path,"rb");
  assert(f&&n<=sizeof(got));
  assert(fread(got,1,n,f)==n&&fgetc(f)==EOF);
  assert(fclose(f)==0);
  assert(memcmp(got,bytes,n)==0);
}
static void assert_loaded_second(int family,const char *a,const char *b,
                                 Context *ctx){
  RikituScene s;
  RikituLoop l;
  RikituCampaign c=ctx->campaign;
  if(family==MAP){
    scene_for(MAP,0,&s,&l);
    assert(rikitu_save_load_pair(a,b,&s,&l));
    assert(s.reached_checkpoints==2&&s.hearts==2);
  }else if(family==CAMPAIGN){
    assert(rikitu_campaign_save_load_pair(a,b,&c));
    assert(c.wallet==160&&c.level==4);
  }else{
    scene_for(LEVEL,0,&s,&l);
    assert(rikitu_level_save_load_pair(a,b,&s,&l,&c));
    assert(s.reached_checkpoints==2&&s.hearts==2);
    assert(s.stage.coin_count==2&&s.stage.coins[1]==1);
  }
}
static void wait_child(pid_t pid,int killed){
  int status=0;
  assert(waitpid(pid,&status,0)==pid);
  if(killed)
    assert(WIFSIGNALED(status)&&WTERMSIG(status)==SIGKILL);
  else
    assert(WIFEXITED(status)&&WEXITSTATUS(status)==0);
}
static void cleanup(const char *path){
  char buf[260];
  int len;
  (void)remove(path);
  len=snprintf(buf,sizeof(buf),"%s.pending",path);
  assert(len>0&&len<(int)sizeof(buf));(void)remove(buf);
  len=snprintf(buf,sizeof(buf),"%s.pending.lock",path);
  assert(len>0&&len<(int)sizeof(buf));(void)remove(buf);
  len=snprintf(buf,sizeof(buf),"%s.pair.lock",path);
  assert(len>0&&len<(int)sizeof(buf));(void)remove(buf);
}
static void run_case(int family,int newer_b,int killed){
  char a[200],b[200],marker[240],lock[260];
  const char *ext=family==MAP?"rks":family==CAMPAIGN?"rkc":"rkl";
  const char *older,*backup;
  uint8_t initial1[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t initial2[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t first3[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t second3[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t second4[RIKITU_LEVEL_SAVE_BYTES];
  size_t n=bytes_of(family);
  pid_t first,second;
  int pipefd[2],status=0,nchars;
  char ready='R';
  struct timespec delay={0,160000000L};
  struct stat st;
  Context ctx;
  const char *minpath;
  memset(&ctx,0,sizeof(ctx));
  ctx.family=family;
  assert(rikitu_campaign_init(&ctx.campaign,4));
  ctx.campaign.wallet=157;
  nchars=snprintf(a,sizeof(a),"/tmp/rikitu-stage112-%s-a.%s",ext,ext);
  assert(nchars>0&&nchars<(int)sizeof(a));
  nchars=snprintf(b,sizeof(b),"/tmp/rikitu-stage112-%s-b.%s",ext,ext);
  assert(nchars>0&&nchars<(int)sizeof(b));
  nchars=snprintf(marker,sizeof(marker),"/tmp/rikitu-stage112-%s-encoder.marker",ext);
  assert(nchars>0&&nchars<(int)sizeof(marker));
  cleanup(a);cleanup(b);(void)remove(marker);
  older=newer_b?a:b;
  backup=newer_b?b:a;
  ctx.writer=0;assert(codec_encode(initial1,n,1,&ctx));
  ctx.writer=1;assert(codec_encode(initial2,n,2,&ctx));
  ctx.writer=1;assert(codec_encode(first3,n,3,&ctx));
  ctx.writer=2;assert(codec_encode(second3,n,3,&ctx));
  ctx.writer=2;assert(codec_encode(second4,n,4,&ctx));
  write_record(older,initial1,n);
  write_record(backup,initial2,n);
  assert(pipe(pipefd)==0);
  first=fork();assert(first>=0);
  if(first==0){
    (void)close(pipefd[0]);(void)close(pipefd[1]);
    ctx.writer=1;ctx.pause=1;ctx.entered_marker=NULL;
    _exit(rikitu_staged_pair_commit(a,b,n,codec_valid,codec_encode,
                                    &ctx,NULL)?0:21);
  }
  assert(waitpid(first,&status,WUNTRACED)==first);
  assert(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGSTOP);
  /* Pair lock acquired; both existing slots remain untouched. */
  assert_file(older,initial1,n);
  assert_file(backup,initial2,n);
  second=fork();assert(second>=0);
  if(second==0){
    (void)close(pipefd[0]);
    assert(write(pipefd[1],&ready,1)==1);
    (void)close(pipefd[1]);
    ctx.writer=2;ctx.pause=0;ctx.entered_marker=marker;
    /* Reverse arguments: lock name must still be lexically canonical. */
    _exit(rikitu_staged_pair_commit(b,a,n,codec_valid,codec_encode,
                                    &ctx,NULL)?0:22);
  }
  (void)close(pipefd[1]);
  assert(read(pipefd[0],&ready,1)==1&&ready=='R');
  (void)close(pipefd[0]);
  assert(nanosleep(&delay,NULL)==0);
  assert(waitpid(second,&status,WNOHANG)==0);
  assert(stat(marker,&st)!=0&&errno==ENOENT); /* cannot reach encoder */
  assert_file(older,initial1,n);
  assert_file(backup,initial2,n);
  if(killed){
    assert(kill(first,SIGKILL)==0);
    wait_child(first,1);
  }else{
    assert(kill(first,SIGCONT)==0);
    wait_child(first,0);
    assert_file(older,first3,n);
    assert_file(backup,initial2,n);
  }
  wait_child(second,0);
  assert(stat(marker,&st)==0);
  if(killed){
    assert_file(older,second3,n); /* chose gen3 after kill */
    assert_file(backup,initial2,n);
  }else{
    assert_file(older,first3,n);
    assert_file(backup,second4,n); /* re-read gen3, chose gen4 */
  }
  ctx.writer=2;ctx.entered_marker=NULL;
  assert_loaded_second(family,a,b,&ctx);
  minpath=strcmp(a,b)<0?a:b;
  nchars=snprintf(lock,sizeof(lock),"%s.pair.lock",minpath);
  assert(nchars>0&&nchars<(int)sizeof(lock));
  assert(stat(lock,&st)==0&&S_ISREG(st.st_mode));
  (void)remove(marker);
  cleanup(a);cleanup(b);
  printf("PASS Stage112 %s newer=%c first=%s: pair election serialized\n",
         ext,newer_b?'B':'A',killed?"SIGKILL":"resume");
}
int main(void){
  int f,b,k;
  for(f=MAP;f<=LEVEL;f++)
    for(b=0;b<2;b++)
      for(k=RESUME;k<=KILL;k++)
        run_case(f,b,k);
  puts("PASS Stage112 twelve competing-process paired save elections");
  return 0;
}
