/* Stage168: an ordinary-input fall into MAP01's second gap then respawn.
 * This is an asset-free deterministic recovery regression, NOT player timing.
 */
#include "rikitu_flower.h"
#include "rikitu_map01_telemetry.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct FaultRecovery {
  RikituScene scene;
  RikituLoop loop;
  RikituMap01Telemetry telemetry;
  unsigned frames,ticks,jumps;
  int phase, prior_damage, prior_coins;
  float first_checkpoint_time, respawn_time, recovery_time;
} FaultRecovery;
static int close_to(float a,float b,float tolerance){
  return isfinite(a)&&isfinite(b)&&fabsf(a-b)<=tolerance;
}
static void traverse(FaultRecovery *r,int hz){
  int f;
  assert(hz==30||hz==60||hz==120);
  memset(r,0,sizeof(*r));
  rikitu_flower_init(&r->scene);
  r->loop.player=r->scene.player;
  assert(rikitu_map01_telemetry_start(&r->telemetry,&r->scene,&r->loop));
  for(f=0;f<hz*45;f++){
    unsigned input=RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN;
    const float old_x=r->loop.player.x;
    int n,jump_requested=0;
    /* The first jump collects required part #1 and clears gap #1.
     * Then intentionally omit jumping at the next gap x3020..3095.
     * After actual respawn, run ahead again with ordinary controls. */
    if(r->phase==0 && r->jumps==0 && r->loop.player.grounded &&
       r->loop.player.x>=1542.0f && r->loop.player.x<1600.0f){
      input|=RIKITU_INPUT_JUMP;
      jump_requested=1;
    }
    n=rikitu_demo_advance(&r->scene,&r->loop,1.0f/(float)hz,input);
    assert(n>=0);
    assert(rikitu_map01_telemetry_observe(&r->telemetry,
                                           &r->scene,&r->loop,n));
    r->frames++;
    r->ticks+=(unsigned)n;
    if(jump_requested && n>0)r->jumps++;
    if(r->phase==0&&r->scene.reached_checkpoints==1&&
       r->loop.player.x>=2100.0f){
      assert(r->scene.stage.fragment_count==1);
      assert(r->scene.stage.fragments[0]==1);
      r->first_checkpoint_time=r->loop.active_seconds;
      r->prior_damage=r->scene.damage_count;
      r->prior_coins=r->scene.stage.coin_count;
      r->phase=1;
    }else if(r->phase==1 && old_x>2950.0f &&
             r->loop.player.x<=2070.0f &&
             r->telemetry.large_left_discontinuities>0u){
      /* Real engine recovery must use already activated checkpoint.
       * There are no direct position, health or collectible writes. */
      assert(r->scene.active_checkpoint==2050.0f);
      assert(r->scene.reached_checkpoints==1);
      assert(r->scene.stage.fragments[0]==1);
      assert(r->scene.stage.fragment_count==1);
      assert(r->scene.damage_count>r->prior_damage);
      assert(r->scene.stage.coin_count>=r->prior_coins);
      r->respawn_time=r->loop.active_seconds;
      r->phase=2;
    }else if(r->phase==2 && r->loop.player.x>=2210.0f){
      r->recovery_time=r->loop.active_seconds;
      r->phase=3;
      break;
    }
  }
  printf("Stage168 MAP01 %dHz: phase=%d cp=%.0f part=%d coins=%d hits=%d discontinuities=%u backwalk=%.1f time=%.2f first=%.2f fall=%.2f retry=%.2f\n",
         hz,r->phase,r->scene.active_checkpoint,
         r->scene.stage.fragment_count,r->scene.stage.coin_count,
         r->scene.damage_count,r->telemetry.large_left_discontinuities,
         r->telemetry.leftward_distance_px,r->loop.active_seconds,
         r->first_checkpoint_time,r->respawn_time,r->recovery_time);
  fflush(stdout);
  assert(r->phase==3);
  assert(r->jumps==1u);
  assert(r->first_checkpoint_time>0.0f);
  assert(r->respawn_time>r->first_checkpoint_time);
  assert(r->recovery_time>r->respawn_time);
  assert(r->loop.active_seconds<45.0f);
  assert(r->scene.active_checkpoint==2050.0f);
  assert(r->scene.reached_checkpoints==1);
  assert(r->scene.stage.fragments[0]==1);
  assert(r->scene.stage.fragment_count==1);
  assert(r->scene.hearts>0);
  assert(r->telemetry.initialized);
  assert(r->telemetry.large_left_discontinuities==1u);
  assert(r->telemetry.leftward_frames==0u);
  assert(r->telemetry.leftward_distance_px==0.0f);
  assert(r->telemetry.new_fragments==1u);
  assert(r->telemetry.new_checkpoints==1u);
  assert(r->telemetry.new_damage==(unsigned)r->scene.damage_count);
  assert(r->telemetry.new_coins==(unsigned)r->scene.stage.coin_count);
  assert(r->telemetry.completions==0u);
  assert(r->telemetry.observed_frames==r->frames);
  assert(r->telemetry.observed_ticks==r->ticks);
}
int main(void){
  FaultRecovery a,b,c;
  traverse(&a,30);
  traverse(&b,60);
  traverse(&c,120);
  assert(close_to(a.first_checkpoint_time,b.first_checkpoint_time,0.15f));
  assert(close_to(b.first_checkpoint_time,c.first_checkpoint_time,0.15f));
  assert(close_to(a.respawn_time,b.respawn_time,0.15f));
  assert(close_to(b.respawn_time,c.respawn_time,0.15f));
  assert(a.scene.damage_count==b.scene.damage_count);
  assert(b.scene.damage_count==c.scene.damage_count);
  assert(a.scene.stage.coin_count==b.scene.stage.coin_count);
  assert(b.scene.stage.coin_count==c.scene.stage.coin_count);
  puts("PASS Stage168: native fall and checkpoint respawn preserve mandatory fragment; no false walk-back and no teleport");
  return 0;
}
