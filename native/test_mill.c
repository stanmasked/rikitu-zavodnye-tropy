#include "rikitu_mill.h"
#include "rikitu_boss.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int closef_(float x,float y){return fabsf(x-y)<.025f;}
int main(void){
  RikituScene s;
  RikituLoop l;
  RikituBoss b;
  RikituPlayer p;
  int hit;
  rikitu_mill_init(&s);
  assert(s.level_id==2 && s.world_width==8550.0f&&s.gate_x==8370.0f);
  assert(s.solid_count==18 && s.coin_total==90 && s.enemy_count==6);
  assert(s.checkpoint_total==3);
  assert(s.stage.boss_required && !s.stage.boss_defeated);
  assert(s.boss.hp==3 && s.boss.alive && closef_(s.boss.center,7910.0f));
  assert(s.solids[0].w==1810.0f && s.solids[5].x==545.0f);
  assert(s.checkpoints[0]==2200.0f && s.checkpoints[2]==6980.0f);
  assert(s.fragments[0].x==1650.0f && s.fragments[2].x==7030.0f);
  assert(!rikitu_stage_can_finish(&s.stage));
  for(hit=0;hit<3;hit++){
    b=s.boss;
    rikitu_player_init(&p,7911.0f,358.0f);
    p.vy=120.0f;
    b.hit_seconds=0.0f;
    b.hp=3-hit;
    b.alive=1;
    {
      RikituBossEvent e=rikitu_boss_tick(&b,&p,396.0f,RIKITU_FIXED_DT);
      assert(e==(hit==2?RIKITU_BOSS_DEFEATED:RIKITU_BOSS_HURT));
      assert(b.hp==2-hit&&b.alive==(hit<2));
      assert(closef_(p.vy,-490.0f));
      assert(closef_(p.y,355.0f));
    }
    if(hit==0){
      /* Invulnerable for 0.85s after a stomp and not damaged by contact. */
      p.y=399.0f;p.vy=0;
      assert(rikitu_boss_tick(&b,&p,442.0f,RIKITU_FIXED_DT)==RIKITU_BOSS_NONE);
      assert(b.hp==2);
    }
  }
  /* Enemy should walk at 48/s, with +/-80 around original spawn. */
  b=s.boss;
  rikitu_player_init(&p,105.0f,413.0f);
  assert(rikitu_boss_tick(&b,&p,456.0f,RIKITU_FIXED_DT)==RIKITU_BOSS_NONE);
  assert(closef_(b.x,7910.0f+48.0f/60.0f));
  b.x=b.center+80.0f;b.dir=1;
  assert(rikitu_boss_tick(&b,&p,456.0f,RIKITU_FIXED_DT)==RIKITU_BOSS_NONE);
  assert(b.dir==-1);
  /* Three fragments are insufficient before defeating boss. */
  for(hit=0;hit<3;hit++)s.stage.fragments[hit]=1;
  s.stage.fragment_count=3;
  assert(!rikitu_stage_can_finish(&s.stage));
  s.stage.boss_defeated=1;
  assert(rikitu_stage_can_finish(&s.stage));
  {
    RikituRect gate={8350.0f,413.0f,33.0f,43.0f};
    assert(rikitu_stage_touch_gate(&s.stage,gate,s.gate_x)==RIKITU_GATE_COMPLETED);
  }
  /* Integration: player lands a real physics tick onto the native boss. */
  rikitu_mill_init(&s);
  memset(&l,0,sizeof(l));
  l.player=s.player;
  l.player.x=7911.0f;l.player.y=352.0f;l.player.vy=330.0f;
  l.player.grounded=0;
  assert(rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,0)==1);
  assert(s.boss.hp==2 && l.player.vy==-490.0f && s.hearts==3);
  assert(rikitu_boss_tick(NULL,&p,0,RIKITU_FIXED_DT)==RIKITU_BOSS_INVALID);
  assert(rikitu_boss_tick(&b,&p,0,-1.0f)==RIKITU_BOSS_INVALID);
  puts("PASS MAP02 mill: exact map, 90 coins, six enemies, 3-hit native boss, bounce, immunity and gated exit");
  return 0;
}
