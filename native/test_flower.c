#include "rikitu_flower.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int approx(float a,float b){return fabsf(a-b)<0.05f;}
static void setup(RikituScene *s,RikituLoop *l){
  rikitu_flower_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
int main(void){
  RikituScene s;
  RikituLoop loop;
  int i;
  setup(&s,&loop);
  assert(s.level_id==1 && s.world_width==7900.0f && s.gate_x==7770.0f);
  assert(s.solid_count==19 && s.coin_total==90);
  assert(s.belt_count==0 && s.launcher_count==0);
  assert(s.checkpoint_total==3 && s.reached_checkpoints==0);
  assert(approx(s.active_checkpoint,105.0f));
  assert(s.solids[0].x==0.0f && s.solids[0].w==1660.0f);
  assert(s.solids[1].x==1735.0f && s.solids[1].w==1285.0f);
  assert(s.solids[4].x==5865.0f && s.solids[4].w==2035.0f);
  assert(s.solids[5].x==490.0f && s.solids[5].y==390.0f);
  assert(s.solids[18].x==7330.0f && s.solids[18].y==388.0f);
  assert(s.fragments[0].x==1530.0f && s.fragments[1].x==3650.0f);
  assert(s.fragments[2].x==6870.0f);
  assert(s.checkpoints[0]==2050.0f && s.checkpoints[1]==4490.0f &&
         s.checkpoints[2]==6150.0f);
  /* All 90 coins live within the real level and no coordinate is duplicated. */
  for(i=0;i<(int)s.coin_total;i++){
    int j;
    assert(s.coins[i].x>=0.0f && s.coins[i].x<s.world_width);
    for(j=0;j<i;j++)
      assert(s.coins[i].x!=s.coins[j].x || s.coins[i].y!=s.coins[j].y);
  }
  /* The browser spawns at x=105 and grounds the first tick. */
  assert(approx(s.player.x,105.0f) && approx(s.player.y,413.0f));
  assert(!s.player.grounded);
  assert(rikitu_demo_advance(&s,&loop,RIKITU_FIXED_DT,0)==1);
  assert(s.player.grounded);
  /* Gap checks: there is no artificial solid over x=1680. */
  for(i=0;i<(int)s.solid_count;i++){
    RikituRect *b=&s.solids[i];
    if(b->y==456.0f)
      assert(!(1680.0f>=b->x && 1680.0f<b->x+b->w));
  }
  /* All three checkpoints are crossed in order, each only once. */
  for(i=0;i<3;i++){
    loop.player.x=s.checkpoints[i]-1.0f;
    loop.player.y=413.0f;
    loop.player.grounded=1;
    assert(rikitu_demo_advance(&s,&loop,RIKITU_FIXED_DT,RIKITU_INPUT_RIGHT)==1);
    assert(s.reached_checkpoints==i+1);
    assert(approx(s.active_checkpoint,s.checkpoints[i]));
    assert(rikitu_demo_advance(&s,&loop,RIKITU_FIXED_DT,0)==1);
    assert(s.reached_checkpoints==i+1);
  }
  /* AABB pickup rects are unchanged; items are collected once. */
  setup(&s,&loop);
  for(i=0;i<3;i++){
    const RikituPoint p=s.fragments[i];
    const RikituRect collector={p.x-10.0f,p.y-10.0f,33.0f,43.0f};
    assert(rikitu_stage_pickups(&s.stage,collector,s.fragments,
                                s.coins,s.coin_total)>=1);
  }
  assert(s.stage.fragment_count==3);
  assert(rikitu_stage_can_finish(&s.stage));
  loop.player.x=7765.0f;loop.player.y=413.0f;loop.player.grounded=1;
  assert(rikitu_demo_advance(&s,&loop,RIKITU_FIXED_DT,RIKITU_INPUT_RIGHT)==1);
  assert(s.stage.completed);
  assert(rikitu_demo_advance(&s,&loop,1.0f,RIKITU_INPUT_RIGHT)==0);
  /* Invalid counters fail, and the old synthetic demo remains loadable. */
  setup(&s,&loop);s.solid_count=RIKITU_MAX_SOLIDS+1;
  assert(rikitu_demo_advance(&s,&loop,RIKITU_FIXED_DT,0)==-1);
  rikitu_demo_init(&s);
  assert(s.solid_count==RIKITU_DEMO_SOLIDS);
  assert(s.coin_total==RIKITU_DEMO_COINS);
  puts("PASS native Flower Trail: true 7900-unit map, all 19 solids, 90 coins, checkpoints, pickup and gate");
  return 0;
}
