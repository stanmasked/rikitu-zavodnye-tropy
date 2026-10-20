#include "rikitu_workshop_guest_qa.h"
#include "rikitu_workshop.h"
#include "rikitu_demo.h"
#include <math.h>
#include <string.h>
static int close_(float a,float b){return fabsf(a-b)<0.09f;}
static void reset(RikituScene *s,RikituLoop *l){
  rikitu_workshop_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
static int belt_step(RikituScene *s,RikituLoop *l,
                     float x,unsigned input,float expected_x){
  reset(s,l);
  l->player.x=x;l->player.y=413.0f;l->player.grounded=1;
  return rikitu_demo_advance(s,l,RIKITU_FIXED_DT,input)==1 &&
         close_(l->player.x,expected_x) && l->player.grounded &&
         s->hearts==3 && s->damage_count==0;
}
unsigned rikitu_guest_workshop_checks(void){
  RikituScene s;
  RikituLoop l;
  unsigned bits=0;
  int good,i;
  reset(&s,&l);
  if(s.level_id==5 && s.world_width==9140.0f && s.gate_x==8950.0f &&
     s.solid_count==18 && s.coin_total==90 && s.enemy_count==6 &&
     s.checkpoint_total==3 && s.belt_count==3 &&
     !s.stage.boss_required && s.lever_count==0 &&
     s.belts[0].x==1070.0f && s.belts[0].width==300.0f &&
     s.belts[0].speed==48.0f &&
     s.belts[1].x==3890.0f && s.belts[1].width==300.0f &&
     s.belts[1].speed==-42.0f &&
     s.belts[2].x==6960.0f && s.belts[2].width==300.0f &&
     s.belts[2].speed==52.0f)
    bits|=RIKITU_WORKSHOP_QA_DATA;
  good=belt_step(&s,&l,1100.0f,0,1100.0f+48.0f*RIKITU_FIXED_DT);
  good=good && belt_step(&s,&l,7000.0f,0,
                          7000.0f+52.0f*RIKITU_FIXED_DT);
  good=good && belt_step(&s,&l,1100.0f,RIKITU_INPUT_RIGHT,
                          1100.0f+(225.0f+48.0f)*RIKITU_FIXED_DT);
  if(good)bits|=RIKITU_WORKSHOP_QA_FORWARD;
  good=belt_step(&s,&l,4000.0f,0,4000.0f-42.0f*RIKITU_FIXED_DT);
  good=good && belt_step(&s,&l,4000.0f,RIKITU_INPUT_LEFT,
                          4000.0f+(-225.0f-42.0f)*RIKITU_FIXED_DT);
  good=good && belt_step(&s,&l,4000.0f,RIKITU_INPUT_RIGHT,
                          4000.0f+(225.0f-42.0f)*RIKITU_FIXED_DT);
  if(good)bits|=RIKITU_WORKSHOP_QA_REVERSE;
  reset(&s,&l);
  l.player.x=1100.0f;l.player.y=413.0f;l.player.grounded=1;
  good=rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,RIKITU_INPUT_JUMP)==1 &&
       !l.player.grounded && l.player.vy<0.0f &&
       close_(l.player.x,1100.0f);
  reset(&s,&l);
  l.player.x=4000.0f;l.player.y=290.0f;l.player.grounded=0;
  good=good && rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
       close_(l.player.x,4000.0f);
  reset(&s,&l);
  good=good && !rikitu_stage_can_finish(&s.stage);
  for(i=0;i<3;i++)s.stage.fragments[i]=1;
  s.stage.fragment_count=3;
  good=good && rikitu_stage_can_finish(&s.stage);
  l.player.x=8930.0f;l.player.y=413.0f;l.player.grounded=1;
  good=good && rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
       s.stage.completed && !s.stage.boss_required;
  if(good)bits|=RIKITU_WORKSHOP_QA_AIRBORNE_GATE;
  return bits;
}
