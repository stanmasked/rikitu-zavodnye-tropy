#include "rikitu_combat_guest_qa.h"
#include "rikitu_flower.h"
#include <math.h>
#include <string.h>
static void start(RikituScene *s,RikituLoop *l) {
  rikitu_flower_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
static int near(float x,float y,float eps) {
  return fabsf(x-y)<eps;
}
unsigned rikitu_guest_combat_checks(void) {
  RikituScene s;
  RikituLoop l;
  unsigned result=0;
  int i,ok;
  /* Verify that a real engine tick moves the initial enemy left at 43/s. */
  start(&s,&l);
  ok=(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
      near(s.enemies[0].x,820.0f-43.0f/60.0f,0.02f) &&
      s.enemies[0].dir==-1);
  if(ok)result|=RIKITU_QA_PATROL;

  /* Side-on collision hurts once, and the next 30 overlapping ticks
   * cannot drain hearts during the 1.6-second invulnerability interval. */
  start(&s,&l);
  l.player.x=825.0f;
  l.player.y=413.0f;
  l.player.grounded=1;
  ok=(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
      s.hearts==2 && s.damage_count==1 &&
      s.invulnerability_seconds>1.55f);
  for(i=0;ok&&i<30;i++)
    if(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)!=1)ok=0;
  if(ok && s.hearts==2 && s.damage_count==1)
    result|=RIKITU_QA_CONTACT;

  /* Descend onto the enemy in the native game, killing the patrol and
   * bouncing the player without taking damage. */
  start(&s,&l);
  l.player.x=825.0f;
  l.player.y=381.0f;
  l.player.vy=300.0f;
  l.player.grounded=0;
  if(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
     s.enemies[0].dead && near(l.player.vy,-360.0f,0.03f) &&
     s.hearts==3 && s.damage_count==0)
    result|=RIKITU_QA_STOMP;

  /* A fall past 690 returns to the first unlocked checkpoint and
   * retains previously earned fragments, coins and defeated enemies. */
  start(&s,&l);
  s.active_checkpoint=2050.0f;
  s.reached_checkpoints=1;
  s.stage.fragments[0]=1;
  s.stage.fragment_count=1;
  s.stage.coins[0]=1;
  s.stage.coin_count=1;
  s.enemies[0].dead=1;
  l.player.x=1690.0f;
  l.player.y=700.0f;
  l.player.vy=50.0f;
  if(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
     near(l.player.x,2050.0f,0.03f) &&
     near(l.player.y,413.0f,0.03f) &&
     near(s.invulnerability_seconds,1.4f,0.02f) &&
     s.hearts==2 && s.damage_count==1 &&
     s.stage.fragment_count==1 && s.stage.coin_count==1 &&
     s.reached_checkpoints==1 && s.enemies[0].dead)
    result|=RIKITU_QA_RESPAWN;
  return result;
}
