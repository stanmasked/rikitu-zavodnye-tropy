#include "rikitu_bridge_guest_qa.h"
#include "rikitu_bridge.h"
#include <math.h>
#include <string.h>
static int near(float a,float b){return fabsf(a-b)<0.05f;}
static void reset(RikituScene *s,RikituLoop *l){
  rikitu_bridge_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
unsigned rikitu_guest_bridge_checks(void){
  RikituScene s;
  RikituLoop l;
  unsigned result=0;
  int good=1,i;
  static const float lever_x[3]={2260.0f,4930.0f,6990.0f};
  reset(&s,&l);
  if(s.level_id==4 && s.world_width==8880.0f &&
     s.gate_x==8740.0f && s.solid_count==18 &&
     s.coin_total==90 && s.checkpoint_total==3 &&
     s.enemy_count==6 && s.lever_count==3 &&
     s.boss.alive && s.boss.hp==3 && s.boss.x==8290.0f)
    result|=RIKITU_BRIDGE_QA_MAP;
  for(i=0;i<3;i++){
    l.player.x=lever_x[i];l.player.y=413.0f;l.player.grounded=1;
    if(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)!=1 ||
       !s.levers[i].on || s.levers_on!=i+1)good=0;
    if(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)!=1 ||
       s.levers_on!=i+1)good=0;
  }
  if(good)result|=RIKITU_BRIDGE_QA_LEVERS;
  reset(&s,&l);
  l.player.x=8291.0f;l.player.y=352.0f;
  l.player.vy=330.0f;l.player.grounded=0;
  if(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
     s.boss.hp==2 && s.boss.hit_seconds>0.8f &&
     s.boss.alive && near(l.player.vy,-490.0f) &&
     s.hearts==3 && !s.stage.boss_defeated)
    result|=RIKITU_BRIDGE_QA_GUARDIAN;
  /* Stage 4: switches are visual only, but defeating guardian required. */
  reset(&s,&l);
  good=!rikitu_stage_can_finish(&s.stage);
  for(i=0;i<3;i++)s.stage.fragments[i]=1;
  s.stage.fragment_count=3;
  good=good&&!rikitu_stage_can_finish(&s.stage);
  s.boss.hp=1;
  l.player.x=8291.0f;l.player.y=352.0f;
  l.player.vy=330.0f;l.player.grounded=0;
  good=good && rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
       !s.boss.alive && s.stage.boss_defeated &&
       s.levers_on==0 && rikitu_stage_can_finish(&s.stage);
  if(good){
    l.player.x=8720.0f;l.player.y=413.0f;l.player.vy=0.0f;
    l.player.grounded=1;
    good=rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
         s.stage.completed && s.levers_on==0;
  }
  if(good)result|=RIKITU_BRIDGE_QA_GATE;
  return result;
}
