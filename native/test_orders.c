#include "rikitu_orders.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int close_(float a,float b){return fabsf(a-b)<0.09f;}
static void reset(RikituScene *s,RikituLoop *l){
  rikitu_orders_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
int main(void){
  RikituScene s;
  RikituLoop l;
  int i;
  const float levers[3]={2120.0f,5390.0f,7560.0f};
  const float belts[3]={950.0f,4380.0f,7800.0f};
  const float speeds[3]={45.0f,-48.0f,47.0f};
  reset(&s,&l);
  assert(s.level_id==8 && s.world_width==10180.0f && s.gate_x==9990.0f);
  assert(s.solid_count==18 && s.coin_total==90 && s.enemy_count==6);
  assert(s.checkpoint_total==3 && s.belt_count==3 && s.lever_count==3);
  assert(s.boss.hp==3 && s.boss.alive && s.boss.center==9470.0f);
  assert(s.stage.boss_required && !s.stage.boss_defeated);
  assert(s.checkpoints[0]==2380.0f && s.checkpoints[2]==7710.0f);
  assert(s.fragments[0].x==1820.0f && s.fragments[2].x==8450.0f);
  for(i=0;i<3;i++){
    reset(&s,&l);
    l.player.x=belts[i];l.player.y=413.0f;l.player.grounded=1;
    assert(close_(s.belts[i].speed,speeds[i]));
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(close_(l.player.x,belts[i]+speeds[i]*RIKITU_FIXED_DT));
  }
  for(i=0;i<3;i++){
    reset(&s,&l);
    l.player.x=levers[i];l.player.y=413.0f;l.player.grounded=1;
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(s.levers[i].on && s.levers_on==1);
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(s.levers_on==1);
  }
  reset(&s,&l);
  l.player.x=2120.0f;l.player.y=280.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(s.levers_on==0);
  /* Decorative levers are not required for the gate, but boss is. */
  reset(&s,&l);
  for(i=0;i<3;i++)s.stage.fragments[i]=1;
  s.stage.fragment_count=3;
  assert(!rikitu_stage_can_finish(&s.stage));
  l.player.x=9975.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!s.stage.completed);
  s.boss.hp=1;
  l.player.x=9471.0f;l.player.y=352.0f;l.player.vy=330.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!s.boss.alive && s.stage.boss_defeated);
  assert(rikitu_stage_can_finish(&s.stage) && s.levers_on==0);
  l.player.x=9975.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(s.stage.completed);
  reset(&s,&l);
  s.lever_count=RIKITU_MAX_LEVERS+1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==-1);
  puts("PASS MAP08 Orders Depot: 18 solids, 90 coins, three belts, switches, boss-only gate");
  return 0;
}
