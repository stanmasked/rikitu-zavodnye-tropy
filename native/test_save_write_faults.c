/* Stage103: deterministic partial fwrite and failing fflush on the REAL
 * RKS1 / RKC2 / RKL3 pair writers, exercising both A/B directions.
 *
 * This test uses GNU/ELF --wrap=fwrite and --wrap=fflush in the host CI
 * executable ONLY. The shipping codecs/writers are linked without edits.
 * Every injected write creates a truncated target record: both the short
 * fwrite path and the fflush-error path must preserve the intact backup.
 * Then an unmodified retry must recover generation 2 and the new progress.
 *
 * A flush error is emulated AFTER a real fflush and deterministic truncation:
 * it exercises writer error handling and recovery, not the hardware timing
 * of an SD card or a real OS writeback failure.
 */
#define _POSIX_C_SOURCE 200809L
#include "rikitu_save.h"
#include "rikitu_campaign_save.h"
#include "rikitu_level_save.h"
#include "rikitu_flower.h"
#include "rikitu_bridge.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define S_A "/tmp/rikitu-stage103-map-a.rks"
#define S_B "/tmp/rikitu-stage103-map-b.rks"
#define C_A "/tmp/rikitu-stage103-campaign-a.rkc"
#define C_B "/tmp/rikitu-stage103-campaign-b.rkc"
#define L_A "/tmp/rikitu-stage103-level-a.rkl"
#define L_B "/tmp/rikitu-stage103-level-b.rkl"

typedef enum { F_NONE=0, F_SHORT=1, F_FLUSH=2 } Fault;
static Fault fault=F_NONE;
static unsigned injections=0;
static size_t cut_bytes=0;

/* Wrap precisely the libc calls originating from the pair writers.
 * Unwrapped __real_* symbols still invoke the actual C library. */
extern size_t __real_fwrite(const void *,size_t,size_t,FILE *);
extern int __real_fflush(FILE *);

