#include "rikitu_level_save.h"
#include "rikitu_mill.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define A "/tmp/rikitu-rkl3-regression-a.rkl"
#define B "/tmp/rikitu-rkl3-regression-b.rkl"
#define LEGACY "/tmp/rikitu-rkl3-regression-map01.rks"
#define CAMPAIGN "/tmp/rikitu-rkl3-regression-campaign.rkc"
static void reset(RikituScene *s,RikituLoop *l){
  rikitu_mill_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
int main(void){
  RikituScene s,fresh;
  RikituLoop loop,restored;
  RikituCampaign c,wrong;
  uint8_t raw[RIKITU_LEVEL_SAVE_BYTES],small[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t gen=0;
  FILE *f;
  int i;
  (void)remove(A);(void)remove(B);
  assert(rikitu_campaign_init(&c,2));
  c.wallet=190;
  reset(&s,&loop);
  assert(rikitu_level_save_encode(raw,&s,&loop,&c,7));
  assert(rikitu_level_save_inspect(raw,sizeof(raw),&gen) && gen==7);
  memcpy(small,raw,sizeof(small));small[14]^=1;
  assert(!rikitu_level_save_inspect(small,sizeof(small),NULL));
  assert(!rikitu_level_save_inspect(raw,sizeof(raw)-1,NULL));
  assert(!rikitu_level_save_load_pair(A,B,&s,&loop,&c));
  assert(!rikitu_level_save_store_pair(A,A,&s,&loop,&c));
  assert(rikitu_level_save_store_pair(A,B,&s,&loop,&c));
  /* A single valid RKL3 file cannot stand in for both recovery slots. */
  reset(&fresh,&restored);
  fresh.hearts=1;
  fresh.reached_checkpoints=2;
  restored.active_seconds=777.0f;
  restored.player.x=123.0f;
  assert(!rikitu_level_save_load_pair(A,A,&fresh,&restored,&c));
  assert(fresh.hearts==1 && fresh.reached_checkpoints==2 &&
         restored.active_seconds==777.0f && restored.player.x==123.0f);
  s.hearts=2;
  s.stage.coins[0]=1;s.stage.coin_count=1;
  s.active_checkpoint=s.checkpoints[0];s.reached_checkpoints=1;
  assert(rikitu_level_save_store_pair(A,B,&s,&loop,&c));
  s.hearts=1;
  s.stage.coins[11]=1;s.stage.coin_count=2;
  assert(rikitu_level_save_store_pair(A,B,&s,&loop,&c));
  reset(&fresh,&restored);
  assert(rikitu_level_save_load_pair(A,B,&fresh,&restored,&c));
  assert(fresh.hearts==1 && fresh.reached_checkpoints==1 &&
         fresh.stage.coin_count==2);
  assert(fresh.stage.coins[0]&&fresh.stage.coins[11]);
  /* Corrupt newest slot A (third save) and recover older B (second). */
  f=fopen(A,"r+b");assert(f);
  assert(fseek(f,14,SEEK_SET)==0 && fputc(255,f)!=EOF);
  assert(fclose(f)==0);
  reset(&fresh,&restored);
  assert(rikitu_level_save_load_pair(A,B,&fresh,&restored,&c));
  assert(fresh.hearts==2 && fresh.stage.coin_count==1);
  /* A truncated write cannot destroy valid slot B. */
  f=fopen(A,"wb");assert(f);
  for(i=0;i<5;i++)assert(fputc('X',f)!=EOF);
  assert(fclose(f)==0);
  reset(&fresh,&restored);
  assert(rikitu_level_save_load_pair(A,B,&fresh,&restored,&c));
  assert(fresh.hearts==2);
  wrong=c;wrong.wallet++;
  reset(&fresh,&restored);
  assert(!rikitu_level_save_load_pair(A,B,&fresh,&restored,&wrong));
  assert(fresh.hearts==3 && fresh.reached_checkpoints==0);
  /* The RKL3 writer rejects both older save families even on disk. */
  f=fopen(LEGACY,"wb");assert(f);
  assert(fwrite("RKS1-KEEP",1,9,f)==9 && fclose(f)==0);
  f=fopen(CAMPAIGN,"wb");assert(f);
  assert(fwrite("RKC2-KEEP",1,9,f)==9 && fclose(f)==0);
  assert(!rikitu_level_save_store_pair(LEGACY,B,&s,&loop,&c));
  assert(!rikitu_level_save_store_pair(CAMPAIGN,B,&s,&loop,&c));
  assert(!rikitu_level_save_load_pair(LEGACY,B,&fresh,&restored,&c));
  f=fopen(LEGACY,"rb");assert(f);
  assert(fread(small,1,9,f)==9&&fclose(f)==0);
  assert(memcmp(small,"RKS1-KEEP",9)==0);
  f=fopen(CAMPAIGN,"rb");assert(f);
  assert(fread(small,1,9,f)==9&&fclose(f)==0);
  assert(memcmp(small,"RKC2-KEEP",9)==0);
  /* One good current wallet checkpoint (A, gen10), alongside a
   * CRC-valid but foreign wallet save (B, gen500). The writer MUST
   * replace the foreign slot B, preserving A as a crash fallback.
   * The previous generation-only selection incorrectly destroyed A. */
  {
    RikituCampaign foreign;
    uint8_t valid[RIKITU_LEVEL_SAVE_BYTES];
    uint8_t other[RIKITU_LEVEL_SAVE_BYTES];
    uint8_t observed[RIKITU_LEVEL_SAVE_BYTES];
    foreign=c;
    foreign.wallet++;
    reset(&s,&loop);
    assert(rikitu_level_save_encode(valid,&s,&loop,&c,10));
    assert(rikitu_level_save_encode(other,&s,&loop,&foreign,500));
    f=fopen(A,"wb");assert(f);
    assert(fwrite(valid,1,sizeof(valid),f)==sizeof(valid));
    assert(fclose(f)==0);
    f=fopen(B,"wb");assert(f);
    assert(fwrite(other,1,sizeof(other),f)==sizeof(other));
    assert(fclose(f)==0);
    s.stage.coins[0]=1;s.stage.coin_count=1;
    assert(rikitu_level_save_store_pair(A,B,&s,&loop,&c));
    f=fopen(A,"rb");assert(f);
    assert(fread(observed,1,sizeof(observed),f)==sizeof(observed));
    assert(fclose(f)==0);
    assert(memcmp(observed,valid,sizeof(valid))==0);
    /* If latest B tears after A survived, it must restore A. */
    f=fopen(B,"r+b");assert(f);
    assert(fseek(f,15,SEEK_SET)==0 && fputc(255,f)!=EOF);
    assert(fclose(f)==0);
    reset(&fresh,&restored);
    assert(rikitu_level_save_load_pair(A,B,&fresh,&restored,&c));
    assert(fresh.stage.coin_count==0 && fresh.hearts==3);
  }
  /* Mirror the cross-wallet collision: active save lives in B, while
   * a foreign record in A has a much newer generation. Keeping B
   * intact is crucial if the first write to A is interrupted. */
  {
    RikituCampaign foreign;
    uint8_t good[RIKITU_LEVEL_SAVE_BYTES];
    uint8_t stale[RIKITU_LEVEL_SAVE_BYTES];
    uint8_t observed[RIKITU_LEVEL_SAVE_BYTES];
    uint32_t generation=0;
    foreign=c;foreign.wallet++;
    reset(&s,&loop);
    assert(rikitu_level_save_encode(good,&s,&loop,&c,23));
    assert(rikitu_level_save_encode(stale,&s,&loop,&foreign,900));
    f=fopen(A,"wb");assert(f);
    assert(fwrite(stale,1,sizeof(stale),f)==sizeof(stale));
    assert(fclose(f)==0);
    f=fopen(B,"wb");assert(f);
    assert(fwrite(good,1,sizeof(good),f)==sizeof(good));
    assert(fclose(f)==0);
    s.stage.coins[0]=1;s.stage.coin_count=1;
    assert(rikitu_level_save_store_pair(A,B,&s,&loop,&c));
    f=fopen(B,"rb");assert(f);
    assert(fread(observed,1,sizeof(observed),f)==sizeof(observed));
    assert(fclose(f)==0);
    assert(memcmp(observed,good,sizeof(good))==0);
    f=fopen(A,"rb");assert(f);
    assert(fread(observed,1,sizeof(observed),f)==sizeof(observed));
    assert(fclose(f)==0);
    assert(rikitu_level_save_inspect(observed,sizeof(observed),
                                     &generation) && generation==24);
    reset(&fresh,&restored);
    assert(rikitu_level_save_load_pair(A,B,&fresh,&restored,&c));
    assert(fresh.stage.coin_count==1 && fresh.stage.coins[0]);
    /* A failed new write must leave the older B recoverable. */
    f=fopen(A,"r+b");assert(f);
    assert(fseek(f,14,SEEK_SET)==0 && fputc(255,f)!=EOF);
    assert(fclose(f)==0);
    reset(&fresh,&restored);
    assert(rikitu_level_save_load_pair(A,B,&fresh,&restored,&c));
    assert(fresh.stage.coin_count==0 && fresh.hearts==3);
  }
  (void)remove(A);(void)remove(B);
  (void)remove(LEGACY);(void)remove(CAMPAIGN);
  puts("PASS RKL3 dual-slot: CRC, pickup resume, torn write and RKS1/RKC2 isolation");
  return 0;
}
