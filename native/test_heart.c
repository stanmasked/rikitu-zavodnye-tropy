#include "rikitu_heart.h"
#include "rikitu_demo.h"
#include "rikitu_surfaces.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int near_(float a,float b){return fabsf(a-b)<0.09f;}
static void reset_(RikituScene *s,RikituLoop *l){
  rikitu_heart_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
int main(void){
  RikituScene s;RikituLoop l;int i;
  static const float launch_x[5]={1700.0f,5640.0f,8580.0f,3310.0f,7410.0f};
  static const float launch_v[5]={-705.0f,-705.0f,-705.0f,-675.0f,-675.0f};
  static const float belt_x[3]={930.0f,5130.0f,9250.0f};
  static const float belt_speed[3]={44.0f,-44.0f,45.0f};
  reset_(&s,&l);
  assert(s.level_id==16 && s.world_width==11750.0f && s.gate_x==11560.0f);
  assert(s.solid_count==18 && s.coin_total==90 && s.enemy_count==6);
  assert(RIKITU_MAX_LAUNCHERS==5 && s.launcher_count==5 && s.belt_count==3);
  assert(s.checkpoint_total==3 && s.checkpoints[0]==2930.0f);
  assert(s.checkpoints[1]==6260.0f && s.checkpoints[2]==9260.0f);
  assert(s.fragments[0].x==2020.0f && s.fragments[2].x==9950.0f);
  assert(s.stage.boss_required && s.boss.alive && s.boss.hp==3);
  assert(s.boss.center==10950.0f && !rikitu_stage_can_finish(&s.stage));
  for(i=0;i<5;i++){
    reset_(&s,&l);
    l.player.x=launch_x[i];l.player.y=413.0f;l.player.grounded=1;
    assert(s.launchers[i].kind==(i<3?RIKITU_LAUNCH_STEAM:RIKITU_LAUNCH_CLOCK));
    assert(s.launchers[i].y==456.0f);
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(near_(l.player.vy,launch_v[i]) && !l.player.grounded);
    assert(near_(l.player.x,launch_x[i]) && s.hearts==3 && s.damage_count==0);
    /* Directional input remains respected, without a forced horizontal shove. */
    reset_(&s,&l);
    l.player.x=launch_x[i];l.player.y=413.0f;l.player.grounded=1;
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,RIKITU_INPUT_RIGHT)==1);
    assert(near_(l.player.vy,launch_v[i]));
    assert(near_(l.player.x,launch_x[i]+225.0f*RIKITU_FIXED_DT));
  }
  reset_(&s,&l);
  l.player.x=1700.0f;l.player.y=300.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!near_(l.player.vy,-705.0f));
  reset_(&s,&l);
  l.player.x=3310.0f;l.player.y=300.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!near_(l.player.vy,-675.0f));
  /* Steam takes priority over an overlapping clock pad, regardless of array order. */
  {
    const RikituLauncher mixed[2]={
      {RIKITU_LAUNCH_CLOCK,1700.0f,456.0f},
      {RIKITU_LAUNCH_STEAM,1700.0f,456.0f}
    };
    RikituPlayer p;
    rikitu_player_init(&p,1700.0f,413.0f);
    p.grounded=1;
    assert(rikitu_apply_surfaces(&p,RIKITU_FIXED_DT,11750.0f,
                                 NULL,0,mixed,2)==RIKITU_LAUNCH_STEAM);
    assert(near_(p.vy,-705.0f) && !p.grounded);
  }
  for(i=0;i<3;i++){
    reset_(&s,&l);
    l.player.x=belt_x[i];l.player.y=413.0f;l.player.grounded=1;
    assert(near_(s.belts[i].speed,belt_speed[i]));
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(near_(l.player.x,belt_x[i]+belt_speed[i]*RIKITU_FIXED_DT));
  }
  reset_(&s,&l);
  l.player.x=2940.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near_(s.active_checkpoint,2930.0f));
  l.player.x=6270.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near_(s.active_checkpoint,6260.0f));
  l.player.x=9270.0f;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near_(s.active_checkpoint,9260.0f));
  for(i=0;i<3;i++)s.stage.fragments[i]=1;
  s.stage.fragment_count=3;
  assert(!rikitu_stage_can_finish(&s.stage));
  l.player.x=11542.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!s.stage.completed);
  /* Isolate the final legitimate stomp; separate QA checks the three-hit cycle. */
  s.boss.hp=1;
  l.player.x=10951.0f;l.player.y=352.0f;l.player.vy=330.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!s.boss.alive && s.stage.boss_defeated && s.hearts==3);
  assert(near_(l.player.vy,-490.0f) && rikitu_stage_can_finish(&s.stage));
  l.player.x=11542.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(s.stage.completed);
  /* Five is a valid physical count; six is an error before any access. */
  reset_(&s,&l);
  s.launcher_count=RIKITU_MAX_LAUNCHERS+1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==-1);
  puts("PASS MAP16 Heart Chamber: 5 mixed lifts, 3 belts, boss gate and bounds");
  return 0;
}
