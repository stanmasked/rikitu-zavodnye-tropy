#include "rikitu_delivery.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int near(float a,float b){return fabsf(a-b)<0.09f;}
static void reset(RikituScene *s,RikituLoop *l){
  rikitu_delivery_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
int main(void){
  RikituScene s;
  RikituLoop l;
  int i;
  const float belt_x[3]={650.0f,4100.0f,7520.0f};
  const float belt_speed[3]={42.0f,-48.0f,50.0f};
  const float vent_x[3]={1350.0f,4740.0f,7910.0f};
  reset(&s,&l);
  assert(s.level_id==6 && s.world_width==9910.0f && s.gate_x==9730.0f);
  assert(s.solid_count==18 && s.coin_total==90 && s.belt_count==3);
  assert(s.launcher_count==3 && s.enemy_count==6 && s.checkpoint_total==3);
  assert(s.stage.boss_required && !s.stage.boss_defeated);
  assert(s.boss.alive && s.boss.hp==3 && s.boss.center==9210.0f);
  assert(s.fragments[0].x==1810.0f && s.fragments[2].x==8120.0f);
  assert(s.checkpoints[0]==2300.0f && s.checkpoints[1]==5060.0f && s.checkpoints[2]==7360.0f);
  for(i=0;i<3;i++){
    reset(&s,&l);
    l.player.x=belt_x[i];l.player.y=413.0f;l.player.grounded=1;
    assert(near(s.belts[i].speed,belt_speed[i]));
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(near(l.player.x,belt_x[i]+belt_speed[i]*RIKITU_FIXED_DT));
    assert(l.player.grounded && s.hearts==3);
  }
  reset(&s,&l);
  l.player.x=4100.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,RIKITU_INPUT_RIGHT)==1);
  assert(near(l.player.x,4100.0f+(225.0f-48.0f)*RIKITU_FIXED_DT));
  for(i=0;i<3;i++){
    reset(&s,&l);
    l.player.x=vent_x[i];l.player.y=413.0f;l.player.grounded=1;
    assert(s.launchers[i].kind==RIKITU_LAUNCH_STEAM);
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(near(l.player.vy,-705.0f)&&!l.player.grounded);
    assert(s.hearts==3 && s.damage_count==0);
  }
  reset(&s,&l);
  l.player.x=1350.0f;l.player.y=310.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!near(l.player.vy,-705.0f));
  /* Boss is mandatory even with all fragments, unlike MAP05. */
  reset(&s,&l);
  for(i=0;i<3;i++)s.stage.fragments[i]=1;
  s.stage.fragment_count=3;
  assert(!rikitu_stage_can_finish(&s.stage));
  s.boss.hp=1;
  l.player.x=9211.0f;l.player.y=352.0f;l.player.vy=330.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!s.boss.alive && s.boss.hp==0 && s.stage.boss_defeated);
  assert(near(l.player.vy,-490.0f) && rikitu_stage_can_finish(&s.stage));
  l.player.x=9715.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(s.stage.completed);
  reset(&s,&l);
  s.launcher_count=RIKITU_MAX_LAUNCHERS+1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==-1);
  puts("PASS MAP06 Delivery: 18 solids, 90 coins, 3 belts, 3 steam vents, 6 patrols and 3-hit boss gate");
  return 0;
}
