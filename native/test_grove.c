#include "rikitu_grove.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int closef_(float a,float b){return fabsf(a-b)<0.05f;}
static void setup(RikituScene *s,RikituLoop *l){
  rikitu_grove_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
int main(void) {
  RikituScene s;
  RikituLoop l;
  const float mushrooms[3]={940.0f,2670.0f,5660.0f};
  int i,j;
  setup(&s,&l);
  assert(s.level_id==3 && s.world_width==8270.0f && s.gate_x==8140.0f);
  assert(s.solid_count==18 && s.coin_total==90 && s.enemy_count==6);
  assert(s.launcher_count==3 && s.belt_count==0 && !s.stage.boss_required);
  assert(s.checkpoint_total==3 && s.checkpoints[0]==2010.0f &&
         s.checkpoints[1]==4330.0f && s.checkpoints[2]==6520.0f);
  assert(s.fragments[0].x==1530.0f && s.fragments[2].x==7040.0f);
  assert(s.solids[0].w==1670.0f && s.solids[5].x==520.0f);
  /* All three native mushrooms use the correct rectangular overlap
   * and vertical spring impulse; each is evaluated by a real game tick. */
  for(i=0;i<3;i++){
    setup(&s,&l);
    assert(s.launchers[i].kind==RIKITU_LAUNCH_MUSHROOM);
    assert(closef_(s.launchers[i].x,mushrooms[i]));
    assert(s.launchers[i].y==456.0f);
    l.player.x=mushrooms[i];l.player.y=413.0f;l.player.grounded=1;
    assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
    assert(closef_(l.player.vy,-730.0f)&&!l.player.grounded);
    assert(s.hearts==3 && s.damage_count==0);
  }
  /* Mushroom activation requires player to be grounded and within +-7
   * vertical game units: cannot trigger during an airborne shortcut. */
  setup(&s,&l);
  l.player.x=940.0f;l.player.y=350.0f;l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(l.player.vy!=-730.0f);
  /* 90 browser coins must lie within the actual map and be unique. */
  for(i=0;i<90;i++){
    assert(s.coins[i].x>=0 && s.coins[i].x<8270.0f);
    for(j=0;j<i;j++)
      assert(s.coins[i].x!=s.coins[j].x || s.coins[i].y!=s.coins[j].y);
  }
  /* No mini-boss: three fragments alone permit passage through gate. */
  setup(&s,&l);
  assert(!rikitu_stage_can_finish(&s.stage));
  for(i=0;i<3;i++)s.stage.fragments[i]=1;
  s.stage.fragment_count=3;
  assert(rikitu_stage_can_finish(&s.stage));
  l.player.x=8120.0f;l.player.y=413.0f;l.player.grounded=1;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(s.stage.completed);
  puts("PASS MAP03 Mushroom Grove: source-parity geometry, all 3 native springs, 90 coins, 3 checkpoints, exit");
  return 0;
}
