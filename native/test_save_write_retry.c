/* Stage102 — non-destructive save failure and retry on all three formats.
 *
 * A directory deliberately occupies the next writer's slot path, so fopen
 * ("wb") fails deterministically while the other slot contains one valid
 * checkpoint. The writer must return failure; the source slot must remain
 * byte-identical and loadable; clearing the obstacle must allow a retry.
 *
 * Both A-backed and B-backed orientations are covered for RKS1/RKC2/RKL3.
 * This models failure to open the target path, not a partial SD write,
 * fsync durability, write callback failure or physical power loss.
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

#define RKS_A "/tmp/rikitu-stage102-map-a.rks"
#define RKS_B "/tmp/rikitu-stage102-map-b.rks"
#define RKC_A "/tmp/rikitu-stage102-campaign-a.rkc"
#define RKC_B "/tmp/rikitu-stage102-campaign-b.rkc"
#define RKL_A "/tmp/rikitu-stage102-level-a.rkl"
#define RKL_B "/tmp/rikitu-stage102-level-b.rkl"

static void clean(const char *a,const char *b){
  (void)remove(a);
  (void)remove(b);
}
static void read_exact(const char *path,uint8_t *out,size_t size){
  FILE *f=fopen(path,"rb");
  assert(f);
  assert(fread(out,1,size,f)==size);
  assert(fgetc(f)==EOF);
  assert(fclose(f)==0);
}
static void unchanged(const char *path,const uint8_t *expected,size_t size){
  uint8_t local[RIKITU_LEVEL_SAVE_BYTES];
  assert(size<=sizeof(local));
  read_exact(path,local,size);
  assert(memcmp(local,expected,size)==0);
}
static void seed_flower(RikituScene *s,RikituLoop *l){
  rikitu_flower_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
static void seed_bridge(RikituScene *s,RikituLoop *l){
  rikitu_bridge_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
static void upgraded_flower(RikituScene *s,RikituLoop *l){
  s->active_checkpoint=s->checkpoints[0];
  s->reached_checkpoints=1;
  s->hearts=2;
  l->active_seconds=20.0f;
}
static void upgraded_bridge(RikituScene *s,RikituLoop *l){
  s->active_checkpoint=s->checkpoints[0];
  s->reached_checkpoints=1;
  s->hearts=2;
  s->stage.coins[0]=1;
  s->stage.coin_count=1;
  l->active_seconds=20.0f;
}
static void setup_blocked(const char *a,const char *b,int valid_in_b,
                          const char **valid,const char **blocked){
  *valid=valid_in_b?b:a;
  *blocked=valid_in_b?a:b;
  if(valid_in_b)assert(rename(a,b)==0);
  /* An empty directory cannot be truncated as a regular save file.
   * No platform-specific permissions or root/non-root assumptions. */
  assert(mkdir(*blocked,0700)==0);
}
static void unblock(const char *blocked){
  assert(rmdir(blocked)==0);
}

