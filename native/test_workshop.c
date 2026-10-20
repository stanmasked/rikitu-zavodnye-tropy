#include "rikitu_workshop.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int near(float a,float b){return fabsf(a-b)<0.08f;}
static void reset(RikituScene *s,RikituLoop *l){
  rikitu_workshop_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
static void on_belt(RikituScene *s,RikituLoop *l,float x){
  reset(s,l);
  l->player.x=x;l->player.y=413.0f;l->player.grounded=1;
}
int main(void){
  RikituScene s;
  RikituLoop l;
  int i;
  static const float at[3]={1100.0f,4000.0f,7000.0f};
  static const float speed[3]={48.0f,-42.0f,52.0f};
  reset(&s,&l);
  assert(s.level_id==5 && s.world_width==9140.0f && s.gate_x==8950.0f);
  assert(s.solid_count==18 && s.coin_total==90 && s.belt_count==3);
  assert(s.enemy_count==6 && s.checkpoint_total==3 && s.lever_count==0);
  assert(!s.stage.boss_required && !s.boss.alive);
  assert(s.fragments[0].x==1700.0f && s.fragments[2].x==7780.0f);
  assert(s.checkpoints[0]==2200.0f && s.checkpoints[1]==4760.0f &&
         s.checkpoints[2]==6920.0f);
  for(i=0;i<3;i++){
    on_belt(&s,&l,at[i]);
    assert(near(s.belts[i].speed,speed[i]));
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(near(l.player.x,at[i]+speed[i]*RIKITU_FIXED_DT));
    assert(l.player.grounded && s.hearts==3);
  }
  /* Belt velocity adds to, rather than replacing, movement input. */
  on_belt(&s,&l,1100.0f);
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,RIKITU_INPUT_RIGHT)==1);
  assert(near(l.player.x,1100.0f+(225.0f+48.0f)*RIKITU_FIXED_DT));
  on_belt(&s,&l,1100.0f);
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,RIKITU_INPUT_LEFT)==1);
  assert(near(l.player.x,1100.0f+(-225.0f+48.0f)*RIKITU_FIXED_DT));
  /* No belt acceleration while airborne (jumping or falling). */
  on_belt(&s,&l,1100.0f);
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,RIKITU_INPUT_JUMP)==1);
  assert(near(l.player.x,1100.0f)&&!l.player.grounded&&l.player.vy<0.0f);
  reset(&s,&l);
  l.player.x=1100.0f;l.player.y=290.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near(l.player.x,1100.0f));
  /* Edge overlap must be strict and spatially bounded. */
  on_belt(&s,&l,1037.0f);
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near(l.player.x,1037.0f));
  /* Fixed-tick progress and fragment-only gate condition (no boss). */
  reset(&s,&l);
  l.player.x=2201.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near(s.active_checkpoint,2200.0f));
  l.player.x=4761.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near(s.active_checkpoint,4760.0f));
  l.player.x=6921.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near(s.active_checkpoint,6920.0f));
  l.player.x=8935.0f;
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
  puts("PASS MAP05 Workshop: exact source geometry, three directional belts, airborne safety, checkpoints and fragment-only gate");
  return 0;
}
