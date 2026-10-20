#include "rikitu_grove_guest_qa.h"
#include "rikitu_grove.h"
#include <math.h>
#include <string.h>
static int close_(float a,float b){return fabsf(a-b)<0.05f;}
static void init(RikituScene *s,RikituLoop *l){
  rikitu_grove_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
static int spring(RikituScene *s,RikituLoop *l,float x){
  init(s,l);
  l->player.x=x;
  l->player.y=413.0f;
  l->player.grounded=1;
  if(rikitu_demo_advance(s,l,RIKITU_FIXED_DT,0)!=1)return 0;
  return close_(l->player.vy,-730.0f) &&
         !l->player.grounded &&
         s->hearts==3 && s->damage_count==0;
}
unsigned rikitu_guest_grove_checks(void){
  RikituScene s;
  RikituLoop l;
  unsigned result=0;
  int good;
  init(&s,&l);
  if(s.level_id==3 && s.world_width==8270.0f &&
     s.gate_x==8140.0f && s.solid_count==18 &&
     s.coin_total==90 && s.enemy_count==6 &&
     s.checkpoint_total==3 && s.launcher_count==3 &&
     s.launchers[0].kind==RIKITU_LAUNCH_MUSHROOM &&
     s.launchers[1].kind==RIKITU_LAUNCH_MUSHROOM &&
     s.launchers[2].kind==RIKITU_LAUNCH_MUSHROOM &&
     s.launchers[0].x==940.0f &&
     s.launchers[1].x==2670.0f &&
     s.launchers[2].x==5660.0f &&
     !s.stage.boss_required)
    result|=RIKITU_GROVE_QA_DATA;
  if(spring(&s,&l,940.0f))
    result|=RIKITU_GROVE_QA_FIRST;
  good=spring(&s,&l,2670.0f);
  if(good)good=spring(&s,&l,5660.0f);
  if(good)result|=RIKITU_GROVE_QA_OTHERS;
  init(&s,&l);
  l.player.x=940.0f;
  l.player.y=350.0f;
  l.player.grounded=0;
  good=(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
        !close_(l.player.vy,-730.0f));
  init(&s,&l);
  good=good&&!rikitu_stage_can_finish(&s.stage);
  if(good){
    int i;
    for(i=0;i<3;i++)s.stage.fragments[i]=1;
    s.stage.fragment_count=3;
    good=rikitu_stage_can_finish(&s.stage);
  }
  if(good){
    l.player.x=8120.0f;l.player.y=413.0f;l.player.grounded=1;
    good=(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
          s.stage.completed);
  }
  if(good)result|=RIKITU_GROVE_QA_GATE;
  return result;
}
