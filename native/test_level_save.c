#include "rikitu_level_save.h"
#include "rikitu_mill.h"
#include "rikitu_gardens.h"
#include "rikitu_heart.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void init_loop(RikituLoop *l,const RikituScene *s){
  memset(l,0,sizeof(*l));l->player=s->player;
}
int main(void){
  RikituScene s,t;
  RikituLoop l,restored;
  RikituCampaign c,wrong;
  uint8_t raw[RIKITU_LEVEL_SAVE_BYTES],bad[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t generation=0;
  assert(rikitu_campaign_init(&c,2));
  c.wallet=157;
  rikitu_mill_init(&s);init_loop(&l,&s);
  s.active_checkpoint=s.checkpoints[1];s.reached_checkpoints=2;
  s.hearts=2;s.damage_count=5;
  s.stage.fragments[0]=1;s.stage.fragments[2]=1;s.stage.fragment_count=2;
  s.stage.coins[0]=1;s.stage.coins[23]=1;s.stage.coins[89]=1;
  s.stage.coin_count=3;
  s.enemies[0].dead=1;s.enemies[5].dead=1;
  s.boss.hp=2;s.boss.alive=1;s.boss.hit_seconds=0.5f;
  l.active_seconds=123.456f;
  assert(rikitu_level_save_encode(raw,&s,&l,&c,17));
  assert(raw[0]=='R'&&raw[1]=='K'&&raw[2]=='L'&&raw[3]=='3');
  rikitu_mill_init(&t);init_loop(&restored,&t);
  assert(rikitu_level_save_decode(&t,&restored,&c,raw,sizeof(raw),&generation));
  assert(generation==17 && t.level_id==2 && t.reached_checkpoints==2);
  assert(t.active_checkpoint==s.checkpoints[1] && t.hearts==2);
  assert(t.stage.fragment_count==2 && t.stage.coin_count==3);
  assert(t.stage.fragments[0]&&t.stage.fragments[2]&&!t.stage.fragments[1]);
  assert(t.stage.coins[0]&&t.stage.coins[23]&&t.stage.coins[89]);
  assert(t.enemies[0].dead&&t.enemies[5].dead&&!t.enemies[1].dead);
  assert(t.boss.hp==2&&t.boss.alive&&t.boss.hit_seconds==0.0f);
  assert(t.boss.x==t.boss.center && !t.stage.completed);
  assert(t.invulnerability_seconds==1.0f);
  assert(restored.player.x==s.checkpoints[1] && restored.player.y==413.0f);
  assert(!restored.player.grounded && fabsf(restored.active_seconds-123.456f)<0.01f);
  /* Corrupt checks and wrong stage/wallet must leave the target untouched. */
  rikitu_mill_init(&t);init_loop(&restored,&t);
  memcpy(bad,raw,sizeof(raw));bad[15]^=0x10;
  assert(!rikitu_level_save_decode(&t,&restored,&c,bad,sizeof(bad),NULL));
  assert(t.reached_checkpoints==0&&t.hearts==3);
  assert(!rikitu_level_save_decode(&t,&restored,&c,raw,sizeof(raw)-1,NULL));
  assert(!rikitu_level_save_decode(&t,&restored,&c,raw,sizeof(raw)+1,NULL));
  wrong=c;wrong.wallet++;
  assert(!rikitu_level_save_decode(&t,&restored,&wrong,raw,sizeof(raw),NULL));
  assert(rikitu_campaign_init(&wrong,3));
  assert(!rikitu_level_save_decode(&t,&restored,&wrong,raw,sizeof(raw),NULL));
  assert(!rikitu_level_save_encode(NULL,&s,&l,&c,1));
  assert(!rikitu_level_save_encode(raw,&s,&l,&c,0));
  s.stage.coin_count=4;
  assert(!rikitu_level_save_encode(raw,&s,&l,&c,1));
  s.stage.coin_count=3;s.stage.completed=1;
  assert(!rikitu_level_save_encode(raw,&s,&l,&c,1));
  /* Levers are persisted on an optional-lever world without bosses. */
  assert(rikitu_campaign_init(&c,13));
  rikitu_gardens_init(&s);init_loop(&l,&s);
  s.levers[0].on=1;s.levers[2].on=1;s.levers_on=2;
  assert(rikitu_level_save_encode(raw,&s,&l,&c,2));
  rikitu_gardens_init(&t);init_loop(&restored,&t);
  assert(rikitu_level_save_decode(&t,&restored,&c,raw,sizeof(raw),NULL));
  assert(t.levers[0].on&&t.levers[2].on&&!t.levers[1].on);
  assert(t.levers_on==2&&!t.stage.boss_required);
  /* Final stage boss defeat can be resumed without duplicate reward. */
  assert(rikitu_campaign_init(&c,16));
  c.wallet=470;c.purchased_hearts=2;c.max_hearts=5;
  rikitu_heart_init(&s);init_loop(&l,&s);
  s.max_hearts=5;s.hearts=4;s.boss.hp=0;s.boss.alive=0;
  s.stage.boss_defeated=1;
  s.reached_checkpoints=3;s.active_checkpoint=s.checkpoints[2];
  s.stage.fragments[0]=s.stage.fragments[1]=s.stage.fragments[2]=1;
  s.stage.fragment_count=3;
  assert(rikitu_level_save_encode(raw,&s,&l,&c,0xfffffffeu));
  rikitu_heart_init(&t);init_loop(&restored,&t);
  t.max_hearts=5;t.hearts=5;
  assert(rikitu_level_save_decode(&t,&restored,&c,raw,sizeof(raw),&generation));
  assert(generation==0xfffffffeu && t.level_id==16);
  assert(t.hearts==4 && t.boss.hp==0 && !t.boss.alive);
  assert(t.stage.boss_defeated && rikitu_stage_can_finish(&t.stage));
  assert(!t.stage.completed && t.reached_checkpoints==3);
  assert(t.launcher_count==5);
  puts("PASS RKL3: 16-map schema, CRC, checkpoints, pickups, enemies, levers and boss state");
  return 0;
}
