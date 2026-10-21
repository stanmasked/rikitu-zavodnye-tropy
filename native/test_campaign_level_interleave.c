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
  clean();
  puts("PASS Stage100 RKC2/RKL3 interrupted ordering: chapter and wallet isolation, rollback and surviving backup");
  return 0;
}
