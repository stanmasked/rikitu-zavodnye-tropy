/* Stage113: real RKS1/RKC2/RKL3 wrap-safe A/B pair election, including
 * RKL3 foreign-campaign rejection and pair-lock symlink protection.
 *
 * Experimental Linux-only API, unchanged production pair loaders.
 * No Switch production writer is modified by this fixture.
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum { MAP=0,CAMPAIGN=1,LEVEL=2 };
typedef struct {int family;int phase;RikituCampaign campaign;} Context;
static size_t record_size(int f){
  return f==MAP?RIKITU_SAVE_BYTES:
    f==CAMPAIGN?RIKITU_CAMPAIGN_SAVE_BYTES:RIKITU_LEVEL_SAVE_BYTES;
}
static void scene(int f,int phase,RikituScene *s,RikituLoop *l){
  int i;
  if(f==MAP)rikitu_flower_init(s);else rikitu_bridge_init(s);
  memset(l,0,sizeof(*l));l->player=s->player;
  if(phase){
    s->reached_checkpoints=phase;
    s->active_checkpoint=s->checkpoints[phase-1];
    s->hearts=3-phase;
    if(f==LEVEL){
      s->stage.coin_count=phase;
      for(i=0;i<phase;i++)s->stage.coins[i]=1;
    }
  }
}
static int encode(uint8_t *bytes,size_t n,uint32_t gen,void *user){
  Context *ctx=(Context*)user;
  RikituScene s;
  RikituLoop l;
  RikituCampaign c=ctx->campaign;
  if(n!=record_size(ctx->family))return 0;
  if(ctx->family==CAMPAIGN){
    c.wallet=157+ctx->phase;
    return rikitu_campaign_save_encode(bytes,&c,gen);
  }
  scene(ctx->family,ctx->phase,&s,&l);
  if(ctx->family==MAP)return rikitu_save_encode(bytes,&s,&l,gen);
  return rikitu_level_save_encode(bytes,&s,&l,&c,gen);
}
static int valid(const uint8_t *bytes,size_t n,void *user,uint32_t *gen){
  Context *ctx=(Context*)user;
  RikituScene s;
  RikituLoop l;
  RikituCampaign c=ctx->campaign;
  if(ctx->family==MAP){
    scene(MAP,0,&s,&l);
    return rikitu_save_decode(&s,&l,bytes,n,gen);
  }
  if(ctx->family==CAMPAIGN)
    return rikitu_campaign_save_decode(&c,bytes,n,gen);
  /* Unlike a CRC-only inspection, decode rejects a foreign wallet. */
  scene(LEVEL,0,&s,&l);
  return rikitu_level_save_decode(&s,&l,&c,bytes,n,gen);
}
static void make_record(Context *ctx,int phase,uint32_t gen,
                        uint8_t out[RIKITU_LEVEL_SAVE_BYTES]){
  int saved=ctx->phase;
  ctx->phase=phase;
  assert(encode(out,record_size(ctx->family),gen,ctx));
  ctx->phase=saved;
}
static void write_file(const char *path,const uint8_t *bytes,size_t n){
  FILE *f=fopen(path,"wb");
  assert(f);
  assert(fwrite(bytes,1,n,f)==n && fflush(f)==0);
  assert(fclose(f)==0);
}
static void assert_file(const char *path,const uint8_t *bytes,size_t n){
  uint8_t got[RIKITU_LEVEL_SAVE_BYTES];
  FILE *f=fopen(path,"rb");
  assert(f&&n<=sizeof(got));
  assert(fread(got,1,n,f)==n&&fgetc(f)==EOF);
  assert(fclose(f)==0);
  assert(memcmp(bytes,got,n)==0);
}
static void assert_loaded(int f,const char *a,const char *b,
                          Context *ctx,int phase){
  RikituScene s;RikituLoop l;RikituCampaign c=ctx->campaign;
  if(f==CAMPAIGN){
    assert(rikitu_campaign_save_load_pair(a,b,&c));
    assert(c.wallet==157+phase && c.level==4);
  }else{
    scene(f,0,&s,&l);
    if(f==MAP)assert(rikitu_save_load_pair(a,b,&s,&l));
    else{
      assert(rikitu_level_save_load_pair(a,b,&s,&l,&c));
      assert(s.stage.coin_count==phase);
    }
    assert(s.reached_checkpoints==phase&&s.hearts==3-phase);
  }
}
static void paths(int family,char a[200],char b[200],char lock[240]){
  const char *ext=family==MAP?"rks":family==CAMPAIGN?"rkc":"rkl";
  int count=snprintf(a,200,"/tmp/rikitu-stage113-%s-a.%s",ext,ext);
  assert(count>0&&count<200);
  count=snprintf(b,200,"/tmp/rikitu-stage113-%s-b.%s",ext,ext);
  assert(count>0&&count<200);
  count=snprintf(lock,240,"%s.pair.lock",a);
  assert(count>0&&count<240);
}
static void clean(const char *a,const char *b,const char *lock){
  char buf[240];int n;
  (void)remove(a);(void)remove(b);(void)remove(lock);
  n=snprintf(buf,sizeof(buf),"%s.pending",a);
  assert(n>0&&n<(int)sizeof(buf));(void)remove(buf);
  n=snprintf(buf,sizeof(buf),"%s.pending.lock",a);
  assert(n>0&&n<(int)sizeof(buf));(void)remove(buf);
  n=snprintf(buf,sizeof(buf),"%s.pending",b);
  assert(n>0&&n<(int)sizeof(buf));(void)remove(buf);
  n=snprintf(buf,sizeof(buf),"%s.pending.lock",b);
  assert(n>0&&n<(int)sizeof(buf));(void)remove(buf);
}
static void init_ctx(Context *ctx,int family){
  memset(ctx,0,sizeof(*ctx));
  ctx->family=family;
  assert(rikitu_campaign_init(&ctx->campaign,4));
  ctx->campaign.wallet=157;
}
static void test_wrap(int family,int max_b){
  char a[200],b[200],lock[240];
  const char *max_slot,*one_slot;
  uint8_t max_record[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t one_record[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t two_record[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t generation=0;
  Context ctx;
  size_t n=record_size(family);
  paths(family,a,b,lock);clean(a,b,lock);init_ctx(&ctx,family);
  max_slot=max_b?b:a;one_slot=max_b?a:b;
  make_record(&ctx,0,UINT32_MAX,max_record);
  make_record(&ctx,1,1,one_record);
  make_record(&ctx,2,2,two_record);
  write_file(max_slot,max_record,n);
  write_file(one_slot,one_record,n);
  ctx.phase=2;
  assert(rikitu_staged_pair_commit(b,a,n,valid,encode,&ctx,&generation));
  assert(generation==2);
  assert_file(max_slot,two_record,n);
  assert_file(one_slot,one_record,n);
  assert_loaded(family,a,b,&ctx,2);
  clean(a,b,lock);
  printf("PASS Stage113 wrap family=%d max=%c: gen2 replaces uint32 max\n",
         family,max_b?'B':'A');
}
static void test_foreign_wallet(int foreign_b){
  char a[200],b[200],lock[240];
  const char *foreign,*current;
  uint8_t bad[RIKITU_LEVEL_SAVE_BYTES],good[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t updated[RIKITU_LEVEL_SAVE_BYTES];
  Context ctx,alien;
  uint32_t generation=0;
  paths(LEVEL,a,b,lock);clean(a,b,lock);init_ctx(&ctx,LEVEL);
  foreign=foreign_b?b:a;current=foreign_b?a:b;
  alien=ctx;alien.campaign.wallet=158;
  make_record(&alien,1,1000,bad);
  make_record(&ctx,1,4,good);
  make_record(&ctx,2,5,updated);
  write_file(foreign,bad,sizeof(bad));
  write_file(current,good,sizeof(good));
  ctx.phase=2;
  assert(rikitu_staged_pair_commit(a,b,sizeof(updated),valid,encode,
                                   &ctx,&generation));
  assert(generation==5);
  assert_file(foreign,updated,sizeof(updated));
  assert_file(current,good,sizeof(good));
  assert_loaded(LEVEL,a,b,&ctx,2);
  clean(a,b,lock);
  printf("PASS Stage113 foreign-wallet=%c: ignored CRC-valid gen1000\n",
         foreign_b?'B':'A');
}
static void test_lock_symlink(int family,int victim_b){
  char a[200],b[200],lock[240];
  const char *victim;
  uint8_t older[RIKITU_LEVEL_SAVE_BYTES],backup[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t updated[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t generation=0;
  struct stat st;
  Context ctx;
  size_t n=record_size(family);
  paths(family,a,b,lock);clean(a,b,lock);init_ctx(&ctx,family);
  make_record(&ctx,0,1,older);
  make_record(&ctx,1,2,backup);
  make_record(&ctx,2,3,updated);
  write_file(a,older,n);write_file(b,backup,n);
  victim=victim_b?b:a;
  assert(symlink(victim,lock)==0);
  assert(lstat(lock,&st)==0&&S_ISLNK(st.st_mode));
  ctx.phase=2;
  assert(!rikitu_staged_pair_commit(a,b,n,valid,encode,&ctx,&generation));
  assert(lstat(lock,&st)==0&&S_ISLNK(st.st_mode));
  assert_file(a,older,n);assert_file(b,backup,n);
  assert(unlink(lock)==0);
  assert(rikitu_staged_pair_commit(a,b,n,valid,encode,&ctx,&generation));
  assert(generation==3);
  assert_file(a,updated,n);assert_file(b,backup,n);
  assert_loaded(family,a,b,&ctx,2);
  clean(a,b,lock);
  printf("PASS Stage113 lock-symlink family=%d victim=%c\n",
         family,victim_b?'B':'A');
}
int main(void){
  int f,b;
  for(f=MAP;f<=LEVEL;f++)
    for(b=0;b<2;b++){
      test_wrap(f,b);
      test_lock_symlink(f,b);
    }
  for(b=0;b<2;b++)test_foreign_wallet(b);
  puts("PASS Stage113: 14 wrap/identity/lock-symlink cases");
  return 0;
}
