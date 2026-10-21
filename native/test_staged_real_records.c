/* Stage107 — exercise the *real* RKS1/RKC2/RKL3 codecs and loaders
 * with experimental staged writes, independent of the shipping writers.
 *
 * Each family x A/B orientation:
 *   genuine gen1 saves survive an incomplete leftover .pending file;
 *   stage gen2 to alternate live slot and load gen2, preserving gen1;
 *   stage gen3 over existing gen1, preserving valid gen2;
 *   corrupt gen3, verify production loader selects gen2;
 *   retry staged gen3 and confirm newest selected, with no .pending.
 *
 * Six independent test cases per GCC or Clang executable.
 * No shipping save API is patched or replaced by this fixture.
 */
#include "rikitu_flower.h"
#include "rikitu_bridge.h"
#include "rikitu_save.h"
#include "rikitu_campaign_save.h"
#include "rikitu_level_save.h"
#include "rikitu_staged_slot.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define RKS_A "/tmp/rikitu-stage107-map-a.rks"
#define RKS_B "/tmp/rikitu-stage107-map-b.rks"
#define RKC_A "/tmp/rikitu-stage107-campaign-a.rkc"
#define RKC_B "/tmp/rikitu-stage107-campaign-b.rkc"
#define RKL_A "/tmp/rikitu-stage107-level-a.rkl"
#define RKL_B "/tmp/rikitu-stage107-level-b.rkl"

