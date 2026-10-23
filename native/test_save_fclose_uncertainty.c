/* Stage104 — ambiguous fclose after valid fwrite/fflush.
 *
 * GNU/Linux host-only --wrap=fclose exercises REAL production pair writers.
 * Both paths return EOF/EIO to caller after the underlying close succeeds:
 *  (a) torn: the temporary .pending record is truncated before close;
 *  (b) complete: the temporary record has complete CRC-valid bytes.
 * In both cases the staged writer must reject the ambiguous close,
 * remove .pending, leave both live slots untouched and allow retry.
 * A complete yet uncommitted temporary file is NOT a valid live gen2 slot.
 *
 * This reveals an important limit: return of EIO does not prove durable
 * storage; successful CRC readback cannot prove a real SD flush/power cut.
 */
#define _POSIX_C_SOURCE 200809L
#include "rikitu_save.h"
#include "rikitu_campaign_save.h"
#include "rikitu_level_save.h"
#include "rikitu_flower.h"
#include "rikitu_bridge.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define S_A "/tmp/rikitu-stage104-map-a.rks"
#define S_B "/tmp/rikitu-stage104-map-b.rks"
#define C_A "/tmp/rikitu-stage104-campaign-a.rkc"
#define C_B "/tmp/rikitu-stage104-campaign-b.rkc"
#define L_A "/tmp/rikitu-stage104-level-a.rkl"
#define L_B "/tmp/rikitu-stage104-level-b.rkl"

typedef enum { CLOSE_NONE=0, CLOSE_TORN=1, CLOSE_COMPLETE=2 } CloseFault;
static CloseFault fault=CLOSE_NONE;
static unsigned injected=0;
static size_t expected_size=0;
extern int __real_fclose(FILE *);

