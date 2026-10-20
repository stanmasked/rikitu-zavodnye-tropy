#include "rikitu_crystal.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int near_(float x,float y){return fabsf(x-y)<0.09f;}
static void reset(RikituScene *s,RikituLoop *l){
  rikitu_crystal_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
int main(void){
  RikituScene s;
  RikituLoop l;
  int i;
  const float crystal_x[3]={1120.0f,3980.0f,7650.0f};
  reset(&s,&l);
  assert(s.level_id==11 && s.world_width==9820.0f && s.gate_x==9620.0f);
  assert(s.solid_count==18 && s.coin_total==90 && s.enemy_count==6);
  assert(s.launcher_count==3 && s.belt_count==0 && s.checkpoint_total==3);
  assert(s.fragments[0].x==1770.0f && s.fragments[2].x==8440.0f);
  assert(s.checkpoints[0]==2370.0f && s.checkpoints[1]==5050.0f && s.checkpoints[2]==7470.0f);
  assert(!s.stage.boss_required && !s.boss.alive);
  for(i=0;i<3;i++){
    reset(&s,&l);
    l.player.x=crystal_x[i];l.player.y=413.0f;l.player.grounded=1;
    assert(s.launchers[i].kind==RIKITU_LAUNCH_CRYSTAL && s.launchers[i].y==456.0f);
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(near_(l.player.vy,-690.0f) && !l.player.grounded && near_(l.player.x,crystal_x[i]));
    assert(s.hearts==3 && s.damage_count==0);
    reset(&s,&l);
    l.player.x=crystal_x[i];l.player.y=413.0f;l.player.grounded=1;
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,RIKITU_INPUT_RIGHT)==1);
    assert(near_(l.player.vy,-690.0f) && near_(l.player.x,crystal_x[i]+225.0f*RIKITU_FIXED_DT));
  }
  reset(&s,&l);
  l.player.x=1120.0f;l.player.y=310.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!near_(l.player.vy,-690.0f));
  reset(&s,&l);
  l.player.x=2375.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near_(s.active_checkpoint,2370.0f));
  l.player.x=5055.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near_(s.active_checkpoint,5050.0f));
  l.player.x=7475.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near_(s.active_checkpoint,7470.0f));
  l.player.x=9605.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!s.stage.completed);
  for(i=0;i<3;i++){
    s.stage.fragments[i]=1;
  }
  s.stage.fragment_count=3;
  assert(rikitu_stage_can_finish(&s.stage));
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(s.stage.completed);
  reset(&s,&l);
  s.launcher_count=RIKITU_MAX_LAUNCHERS+1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==-1);
  puts("PASS MAP11 Crystal Caves: three -690 springs, no airborne retrigger and fragment exit");
  return 0;
}
