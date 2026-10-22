/* Stage100: RKC2 campaign and RKL3 checkpoint are independently committed.
 * Exercise an interruption BETWEEN commits, including the reverse rollback.
 * Verify the production readers never load a CRC-valid, high-generation
 * checkpoint from a different chapter or wallet identity. No filesystem
 * power cut is simulated. */
#include "rikitu_grove.h"
#include "rikitu_bridge.h"
#include "rikitu_campaign_save.h"
#include "rikitu_level_save.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CAM_A "/tmp/rikitu-stage100-campaign-a.rkc"
#define CAM_B "/tmp/rikitu-stage100-campaign-b.rkc"
#define LVL_A "/tmp/rikitu-stage100-level-a.rkl"
#define LVL_B "/tmp/rikitu-stage100-level-b.rkl"

static uint32_t record_crc32(const uint8_t *p,size_t n){
  uint32_t value=0xffffffffu;
  size_t i;unsigned bit;
  for(i=0;i<n;i++){
    value^=p[i];
    for(bit=0;bit<8;bit++)
      value=(value>>1)^((value&1u)?0xedb88320u:0u);
  }
  return ~value;
}
static void put32le(uint8_t *p,uint32_t value){
  unsigned i;
  for(i=0;i<4;i++)p[i]=(uint8_t)(value>>(8u*i));
}
static void clean(void){
  (void)remove(CAM_A);(void)remove(CAM_B);
  (void)remove(LVL_A);(void)remove(LVL_B);
}
static void chapter(RikituCampaign *c,int level,int wallet){
  assert(rikitu_campaign_init(c,level));
  c->wallet=wallet;
  assert(c->mode==RIKITU_CAMPAIGN_PLAYING);
  assert(rikitu_campaign_valid(c));
}
static void fresh(RikituScene *s,RikituLoop *l,const RikituCampaign *c){
  assert(c&&c->mode==RIKITU_CAMPAIGN_PLAYING);
  if(c->level==3)rikitu_grove_init(s);
  else if(c->level==4)rikitu_bridge_init(s);
  else assert(!"Unexpected chapter");
  s->hearts=s->max_hearts=c->max_hearts;
  memset(l,0,sizeof(*l));
  l->player=s->player;
  assert(s->level_id==c->level && s->stage.coin_count==0);
}
static void checkpoint(RikituScene *s,RikituLoop *l,int index,int hearts){
  int i;
  assert(index>=1 && index<=2);
  s->active_checkpoint=s->checkpoints[index-1];
  s->reached_checkpoints=index;
  s->hearts=hearts;
  for(i=0;i<index;i++)s->stage.coins[i]=1;
  s->stage.coin_count=index;
  l->active_seconds=(float)(index*20);
}
static void snapshot(const char *p,uint8_t *data,size_t size){
  FILE *f=fopen(p,"rb");
  assert(f);
  assert(fread(data,1,size,f)==size && fgetc(f)==EOF);
  assert(fclose(f)==0);
}
static void corrupt(const char *p){
  FILE *f=fopen(p,"r+b");
  int first;
  assert(f);
  assert(fseek(f,12,SEEK_SET)==0);
  first=fgetc(f);
  assert(first!=EOF);
  assert(fseek(f,12,SEEK_SET)==0);
  assert(fputc(first^0x40,f)!=EOF && fclose(f)==0);
}
static void restore_campaign(RikituCampaign *c,int level,int wallet){
  memset(c,0,sizeof(*c));
  assert(rikitu_campaign_save_load_pair(CAM_A,CAM_B,c));
  assert(c->level==level && c->wallet==wallet &&
         c->mode==RIKITU_CAMPAIGN_PLAYING);
}
static void restore_level(RikituScene *s,RikituLoop *l,
                          const RikituCampaign *c,int checkpoint_index,
                          int hearts){
  fresh(s,l,c);
  assert(rikitu_level_save_load_pair(LVL_A,LVL_B,s,l,c));
  assert(s->reached_checkpoints==checkpoint_index);
  assert(s->hearts==hearts && s->stage.coin_count==checkpoint_index);
}
static void reject_foreign_level(RikituScene *s,RikituLoop *l,
                                 const RikituCampaign *c){
  fresh(s,l,c);
  assert(!rikitu_level_save_load_pair(LVL_A,LVL_B,s,l,c));
  /* Rejection must not partially mutate the chapter's live scene. */
  assert(s->level_id==c->level && s->stage.coin_count==0);
  assert(s->reached_checkpoints==0 && s->hearts==c->max_hearts);
}
int main(void){
  RikituCampaign c3,c4,recovered,changed_wallet;
  RikituScene s;
  RikituLoop l;
  uint8_t old_checkpoint[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t restored_backup[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t gen=0;

  clean();
  chapter(&c3,3,120);
  assert(rikitu_campaign_save_store_pair(CAM_A,CAM_B,&c3));
  assert(rikitu_campaign_save_store_pair(CAM_A,CAM_B,&c3));
  fresh(&s,&l,&c3);
  checkpoint(&s,&l,1,3);
  assert(rikitu_level_save_store_pair(LVL_A,LVL_B,&s,&l,&c3));
  checkpoint(&s,&l,2,2);
  assert(rikitu_level_save_store_pair(LVL_A,LVL_B,&s,&l,&c3));
  snapshot(LVL_B,old_checkpoint,sizeof(old_checkpoint));
  assert(rikitu_level_save_inspect(old_checkpoint,
                                   sizeof(old_checkpoint),&gen)&&gen==2);
  restore_campaign(&recovered,3,120);
  restore_level(&s,&l,&recovered,2,2);

  /* Interruption #1: RKC2 now says MAP04 but RKL3 still says MAP03.
   * Even a CRC-valid MAP03 level generation 2 must NOT load in MAP04. */
  chapter(&c4,4,120); /* Same wallet: chapter identity alone must reject. */
  assert(rikitu_campaign_save_store_pair(CAM_A,CAM_B,&c4));
  restore_campaign(&recovered,4,120);
  reject_foreign_level(&s,&l,&recovered);
  /* New MAP04 checkpoint writes to foreign A, keeping old MAP03 B. */
  checkpoint(&s,&l,1,3);
  assert(rikitu_level_save_store_pair(LVL_A,LVL_B,&s,&l,&recovered));
  snapshot(LVL_B,restored_backup,sizeof(restored_backup));
  assert(memcmp(restored_backup,old_checkpoint,sizeof(old_checkpoint))==0);
  restore_level(&s,&l,&recovered,1,3);

  /* Interruption #2: latest RKC2 is torn; older MAP03 RKC2 B survives.
   * The newer MAP04 RKL3 A becomes foreign. The surviving MAP03 RKL3 B
   * must be restored, despite the mixed generations and same wallet. */
  corrupt(CAM_A);
  restore_campaign(&recovered,3,120);
  restore_level(&s,&l,&recovered,2,2);

  /* A repaired MAP03 checkpoint must replace foreign A rather than
   * overwrite its only surviving matching backup in B. */
  checkpoint(&s,&l,2,1);
  assert(rikitu_level_save_store_pair(LVL_A,LVL_B,&s,&l,&recovered));
  snapshot(LVL_B,restored_backup,sizeof(restored_backup));
  assert(memcmp(restored_backup,old_checkpoint,sizeof(old_checkpoint))==0);
  restore_level(&s,&l,&recovered,2,1);

  /* Interruption #3: same MAP03 but different wallet. A checkpoint is
   * bound to the campaign's wallet/heart identity, not just level number. */
  changed_wallet=recovered;changed_wallet.wallet=121;
  assert(rikitu_campaign_valid(&changed_wallet));
  assert(rikitu_campaign_save_store_pair(CAM_A,CAM_B,&changed_wallet));
  restore_campaign(&recovered,3,121);
  reject_foreign_level(&s,&l,&recovered);
  checkpoint(&s,&l,1,3);
  assert(rikitu_level_save_store_pair(LVL_A,LVL_B,&s,&l,&recovered));
  snapshot(LVL_B,restored_backup,sizeof(restored_backup));
  assert(memcmp(restored_backup,old_checkpoint,sizeof(old_checkpoint))==0);
  restore_level(&s,&l,&recovered,1,3);

  /* Roll back the new RKC2. New wallet121 RKL3 A must be rejected and
   * old wallet120 RKL3 B must still restore the actual MAP03 progress. */
  corrupt(CAM_A);
  restore_campaign(&recovered,3,120);
  restore_level(&s,&l,&recovered,2,2);
  puts("PASS Stage100 RKC2/RKL3 interrupted ordering: chapter and wallet isolation, rollback and surviving backup");

  /* Stage132: a CRC-valid but semantically impossible record with the
   * right campaign identity and a much higher generation must not outrank
   * the last fully restorable checkpoint during a production write. */
  {
    RikituScene invalid_scene,loaded;
    RikituLoop invalid_loop,loaded_loop;
    uint8_t good[RIKITU_LEVEL_SAVE_BYTES];
    uint8_t invalid[RIKITU_LEVEL_SAVE_BYTES];
    uint8_t observed[RIKITU_LEVEL_SAVE_BYTES];
    uint32_t generation=0;
    FILE *f;
    clean();
    chapter(&c3,3,120); /* Odd-numbered level: no boss exists. */
    fresh(&s,&l,&c3);
    checkpoint(&s,&l,1,3);
    assert(rikitu_level_save_encode(good,&s,&l,&c3,10));
    memcpy(invalid,good,sizeof(invalid));
    invalid[9]=1; /* CRC-valid but impossible non-boss HP. */
    put32le(invalid+32,500);
    put32le(invalid+60,record_crc32(invalid,60));
    assert(rikitu_level_save_inspect(invalid,sizeof(invalid),&generation)&&
           generation==500);
    fresh(&invalid_scene,&invalid_loop,&c3);
    assert(!rikitu_level_save_decode(&invalid_scene,&invalid_loop,&c3,
                                     invalid,sizeof(invalid),NULL));
    f=fopen(LVL_A,"wb");assert(f);
    assert(fwrite(good,1,sizeof(good),f)==sizeof(good));
    assert(fclose(f)==0);
    f=fopen(LVL_B,"wb");assert(f);
    assert(fwrite(invalid,1,sizeof(invalid),f)==sizeof(invalid));
    assert(fclose(f)==0);

    checkpoint(&s,&l,2,2);
    assert(rikitu_level_save_store_pair(LVL_A,LVL_B,&s,&l,&c3));
    snapshot(LVL_A,observed,sizeof(observed));
    assert(memcmp(observed,good,sizeof(good))==0);
    snapshot(LVL_B,observed,sizeof(observed));
    assert(rikitu_level_save_inspect(observed,sizeof(observed),&generation)&&
           generation==11);
    fresh(&loaded,&loaded_loop,&c3);
    assert(rikitu_level_save_load_pair(LVL_A,LVL_B,&loaded,&loaded_loop,&c3));
    assert(loaded.reached_checkpoints==2&&loaded.hearts==2&&
           loaded.stage.coin_count==2);
    puts("PASS Stage132 RKL3 semantic validation preserves the last good backup");
  }
  clean();
  return 0;
}