int __wrap_fclose(FILE *stream){
  int fd,flags,rc;
  if(fault==CLOSE_NONE || !stream)return __real_fclose(stream);
  fd=fileno(stream);
  assert(fd>=0);
  flags=fcntl(fd,F_GETFL);
  assert(flags>=0);
  /* Writer's read_slot() closes must NOT count as injected failures. */
  if((flags&O_ACCMODE)==O_RDONLY)return __real_fclose(stream);
  assert(++injected==1);
  assert(expected_size>=4);
  if(fault==CLOSE_TORN){
    struct stat st;
    /* The real writer already called fflush successfully. */
    assert(fstat(fd,&st)==0 && S_ISREG(st.st_mode));
    assert(st.st_size==(off_t)expected_size);
    assert(ftruncate(fd,(off_t)(expected_size/2))==0);
  }
  rc=__real_fclose(stream);
  assert(rc==0); /* Close real descriptor, only then report injected EIO. */
  errno=EIO;
  return EOF;
}
static void begin_fault(CloseFault mode,size_t n){
  assert(fault==CLOSE_NONE && injected==0);
  assert(mode==CLOSE_TORN || mode==CLOSE_COMPLETE);
  fault=mode;expected_size=n;
}
static void end_fault(void){
  assert(injected==1);
  fault=CLOSE_NONE;injected=0;expected_size=0;
}
static void reset_pair(const char *a,const char *b){
  assert(fault==CLOSE_NONE);
  (void)remove(a);(void)remove(b);
}
static void pair_paths(const char *a,const char *b,int backup_b,
                       const char **backup,const char **target){
  *backup=backup_b?b:a;
  *target=backup_b?a:b;
  if(backup_b)assert(rename(a,b)==0);
}
static void read_exact(const char *path,uint8_t *data,size_t size){
  FILE *f=fopen(path,"rb");
  assert(f);
  assert(fread(data,1,size,f)==size && fgetc(f)==EOF);
  assert(fclose(f)==0);
}
static void assert_same(const char *path,const uint8_t *original,size_t size){
  uint8_t data[RIKITU_LEVEL_SAVE_BYTES];
  assert(size<=sizeof(data));
  read_exact(path,data,size);
  assert(memcmp(data,original,size)==0);
}
static void assert_pending_absent(const char *target){
  struct stat st;
  char pending[256];
  int n=snprintf(pending,sizeof(pending),"%s.pending",target);
  assert(n>0 && (size_t)n<sizeof(pending));
  assert(lstat(pending,&st)!=0 && errno==ENOENT);
}
static void assert_failed_stage_clean(const char *target){
  struct stat st;
  assert(lstat(target,&st)!=0 && errno==ENOENT);
  assert_pending_absent(target);
}
static void flower(RikituScene *s,RikituLoop *l){
  rikitu_flower_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
static void bridge(RikituScene *s,RikituLoop *l){
  rikitu_bridge_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
static void stage_flower(RikituScene *s,RikituLoop *l,int stage){
  s->reached_checkpoints=stage;
  s->active_checkpoint=s->checkpoints[stage-1];
  s->hearts=stage==1?2:1;
  l->active_seconds=(float)(stage*20);
}
static void stage_bridge(RikituScene *s,RikituLoop *l,int stage){
  int i;
  s->reached_checkpoints=stage;
  s->active_checkpoint=s->checkpoints[stage-1];
  s->hearts=stage==1?2:1;
  s->stage.coin_count=stage;
  for(i=0;i<stage;i++)s->stage.coins[i]=1;
  l->active_seconds=(float)(stage*20);
}
static void test_rks(CloseFault mode,int backup_b){
  RikituScene s,recovered;
  RikituLoop l,loaded;
  uint8_t old[RIKITU_SAVE_BYTES],latest[RIKITU_SAVE_BYTES];
  const char *backup,*target;
  uint32_t generation=0;
  reset_pair(S_A,S_B);
  flower(&s,&l);
  assert(rikitu_save_store_pair(S_A,S_B,&s,&l));
  pair_paths(S_A,S_B,backup_b,&backup,&target);
  read_exact(backup,old,sizeof(old));
  stage_flower(&s,&l,1);
  begin_fault(mode,sizeof(old));
  assert(!rikitu_save_store_pair(S_A,S_B,&s,&l));
  end_fault();
  assert_same(backup,old,sizeof(old));
  assert_failed_stage_clean(target);
  flower(&recovered,&loaded);
  assert(rikitu_save_load_pair(S_A,S_B,&recovered,&loaded));
  assert(recovered.reached_checkpoints==0 && recovered.hearts==3);
  assert(rikitu_save_store_pair(S_A,S_B,&s,&l));
  assert_same(backup,old,sizeof(old));
  read_exact(target,latest,sizeof(latest));
  assert(rikitu_save_decode(&recovered,&loaded,latest,sizeof(latest),&generation));
  assert(generation==2);
  assert_pending_absent(target);
  flower(&recovered,&loaded);
  assert(rikitu_save_load_pair(S_A,S_B,&recovered,&loaded));
  assert(recovered.reached_checkpoints==1 && recovered.hearts==2);
  reset_pair(S_A,S_B);
}
static void test_rkc(CloseFault mode,int backup_b){
  RikituCampaign c,recovered;
  uint8_t old[RIKITU_CAMPAIGN_SAVE_BYTES];
  uint8_t latest[RIKITU_CAMPAIGN_SAVE_BYTES];
  const char *backup,*target;
  uint32_t generation=0;
  reset_pair(C_A,C_B);
  assert(rikitu_campaign_init(&c,4));
  c.wallet=157;
  assert(rikitu_campaign_save_store_pair(C_A,C_B,&c));
  pair_paths(C_A,C_B,backup_b,&backup,&target);
  read_exact(backup,old,sizeof(old));
  c.wallet=158;
  begin_fault(mode,sizeof(old));
  assert(!rikitu_campaign_save_store_pair(C_A,C_B,&c));
  end_fault();
  assert_same(backup,old,sizeof(old));
  assert_failed_stage_clean(target);
  memset(&recovered,0,sizeof(recovered));
  assert(rikitu_campaign_save_load_pair(C_A,C_B,&recovered));
  assert(recovered.wallet==157 && recovered.level==4);
  assert(rikitu_campaign_save_store_pair(C_A,C_B,&c));
  assert_same(backup,old,sizeof(old));
  read_exact(target,latest,sizeof(latest));
  assert(rikitu_campaign_save_decode(&recovered,latest,sizeof(latest),&generation));
  assert(generation==2);
  assert_pending_absent(target);
  memset(&recovered,0,sizeof(recovered));
  assert(rikitu_campaign_save_load_pair(C_A,C_B,&recovered));
  assert(recovered.wallet==158 && recovered.level==4);
  reset_pair(C_A,C_B);
}
static void test_rkl(CloseFault mode,int backup_b){
  RikituCampaign c;
  RikituScene s,recovered;
  RikituLoop l,loaded;
  uint8_t old[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t latest[RIKITU_LEVEL_SAVE_BYTES];
  const char *backup,*target;
  uint32_t generation=0;
  reset_pair(L_A,L_B);
  assert(rikitu_campaign_init(&c,4));c.wallet=157;
  bridge(&s,&l);
  assert(rikitu_level_save_store_pair(L_A,L_B,&s,&l,&c));
  pair_paths(L_A,L_B,backup_b,&backup,&target);
  read_exact(backup,old,sizeof(old));
  stage_bridge(&s,&l,1);
  begin_fault(mode,sizeof(old));
  assert(!rikitu_level_save_store_pair(L_A,L_B,&s,&l,&c));
  end_fault();
  assert_same(backup,old,sizeof(old));
  assert_failed_stage_clean(target);
  bridge(&recovered,&loaded);
  assert(rikitu_level_save_load_pair(L_A,L_B,&recovered,&loaded,&c));
  assert(recovered.reached_checkpoints==0 && recovered.stage.coin_count==0);
  assert(recovered.hearts==3);
  assert(rikitu_level_save_store_pair(L_A,L_B,&s,&l,&c));
  assert_same(backup,old,sizeof(old));
  read_exact(target,latest,sizeof(latest));
  assert(rikitu_level_save_inspect(latest,sizeof(latest),&generation));
  assert(generation==2);
  assert_pending_absent(target);
  bridge(&recovered,&loaded);
  assert(rikitu_level_save_load_pair(L_A,L_B,&recovered,&loaded,&c));
  assert(recovered.reached_checkpoints==1 && recovered.hearts==2);
  assert(recovered.stage.coin_count==1 && recovered.stage.coins[0]==1);
  reset_pair(L_A,L_B);
}
int main(void){
  CloseFault mode;
  int backup_b;
  for(mode=CLOSE_TORN;mode<=CLOSE_COMPLETE;mode=(CloseFault)(mode+1)){
    for(backup_b=0;backup_b<=1;backup_b++){
      test_rks(mode,backup_b);
      test_rkc(mode,backup_b);
      test_rkl(mode,backup_b);
      printf("PASS Stage104 fclose %s, old slot %c: all RKS1/RKC2/RKL3 assertions\n",
             mode==CLOSE_TORN?"torn":"complete",backup_b?'B':'A');
    }
  }
  puts("PASS Stage104: 12 ambiguous-close cases; both live slots preserved, pending cleaned, retry verified");
  return 0;
}