static void add_suffix(char *dst,size_t n,const char *path){
  const char suffix[]=".pending";
  size_t len=strlen(path);
  assert(len+sizeof(suffix)<=n);
  memcpy(dst,path,len);
  memcpy(dst+len,suffix,sizeof(suffix));
}
static void write_exact(const char *path,const uint8_t *data,size_t n){
  FILE *f=fopen(path,"wb");
  assert(f);
  assert(fwrite(data,1,n,f)==n && fflush(f)==0);
  assert(fclose(f)==0);
}
static void read_exact(const char *path,uint8_t *out,size_t n){
  FILE *f=fopen(path,"rb");
  assert(f);
  assert(fread(out,1,n,f)==n && fgetc(f)==EOF);
  assert(fclose(f)==0);
}
static int equal_file(const char *path,const uint8_t *value,size_t n){
  uint8_t actual[RIKITU_LEVEL_SAVE_BYTES];
  assert(n<=sizeof(actual));
  read_exact(path,actual,n);
  return memcmp(actual,value,n)==0;
}
static void clean_pair(const char *a,const char *b){
  char temp[240];
  assert(strlen(a)<200 && strlen(b)<200);
  (void)remove(a);(void)remove(b);
  add_suffix(temp,sizeof(temp),a);(void)remove(temp);
  add_suffix(temp,sizeof(temp),b);(void)remove(temp);
}
static void stale_pending(const char *target){
  const uint8_t torn[]={0x52,0x49,0x4b,0x49,0x54,0x55,0xff};
  char temp[240];
  add_suffix(temp,sizeof(temp),target);
  write_exact(temp,torn,sizeof(torn));
}
static void assert_no_pending(const char *a,const char *b){
  char temp[240];
  FILE *f;
  add_suffix(temp,sizeof(temp),a);
  f=fopen(temp,"rb");assert(!f);
  add_suffix(temp,sizeof(temp),b);
  f=fopen(temp,"rb");assert(!f);
}
static void flip_latest(const char *target,const uint8_t *record,size_t n){
  uint8_t copy[RIKITU_LEVEL_SAVE_BYTES];
  assert(n<=sizeof(copy) && n>8);
  memcpy(copy,record,n);
  copy[8]^=0x40; /* CRC is intentionally no longer valid. */
  write_exact(target,copy,n);
}
static void flower(RikituScene *s,RikituLoop *l,int phase){
  rikitu_flower_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
  if(phase>0){
    s->reached_checkpoints=phase;
    s->active_checkpoint=s->checkpoints[phase-1];
    s->hearts=3-phase;
    l->active_seconds=(float)(20*phase);
  }
}
static void bridge(RikituScene *s,RikituLoop *l,int phase){
  int i;
  rikitu_bridge_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
  if(phase>0){
    s->reached_checkpoints=phase;
    s->active_checkpoint=s->checkpoints[phase-1];
    s->hearts=3-phase;
    s->stage.coin_count=phase;
    for(i=0;i<phase;i++)s->stage.coins[i]=1;
    l->active_seconds=(float)(20*phase);
  }
}
static void test_rks(int backup_b){
  RikituScene s,loaded;
  RikituLoop l,loop_loaded;
  uint8_t versions[3][RIKITU_SAVE_BYTES];
  const char *older=backup_b?RKS_B:RKS_A;
  const char *newer=backup_b?RKS_A:RKS_B;
  int i;
  clean_pair(RKS_A,RKS_B);
  for(i=0;i<3;i++){
    flower(&s,&l,i);
    assert(rikitu_save_encode(versions[i],&s,&l,(uint32_t)(i+1)));
  }
  write_exact(older,versions[0],RIKITU_SAVE_BYTES);
  stale_pending(newer);
  flower(&loaded,&loop_loaded,0);
  assert(rikitu_save_load_pair(RKS_A,RKS_B,&loaded,&loop_loaded));
  assert(loaded.reached_checkpoints==0 && loaded.hearts==3);
  assert(rikitu_staged_slot_commit(newer,versions[1],RIKITU_SAVE_BYTES));
  assert(equal_file(older,versions[0],RIKITU_SAVE_BYTES));
  assert(equal_file(newer,versions[1],RIKITU_SAVE_BYTES));
  assert_no_pending(RKS_A,RKS_B);
  flower(&loaded,&loop_loaded,0);
  assert(rikitu_save_load_pair(RKS_A,RKS_B,&loaded,&loop_loaded));
  assert(loaded.reached_checkpoints==1 && loaded.hearts==2);
  stale_pending(older);
  assert(rikitu_staged_slot_commit(older,versions[2],RIKITU_SAVE_BYTES));
  assert(equal_file(newer,versions[1],RIKITU_SAVE_BYTES));
  assert_no_pending(RKS_A,RKS_B);
  flower(&loaded,&loop_loaded,0);
  assert(rikitu_save_load_pair(RKS_A,RKS_B,&loaded,&loop_loaded));
  assert(loaded.reached_checkpoints==2 && loaded.hearts==1);
  flip_latest(older,versions[2],RIKITU_SAVE_BYTES);
  flower(&loaded,&loop_loaded,0);
  assert(rikitu_save_load_pair(RKS_A,RKS_B,&loaded,&loop_loaded));
  assert(loaded.reached_checkpoints==1 && loaded.hearts==2);
  assert(rikitu_staged_slot_commit(older,versions[2],RIKITU_SAVE_BYTES));
  flower(&loaded,&loop_loaded,0);
  assert(rikitu_save_load_pair(RKS_A,RKS_B,&loaded,&loop_loaded));
  assert(loaded.reached_checkpoints==2 && loaded.hearts==1);
  assert(equal_file(newer,versions[1],RIKITU_SAVE_BYTES));
  clean_pair(RKS_A,RKS_B);
  printf("PASS Stage107 RKS1 backup=%c: staged gen1/gen2/gen3 + corrupt latest fallback\n",backup_b?'B':'A');
}
static void test_rkc(int backup_b){
  RikituCampaign c,loaded;
  uint8_t versions[3][RIKITU_CAMPAIGN_SAVE_BYTES];
  const char *older=backup_b?RKC_B:RKC_A;
  const char *newer=backup_b?RKC_A:RKC_B;
  int i;
  clean_pair(RKC_A,RKC_B);
  for(i=0;i<3;i++){
    assert(rikitu_campaign_init(&c,4));
    c.wallet=157+i;
    assert(rikitu_campaign_save_encode(versions[i],&c,(uint32_t)(i+1)));
  }
  write_exact(older,versions[0],RIKITU_CAMPAIGN_SAVE_BYTES);
  stale_pending(newer);
  memset(&loaded,0,sizeof(loaded));
  assert(rikitu_campaign_save_load_pair(RKC_A,RKC_B,&loaded));
  assert(loaded.wallet==157 && loaded.level==4);
  assert(rikitu_staged_slot_commit(newer,versions[1],RIKITU_CAMPAIGN_SAVE_BYTES));
  assert(equal_file(older,versions[0],RIKITU_CAMPAIGN_SAVE_BYTES));
  assert_no_pending(RKC_A,RKC_B);
  memset(&loaded,0,sizeof(loaded));
  assert(rikitu_campaign_save_load_pair(RKC_A,RKC_B,&loaded));
  assert(loaded.wallet==158);
  stale_pending(older);
  assert(rikitu_staged_slot_commit(older,versions[2],RIKITU_CAMPAIGN_SAVE_BYTES));
  assert(equal_file(newer,versions[1],RIKITU_CAMPAIGN_SAVE_BYTES));
  assert_no_pending(RKC_A,RKC_B);
  memset(&loaded,0,sizeof(loaded));
  assert(rikitu_campaign_save_load_pair(RKC_A,RKC_B,&loaded));
  assert(loaded.wallet==159);
  flip_latest(older,versions[2],RIKITU_CAMPAIGN_SAVE_BYTES);
  memset(&loaded,0,sizeof(loaded));
  assert(rikitu_campaign_save_load_pair(RKC_A,RKC_B,&loaded));
  assert(loaded.wallet==158);
  assert(rikitu_staged_slot_commit(older,versions[2],RIKITU_CAMPAIGN_SAVE_BYTES));
  memset(&loaded,0,sizeof(loaded));
  assert(rikitu_campaign_save_load_pair(RKC_A,RKC_B,&loaded));
  assert(loaded.wallet==159);
  assert(equal_file(newer,versions[1],RIKITU_CAMPAIGN_SAVE_BYTES));
  clean_pair(RKC_A,RKC_B);
  printf("PASS Stage107 RKC2 backup=%c: staged gen1/gen2/gen3 + corrupt latest fallback\n",backup_b?'B':'A');
}
static void test_rkl(int backup_b){
  RikituCampaign c;
  RikituScene s,loaded;
  RikituLoop l,loop_loaded;
  uint8_t versions[3][RIKITU_LEVEL_SAVE_BYTES];
  const char *older=backup_b?RKL_B:RKL_A;
  const char *newer=backup_b?RKL_A:RKL_B;
  int i;
  clean_pair(RKL_A,RKL_B);
  assert(rikitu_campaign_init(&c,4));
  c.wallet=157;
  for(i=0;i<3;i++){
    bridge(&s,&l,i);
    assert(rikitu_level_save_encode(versions[i],&s,&l,&c,(uint32_t)(i+1)));
  }
  write_exact(older,versions[0],RIKITU_LEVEL_SAVE_BYTES);
  stale_pending(newer);
  bridge(&loaded,&loop_loaded,0);
  assert(rikitu_level_save_load_pair(RKL_A,RKL_B,&loaded,&loop_loaded,&c));
  assert(loaded.reached_checkpoints==0 && loaded.stage.coin_count==0);
  assert(rikitu_staged_slot_commit(newer,versions[1],RIKITU_LEVEL_SAVE_BYTES));
  assert(equal_file(older,versions[0],RIKITU_LEVEL_SAVE_BYTES));
  assert_no_pending(RKL_A,RKL_B);
  bridge(&loaded,&loop_loaded,0);
  assert(rikitu_level_save_load_pair(RKL_A,RKL_B,&loaded,&loop_loaded,&c));
  assert(loaded.reached_checkpoints==1 && loaded.hearts==2 && loaded.stage.coin_count==1);
  stale_pending(older);
  assert(rikitu_staged_slot_commit(older,versions[2],RIKITU_LEVEL_SAVE_BYTES));
  assert(equal_file(newer,versions[1],RIKITU_LEVEL_SAVE_BYTES));
  assert_no_pending(RKL_A,RKL_B);
  bridge(&loaded,&loop_loaded,0);
  assert(rikitu_level_save_load_pair(RKL_A,RKL_B,&loaded,&loop_loaded,&c));
  assert(loaded.reached_checkpoints==2 && loaded.hearts==1 && loaded.stage.coin_count==2);
  flip_latest(older,versions[2],RIKITU_LEVEL_SAVE_BYTES);
  bridge(&loaded,&loop_loaded,0);
  assert(rikitu_level_save_load_pair(RKL_A,RKL_B,&loaded,&loop_loaded,&c));
  assert(loaded.reached_checkpoints==1 && loaded.hearts==2 && loaded.stage.coin_count==1);
  assert(rikitu_staged_slot_commit(older,versions[2],RIKITU_LEVEL_SAVE_BYTES));
  bridge(&loaded,&loop_loaded,0);
  assert(rikitu_level_save_load_pair(RKL_A,RKL_B,&loaded,&loop_loaded,&c));
  assert(loaded.reached_checkpoints==2 && loaded.hearts==1 && loaded.stage.coin_count==2);
  assert(equal_file(newer,versions[1],RIKITU_LEVEL_SAVE_BYTES));
  clean_pair(RKL_A,RKL_B);
  printf("PASS Stage107 RKL3 backup=%c: staged gen1/gen2/gen3 + corrupt latest fallback\n",backup_b?'B':'A');
}
int main(void){
  int b;
  for(b=0;b<2;b++){
    test_rks(b);
    test_rkc(b);
    test_rkl(b);
  }
  puts("PASS Stage107: six real-format staged save, fallback and repair scenarios");
  return 0;
}