static void test_rks(int valid_in_b){
  RikituScene s,d;
  RikituLoop l,loaded;
  uint8_t old[RIKITU_SAVE_BYTES];
  const char *valid,*blocked;
  clean(RKS_A,RKS_B);
  seed_flower(&s,&l);
  assert(rikitu_save_store_pair(RKS_A,RKS_B,&s,&l));
  setup_blocked(RKS_A,RKS_B,valid_in_b,&valid,&blocked);
  read_exact(valid,old,sizeof(old));
  upgraded_flower(&s,&l);
  assert(!rikitu_save_store_pair(RKS_A,RKS_B,&s,&l));
  unchanged(valid,old,sizeof(old));
  seed_flower(&d,&loaded);
  assert(!rikitu_save_load_pair(RKS_A,RKS_B,&d,&loaded));
  assert(d.reached_checkpoints==0 && d.hearts==3);
  unblock(blocked);
  seed_flower(&d,&loaded);
  assert(rikitu_save_load_pair(RKS_A,RKS_B,&d,&loaded));
  assert(d.reached_checkpoints==0 && d.hearts==3);
  assert(rikitu_save_store_pair(RKS_A,RKS_B,&s,&l));
  unchanged(valid,old,sizeof(old));
  seed_flower(&d,&loaded);
  assert(rikitu_save_load_pair(RKS_A,RKS_B,&d,&loaded));
  assert(d.reached_checkpoints==1 && d.hearts==2);
  assert(d.active_checkpoint==s.active_checkpoint);
  clean(RKS_A,RKS_B);
  printf("PASS Stage102 RKS1 backup=%c: failed target open preserved original; retry restores checkpoint\n",
         valid_in_b?'B':'A');
}
static void test_rkc(int valid_in_b){
  RikituCampaign campaign,loaded;
  uint8_t old[RIKITU_CAMPAIGN_SAVE_BYTES];
  const char *valid,*blocked;
  clean(RKC_A,RKC_B);
  assert(rikitu_campaign_init(&campaign,4));
  campaign.wallet=157;
  assert(rikitu_campaign_save_store_pair(RKC_A,RKC_B,&campaign));
  setup_blocked(RKC_A,RKC_B,valid_in_b,&valid,&blocked);
  read_exact(valid,old,sizeof(old));
  campaign.wallet=158;
  assert(!rikitu_campaign_save_store_pair(RKC_A,RKC_B,&campaign));
  unchanged(valid,old,sizeof(old));
  memset(&loaded,0,sizeof(loaded));
  assert(!rikitu_campaign_save_load_pair(RKC_A,RKC_B,&loaded));
  assert(loaded.wallet==0 && loaded.level==0);
  unblock(blocked);
  memset(&loaded,0,sizeof(loaded));
  assert(rikitu_campaign_save_load_pair(RKC_A,RKC_B,&loaded));
  assert(loaded.wallet==157 && loaded.level==4);
  assert(rikitu_campaign_save_store_pair(RKC_A,RKC_B,&campaign));
  unchanged(valid,old,sizeof(old));
  memset(&loaded,0,sizeof(loaded));
  assert(rikitu_campaign_save_load_pair(RKC_A,RKC_B,&loaded));
  assert(loaded.wallet==158 && loaded.level==4);
  clean(RKC_A,RKC_B);
  printf("PASS Stage102 RKC2 backup=%c: failed target open preserved campaign; retry restores wallet\n",
         valid_in_b?'B':'A');
}
static void test_rkl(int valid_in_b){
  RikituScene s,d;
  RikituLoop l,loaded;
  RikituCampaign campaign;
  uint8_t old[RIKITU_LEVEL_SAVE_BYTES];
  const char *valid,*blocked;
  clean(RKL_A,RKL_B);
  assert(rikitu_campaign_init(&campaign,4));
  campaign.wallet=157;
  seed_bridge(&s,&l);
  assert(rikitu_level_save_store_pair(RKL_A,RKL_B,&s,&l,&campaign));
  setup_blocked(RKL_A,RKL_B,valid_in_b,&valid,&blocked);
  read_exact(valid,old,sizeof(old));
  upgraded_bridge(&s,&l);
  assert(!rikitu_level_save_store_pair(RKL_A,RKL_B,&s,&l,&campaign));
  unchanged(valid,old,sizeof(old));
  seed_bridge(&d,&loaded);
  assert(!rikitu_level_save_load_pair(RKL_A,RKL_B,&d,&loaded,&campaign));
  assert(d.reached_checkpoints==0 && d.hearts==3);
  assert(d.stage.coin_count==0);
  unblock(blocked);
  seed_bridge(&d,&loaded);
  assert(rikitu_level_save_load_pair(RKL_A,RKL_B,&d,&loaded,&campaign));
  assert(d.reached_checkpoints==0 && d.hearts==3);
  assert(d.stage.coin_count==0);
  assert(rikitu_level_save_store_pair(RKL_A,RKL_B,&s,&l,&campaign));
  unchanged(valid,old,sizeof(old));
  seed_bridge(&d,&loaded);
  assert(rikitu_level_save_load_pair(RKL_A,RKL_B,&d,&loaded,&campaign));
  assert(d.reached_checkpoints==1 && d.hearts==2);
  assert(d.stage.coin_count==1 && d.stage.coins[0]);
  clean(RKL_A,RKL_B);
  printf("PASS Stage102 RKL3 backup=%c: failed target open preserved checkpoint; retry restores progress\n",
         valid_in_b?'B':'A');
}
int main(void){
  int b;
  for(b=0;b<=1;b++){
    test_rks(b);
    test_rkc(b);
    test_rkl(b);
  }
  puts("PASS Stage102: six fail-fast writer/retry scenarios, no surviving-slot overwrite");
  return 0;
}
