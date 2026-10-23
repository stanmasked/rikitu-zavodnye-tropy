/* Stage144 — post-failure store retry after semantic fallback.
 * CRC-valid but semantically impossible records must not outrank
 * the last fully restorable save in production RKS1/RKC2/RKL3 slot pairs.
 *
 * Exercise public production load/store APIs with both A/B orientations.
 * After fallback, a store must preserve the valid slot byte-for-byte and
 * replace the invalid high-generation slot using the next valid generation.
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
#include <sys/stat.h>
#include <unistd.h>

#define RKS_A "/tmp/rikitu-stage143-map-a.rks"
#define RKS_B "/tmp/rikitu-stage143-map-b.rks"
#define RKC_A "/tmp/rikitu-stage143-campaign-a.rkc"
#define RKC_B "/tmp/rikitu-stage143-campaign-b.rkc"
#define RKL_A "/tmp/rikitu-stage143-level-a.rkl"
#define RKL_B "/tmp/rikitu-stage143-level-b.rkl"

static void put32(uint8_t *p,uint32_t v){
  unsigned i;for(i=0;i<4;i++)p[i]=(uint8_t)(v>>(8u*i));
}
static uint32_t crc32(const uint8_t *p,size_t n){
  uint32_t c=0xffffffffu;
  size_t i;unsigned j;
  for(i=0;i<n;i++){
    c^=p[i];
    for(j=0;j<8;j++)c=(c>>1)^((c&1u)?0xedb88320u:0u);
  }
  return ~c;
}
static void repair_crc(uint8_t *record,size_t n){
  assert(n>=4u);
  put32(record+n-4u,crc32(record,n-4u));
}
static void append_suffix(char *dst,size_t cap,const char *path,
                          const char *suffix){
  size_t a=strlen(path),b=strlen(suffix);
  assert(a+b+1u<=cap);
  memcpy(dst,path,a);memcpy(dst+a,suffix,b+1u);
}
static void write_exact(const char *path,const uint8_t *data,size_t n){
  FILE *f=fopen(path,"wb");
  assert(f);assert(fwrite(data,1,n,f)==n);
  assert(fflush(f)==0);assert(fclose(f)==0);
}
static void read_exact(const char *path,uint8_t *out,size_t n){
  FILE *f=fopen(path,"rb");
  assert(f);assert(fread(out,1,n,f)==n);
  assert(fgetc(f)==EOF);assert(fclose(f)==0);
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
    append_suffix(sidecar,sizeof(sidecar),paths[i],".pending.lock");
    (void)remove(sidecar);
  }
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

/* A directory at the staging path reliably simulates failure to open the
 * temporary write without touching either live record. Never delete live
 * slots in order to inject this fault. */
static void block_pending(const char *slot){
  char path[256];
  append_suffix(path,sizeof(path),slot,".pending");
  assert(mkdir(path,0700)==0);
}
static void unblock_pending(const char *slot){
  char path[256];
  append_suffix(path,sizeof(path),slot,".pending");
  assert(rmdir(path)==0);
}
static void assert_slot_bytes(const char *path,const uint8_t *record,size_t n){
  uint8_t found[RIKITU_LEVEL_SAVE_BYTES];
  assert(n<=sizeof(found));
  read_exact(path,found,n);
  assert(memcmp(found,record,n)==0);
}

static void make_flower(RikituScene *s,RikituLoop *l,int phase){
  rikitu_flower_init(s);memset(l,0,sizeof(*l));l->player=s->player;
  if(phase>0){
    s->reached_checkpoints=phase;s->active_checkpoint=s->checkpoints[phase-1];
    s->hearts=3-phase;l->active_seconds=(float)(20*phase);
  }
}
static void make_bridge(RikituScene *s,RikituLoop *l,int phase){
  int i;
  rikitu_bridge_init(s);memset(l,0,sizeof(*l));l->player=s->player;
  if(phase>0){
    s->reached_checkpoints=phase;s->active_checkpoint=s->checkpoints[phase-1];
    s->hearts=3-phase;s->stage.coin_count=phase;
    for(i=0;i<phase;i++)s->stage.coins[i]=1;
    l->active_seconds=(float)(20*phase);
  }
}
static void make_campaign(RikituCampaign *c,int wallet){
  assert(rikitu_campaign_init(c,4));c->wallet=wallet;
}

