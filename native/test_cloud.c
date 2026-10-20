#include "rikitu_cloud.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int close_(float a,float b){return fabsf(a-b)<0.09f;}
static void reset(RikituScene *s,RikituLoop *l){
  rikitu_cloud_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
int main(void){
  RikituScene s;
  RikituLoop l;
  int i;
  const float gusts[4]={980.0f,4300.0f,7120.0f,8620.0f};
  const float belt_x[2]={2900.0f,6600.0f};
  const float belt_speed[2]={38.0f,-38.0f};
  reset(&s,&l);
  assert(s.level_id==10 && s.world_width==10320.0f && s.gate_x==10140.0f);
  assert(s.solid_count==18 && s.coin_total==90 && s.enemy_count==6);
  assert(s.launcher_count==4 && s.belt_count==2 && s.checkpoint_total==3);
  assert(s.fragments[0].x==1900.0f && s.fragments[2].x==8740.0f);
  assert(s.checkpoints[0]==2470.0f && s.checkpoints[1]==5420.0f && s.checkpoints[2]==8020.0f);
  assert(s.stage.boss_required && s.boss.alive && s.boss.hp==3 && s.boss.center==9650.0f);
  for(i=0;i<4;i++){
    reset(&s,&l);
    l.player.x=gusts[i];l.player.y=413.0f;l.player.grounded=1;
    assert(s.launchers[i].kind==RIKITU_LAUNCH_GUST && s.launchers[i].y==456.0f);
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(close_(l.player.vy,-690.0f) && !l.player.grounded && close_(l.player.x,gusts[i]));
    reset(&s,&l);
    l.player.x=gusts[i];l.player.y=413.0f;l.player.grounded=1;
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,RIKITU_INPUT_RIGHT)==1);
    assert(close_(l.player.vy,-690.0f) && close_(l.player.x,gusts[i]+225.0f*RIKITU_FIXED_DT));
  }
  reset(&s,&l);
  l.player.x=980.0f;l.player.y=301.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!close_(l.player.vy,-690.0f));
  for(i=0;i<2;i++){
    reset(&s,&l);
    l.player.x=belt_x[i];l.player.y=413.0f;l.player.grounded=1;
    assert(s.belts[i].speed==belt_speed[i]);
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(close_(l.player.x,belt_x[i]+belt_speed[i]*RIKITU_FIXED_DT));
  }
  reset(&s,&l);
  l.player.x=2500.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && close_(s.active_checkpoint,2470.0f));
  l.player.x=5450.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && close_(s.active_checkpoint,5420.0f));
  l.player.x=8040.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && close_(s.active_checkpoint,8020.0f));
  for(i=0;i<3;i++)s.stage.fragments[i]=1;
  s.stage.fragment_count=3;
  assert(!rikitu_stage_can_finish(&s.stage));
  l.player.x=10122.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && !s.stage.completed);
  s.boss.hp=1;
  l.player.x=9651.0f;l.player.y=352.0f;l.player.vy=330.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!s.boss.alive && s.stage.boss_defeated && close_(l.player.vy,-490.0f));
  assert(rikitu_stage_can_finish(&s.stage));
  l.player.x=10122.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && s.stage.completed);
  reset(&s,&l);s.launcher_count=RIKITU_MAX_LAUNCHERS+1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==-1);
  puts("PASS MAP10 Cloud Dock: 4 gusts, 2 belts, 3 checkpoints, boss and fragment gate");
  return 0;
}
