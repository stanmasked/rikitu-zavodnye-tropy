/* Stage140 — production RKS1/RKC2/RKL3 writers must fail closed on
 * ambiguous valid slot pairs, while identical duplicate slots remain usable.
 *
 * This exercises the public production store/load APIs, not only the lower
 * level staged-pair helper. All conflicting records have valid codec CRCs.
 */
#include "rikitu_flower.h"
#include "rikitu_bridge.h"
#include "rikitu_save.h"
#include "rikitu_campaign_save.h"
#include "rikitu_level_save.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define RKS_A "/tmp/rikitu-stage140-map-a.rks"
#define RKS_B "/tmp/rikitu-stage140-map-b.rks"
#define RKC_A "/tmp/rikitu-stage140-campaign-a.rkc"
#define RKC_B "/tmp/rikitu-stage140-campaign-b.rkc"
#define RKL_A "/tmp/rikitu-stage140-level-a.rkl"
#define RKL_B "/tmp/rikitu-stage140-level-b.rkl"

static void append_suffix(char *dst,size_t cap,const char *path,
                          const char *suffix){
  size_t a=strlen(path),b=strlen(suffix);
  assert(a+b+1u<=cap);
  memcpy(dst,path,a);
  memcpy(dst+a,suffix,b+1u);
}
static void write_exact(const char *path,const uint8_t *data,size_t n){
  FILE *f=fopen(path,"wb");
  assert(f);
  assert(fwrite(data,1,n,f)==n);
  assert(fflush(f)==0);
  assert(fclose(f)==0);
}
static void read_exact(const char *path,uint8_t *out,size_t n){
  FILE *f=fopen(path,"rb");
  assert(f);
  assert(fread(out,1,n,f)==n);
  assert(fgetc(f)==EOF);
  assert(fclose(f)==0);
}
static int equal_file(const char *path,const uint8_t *expected,size_t n){
  uint8_t actual[RIKITU_LEVEL_SAVE_BYTES];
  assert(n<=sizeof(actual));
  read_exact(path,actual,n);
  return memcmp(actual,expected,n)==0;
}
static void clean_pair(const char *a,const char *b){
  char sidecar[256],lock[256];
  const char *paths[2]={a,b};
  unsigned i;
  (void)remove(a);(void)remove(b);
  for(i=0;i<2;i++){
    append_suffix(sidecar,sizeof(sidecar),paths[i],".pending");
    (void)remove(sidecar);
    append_suffix(sidecar,sizeof(sidecar),paths[i],".hold");
    (void)remove(sidecar);
  }
  /* A sorts before B, so it names the stable pair-wide lock file. */
  append_suffix(lock,sizeof(lock),a,".pair.lock");
  (void)remove(lock);
}
static void assert_no_sidecars(const char *a,const char *b){
  char sidecar[256];
  const char *paths[2]={a,b};
  unsigned i;
  FILE *f;
  for(i=0;i<2;i++){
    append_suffix(sidecar,sizeof(sidecar),paths[i],".pending");
    f=fopen(sidecar,"rb");assert(!f);
    append_suffix(sidecar,sizeof(sidecar),paths[i],".hold");
    f=fopen(sidecar,"rb");assert(!f);
  }
}
static void make_flower(RikituScene *s,RikituLoop *l,int phase){
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
static void make_bridge(RikituScene *s,RikituLoop *l,int phase){
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
static void make_campaign(RikituCampaign *c,int wallet){
  assert(rikitu_campaign_init(c,4));
  c->wallet=wallet;
}
static void test_rks_conflict(int halfway){
  RikituScene sa,sb,request,loaded;
  RikituLoop la,lb,request_loop,loaded_loop;
  unsigned char scene_snapshot[sizeof(RikituScene)];
  unsigned char loop_snapshot[sizeof(RikituLoop)];
  uint8_t a[RIKITU_SAVE_BYTES],b[RIKITU_SAVE_BYTES];
  uint32_t ga=8u,gb=halfway?0x80000008u:8u;
  clean_pair(RKS_A,RKS_B);
  make_flower(&sa,&la,0);make_flower(&sb,&lb,1);
  make_flower(&request,&request_loop,2);
  assert(rikitu_save_encode(a,&sa,&la,ga));
  assert(rikitu_save_encode(b,&sb,&lb,gb));
  assert(memcmp(a,b,sizeof(a))!=0);
  write_exact(RKS_A,a,sizeof(a));write_exact(RKS_B,b,sizeof(b));
  memset(&loaded,0x5a,sizeof(loaded));
  memset(&loaded_loop,0xa5,sizeof(loaded_loop));
  memcpy(scene_snapshot,&loaded,sizeof(loaded));
  memcpy(loop_snapshot,&loaded_loop,sizeof(loaded_loop));
  assert(!rikitu_save_load_pair(RKS_A,RKS_B,&loaded,&loaded_loop));
  assert(memcmp(scene_snapshot,&loaded,sizeof(loaded))==0);
  assert(memcmp(loop_snapshot,&loaded_loop,sizeof(loaded_loop))==0);
  assert(!rikitu_save_store_pair(RKS_A,RKS_B,&request,&request_loop));
  assert(equal_file(RKS_A,a,sizeof(a))&&equal_file(RKS_B,b,sizeof(b)));
  assert_no_sidecars(RKS_A,RKS_B);
  printf("PASS Stage140 RKS1 rejects %s ambiguity without writes\n",
         halfway?"half-range":"equal-generation");
  clean_pair(RKS_A,RKS_B);
}
static void test_rkc_conflict(int halfway){
  RikituCampaign ca,cb,request,loaded;
  unsigned char campaign_snapshot[sizeof(RikituCampaign)];
  uint8_t a[RIKITU_CAMPAIGN_SAVE_BYTES],b[RIKITU_CAMPAIGN_SAVE_BYTES];
  uint32_t ga=8u,gb=halfway?0x80000008u:8u;
  clean_pair(RKC_A,RKC_B);
  make_campaign(&ca,157);make_campaign(&cb,158);make_campaign(&request,159);
  assert(rikitu_campaign_save_encode(a,&ca,ga));
  assert(rikitu_campaign_save_encode(b,&cb,gb));
  assert(memcmp(a,b,sizeof(a))!=0);
  write_exact(RKC_A,a,sizeof(a));write_exact(RKC_B,b,sizeof(b));
  memset(&loaded,0x5a,sizeof(loaded));
  memcpy(campaign_snapshot,&loaded,sizeof(loaded));
  assert(!rikitu_campaign_save_load_pair(RKC_A,RKC_B,&loaded));
  assert(memcmp(campaign_snapshot,&loaded,sizeof(loaded))==0);
  assert(!rikitu_campaign_save_store_pair(RKC_A,RKC_B,&request));
  assert(equal_file(RKC_A,a,sizeof(a))&&equal_file(RKC_B,b,sizeof(b)));
  assert_no_sidecars(RKC_A,RKC_B);
  printf("PASS Stage140 RKC2 rejects %s ambiguity without writes\n",
         halfway?"half-range":"equal-generation");
  clean_pair(RKC_A,RKC_B);
}
static void test_rkl_conflict(int halfway){
  RikituCampaign c;
  RikituScene sa,sb,request,loaded;
  RikituLoop la,lb,request_loop,loaded_loop;
  unsigned char scene_snapshot[sizeof(RikituScene)];
  unsigned char loop_snapshot[sizeof(RikituLoop)];
  uint8_t a[RIKITU_LEVEL_SAVE_BYTES],b[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t ga=8u,gb=halfway?0x80000008u:8u;
  clean_pair(RKL_A,RKL_B);
  make_campaign(&c,157);
  make_bridge(&sa,&la,0);make_bridge(&sb,&lb,1);
  make_bridge(&request,&request_loop,2);
  assert(rikitu_level_save_encode(a,&sa,&la,&c,ga));
  assert(rikitu_level_save_encode(b,&sb,&lb,&c,gb));
  assert(memcmp(a,b,sizeof(a))!=0);
  write_exact(RKL_A,a,sizeof(a));write_exact(RKL_B,b,sizeof(b));
  make_bridge(&loaded,&loaded_loop,0);
  memcpy(scene_snapshot,&loaded,sizeof(loaded));
  memcpy(loop_snapshot,&loaded_loop,sizeof(loaded_loop));
  assert(!rikitu_level_save_load_pair(RKL_A,RKL_B,&loaded,&loaded_loop,&c));
  assert(memcmp(scene_snapshot,&loaded,sizeof(loaded))==0);
  assert(memcmp(loop_snapshot,&loaded_loop,sizeof(loaded_loop))==0);
  assert(!rikitu_level_save_store_pair(RKL_A,RKL_B,&request,&request_loop,&c));
  assert(equal_file(RKL_A,a,sizeof(a))&&equal_file(RKL_B,b,sizeof(b)));
  assert_no_sidecars(RKL_A,RKL_B);
  printf("PASS Stage140 RKL3 rejects %s ambiguity without writes\n",
         halfway?"half-range":"equal-generation");
  clean_pair(RKL_A,RKL_B);
}
static void test_rks_duplicate(void){
  RikituScene old,request,loaded;
  RikituLoop old_loop,request_loop,loaded_loop;
  uint8_t record[RIKITU_SAVE_BYTES];
  clean_pair(RKS_A,RKS_B);
  make_flower(&old,&old_loop,0);make_flower(&request,&request_loop,1);
  RikituScene duplicate_loaded;
  RikituLoop duplicate_loop;
  assert(rikitu_save_encode(record,&old,&old_loop,8u));
  write_exact(RKS_A,record,sizeof(record));write_exact(RKS_B,record,sizeof(record));
  make_flower(&duplicate_loaded,&duplicate_loop,0);
  assert(rikitu_save_load_pair(RKS_A,RKS_B,&duplicate_loaded,&duplicate_loop));
  assert(duplicate_loaded.reached_checkpoints==0&&duplicate_loaded.hearts==3);
  assert(rikitu_save_store_pair(RKS_A,RKS_B,&request,&request_loop));
  assert(equal_file(RKS_A,record,sizeof(record)));
  assert(!equal_file(RKS_B,record,sizeof(record)));
  make_flower(&loaded,&loaded_loop,0);
  assert(rikitu_save_load_pair(RKS_A,RKS_B,&loaded,&loaded_loop));
  assert(loaded.reached_checkpoints==1&&loaded.hearts==2);
  assert_no_sidecars(RKS_A,RKS_B);
  clean_pair(RKS_A,RKS_B);
}
static void test_rkc_duplicate(void){
  RikituCampaign old,request,loaded;
  uint8_t record[RIKITU_CAMPAIGN_SAVE_BYTES];
  clean_pair(RKC_A,RKC_B);
  make_campaign(&old,157);make_campaign(&request,159);
  assert(rikitu_campaign_save_encode(record,&old,8u));
  write_exact(RKC_A,record,sizeof(record));write_exact(RKC_B,record,sizeof(record));
  memset(&loaded,0,sizeof(loaded));
  assert(rikitu_campaign_save_load_pair(RKC_A,RKC_B,&loaded));
  assert(loaded.wallet==157);
  assert(rikitu_campaign_save_store_pair(RKC_A,RKC_B,&request));
  assert(equal_file(RKC_A,record,sizeof(record)));
  assert(!equal_file(RKC_B,record,sizeof(record)));
  memset(&loaded,0,sizeof(loaded));
  assert(rikitu_campaign_save_load_pair(RKC_A,RKC_B,&loaded));
  assert(loaded.wallet==159);
  assert_no_sidecars(RKC_A,RKC_B);
  clean_pair(RKC_A,RKC_B);
}
static void test_rkl_duplicate(void){
  RikituCampaign c;
  RikituScene old,request,loaded;
  RikituLoop old_loop,request_loop,loaded_loop;
  uint8_t record[RIKITU_LEVEL_SAVE_BYTES];
  clean_pair(RKL_A,RKL_B);
  make_campaign(&c,157);
  make_bridge(&old,&old_loop,0);make_bridge(&request,&request_loop,1);
  assert(rikitu_level_save_encode(record,&old,&old_loop,&c,8u));
  write_exact(RKL_A,record,sizeof(record));write_exact(RKL_B,record,sizeof(record));
  make_bridge(&loaded,&loaded_loop,0);
  assert(rikitu_level_save_load_pair(RKL_A,RKL_B,&loaded,&loaded_loop,&c));
  assert(loaded.reached_checkpoints==0&&loaded.stage.coin_count==0);
  assert(rikitu_level_save_store_pair(RKL_A,RKL_B,&request,&request_loop,&c));
  assert(equal_file(RKL_A,record,sizeof(record)));
  assert(!equal_file(RKL_B,record,sizeof(record)));
  make_bridge(&loaded,&loaded_loop,0);
  assert(rikitu_level_save_load_pair(RKL_A,RKL_B,&loaded,&loaded_loop,&c));
  assert(loaded.reached_checkpoints==1&&loaded.hearts==2&&
         loaded.stage.coin_count==1);
  assert_no_sidecars(RKL_A,RKL_B);
  clean_pair(RKL_A,RKL_B);
}
int main(void){
  test_rks_conflict(0);test_rks_conflict(1);
  test_rkc_conflict(0);test_rkc_conflict(1);
  test_rkl_conflict(0);test_rkl_conflict(1);
  test_rks_duplicate();test_rkc_duplicate();test_rkl_duplicate();
  puts("PASS Stage140: all production save wrappers fail closed; identical duplicates recover");
  return 0;
}