static void test_rks_semantic_fallback(int reverse){
  RikituScene old_scene,request_scene,loaded_scene,decoded_scene;
  RikituLoop old_loop,request_loop,loaded_loop,decoded_loop;
  uint8_t old_record[RIKITU_SAVE_BYTES],bad_record[RIKITU_SAVE_BYTES];
  uint8_t a[RIKITU_SAVE_BYTES],b[RIKITU_SAVE_BYTES];
  const uint8_t *replacement;
  uint32_t generation=0;
  clean_pair(RKS_A,RKS_B);
  make_flower(&old_scene,&old_loop,1);
  make_flower(&request_scene,&request_loop,2);
  assert(rikitu_save_encode(old_record,&old_scene,&old_loop,10u));
  assert(rikitu_save_encode(bad_record,&request_scene,&request_loop,99u));
  bad_record[7]=0u; /* CRC-valid but impossible heart count. */
  repair_crc(bad_record,sizeof(bad_record));
  if(reverse){write_exact(RKS_A,bad_record,sizeof(bad_record));write_exact(RKS_B,old_record,sizeof(old_record));}
  else{write_exact(RKS_A,old_record,sizeof(old_record));write_exact(RKS_B,bad_record,sizeof(bad_record));}
  make_flower(&loaded_scene,&loaded_loop,0);
  assert(rikitu_save_load_pair(RKS_A,RKS_B,&loaded_scene,&loaded_loop));
  assert(loaded_scene.reached_checkpoints==1&&loaded_scene.hearts==2);
  block_pending(reverse?RKS_A:RKS_B);
  assert(!rikitu_save_store_pair(RKS_A,RKS_B,&request_scene,&request_loop));
  assert_slot_bytes(RKS_A,reverse?bad_record:old_record,sizeof(a));
  assert_slot_bytes(RKS_B,reverse?old_record:bad_record,sizeof(b));
  make_flower(&loaded_scene,&loaded_loop,0);
  assert(rikitu_save_load_pair(RKS_A,RKS_B,&loaded_scene,&loaded_loop));
  assert(loaded_scene.reached_checkpoints==1&&loaded_scene.hearts==2);
  unblock_pending(reverse?RKS_A:RKS_B);
  assert(rikitu_save_store_pair(RKS_A,RKS_B,&request_scene,&request_loop));
  read_exact(RKS_A,a,sizeof(a));read_exact(RKS_B,b,sizeof(b));
  assert((memcmp(a,old_record,sizeof(a))==0) !=
         (memcmp(b,old_record,sizeof(b))==0));
  replacement=memcmp(a,old_record,sizeof(a))==0?b:a;
  make_flower(&decoded_scene,&decoded_loop,0);
  assert(rikitu_save_decode(&decoded_scene,&decoded_loop,replacement,
                            RIKITU_SAVE_BYTES,&generation));
  assert(generation==11u&&decoded_scene.reached_checkpoints==2&&
         decoded_scene.hearts==1);
  /* A second successful write must alternate back to the old slot, retain
   * generation 11 byte-for-byte, and advance exactly to generation 12. */
  {
    uint8_t gen11[RIKITU_SAVE_BYTES],gen12[RIKITU_SAVE_BYTES];
    memcpy(gen11,replacement,sizeof(gen11));
    request_loop.active_seconds=41.0f;
    assert(rikitu_save_store_pair(RKS_A,RKS_B,&request_scene,&request_loop));
    assert_slot_bytes(reverse?RKS_A:RKS_B,gen11,sizeof(gen11));
    read_exact(reverse?RKS_B:RKS_A,gen12,sizeof(gen12));
    make_flower(&decoded_scene,&decoded_loop,0);
    assert(rikitu_save_decode(&decoded_scene,&decoded_loop,gen12,sizeof(gen12),&generation));
    assert(generation==12u&&decoded_scene.reached_checkpoints==2&&
           decoded_loop.active_seconds==41.0f);
    make_flower(&loaded_scene,&loaded_loop,0);
    assert(rikitu_save_load_pair(RKS_A,RKS_B,&loaded_scene,&loaded_loop));
    assert(loaded_scene.reached_checkpoints==2&&loaded_loop.active_seconds==41.0f);
  }
  assert_no_sidecars(RKS_A,RKS_B);
  printf("PASS Stage143 RKS1 semantic fallback orientation=%s\n",reverse?"A-new":"B-new");
  clean_pair(RKS_A,RKS_B);
}
static void test_rkc_semantic_fallback(int reverse){
  RikituCampaign old_campaign,request_campaign,loaded_campaign,decoded_campaign;
  uint8_t old_record[RIKITU_CAMPAIGN_SAVE_BYTES],bad_record[RIKITU_CAMPAIGN_SAVE_BYTES];
  uint8_t a[RIKITU_CAMPAIGN_SAVE_BYTES],b[RIKITU_CAMPAIGN_SAVE_BYTES];
  const uint8_t *replacement;
  uint32_t generation=0;
  clean_pair(RKC_A,RKC_B);
  make_campaign(&old_campaign,157);make_campaign(&request_campaign,159);
  assert(rikitu_campaign_save_encode(old_record,&old_campaign,10u));
  assert(rikitu_campaign_save_encode(bad_record,&request_campaign,99u));
  bad_record[5]=0u; /* CRC-valid but invalid campaign level. */
  repair_crc(bad_record,sizeof(bad_record));
  if(reverse){write_exact(RKC_A,bad_record,sizeof(bad_record));write_exact(RKC_B,old_record,sizeof(old_record));}
  else{write_exact(RKC_A,old_record,sizeof(old_record));write_exact(RKC_B,bad_record,sizeof(bad_record));}
  make_campaign(&loaded_campaign,0);
  assert(rikitu_campaign_save_load_pair(RKC_A,RKC_B,&loaded_campaign));
  assert(loaded_campaign.wallet==157&&loaded_campaign.level==4);
  block_pending(reverse?RKC_A:RKC_B);
  assert(!rikitu_campaign_save_store_pair(RKC_A,RKC_B,&request_campaign));
  assert_slot_bytes(RKC_A,reverse?bad_record:old_record,sizeof(a));
  assert_slot_bytes(RKC_B,reverse?old_record:bad_record,sizeof(b));
  make_campaign(&loaded_campaign,0);
  assert(rikitu_campaign_save_load_pair(RKC_A,RKC_B,&loaded_campaign));
  assert(loaded_campaign.wallet==157&&loaded_campaign.level==4);
  unblock_pending(reverse?RKC_A:RKC_B);
  assert(rikitu_campaign_save_store_pair(RKC_A,RKC_B,&request_campaign));
  read_exact(RKC_A,a,sizeof(a));read_exact(RKC_B,b,sizeof(b));
  assert((memcmp(a,old_record,sizeof(a))==0) !=
         (memcmp(b,old_record,sizeof(b))==0));
  replacement=memcmp(a,old_record,sizeof(a))==0?b:a;
  assert(rikitu_campaign_save_decode(&decoded_campaign,replacement,
                                     sizeof(b),&generation));
  assert(generation==11u&&decoded_campaign.wallet==159&&decoded_campaign.level==4);
  {
    uint8_t gen11[RIKITU_CAMPAIGN_SAVE_BYTES],gen12[RIKITU_CAMPAIGN_SAVE_BYTES];
    memcpy(gen11,replacement,sizeof(gen11));
    request_campaign.wallet=160;
    assert(rikitu_campaign_save_store_pair(RKC_A,RKC_B,&request_campaign));
    assert_slot_bytes(reverse?RKC_A:RKC_B,gen11,sizeof(gen11));
    read_exact(reverse?RKC_B:RKC_A,gen12,sizeof(gen12));
    assert(rikitu_campaign_save_decode(&decoded_campaign,gen12,sizeof(gen12),&generation));
    assert(generation==12u&&decoded_campaign.wallet==160);
    make_campaign(&loaded_campaign,0);
    assert(rikitu_campaign_save_load_pair(RKC_A,RKC_B,&loaded_campaign));
    assert(loaded_campaign.wallet==160);
  }
  assert_no_sidecars(RKC_A,RKC_B);
  printf("PASS Stage143 RKC2 semantic fallback orientation=%s\n",reverse?"A-new":"B-new");
  clean_pair(RKC_A,RKC_B);
}
static void test_rkl_semantic_fallback(int reverse){
  RikituCampaign campaign;
  RikituScene old_scene,request_scene,loaded_scene,decoded_scene;
  RikituLoop old_loop,request_loop,loaded_loop,decoded_loop;
  uint8_t old_record[RIKITU_LEVEL_SAVE_BYTES],bad_record[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t a[RIKITU_LEVEL_SAVE_BYTES],b[RIKITU_LEVEL_SAVE_BYTES];
  const uint8_t *replacement;
  uint32_t generation=0;
  clean_pair(RKL_A,RKL_B);
  make_campaign(&campaign,157);
  make_bridge(&old_scene,&old_loop,1);make_bridge(&request_scene,&request_loop,2);
  assert(rikitu_level_save_encode(old_record,&old_scene,&old_loop,&campaign,10u));
  assert(rikitu_level_save_encode(bad_record,&request_scene,&request_loop,&campaign,99u));
  bad_record[7]=0u; /* CRC-valid but impossible heart count. */
  repair_crc(bad_record,sizeof(bad_record));
  if(reverse){write_exact(RKL_A,bad_record,sizeof(bad_record));write_exact(RKL_B,old_record,sizeof(old_record));}
  else{write_exact(RKL_A,old_record,sizeof(old_record));write_exact(RKL_B,bad_record,sizeof(bad_record));}
  make_bridge(&loaded_scene,&loaded_loop,0);
  assert(rikitu_level_save_load_pair(RKL_A,RKL_B,&loaded_scene,&loaded_loop,&campaign));
  assert(loaded_scene.reached_checkpoints==1&&loaded_scene.hearts==2&&
         loaded_scene.stage.coin_count==1);
  block_pending(reverse?RKL_A:RKL_B);
  assert(!rikitu_level_save_store_pair(RKL_A,RKL_B,&request_scene,&request_loop,&campaign));
  assert_slot_bytes(RKL_A,reverse?bad_record:old_record,sizeof(a));
  assert_slot_bytes(RKL_B,reverse?old_record:bad_record,sizeof(b));
  make_bridge(&loaded_scene,&loaded_loop,0);
  assert(rikitu_level_save_load_pair(RKL_A,RKL_B,&loaded_scene,&loaded_loop,&campaign));
  assert(loaded_scene.reached_checkpoints==1&&loaded_scene.hearts==2);
  unblock_pending(reverse?RKL_A:RKL_B);
  assert(rikitu_level_save_store_pair(RKL_A,RKL_B,&request_scene,&request_loop,&campaign));
  read_exact(RKL_A,a,sizeof(a));read_exact(RKL_B,b,sizeof(b));
  assert((memcmp(a,old_record,sizeof(a))==0) !=
         (memcmp(b,old_record,sizeof(b))==0));
  replacement=memcmp(a,old_record,sizeof(a))==0?b:a;
  make_bridge(&decoded_scene,&decoded_loop,0);
  assert(rikitu_level_save_decode(&decoded_scene,&decoded_loop,&campaign,
                                  replacement,sizeof(b),&generation));
  assert(generation==11u&&decoded_scene.reached_checkpoints==2&&
         decoded_scene.hearts==1&&decoded_scene.stage.coin_count==2);
  {
    uint8_t gen11[RIKITU_LEVEL_SAVE_BYTES],gen12[RIKITU_LEVEL_SAVE_BYTES];
    memcpy(gen11,replacement,sizeof(gen11));
    request_loop.active_seconds=41.0f;
    assert(rikitu_level_save_store_pair(RKL_A,RKL_B,&request_scene,&request_loop,&campaign));
    assert_slot_bytes(reverse?RKL_A:RKL_B,gen11,sizeof(gen11));
    read_exact(reverse?RKL_B:RKL_A,gen12,sizeof(gen12));
    make_bridge(&decoded_scene,&decoded_loop,0);
    assert(rikitu_level_save_decode(&decoded_scene,&decoded_loop,&campaign,gen12,sizeof(gen12),&generation));
    assert(generation==12u&&decoded_scene.stage.coin_count==2&&
           decoded_loop.active_seconds==41.0f);
    make_bridge(&loaded_scene,&loaded_loop,0);
    assert(rikitu_level_save_load_pair(RKL_A,RKL_B,&loaded_scene,&loaded_loop,&campaign));
    assert(loaded_scene.stage.coin_count==2&&loaded_loop.active_seconds==41.0f);
  }
  assert_no_sidecars(RKL_A,RKL_B);
  printf("PASS Stage143 RKL3 semantic fallback orientation=%s\n",reverse?"A-new":"B-new");
  clean_pair(RKL_A,RKL_B);
}
int main(void){
  test_rks_semantic_fallback(0);test_rks_semantic_fallback(1);
  test_rkc_semantic_fallback(0);test_rkc_semantic_fallback(1);
  test_rkl_semantic_fallback(0);test_rkl_semantic_fallback(1);
  puts("PASS Stage143: CRC-valid semantic corruption falls back and repairs RKS1/RKC2/RKL3");
  return 0;
}
