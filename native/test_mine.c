#include "rikitu_mine.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int near_(float a,float b){return fabsf(a-b)<0.09f;}
static void reset(RikituScene *s,RikituLoop *l){
  rikitu_mine_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
int main(void){
  RikituScene s;RikituLoop l;int i;
  const float vent[3]={1480.0f,5140.0f,7580.0f};
  const float belt[3]={800.0f,4670.0f,8350.0f};
  const float speed[3]={43.0f,-44.0f,46.0f};
  const float lever[3]={2590.0f,6030.0f,8260.0f};
  reset(&s,&l);
  assert(s.level_id==12 && s.world_width==10700.0f && s.gate_x==10530.0f);
  assert(s.solid_count==18 && s.coin_total==90 && s.enemy_count==6);
  assert(s.belt_count==3 && s.launcher_count==3 && s.lever_count==3);
  assert(s.checkpoint_total==3 && s.checkpoints[0]==2520.0f && s.checkpoints[2]==8200.0f);
  assert(s.boss.hp==3 && s.boss.alive && s.boss.center==9930.0f && s.stage.boss_required);
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
  reset(&s,&l);l.player.x=1480.0f;l.player.y=302.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!near_(l.player.vy,-705.0f));
  reset(&s,&l);
  for(i=0;i<3;i++)s.stage.fragments[i]=1;
  s.stage.fragment_count=3;
  assert(!rikitu_stage_can_finish(&s.stage) && s.levers_on==0);
  l.player.x=10514.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && !s.stage.completed);
  s.boss.hp=1;
  l.player.x=9931.0f;l.player.y=352.0f;l.player.vy=330.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!s.boss.alive && s.stage.boss_defeated && s.hearts==3);
  assert(rikitu_stage_can_finish(&s.stage) && s.levers_on==0);
  l.player.x=10514.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && s.stage.completed);
  reset(&s,&l);s.belt_count=RIKITU_MAX_BELTS+1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==-1);
  puts("PASS MAP12 Silent Mine: steam, belts, three switches and boss-gated exit");
  return 0;
}
