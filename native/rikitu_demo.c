#include "rikitu_demo.h"
#include <string.h>
#include <math.h>
void rikitu_demo_init(RikituScene *s) {
  unsigned i;
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=3000.0f;s->gate_x=2890.0f;
  s->solid_count=RIKITU_DEMO_SOLIDS;
  s->coin_total=RIKITU_DEMO_COINS;
  s->belt_count=RIKITU_DEMO_BELTS;
  s->launcher_count=RIKITU_DEMO_LAUNCHERS;
  s->active_checkpoint=105.0f;
  s->hearts=RIKITU_STARTING_HEARTS;
  s->max_hearts=RIKITU_STARTING_HEARTS;
  rikitu_player_init(&s->player,120.0f,497.0f);
  s->player.grounded=1;
  rikitu_stage_init(&s->stage,0);
  s->solids[0]=(RikituRect){0.0f,540.0f,3000.0f,180.0f};
  s->solids[1]=(RikituRect){520.0f,468.0f,240.0f,18.0f};
  s->solids[2]=(RikituRect){1140.0f,452.0f,260.0f,18.0f};
  s->solids[3]=(RikituRect){1980.0f,466.0f,240.0f,18.0f};
  s->fragments[0]=(RikituPoint){610.0f,506.0f};
  s->fragments[1]=(RikituPoint){1460.0f,506.0f};
  s->fragments[2]=(RikituPoint){2540.0f,506.0f};
  for(i=0;i<RIKITU_DEMO_COINS;i++){
    s->coins[i].x=210.0f+130.0f*(float)i;
    s->coins[i].y=510.0f;
  }
  s->belts[0]=(RikituConveyor){815.0f,180.0f,85.0f};
  s->belts[1]=(RikituConveyor){1610.0f,160.0f,-65.0f};
  s->launchers[0]=(RikituLauncher){RIKITU_LAUNCH_STEAM,1050.0f,540.0f};
  s->launchers[1]=(RikituLauncher){RIKITU_LAUNCH_CLOCK,1850.0f,540.0f};
}


/* Match browser respawn: keep collected parts, coins, killed enemies and
 * checkpoint progress. Stop the current motion and grant 1.4s safety. */
static void respawn(RikituScene *s,RikituLoop *loop) {
  loop->player.x=s->active_checkpoint;
  loop->player.y=456.0f-loop->player.h;
  loop->player.vx=0.0f;
  loop->player.vy=0.0f;
  loop->player.grounded=0;
  s->invulnerability_seconds=1.4f;
}
static void take_damage(RikituScene *s,RikituLoop *loop) {
  if(s->invulnerability_seconds>0.0f)return;
  s->hearts=rikitu_heart_after_damage(s->hearts);
  s->invulnerability_seconds=1.6f;
  s->damage_count++;
  if(s->hearts==0) {
    s->hearts=s->max_hearts;
    respawn(s,loop);
  }
}
int rikitu_demo_advance(RikituScene *s,RikituLoop *loop,
                        float wall_seconds,unsigned input) {
  int ticks=0;
  size_t i;
  if(!s || !loop || s->solid_count>RIKITU_MAX_SOLIDS ||
     s->coin_total>RIKITU_MAX_COINS ||
     s->belt_count>RIKITU_MAX_BELTS ||
     s->launcher_count>RIKITU_MAX_LAUNCHERS ||
     s->checkpoint_total>RIKITU_MAX_CHECKPOINTS ||
     s->enemy_count>RIKITU_MAX_ENEMIES ||
     s->max_hearts<1 || s->max_hearts>RIKITU_MAX_HEARTS ||
     s->hearts<1 || s->hearts>s->max_hearts ||
     !isfinite(s->invulnerability_seconds) || s->invulnerability_seconds<0.0f ||
     !isfinite(wall_seconds) || wall_seconds<0.0f ||
     !isfinite(s->world_width) || s->world_width<loop->player.w)
    return -1;
  s->paused=loop->paused!=0;
  if(s->paused || s->stage.completed) {
    loop->accumulator_seconds=0.0f;
    s->player=loop->player;
    return 0;
  }
  if(wall_seconds>0.1f)wall_seconds=0.1f;
  loop->accumulator_seconds+=wall_seconds;
  while(loop->accumulator_seconds+0.0000001f>=RIKITU_FIXED_DT && ticks<6) {
    const float old_bottom=loop->player.y+loop->player.h;
    s->invulnerability_seconds=fmaxf(0.0f,
                               s->invulnerability_seconds-RIKITU_FIXED_DT);
    if(!rikitu_player_step(&loop->player,input,RIKITU_FIXED_DT,
                           s->world_width,s->solids,s->solid_count))
      return -1;
    if(rikitu_apply_surfaces(&loop->player,RIKITU_FIXED_DT,s->world_width,
                             s->belts,s->belt_count,
                             s->launchers,s->launcher_count)<0)
      return -1;
    /* Browser's world height is 540; death threshold is 540+150. */
    if(loop->player.y>690.0f) {
      take_damage(s,loop);
      respawn(s,loop);
    }
    for(i=0;i<s->checkpoint_total;i++) {
      if(loop->player.x>=s->checkpoints[i] &&
         s->active_checkpoint<s->checkpoints[i]) {
        s->active_checkpoint=s->checkpoints[i];
        s->reached_checkpoints++;
      }
    }
    {
      const RikituRect player={loop->player.x,loop->player.y,
                               loop->player.w,loop->player.h};
      if(rikitu_stage_pickups(&s->stage,player,s->fragments,
                              s->coins,s->coin_total)<0)
        return -1;
    }
    for(i=0;i<s->enemy_count;i++) {
      const RikituEnemyEvent event=rikitu_enemy_tick(
                        &s->enemies[i],&loop->player,
                        old_bottom,RIKITU_FIXED_DT);
      if(event==RIKITU_ENEMY_INVALID)return -1;
      if(event==RIKITU_ENEMY_CONTACT)take_damage(s,loop);
    }
    {
      const RikituRect player={loop->player.x,loop->player.y,
                               loop->player.w,loop->player.h};
      if(rikitu_stage_touch_gate(&s->stage,player,s->gate_x)==RIKITU_GATE_INVALID)
        return -1;
    }
    loop->accumulator_seconds-=RIKITU_FIXED_DT;
    if(loop->accumulator_seconds<0.0f)loop->accumulator_seconds=0.0f;
    loop->active_seconds+=RIKITU_FIXED_DT;
    ticks++;
    if(s->stage.completed) {
      loop->accumulator_seconds=0.0f;
      break;
    }
  }
  s->player=loop->player;
  return ticks;
}
