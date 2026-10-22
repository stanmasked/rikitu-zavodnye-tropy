/* Stage130: an unreadable slot must never be treated as empty/corrupt.
 * Host-only --wrap=fopen injects EACCES for rb access while wb still works.
 * Both load and store must fail closed and preserve both valid slot files.
 * This is not a physical Switch SDMC/power-loss test.
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
#include <string.h>
#include <unistd.h>
#define MAP_A "/tmp/rikitu-stage130-map-a.rks"
#define MAP_B "/tmp/rikitu-stage130-map-b.rks"
#define CAM_A "/tmp/rikitu-stage130-campaign-a.rkc"
#define CAM_B "/tmp/rikitu-stage130-campaign-b.rkc"
#define LVL_A "/tmp/rikitu-stage130-level-a.rkl"
#define LVL_B "/tmp/rikitu-stage130-level-b.rkl"
static const char *denied_path;
static int deny_read;
extern FILE *__real_fopen(const char *,const char *);
FILE *__wrap_fopen(const char *path,const char *mode){
  if(deny_read&&path&&mode&&strcmp(path,denied_path)==0&&strcmp(mode,"rb")==0){
    errno=EACCES;return NULL;
  }
  return __real_fopen(path,mode);
}
static void read_exact(const char *path,uint8_t *out,size_t n){
  FILE *f=__real_fopen(path,"rb");assert(f);
  assert(fread(out,1,n,f)==n&&fgetc(f)==EOF&&fclose(f)==0);
}
static void same_bytes(const char *path,const uint8_t *expected,size_t n){
  uint8_t got[RIKITU_LEVEL_SAVE_BYTES];assert(n<=sizeof(got));
  read_exact(path,got,n);assert(memcmp(got,expected,n)==0);
}
static void snapshot_pair(const char *a,const char *b,size_t n,uint8_t *x,uint8_t *y){
  read_exact(a,x,n);read_exact(b,y,n);
}
static void verify_pair(const char *a,const char *b,size_t n,const uint8_t *x,const uint8_t *y){
  same_bytes(a,x,n);same_bytes(b,y,n);
}
static void flower(RikituScene *s,RikituLoop *l){
  rikitu_flower_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
static void bridge(RikituScene *s,RikituLoop *l){
  rikitu_bridge_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
static void deny(const char *path){denied_path=path;deny_read=1;}
static void allow_reads(void){deny_read=0;denied_path=NULL;}
static void test_map(int block_b){
  RikituScene s,loaded,before;RikituLoop l,loaded_loop,before_loop;
  uint8_t old_a[RIKITU_SAVE_BYTES],old_b[RIKITU_SAVE_BYTES];
  const char *blocked=block_b?MAP_B:MAP_A;
  (void)remove(MAP_A);(void)remove(MAP_B);allow_reads();
  flower(&s,&l);assert(rikitu_save_store_pair(MAP_A,MAP_B,&s,&l));
  s.reached_checkpoints=1;s.active_checkpoint=s.checkpoints[0];s.hearts=2;l.active_seconds=20.0f;
  assert(rikitu_save_store_pair(MAP_A,MAP_B,&s,&l));
  snapshot_pair(MAP_A,MAP_B,sizeof(old_a),old_a,old_b);
  flower(&loaded,&loaded_loop);loaded.hearts=1;loaded_loop.active_seconds=777.0f;
  before=loaded;before_loop=loaded_loop;deny(blocked);
  assert(!rikitu_save_load_pair(MAP_A,MAP_B,&loaded,&loaded_loop));
  assert(!rikitu_save_store_pair(MAP_A,MAP_B,&s,&l));allow_reads();
  assert(memcmp(&loaded,&before,sizeof(before))==0);
  assert(memcmp(&loaded_loop,&before_loop,sizeof(before_loop))==0);
  verify_pair(MAP_A,MAP_B,sizeof(old_a),old_a,old_b);
  assert(rikitu_save_load_pair(MAP_A,MAP_B,&loaded,&loaded_loop));
  assert(loaded.reached_checkpoints==1&&loaded.hearts==2);
  (void)remove(MAP_A);(void)remove(MAP_B);
}
static void test_campaign(int block_b){
  RikituCampaign c,loaded,before;
  uint8_t old_a[RIKITU_CAMPAIGN_SAVE_BYTES],old_b[RIKITU_CAMPAIGN_SAVE_BYTES];
  const char *blocked=block_b?CAM_B:CAM_A;
  (void)remove(CAM_A);(void)remove(CAM_B);allow_reads();
  assert(rikitu_campaign_init(&c,4));c.wallet=157;
  assert(rikitu_campaign_save_store_pair(CAM_A,CAM_B,&c));c.wallet=158;
  assert(rikitu_campaign_save_store_pair(CAM_A,CAM_B,&c));
  snapshot_pair(CAM_A,CAM_B,sizeof(old_a),old_a,old_b);
  assert(rikitu_campaign_init(&loaded,4));loaded.wallet=999;before=loaded;deny(blocked);
  assert(!rikitu_campaign_save_load_pair(CAM_A,CAM_B,&loaded));
  c.wallet=159;assert(!rikitu_campaign_save_store_pair(CAM_A,CAM_B,&c));allow_reads();
  assert(memcmp(&loaded,&before,sizeof(before))==0);
  verify_pair(CAM_A,CAM_B,sizeof(old_a),old_a,old_b);
  assert(rikitu_campaign_save_load_pair(CAM_A,CAM_B,&loaded));
  assert(loaded.wallet==158&&loaded.level==4);
  (void)remove(CAM_A);(void)remove(CAM_B);
}
static void test_level(int block_b){
  RikituCampaign c;RikituScene s,loaded,before;RikituLoop l,loaded_loop,before_loop;
  uint8_t old_a[RIKITU_LEVEL_SAVE_BYTES],old_b[RIKITU_LEVEL_SAVE_BYTES];
  const char *blocked=block_b?LVL_B:LVL_A;
  (void)remove(LVL_A);(void)remove(LVL_B);allow_reads();
  assert(rikitu_campaign_init(&c,4));c.wallet=157;bridge(&s,&l);
  assert(rikitu_level_save_store_pair(LVL_A,LVL_B,&s,&l,&c));
  s.reached_checkpoints=1;s.active_checkpoint=s.checkpoints[0];s.hearts=2;
  s.stage.coin_count=1;s.stage.coins[0]=1;l.active_seconds=20.0f;
  assert(rikitu_level_save_store_pair(LVL_A,LVL_B,&s,&l,&c));
  snapshot_pair(LVL_A,LVL_B,sizeof(old_a),old_a,old_b);
  bridge(&loaded,&loaded_loop);loaded.hearts=1;loaded_loop.active_seconds=777.0f;
  before=loaded;before_loop=loaded_loop;deny(blocked);
  assert(!rikitu_level_save_load_pair(LVL_A,LVL_B,&loaded,&loaded_loop,&c));
  assert(!rikitu_level_save_store_pair(LVL_A,LVL_B,&s,&l,&c));allow_reads();
  assert(memcmp(&loaded,&before,sizeof(before))==0);
  assert(memcmp(&loaded_loop,&before_loop,sizeof(before_loop))==0);
  verify_pair(LVL_A,LVL_B,sizeof(old_a),old_a,old_b);
  assert(rikitu_level_save_load_pair(LVL_A,LVL_B,&loaded,&loaded_loop,&c));
  assert(loaded.reached_checkpoints==1&&loaded.hearts==2);
  assert(loaded.stage.coin_count==1&&loaded.stage.coins[0]==1);
  (void)remove(LVL_A);(void)remove(LVL_B);
}
int main(void){
  int block_b;for(block_b=0;block_b<=1;block_b++){
    test_map(block_b);test_campaign(block_b);test_level(block_b);
  }
  puts("PASS Stage130: 12 unreadable-slot fail-closed checks; RKS1/RKC2/RKL3 A/B");
  return 0;
}
