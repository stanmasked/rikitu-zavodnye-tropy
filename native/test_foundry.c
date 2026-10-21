#include "rikitu_foundry.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int near_(float a,float b){return fabsf(a-b)<0.09f;}
static void reset(RikituScene *s,RikituLoop *l){
  rikitu_foundry_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
int main(void){
  RikituScene s;RikituLoop l;int i;
  const float vent[3]={1570.0f,5420.0f,8380.0f};
  const float belt[3]={850.0f,4720.0f,9010.0f};
  const float speed[3]={45.0f,-42.0f,47.0f};
  const float lever[3]={2380.0f,6500.0f,8770.0f};
  reset(&s,&l);
  assert(s.level_id==14 && s.world_width==11200.0f && s.gate_x==11020.0f);
  assert(s.solid_count==18 && s.coin_total==90 && s.enemy_count==6);
  assert(s.belt_count==3 && s.launcher_count==3 && s.lever_count==3);
  assert(s.checkpoint_total==3 && s.checkpoints[0]==2600.0f && s.checkpoints[2]==8690.0f);
  assert(s.boss.hp==3 && s.boss.alive && s.boss.center==10470.0f && s.stage.boss_required);
  for(i=0;i<3;i++){
    reset(&s,&l);l.player.x=vent[i];l.player.y=413.0f;l.player.grounded=1;
    assert(s.launchers[i].kind==RIKITU_LAUNCH_STEAM);
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(near_(l.player.vy,-705.0f) && !l.player.grounded);
    reset(&s,&l);l.player.x=belt[i];l.player.y=413.0f;l.player.grounded=1;
    assert(s.belts[i].speed==speed[i]);
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(near_(l.player.x,belt[i]+speed[i]*RIKITU_FIXED_DT));
    reset(&s,&l);l.player.x=lever[i];l.player.y=413.0f;l.player.grounded=1;
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(s.levers[i].on && s.levers_on==1);
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && s.levers_on==1);
  }
  reset(&s,&l);l.player.x=1570.0f;l.player.y=302.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!near_(l.player.vy,-705.0f));
  reset(&s,&l);
  for(i=0;i<3;i++)s.stage.fragments[i]=1;
  s.stage.fragment_count=3;
  assert(!rikitu_stage_can_finish(&s.stage) && s.levers_on==0);
  l.player.x=11004.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && !s.stage.completed);
  s.boss.hp=1;
  l.player.x=10471.0f;l.player.y=352.0f;l.player.vy=330.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!s.boss.alive && s.stage.boss_defeated && s.hearts==3);
  assert(rikitu_stage_can_finish(&s.stage) && s.levers_on==0);
  l.player.x=11004.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && s.stage.completed);
  reset(&s,&l);s.belt_count=RIKITU_MAX_BELTS+1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==-1);
  puts("PASS MAP14 Heart Foundry: steam, belts, three switches and boss-gated exit");
  return 0;
}
