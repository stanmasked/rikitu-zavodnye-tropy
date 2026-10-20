#include "rikitu_gardens.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int near_(float a,float b){return fabsf(a-b)<0.09f;}
static void reset(RikituScene *s,RikituLoop *l){
  rikitu_gardens_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
int main(void){
  RikituScene s;
  RikituLoop l;
  int i;
  const float mushrooms[3]={1100.0f,4780.0f,8190.0f};
  const float levers[3]={2330.0f,6110.0f,8080.0f};
  reset(&s,&l);
  assert(s.level_id==13 && s.world_width==9950.0f && s.gate_x==9780.0f);
  assert(s.solid_count==18 && s.coin_total==90 && s.enemy_count==6);
  assert(s.launcher_count==3 && s.lever_count==3 && s.belt_count==0);
  assert(s.checkpoints[0]==2520.0f && s.checkpoints[1]==5280.0f && s.checkpoints[2]==7950.0f);
  assert(s.fragments[0].x==1840.0f && s.fragments[2].x==8730.0f);
  assert(!s.stage.boss_required && !s.boss.alive);
  for(i=0;i<3;i++){
    reset(&s,&l);
    l.player.x=mushrooms[i];l.player.y=413.0f;l.player.grounded=1;
    assert(s.launchers[i].kind==RIKITU_LAUNCH_MUSHROOM && s.launchers[i].y==456.0f);
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(near_(l.player.vy,-730.0f) && !l.player.grounded);
    assert(near_(l.player.x,mushrooms[i]) && s.hearts==3);
    reset(&s,&l);
    l.player.x=mushrooms[i];l.player.y=413.0f;l.player.grounded=1;
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,RIKITU_INPUT_RIGHT)==1);
    assert(near_(l.player.vy,-730.0f) && near_(l.player.x,mushrooms[i]+225.0f*RIKITU_FIXED_DT));
    reset(&s,&l);
    l.player.x=levers[i];l.player.y=413.0f;l.player.grounded=1;
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(s.levers[i].on && s.levers_on==1);
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && s.levers_on==1);
  }
  reset(&s,&l);
  l.player.x=1100.0f;l.player.y=310.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!near_(l.player.vy,-730.0f));
  reset(&s,&l);
  l.player.x=2330.0f;l.player.y=310.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && s.levers_on==0);
  reset(&s,&l);
  l.player.x=2530.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && near_(s.active_checkpoint,2520.0f));
  l.player.x=5290.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && near_(s.active_checkpoint,5280.0f));
  l.player.x=7960.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && near_(s.active_checkpoint,7950.0f));
  l.player.x=9770.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && !s.stage.completed);
  for(i=0;i<3;i++)s.stage.fragments[i]=1;
  s.stage.fragment_count=3;
  assert(rikitu_stage_can_finish(&s.stage) && s.levers_on==0);
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && s.stage.completed);
  reset(&s,&l);
  s.lever_count=RIKITU_MAX_LEVERS+1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==-1);
  puts("PASS MAP13 Castle Gardens: 3 mushrooms, 3 optional switches, fragments and checkpoints");
  return 0;
}
