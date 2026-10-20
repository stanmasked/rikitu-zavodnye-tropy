#include "rikitu_clock.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int near(float a,float b){return fabsf(a-b)<0.09f;}
static void reset(RikituScene *s,RikituLoop *l){
  rikitu_clock_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
int main(void){
  RikituScene s;
  RikituLoop l;
  int i;
  static const float pad_x[3]={1350.0f,4140.0f,7770.0f};
  reset(&s,&l);
  assert(s.level_id==7 && s.world_width==9470.0f && s.gate_x==9290.0f);
  assert(s.solid_count==18 && s.coin_total==90 && s.enemy_count==6);
  assert(s.launcher_count==3 && s.belt_count==2 && s.checkpoint_total==3);
  assert(s.checkpoints[0]==2240.0f && s.checkpoints[1]==4930.0f);
  assert(s.fragments[0].x==1740.0f && s.fragments[2].x==8110.0f);
  assert(!s.stage.boss_required && !s.boss.alive && !rikitu_stage_can_finish(&s.stage));
  for(i=0;i<3;i++){
    reset(&s,&l);
    l.player.x=pad_x[i];l.player.y=413.0f;l.player.grounded=1;
    assert(s.launchers[i].kind==RIKITU_LAUNCH_CLOCK);
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(near(l.player.vy,-675.0f) && !l.player.grounded);
    assert(s.hearts==3 && s.damage_count==0);
  }
  reset(&s,&l);
  l.player.x=1350.0f;l.player.y=300.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!near(l.player.vy,-675.0f));
  reset(&s,&l);
  l.player.x=2630.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near(l.player.x,2630.0f+40.0f*RIKITU_FIXED_DT));
  reset(&s,&l);
  l.player.x=6080.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near(l.player.x,6080.0f-42.0f*RIKITU_FIXED_DT));
  reset(&s,&l);
  l.player.x=2241.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near(s.active_checkpoint,2240.0f));
  l.player.x=4931.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near(s.active_checkpoint,4930.0f));
  l.player.x=7241.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near(s.active_checkpoint,7240.0f));
  l.player.x=9270.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!s.stage.completed);
  for(i=0;i<3;i++)s.stage.fragments[i]=1;
  s.stage.fragment_count=3;
  assert(rikitu_stage_can_finish(&s.stage));
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(s.stage.completed);
  reset(&s,&l);
  s.belt_count=RIKITU_MAX_BELTS+1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==-1);
  puts("PASS MAP07 Clock Square: three -675 dial boosts, two belts, fragment gate, all geometry");
  return 0;
}
