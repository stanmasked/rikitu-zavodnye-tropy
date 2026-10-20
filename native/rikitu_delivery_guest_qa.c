#include "rikitu_delivery_guest_qa.h"
#include "rikitu_delivery.h"
#include "rikitu_demo.h"
#include <math.h>
#include <string.h>
static int near(float a,float b){return fabsf(a-b)<0.09f;}
static void reset(RikituScene *s,RikituLoop *l){
  rikitu_delivery_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
static int belt(RikituScene *s,RikituLoop *l,float at,
                float speed,unsigned input,float motion){
  reset(s,l);
  l->player.x=at;l->player.y=413.0f;l->player.grounded=1;
  return rikitu_demo_advance(s,l,RIKITU_FIXED_DT,input)==1 &&
         near(l->player.x,at+(speed+motion)*RIKITU_FIXED_DT) &&
         l->player.grounded && s->hearts==3 && s->damage_count==0;
}
unsigned rikitu_guest_delivery_checks(void){
  RikituScene s;
  RikituLoop l;
  unsigned bits=0;
  int good,i;
  static const float steam_x[3]={1350.0f,4740.0f,7910.0f};
  reset(&s,&l);
  if(s.level_id==6 && s.world_width==9910.0f && s.gate_x==9730.0f &&
     s.solid_count==18 && s.coin_total==90 && s.enemy_count==6 &&
     s.checkpoint_total==3 && s.belt_count==3 && s.launcher_count==3 &&
     s.stage.boss_required && s.boss.alive && s.boss.hp==3 &&
     s.boss.center==9210.0f &&
     s.belts[0].x==590.0f && s.belts[0].speed==42.0f &&
     s.belts[1].x==3980.0f && s.belts[1].speed==-48.0f &&
     s.belts[2].x==7480.0f && s.belts[2].speed==50.0f &&
     s.launchers[0].x==1350.0f && s.launchers[1].x==4740.0f &&
     s.launchers[2].x==7910.0f)
    bits|=RIKITU_DELIVERY_QA_DATA;
  good=belt(&s,&l,650.0f,42.0f,0,0.0f);
  good=good && belt(&s,&l,4100.0f,-48.0f,0,0.0f);
  good=good && belt(&s,&l,7520.0f,50.0f,0,0.0f);
  good=good && belt(&s,&l,4100.0f,-48.0f,RIKITU_INPUT_RIGHT,225.0f);
  good=good && belt(&s,&l,4100.0f,-48.0f,RIKITU_INPUT_LEFT,-225.0f);
  if(good)bits|=RIKITU_DELIVERY_QA_BELTS;
  good=1;
  for(i=0;i<3;i++){
    reset(&s,&l);
    l.player.x=steam_x[i];l.player.y=413.0f;l.player.grounded=1;
    if(s.launchers[i].kind!=RIKITU_LAUNCH_STEAM ||
       rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)!=1 ||
       !near(l.player.vy,-705.0f) || l.player.grounded ||
       s.hearts!=3 || s.damage_count!=0)good=0;
  }
  reset(&s,&l);
  l.player.x=1350.0f;l.player.y=300.0f;l.player.grounded=0;
  if(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)!=1 ||
     near(l.player.vy,-705.0f))good=0;
  if(good)bits|=RIKITU_DELIVERY_QA_VENTS;
  reset(&s,&l);
  good=!rikitu_stage_can_finish(&s.stage);
  for(i=0;i<3;i++)s.stage.fragments[i]=1;
  s.stage.fragment_count=3;
  good=good && !rikitu_stage_can_finish(&s.stage);
  /* All three hits are real fixed-timestep boss collisions.
   * Repositioning between hits deliberately isolates combat logic from
   * the untested full-controller route; invulnerability expires through
   * 54 ordinary engine ticks, never by modifying the cooldown. */
  for(i=0;i<3 && good;i++){
    int frame;
    l.player.x=s.boss.x+1.0f;
    l.player.y=352.0f;
    l.player.vy=330.0f;
    l.player.grounded=0;
    good=rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
         s.boss.hp==2-i && s.boss.alive==(i<2) &&
         s.stage.boss_defeated==(i==2) &&
         near(l.player.vy,-490.0f) && s.hearts==3 &&
         s.damage_count==0 &&
         rikitu_stage_can_finish(&s.stage)==(i==2);
    if(!good || i==2)break;
    /* A second stomp during the 0.85s shield must have no effect. */
    l.player.x=s.boss.x+1.0f;
    l.player.y=352.0f;
    l.player.vy=330.0f;
    l.player.grounded=0;
    good=rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
         s.boss.hp==2-i && s.boss.alive &&
         s.boss.hit_seconds>0.0f && s.hearts==3;
    /* Wait outside boss contact while its real timer expires. */
    l.player.x=9060.0f;
    l.player.y=413.0f;
    l.player.vy=0.0f;
    l.player.grounded=1;
    for(frame=0;frame<54 && good;frame++)
      good=rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
           s.hearts==3 && s.damage_count==0 &&
           !s.stage.completed;
    good=good && near(s.boss.hit_seconds,0.0f);
  }
  if(good){
    l.player.x=9715.0f;l.player.y=413.0f;l.player.grounded=1;
    good=rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
         s.stage.completed && !s.boss.alive;
  }
  if(good)bits|=RIKITU_DELIVERY_QA_BOSS_GATE;
  return bits;
}
