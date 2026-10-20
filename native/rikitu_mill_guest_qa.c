#include "rikitu_mill_guest_qa.h"
#include "rikitu_mill.h"
#include <math.h>
#include <string.h>
static int near(float x,float y){return fabsf(x-y)<.05f;}
static void start(RikituScene *s,RikituLoop *l){
  rikitu_mill_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
unsigned rikitu_guest_mill_checks(void){
  RikituScene s;
  RikituLoop l;
  unsigned mask=0;
  int valid=1,i;
  start(&s,&l);
  if(s.level_id==2 && s.world_width==8550.0f &&
     s.solid_count==18 && s.coin_total==90 && s.enemy_count==6 &&
     s.checkpoint_total==3 && s.stage.boss_required &&
     s.boss.alive && s.boss.hp==3 && s.gate_x==8370.0f)
    mask|=RIKITU_MILL_QA_DATA;
  /* Real 60-Hz game tick collides with boss top and bounces back. */
  start(&s,&l);
  l.player.x=7911.0f;l.player.y=352.0f;
  l.player.vy=330.0f;l.player.grounded=0;
  if(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
     s.boss.hp==2 && s.boss.alive && near(l.player.vy,-490.0f) &&
     s.boss.hit_seconds>.80f && s.hearts==3 && !s.stage.boss_defeated)
    mask|=RIKITU_MILL_QA_STOMP;
  /* Side collision drains one heart; 20 more overlapped ticks stay immune. */
  start(&s,&l);
  l.player.x=7912.0f;l.player.y=413.0f;l.player.grounded=1;
  valid=(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
         s.hearts==2 && s.damage_count==1 &&
         s.invulnerability_seconds>1.55f);
  for(i=0;valid&&i<20;i++)
    if(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)!=1)valid=0;
  if(valid && s.hearts==2 && s.damage_count==1)
    mask|=RIKITU_MILL_QA_CONTACT;
  /* Final hit must unlock exit only after all three real fragments. */
  start(&s,&l);
  for(i=0;i<3;i++)s.stage.fragments[i]=1;
  s.stage.fragment_count=3;
  valid=!rikitu_stage_can_finish(&s.stage);
  s.boss.hp=1;
  l.player.x=7911.0f;l.player.y=352.0f;l.player.vy=330.0f;
  l.player.grounded=0;
  if(valid && rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
     !s.boss.alive && s.boss.hp==0 &&
     s.stage.boss_defeated && rikitu_stage_can_finish(&s.stage)){
    l.player.x=8350.0f;l.player.y=413.0f;
    l.player.vy=0.0f;l.player.grounded=1;
    if(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 &&
       s.stage.completed)
      mask|=RIKITU_MILL_QA_GATE;
  }
  return mask;
}
