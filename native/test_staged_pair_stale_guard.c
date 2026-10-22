/* Stage114: optimistic expected-generation compare under A/B pair lock.
 *
 * Three authentic save codecs x two A/B orientations x three histories:
 *   - gen1/gen2 -> gen3; stale gen2 writer rejected; fresh gen3 -> gen4
 *   - wrap UINT32_MAX/gen1; stale expected UINT32_MAX rejected;
 *     expected gen1 writes gen2 into older slot
 *   - no valid records; expected 0 creates gen1; duplicate expected 0
 *     rejected; fresh expected gen1 writes gen2.
 *
 * Rejected encoders are NEVER CALLED, .pending is not created, and both
 * live files stay byte-exact. Real production loaders verify outcomes.
 * Linux-only test of experimental pair writer. No shipping save I/O change.
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

enum { MAP=0,CAMPAIGN=1,LEVEL=2,STANDARD=0,WRAP=1,EMPTY=2 };
typedef struct {
  int family,phase,encode_calls;
  RikituCampaign campaign;
} Context;
static size_t bytes_for(int f){
  return f==MAP?RIKITU_SAVE_BYTES:
         f==CAMPAIGN?RIKITU_CAMPAIGN_SAVE_BYTES:RIKITU_LEVEL_SAVE_BYTES;
}
static void create_scene(int f,int phase,RikituScene *s,RikituLoop *l){
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
static int record_encode(uint8_t *out,size_t n,uint32_t gen,void *opaque){
  Context *ctx=(Context*)opaque;
  RikituScene s;
  RikituLoop l;
  RikituCampaign c=ctx->campaign;
  ctx->encode_calls++;
  if(n!=bytes_for(ctx->family))return 0;
  if(ctx->family==CAMPAIGN){
    c.wallet=157+ctx->phase;
    return rikitu_campaign_save_encode(out,&c,gen);
  }
  create_scene(ctx->family,ctx->phase,&s,&l);
  if(ctx->family==MAP)return rikitu_save_encode(out,&s,&l,gen);
  c.wallet=157;
  return rikitu_level_save_encode(out,&s,&l,&c,gen);
}
static int record_validate(const uint8_t *bytes,size_t n,
                           void *opaque,uint32_t *gen){
  Context *ctx=(Context*)opaque;
  RikituScene s;
  RikituLoop l;
  RikituCampaign c=ctx->campaign;
  if(ctx->family==MAP){
    create_scene(MAP,0,&s,&l);
    return rikitu_save_decode(&s,&l,bytes,n,gen);
  }
  if(ctx->family==CAMPAIGN)
    return rikitu_campaign_save_decode(&c,bytes,n,gen);
  create_scene(LEVEL,0,&s,&l);
  return rikitu_level_save_decode(&s,&l,&c,bytes,n,gen);
}
static void generate(Context *ctx,int phase,uint32_t gen,uint8_t *out){
  int before=ctx->phase;
  ctx->phase=phase;
  assert(record_encode(out,bytes_for(ctx->family),gen,ctx));
  ctx->phase=before;
}
static void write_bytes(const char *path,const uint8_t *bytes,size_t n){
  FILE *f=fopen(path,"wb");
  assert(f);
  assert(fwrite(bytes,1,n,f)==n&&fflush(f)==0);
  assert(fclose(f)==0);
}
static void assert_bytes(const char *path,const uint8_t *expected,size_t n){
  FILE *f=fopen(path,"rb");
  uint8_t got[RIKITU_LEVEL_SAVE_BYTES];
  assert(f&&n<=sizeof(got));
  assert(fread(got,1,n,f)==n&&fgetc(f)==EOF);
  assert(fclose(f)==0);
  assert(memcmp(expected,got,n)==0);
}
static void check_loader(Context *ctx,const char *a,const char *b,int phase){
  RikituScene s;
  RikituLoop l;
  RikituCampaign c=ctx->campaign;
  if(ctx->family==CAMPAIGN){
    assert(rikitu_campaign_save_load_pair(a,b,&c));
    assert(c.level==4&&c.wallet==157+phase);
  }else{
    create_scene(ctx->family,0,&s,&l);
    if(ctx->family==MAP)
      assert(rikitu_save_load_pair(a,b,&s,&l));
    else{
      assert(rikitu_level_save_load_pair(a,b,&s,&l,&c));
      assert(s.stage.coin_count==phase);
    }
    assert(s.reached_checkpoints==phase&&s.hearts==3-phase);
  }
}
static void create_paths(int f,char *a,char *b){
  const char *ext=f==MAP?"rks":f==CAMPAIGN?"rkc":"rkl";
  int count=snprintf(a,200,"/tmp/rikitu-stage114-%s-a.%s",ext,ext);
  assert(count>0&&count<200);
  count=snprintf(b,200,"/tmp/rikitu-stage114-%s-b.%s",ext,ext);
  assert(count>0&&count<200);
}
static void clean_path(const char *path){
  char s[240];
  int count;
  (void)remove(path);
  count=snprintf(s,sizeof(s),"%s.pending",path);
  assert(count>0&&count<(int)sizeof(s));(void)remove(s);
  count=snprintf(s,sizeof(s),"%s.pending.lock",path);
  assert(count>0&&count<(int)sizeof(s));(void)remove(s);
  count=snprintf(s,sizeof(s),"%s.pair.lock",path);
  assert(count>0&&count<(int)sizeof(s));(void)remove(s);
}
static void no_pending(const char *a,const char *b){
  struct stat st;
  char path[240];
  int n=snprintf(path,sizeof(path),"%s.pending",a);
  assert(n>0&&n<(int)sizeof(path));
  assert(stat(path,&st)!=0&&errno==ENOENT);
  n=snprintf(path,sizeof(path),"%s.pending",b);
  assert(n>0&&n<(int)sizeof(path));
  assert(stat(path,&st)!=0&&errno==ENOENT);
}
static void reject_stale(Context *ctx,const char *a,const char *b,
                         uint32_t stale_generation){
  uint32_t reported=0xf00df00du;
  int called=ctx->encode_calls;
  /* Reversal tests canonical lock identity in the same operation. */
  assert(!rikitu_staged_pair_commit_if_current(
    b,a,bytes_for(ctx->family),record_validate,record_encode,ctx,
    stale_generation,&reported));
  assert(reported==0xf00df00du);
  assert(ctx->encode_calls==called);
  no_pending(a,b);
}
static void run_case(int family,int initial_b,int scenario){
  char a[200],b[200];
  const char *newer=initial_b?b:a;
  const char *older=initial_b?a:b;
  uint8_t old[RIKITU_LEVEL_SAVE_BYTES],recent[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t next[RIKITU_LEVEL_SAVE_BYTES],last[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t reported=0;
  Context ctx;
  size_t n=bytes_for(family);
  memset(&ctx,0,sizeof(ctx));
  ctx.family=family;
  assert(rikitu_campaign_init(&ctx.campaign,4));
  ctx.campaign.wallet=157;
  create_paths(family,a,b);
  clean_path(a);clean_path(b);
  if(scenario==STANDARD){
    generate(&ctx,0,1,old);
    generate(&ctx,1,2,recent);
    generate(&ctx,2,3,next);
    generate(&ctx,1,4,last);
    write_bytes(older,old,n);
    write_bytes(newer,recent,n);
    ctx.phase=2;
    assert(rikitu_staged_pair_commit_if_current(
      a,b,n,record_validate,record_encode,&ctx,2,&reported));
    assert(reported==3);
    assert_bytes(older,next,n);
    assert_bytes(newer,recent,n);
    check_loader(&ctx,a,b,2);
    ctx.phase=0; /* stale progress must NEVER supersede gen3. */
    reject_stale(&ctx,a,b,2);
    assert_bytes(older,next,n);
    assert_bytes(newer,recent,n);
    ctx.phase=1;
    assert(rikitu_staged_pair_commit_if_current(
      a,b,n,record_validate,record_encode,&ctx,3,&reported));
    assert(reported==4);
    assert_bytes(older,next,n);
    assert_bytes(newer,last,n);
    check_loader(&ctx,a,b,1);
  }else if(scenario==WRAP){
    generate(&ctx,0,UINT32_MAX,old);
    generate(&ctx,1,1,recent);
    generate(&ctx,2,2,next);
    write_bytes(older,old,n);
    write_bytes(newer,recent,n);
    ctx.phase=0;
    reject_stale(&ctx,a,b,UINT32_MAX);
    assert_bytes(older,old,n);
    assert_bytes(newer,recent,n);
    ctx.phase=2;
    assert(rikitu_staged_pair_commit_if_current(
      a,b,n,record_validate,record_encode,&ctx,1,&reported));
    assert(reported==2);
    assert_bytes(older,next,n);
    assert_bytes(newer,recent,n);
    check_loader(&ctx,a,b,2);
  }else{
    ctx.phase=0;
    generate(&ctx,0,1,old);
    generate(&ctx,1,2,recent);
    /* First write is gen1 even when zero valid slots exist. */
    assert(rikitu_staged_pair_commit_if_current(
      a,b,n,record_validate,record_encode,&ctx,0,&reported));
    assert(reported==1);
    /* With empty pair, the staged implementation initially chooses A. */
    assert_bytes(a,old,n);
    ctx.phase=2;
    reject_stale(&ctx,a,b,0);
    assert_bytes(a,old,n);
    ctx.phase=1;
    assert(rikitu_staged_pair_commit_if_current(
      a,b,n,record_validate,record_encode,&ctx,1,&reported));
    assert(reported==2);
    assert_bytes(b,recent,n);
    check_loader(&ctx,a,b,1);
  }
  no_pending(a,b);
  clean_path(a);clean_path(b);
  printf("PASS Stage114 family=%d newer=%c case=%d guarded generations\n",
         family,initial_b?'B':'A',scenario);
}
int main(void){
  int f,b,s;
  for(f=MAP;f<=LEVEL;f++)
    for(b=0;b<2;b++)
      for(s=STANDARD;s<=EMPTY;s++)
        run_case(f,b,s);
  puts("PASS Stage114 18 stale-snapshot conflict/write protection cases");
  return 0;
}
