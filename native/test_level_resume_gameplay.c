/* Stage70: real game-loop pickups/checkpoint -> RKL3 -> restart -> replay.
 * Controlled positions isolate persistence from unverified controller routes.
 */
#include "rikitu_mill.h"
#include "rikitu_grove.h"
#include "rikitu_demo.h"
#include "rikitu_level_save.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SLOT_A "/tmp/rikitu-stage70-resume-a.rkl"
#define SLOT_B "/tmp/rikitu-stage70-resume-b.rkl"

static void fresh_mill(RikituScene *s,RikituLoop *loop){
  rikitu_mill_init(s);
  memset(loop,0,sizeof(*loop));
  loop->player=s->player;
}
static void tick_at(RikituScene *s,RikituLoop *loop,float x,float y){
  loop->player.x=x;
  loop->player.y=y;
  loop->player.vx=0.0f;
  loop->player.vy=0.0f;
  loop->player.grounded=0;
  assert(rikitu_demo_advance(s,loop,RIKITU_FIXED_DT,0)==1);
}
int main(void){
  RikituScene s,reloaded;
  RikituLoop game,loaded;
  RikituCampaign campaign,wrong;
  FILE *f;
  int saved_coins,saved_fragments;
  (void)remove(SLOT_A);(void)remove(SLOT_B);
  assert(rikitu_campaign_init(&campaign,2));
  campaign.wallet=19;
  fresh_mill(&s,&game);

  /* These pickup flags come from the actual 60Hz C99 engine, not direct
   * bitset assignments or a fabricated save record. */
  tick_at(&s,&game,s.coins[0].x-10.0f,s.coins[0].y-3.0f);
  assert(s.stage.coins[0] && s.stage.coin_count>=1);
  tick_at(&s,&game,s.fragments[0].x,s.fragments[0].y-10.0f);
  assert(s.stage.fragments[0] && s.stage.fragment_count==1);
  /* Isolate save semantics for a boss already damaged in this session.
   * The separate boss tests cover the real three-stomp combat cycle. */
  s.boss.hp=2;
  assert(rikitu_level_save_store_pair(SLOT_A,SLOT_B,&s,&game,&campaign));

  /* Reach a checkpoint through game-step logic, and write newer slot B. */
  tick_at(&s,&game,s.checkpoints[0]+8.0f,413.0f);
  assert(s.reached_checkpoints==1);
  assert(s.active_checkpoint==s.checkpoints[0]);
  assert(rikitu_level_save_store_pair(SLOT_A,SLOT_B,&s,&game,&campaign));
  saved_coins=s.stage.coin_count;
  saved_fragments=s.stage.fragment_count;

  fresh_mill(&reloaded,&loaded);
  assert(rikitu_level_save_load_pair(SLOT_A,SLOT_B,
                                     &reloaded,&loaded,&campaign));
  assert(reloaded.reached_checkpoints==1 &&
         reloaded.active_checkpoint==s.checkpoints[0]);
  assert(reloaded.stage.coin_count==saved_coins &&
         reloaded.stage.fragment_count==saved_fragments);
  assert(reloaded.stage.coins[0] && reloaded.stage.fragments[0]);
  assert(reloaded.boss.hp==2 && reloaded.boss.alive);
  assert(reloaded.invulnerability_seconds>0.0f);
  assert(loaded.player.x==s.checkpoints[0]);

  /* Re-enter the original coin and fragment: both remain collected.
   * The test must NOT rely on RKL3's counters alone. */
  tick_at(&reloaded,&loaded,reloaded.coins[0].x-10.0f,
          reloaded.coins[0].y-3.0f);
  assert(reloaded.stage.coins[0] &&
         reloaded.stage.coin_count==saved_coins);
  tick_at(&reloaded,&loaded,reloaded.fragments[0].x,
          reloaded.fragments[0].y-10.0f);
  assert(reloaded.stage.fragments[0] &&
         reloaded.stage.fragment_count==saved_fragments);
  assert(!rikitu_stage_can_finish(&reloaded.stage));

  /* An RKL3 checkpoint cannot be mistaken for the next chapter. */
  wrong=campaign;wrong.level=3;
  rikitu_grove_init(&reloaded);
  memset(&loaded,0,sizeof(loaded));
  loaded.player=reloaded.player;
  assert(!rikitu_level_save_load_pair(SLOT_A,SLOT_B,
                                     &reloaded,&loaded,&wrong));
  assert(reloaded.level_id==3 && reloaded.stage.coin_count==0);

  /* Nor may it be restored with a different RKC2 wallet. */
  wrong=campaign;wrong.wallet++;
  fresh_mill(&reloaded,&loaded);
  assert(!rikitu_level_save_load_pair(SLOT_A,SLOT_B,
                                     &reloaded,&loaded,&wrong));
  assert(reloaded.stage.coin_count==0 && reloaded.hearts==3);

  /* Break newest slot B, verify fallback to A still retains pickups,
   * while its earlier checkpoint has not yet been reached. */
  f=fopen(SLOT_B,"r+b");assert(f);
  assert(fseek(f,14,SEEK_SET)==0 && fputc(255,f)!=EOF);
  assert(fclose(f)==0);
  fresh_mill(&reloaded,&loaded);
  assert(rikitu_level_save_load_pair(SLOT_A,SLOT_B,
                                     &reloaded,&loaded,&campaign));
  assert(reloaded.reached_checkpoints==0);
  assert(reloaded.stage.coins[0] && reloaded.stage.fragments[0]);
  assert(reloaded.boss.hp==2);
  (void)remove(SLOT_A);(void)remove(SLOT_B);
  puts("PASS game-loop MAP02 coin+fragment -> checkpoint -> RKL3 resume, replay, CRC fallback");
  return 0;
}
