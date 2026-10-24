/* Stage166: retrieve deliberately MISSED MAP01 fragment using normal inputs.
 * Checks real safe backward navigation without test teleports or save edits.
 * Scripted route is NOT human playtime evidence or a 6-8 minute pacing proof.
 */
#include "rikitu_flower.h"
#include "rikitu_map01_telemetry.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct RecoveryRoute {
  RikituScene scene;
  RikituLoop loop;
  RikituMap01Telemetry evidence;
  int phase;
  unsigned jumps, ticks, frames;
  float reverse_at, return_at, acquire_at, checkpoint_at;
} RecoveryRoute;
static int approx(float a,float b,float margin){
  return isfinite(a)&&isfinite(b)&&fabsf(a-b)<=margin;
}
static void test_route(RecoveryRoute *r,int hz){
  int frame;
  assert(hz==30||hz==60||hz==120);
  memset(r,0,sizeof(*r));
  rikitu_flower_init(&r->scene);
  r->loop.player=r->scene.player;
  assert(rikitu_map01_telemetry_start(&r->evidence,&r->scene,&r->loop));
  /* phase 0: approach x1600 on foot, intentionally below x1530 fragment.
   * phase 1: physically reverse left to x1390; no debug reposition.
   * phase 2: retrace right, jump at x~1485 to collect missed fragment.
   * phase 3: stop at x1590 until grounded.
   * phase 4: jump across the first x1660..1735 gap.
   * phase 5: run to first x2050 checkpoint. */
  for(frame=0;frame<hz*35;frame++){
    unsigned input=0;
    int n,pressed=0;
    switch(r->phase){
      case 0:
        input=RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN;
        break;
      case 1:
        input=RIKITU_INPUT_LEFT|RIKITU_INPUT_RUN;
        break;
      case 2:
        input=RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN;
        if(r->jumps==0&&r->loop.player.grounded&&
           r->loop.player.x>=1485.0f){
          input|=RIKITU_INPUT_JUMP;
          pressed=1;
        }
        break;
      case 3:
        input=0;
        break;
      case 4:
        input=RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN;
        if(r->jumps==1){
          input|=RIKITU_INPUT_JUMP;
          pressed=1;
        }
        break;
      case 5:
        input=RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN;
        break;
      default:
        assert(0&&"unreachable route phase");
    }
    n=rikitu_demo_advance(&r->scene,&r->loop,1.0f/(float)hz,input);
    assert(n>=0);
    assert(rikitu_map01_telemetry_observe(&r->evidence,&r->scene,&r->loop,n));
    r->ticks+=(unsigned)n;
    r->frames++;
    /* At 120Hz some display frames have no physics tick: keep holding
     * jump until a real tick consumes it, then release next frame. */
    if(pressed&&n>0)r->jumps++;
    switch(r->phase){
      case 0:
        if(r->loop.player.x>=1590.0f){
          assert(r->scene.stage.fragment_count==0);
          assert(r->scene.stage.fragments[0]==0);
          r->reverse_at=r->loop.active_seconds;
          r->phase=1;
        }
        break;
      case 1:
        if(r->loop.player.x<=1390.0f){
          assert(r->scene.stage.fragment_count==0);
          r->return_at=r->loop.active_seconds;
          r->phase=2;
        }
        break;
      case 2:
        if(r->scene.stage.fragments[0] &&
           r->loop.player.x>=1590.0f){
          r->acquire_at=r->loop.active_seconds;
          r->phase=3;
        }
        assert(r->loop.player.x<1650.0f);
        break;
      case 3:
        if(r->loop.player.grounded){
          r->phase=4;
        }
        break;
      case 4:
        if(r->jumps==2)r->phase=5;
        break;
      case 5:
        if(r->scene.reached_checkpoints>=1&&r->loop.player.x>=2100.0f){
          r->checkpoint_at=r->loop.active_seconds;
          r->phase=6;
        }
        break;
    }
    if(r->phase==6)break;
  }
  printf("Stage166 MAP01 %dHz phase=%d x=%.1f fragments=%d checkpoints=%d jumps=%u damage=%d coins=%d seconds=%.2f reverse=%.2f turn=%.2f recovered=%.2f checkpoint=%.2f\n",
         hz,r->phase,r->loop.player.x,r->scene.stage.fragment_count,
         r->scene.reached_checkpoints,r->jumps,r->scene.damage_count,
         r->scene.stage.coin_count,r->loop.active_seconds,r->reverse_at,
         r->return_at,r->acquire_at,r->checkpoint_at);
  fflush(stdout);
  assert(r->phase==6);
  assert(r->reverse_at>0.0f);
  assert(r->return_at>r->reverse_at);
  assert(r->acquire_at>r->return_at);
  assert(r->checkpoint_at>r->acquire_at);
  assert(r->scene.stage.fragments[0]==1);
  assert(r->scene.stage.fragment_count==1);
  assert(r->scene.reached_checkpoints==1);
  assert(r->scene.active_checkpoint==2050.0f);
  assert(r->jumps==2u);
  assert(r->evidence.new_fragments==1u);
  assert(r->evidence.new_checkpoints==1u);
  /* Recovering the missed fragment includes >180 px of genuine LEFT
   * movement; do not confuse it with an instantaneous respawn jump. */
  assert(r->evidence.leftward_distance_px>180.0f);
  assert(r->evidence.leftward_distance_px<240.0f);
  assert(r->evidence.leftward_frames>0u);
  assert(r->evidence.large_left_discontinuities==0u);
  assert(r->evidence.completions==0u);
  assert(r->evidence.paused_frames==0u);
  assert(r->evidence.observed_ticks==r->ticks);
  assert(r->evidence.new_damage==(unsigned)r->scene.damage_count);
  assert(r->scene.hearts>0);
  assert(r->loop.active_seconds<35.0f);
}
int main(void){
  RecoveryRoute a,b,c;
  test_route(&a,30);
  test_route(&b,60);
  test_route(&c,120);
  assert(approx(a.checkpoint_at,b.checkpoint_at,0.25f));
  assert(approx(b.checkpoint_at,c.checkpoint_at,0.25f));
  assert(approx(a.acquire_at,b.acquire_at,0.25f));
  assert(approx(b.acquire_at,c.acquire_at,0.25f));
  puts("PASS Stage166: missed first fragment recovered by real left/right/jump input, checkpoint reached, observer no duplicate; not human pacing");
  return 0;
}
