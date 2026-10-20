#include "rikitu_mist.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int near(float x,float y){return fabsf(x-y)<.09f;}
static void reset(RikituScene *s,RikituLoop *l){rikitu_mist_init(s);memset(l,0,sizeof(*l));l->player=s->player;}
int main(void){
 RikituScene s;RikituLoop l;int i;
 const float g[3]={1240.0f,3880.0f,7520.0f};
 reset(&s,&l);
 assert(s.level_id==9 && s.world_width==9520.0f && s.gate_x==9330.0f);
 assert(s.solid_count==18 && s.coin_total==90 && s.enemy_count==6);
 assert(s.launcher_count==3 && s.belt_count==0 && s.checkpoint_total==3);
 assert(s.fragments[0].x==1740.0f && s.fragments[2].x==8170.0f);
 assert(!s.stage.boss_required && !s.boss.alive);
 for(i=0;i<3;i++){
  reset(&s,&l);l.player.x=g[i];l.player.y=413.0f;l.player.grounded=1;
  assert(s.launchers[i].kind==RIKITU_LAUNCH_GUST && s.launchers[i].y==456.0f);
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(near(l.player.vy,-690.0f) && !l.player.grounded && near(l.player.x,g[i]));
  assert(s.hearts==3 && s.damage_count==0);
  reset(&s,&l);l.player.x=g[i];l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,RIKITU_INPUT_RIGHT)==1);
  assert(near(l.player.vy,-690.0f) && near(l.player.x,g[i]+225.0f*RIKITU_FIXED_DT));
 }
 reset(&s,&l);l.player.x=1240.0f;l.player.y=305.0f;l.player.grounded=0;
 assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
 assert(!near(l.player.vy,-690.0f));
 reset(&s,&l);
 l.player.x=2290.0f;l.player.y=413.0f;l.player.grounded=1;
 assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && near(s.active_checkpoint,2280.0f));
 l.player.x=4890.0f;assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && near(s.active_checkpoint,4880.0f));
 l.player.x=7300.0f;assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && near(s.active_checkpoint,7290.0f));
 l.player.x=9320.0f;assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && !s.stage.completed);
 for(i=0;i<3;i++){
  s.stage.fragments[i]=1;
 }
 s.stage.fragment_count=3;
 assert(rikitu_stage_can_finish(&s.stage));
 assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1 && s.stage.completed);
 reset(&s,&l);s.launcher_count=RIKITU_MAX_LAUNCHERS+1;
 assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==-1);
 puts("PASS MAP09 mist: 3 gusts -690, zero forced drift, checkpoints and fragment-only exit");
 return 0;
}
