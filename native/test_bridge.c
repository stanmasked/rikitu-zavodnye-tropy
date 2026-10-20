#include "rikitu_bridge.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int closef_(float x,float y){return fabsf(x-y)<0.06f;}
static void start(RikituScene *s,RikituLoop *l){
  rikitu_bridge_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
int main(void){
  RikituScene s;
  RikituLoop l;
  const float switch_x[3]={2260,4930,6990};
  int i;
  start(&s,&l);
  assert(s.level_id==4 && s.world_width==8880.0f && s.gate_x==8740.0f);
  assert(s.solid_count==18 && s.coin_total==90 && s.enemy_count==6);
  assert(s.lever_count==3 && s.levers_on==0 && s.checkpoint_total==3);
  assert(s.checkpoints[0]==2100.0f && s.checkpoints[1]==4570.0f &&
         s.checkpoints[2]==6900.0f);
  assert(s.fragments[0].x==1580.0f&&s.fragments[2].x==7420.0f);
  assert(s.boss.alive && s.boss.hp==3 && closef_(s.boss.x,8290));
  assert(s.stage.boss_required && !s.stage.boss_defeated);
  for(i=0;i<3;i++){
    assert(closef_(s.levers[i].x,switch_x[i]));
    assert(s.levers[i].y==456.0f && !s.levers[i].on);
    /* The browser toggles by player overlap, without button presses. */
    l.player.x=switch_x[i];l.player.y=413.0f;l.player.grounded=1;
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(s.levers[i].on && s.levers_on==i+1);
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(s.levers_on==i+1);
  }
  /* The levers are audiovisual interactions, not mandatory gate keys. */
  start(&s,&l);
  for(i=0;i<3;i++)s.stage.fragments[i]=1;
  s.stage.fragment_count=3;
  assert(!rikitu_stage_can_finish(&s.stage));
  s.stage.boss_defeated=1;
  assert(rikitu_stage_can_finish(&s.stage));
  assert(s.levers_on==0);
  {
    const RikituRect door={8720.0f,413.0f,33.0f,43.0f};
    assert(rikitu_stage_touch_gate(&s.stage,door,8740.0f)==RIKITU_GATE_COMPLETED);
  }
  /* Boss itself still requires a precise downward stomp three times. */
  start(&s,&l);
  for(i=0;i<3;i++){
    RikituBossEvent event;
    RikituPlayer player;
    rikitu_player_init(&player,8291.0f,357.0f);
    player.vy=120.0f;
    s.boss.hit_seconds=0;
    event=rikitu_boss_tick(&s.boss,&player,399.0f,RIKITU_FIXED_DT);
    assert(event==(i==2?RIKITU_BOSS_DEFEATED:RIKITU_BOSS_HURT));
    assert(s.boss.hp==2-i && closef_(player.vy,-490.0f));
  }
  assert(!s.boss.alive);
  /* A too-high player must not remotely activate bridge switches. */
  start(&s,&l);
  l.player.x=2260.0f;l.player.y=250.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(!s.levers[0].on);
  s.lever_count=RIKITU_MAX_LEVERS+1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==-1);
  puts("PASS MAP04 Bridge: exact 8880-unit map, three touch levers, 90 coins, six patrols, 3-hit guardian and boss-only gate");
  return 0;
}