size_t __wrap_fwrite(const void *data,size_t size,size_t count,FILE *stream){
  if(fault!=F_SHORT)return __real_fwrite(data,size,count,stream);
  assert(++injections==1);
  assert(size==1 && count>=2);
  assert(cut_bytes==count/2);
  assert(__real_fwrite(data,1,cut_bytes,stream)==cut_bytes);
  return cut_bytes; /* A POSIX short write as observed by the real writer. */
}
int __wrap_fflush(FILE *stream){
  int fd;
  if(fault!=F_FLUSH)return __real_fflush(stream);
  assert(++injections==1);
  assert(stream && cut_bytes>0);
  assert(__real_fflush(stream)==0);
  fd=fileno(stream);
  assert(fd>=0 && ftruncate(fd,(off_t)cut_bytes)==0);
  errno=EIO;
  return EOF; /* Deterministic mocked flush error after partial persistence. */
}
static void begin_fault(Fault f,size_t bytes){
  assert(f==F_SHORT || f==F_FLUSH);
  assert(fault==F_NONE);
  fault=f;injections=0;cut_bytes=bytes/2;
}
static void end_fault(void){
  assert(injections==1);
  fault=F_NONE;
  cut_bytes=0;
}
static void reset_pair(const char *a,const char *b){
  assert(fault==F_NONE);
  (void)remove(a);
  (void)remove(b);
}
static void pair_paths(const char *a,const char *b,int backup_b,
                       const char **backup,const char **target){
  *backup=backup_b?b:a;
  *target=backup_b?a:b;
  if(backup_b)assert(rename(a,b)==0);
}
static void read_exact(const char *p,uint8_t *data,size_t size){
  FILE *f=fopen(p,"rb");
  assert(f);
  assert(fread(data,1,size,f)==size && fgetc(f)==EOF);
  assert(fclose(f)==0);
}
static void unchanged(const char *p,const uint8_t *original,size_t size){
  uint8_t data[RIKITU_LEVEL_SAVE_BYTES];
  assert(size<=sizeof(data));
  read_exact(p,data,size);
  assert(memcmp(data,original,size)==0);
}
static void assert_truncated(const char *p,size_t full_size){
  struct stat st;
  assert(stat(p,&st)==0);
  assert(S_ISREG(st.st_mode));
  assert(st.st_size==(off_t)(full_size/2));
}
static void flower(RikituScene *s,RikituLoop *l){
  rikitu_flower_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
static void bridge(RikituScene *s,RikituLoop *l){
  rikitu_bridge_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
static void test_rks(Fault mode,int backup_b){
  RikituScene scene,restored;
  RikituLoop loop,loaded;
  uint8_t original[RIKITU_SAVE_BYTES],next[RIKITU_SAVE_BYTES];
  const char *backup,*target;
  uint32_t generation=0;
  reset_pair(S_A,S_B);
  flower(&scene,&loop);
  assert(rikitu_save_store_pair(S_A,S_B,&scene,&loop));
  pair_paths(S_A,S_B,backup_b,&backup,&target);
  read_exact(backup,original,sizeof(original));
  scene.active_checkpoint=scene.checkpoints[0];
  scene.reached_checkpoints=1;
  scene.hearts=2;
  loop.active_seconds=20.0f;
  begin_fault(mode,sizeof(original));
  assert(!rikitu_save_store_pair(S_A,S_B,&scene,&loop));
  end_fault();
  unchanged(backup,original,sizeof(original));
  assert_truncated(target,sizeof(original));
  flower(&restored,&loaded);
  assert(rikitu_save_load_pair(S_A,S_B,&restored,&loaded));
  assert(restored.reached_checkpoints==0 && restored.hearts==3);
  assert(rikitu_save_store_pair(S_A,S_B,&scene,&loop));
  unchanged(backup,original,sizeof(original));
  read_exact(target,next,sizeof(next));
  assert(rikitu_save_decode(&restored,&loaded,next,sizeof(next),&generation));
  assert(generation==2);
  flower(&restored,&loaded);
  assert(rikitu_save_load_pair(S_A,S_B,&restored,&loaded));
  assert(restored.hearts==2 && restored.reached_checkpoints==1);
  assert(restored.active_checkpoint==scene.active_checkpoint);
  reset_pair(S_A,S_B);
}
static void test_rkc(Fault mode,int backup_b){
  RikituCampaign campaign,restored;
  uint8_t original[RIKITU_CAMPAIGN_SAVE_BYTES];
  uint8_t next[RIKITU_CAMPAIGN_SAVE_BYTES];
  const char *backup,*target;
  uint32_t generation=0;
  reset_pair(C_A,C_B);
  assert(rikitu_campaign_init(&campaign,4));
  campaign.wallet=157;
  assert(rikitu_campaign_save_store_pair(C_A,C_B,&campaign));
  pair_paths(C_A,C_B,backup_b,&backup,&target);
  read_exact(backup,original,sizeof(original));
  campaign.wallet=158;
  begin_fault(mode,sizeof(original));
  assert(!rikitu_campaign_save_store_pair(C_A,C_B,&campaign));
  end_fault();
  unchanged(backup,original,sizeof(original));
  assert_truncated(target,sizeof(original));
  memset(&restored,0,sizeof(restored));
  assert(rikitu_campaign_save_load_pair(C_A,C_B,&restored));
  assert(restored.wallet==157 && restored.level==4);
  assert(rikitu_campaign_save_store_pair(C_A,C_B,&campaign));
  unchanged(backup,original,sizeof(original));
  read_exact(target,next,sizeof(next));
  assert(rikitu_campaign_save_decode(&restored,next,sizeof(next),&generation));
  assert(generation==2);
  memset(&restored,0,sizeof(restored));
  assert(rikitu_campaign_save_load_pair(C_A,C_B,&restored));
  assert(restored.wallet==158 && restored.level==4);
  reset_pair(C_A,C_B);
}
static void test_rkl(Fault mode,int backup_b){
  RikituCampaign campaign;
  RikituScene scene,restored;
  RikituLoop loop,loaded;
  uint8_t original[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t next[RIKITU_LEVEL_SAVE_BYTES];
  const char *backup,*target;
  uint32_t generation=0;
  reset_pair(L_A,L_B);
  assert(rikitu_campaign_init(&campaign,4));
  campaign.wallet=157;
  bridge(&scene,&loop);
  assert(rikitu_level_save_store_pair(L_A,L_B,&scene,&loop,&campaign));
  pair_paths(L_A,L_B,backup_b,&backup,&target);
  read_exact(backup,original,sizeof(original));
  scene.active_checkpoint=scene.checkpoints[0];
  scene.reached_checkpoints=1;
  scene.hearts=2;
  scene.stage.coins[0]=1;
  scene.stage.coin_count=1;
  loop.active_seconds=20.0f;
  begin_fault(mode,sizeof(original));
  assert(!rikitu_level_save_store_pair(L_A,L_B,&scene,&loop,&campaign));
  end_fault();
  unchanged(backup,original,sizeof(original));
  assert_truncated(target,sizeof(original));
  bridge(&restored,&loaded);
  assert(rikitu_level_save_load_pair(L_A,L_B,&restored,&loaded,&campaign));
  assert(restored.reached_checkpoints==0 && restored.stage.coin_count==0);
  assert(restored.hearts==3);
  assert(rikitu_level_save_store_pair(L_A,L_B,&scene,&loop,&campaign));
  unchanged(backup,original,sizeof(original));
  read_exact(target,next,sizeof(next));
  assert(rikitu_level_save_inspect(next,sizeof(next),&generation));
  assert(generation==2);
  bridge(&restored,&loaded);
  assert(rikitu_level_save_load_pair(L_A,L_B,&restored,&loaded,&campaign));
  assert(restored.reached_checkpoints==1 && restored.hearts==2);
  assert(restored.stage.coin_count==1 && restored.stage.coins[0]==1);
  reset_pair(L_A,L_B);
}
int main(void){
  int backup_b;
  Fault f;
  for(f=F_SHORT;f<=F_FLUSH;f=(Fault)(f+1)){
    for(backup_b=0;backup_b<=1;backup_b++){
      test_rks(f,backup_b);
      test_rkc(f,backup_b);
      test_rkl(f,backup_b);
      printf("PASS Stage103 %s backup=%c: RKS1 RKC2 RKL3 fallback and retry\n",
             f==F_SHORT?"short fwrite":"failing fflush",
             backup_b?'B':'A');
    }
  }
  puts("PASS Stage103 12 injection/recovery cases; all backups byte-identical");
  return 0;
}
