#include "rikitu_gallery.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int near_(float a,float b){return fabsf(a-b)<0.09f;}
static void reset(RikituScene *s,RikituLoop *l){
  rikitu_gallery_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
int main(void){
  RikituScene s;
  RikituLoop l;
  int i;
  const float pad[3]={1150.0f,4500.0f,8470.0f};
  const float lever[3]={2320.0f,6360.0f,8550.0f};
  reset(&s,&l);
  assert(s.level_id==15 && s.world_width==10300.0f && s.gate_x==10120.0f);
  assert(s.solid_count==18 && s.coin_total==90 && s.enemy_count==6);
  assert(s.launcher_count==3 && s.lever_count==3 && s.belt_count==0);
  assert(s.fragments[0].x==1800.0f && s.fragments[2].x==9290.0f);
  assert(s.checkpoints[0]==2500.0f && s.checkpoints[1]==5430.0f && s.checkpoints[2]==8210.0f);
  assert(!s.stage.boss_required && !s.boss.alive);
  for(i=0;i<3;i++){
    reset(&s,&l);
    l.player.x=pad[i];l.player.y=413.0f;l.player.grounded=1;
    assert(s.launchers[i].kind==RIKITU_LAUNCH_CLOCK && s.launchers[i].y==456.0f);
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(near_(l.player.vy,-675.0f) && !l.player.grounded && near_(l.player.x,pad[i]));
    assert(s.hearts==3 && s.damage_count==0);
    reset(&s,&l);
    l.player.x=pad[i];l.player.y=413.0f;l.player.grounded=1;
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,RIKITU_INPUT_RIGHT)==1);
    assert(near_(l.player.vy,-675.0f) && near_(l.player.x,pad[i]+225.0f*RIKITU_FIXED_DT));
    reset(&s,&l);
    l.player.x=lever[i];l.player.y=413.0f;l.player.grounded=1;
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(s.levers[i].on && s.levers_on==1);
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && s.levers_on==1);
  }
  reset(&s,&l);
  l.player.x=1150.0f;l.player.y=300.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!near_(l.player.vy,-675.0f));
  reset(&s,&l);
  l.player.x=2320.0f;l.player.y=300.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && s.levers_on==0);
  reset(&s,&l);
  l.player.x=2505.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && near_(s.active_checkpoint,2500.0f));
  l.player.x=5435.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && near_(s.active_checkpoint,5430.0f));
  l.player.x=8215.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && near_(s.active_checkpoint,8210.0f));
  l.player.x=10100.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && !s.stage.completed);
  for(i=0;i<3;i++){
    s.stage.fragments[i]=1;
  }
  s.stage.fragment_count=3;
  assert(rikitu_stage_can_finish(&s.stage) && s.levers_on==0);
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && s.stage.completed);
  reset(&s,&l);
  s.lever_count=RIKITU_MAX_LEVERS+1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==-1);
  puts("PASS MAP15 Clock Gallery: three dial boosts, three optional levers, checkpoints and gate");
  return 0;
}
