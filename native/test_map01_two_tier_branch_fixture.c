/* Stage171: disposable two-tier optional-branch layout fixture.
 * Canonical MAP01 source geometry is copied, then two experimental solids
 * and three coin coordinates are added/relocated in this test-only scene.
 * No production level data or runtime behavior is changed.
 */
#include "rikitu_flower.h"
#include "rikitu_map01_telemetry.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct LayoutRun {
  RikituScene scene;
  RikituLoop loop;
  RikituMap01Telemetry telemetry;
  unsigned ticks,frames,jumps;
  int phase;
  float checkpoint_time,platform_a_time,platform_b_time,rejoin_time;
} LayoutRun;

static int close_to(float a,float b,float tolerance) {
  return isfinite(a)&&isfinite(b)&&fabsf(a-b)<=tolerance;
}
static int on_ground(const RikituPlayer *p) {
  return p->grounded&&close_to(p->y,413.0f,0.15f);
}
static void initialize(LayoutRun *r,int prototype) {
  memset(r,0,sizeof(*r));
  rikitu_flower_init(&r->scene);
  r->loop.player=r->scene.player;
  /* Isolate traversal geometry from enemy AI; both baseline and prototype
   * use the same no-enemy test fixture. Production spawns are untouched. */
  r->scene.enemy_count=0;
  if(prototype) {
    /* Disposable layout idea: two raised tiers between existing x=2080 and
     * x=2280 platform (entry) and x=2630..2815 platform (return). */
    r->scene.solids[19]=(RikituRect){2300.0f,390.0f,140.0f,16.0f};
    r->scene.solids[20]=(RikituRect){2460.0f,320.0f,140.0f,16.0f};
    r->scene.solid_count=21;
    /* Remap three optional coins only in this synthetic layout fixture. */
    r->scene.coins[47]=(RikituPoint){2500.0f,270.0f};
    r->scene.coins[48]=(RikituPoint){2540.0f,270.0f};
    r->scene.coins[49]=(RikituPoint){2580.0f,270.0f};
  }
  assert(rikitu_map01_telemetry_start(&r->telemetry,&r->scene,&r->loop));
}
static int observe_frame(LayoutRun *r,int hz,unsigned input) {
  int n=rikitu_demo_advance(&r->scene,&r->loop,1.0f/(float)hz,input);
  assert(n>=0);
  assert(rikitu_map01_telemetry_observe(&r->telemetry,&r->scene,&r->loop,n));
  r->ticks+=(unsigned)n;
  r->frames++;
  return n;
}
static void direct_baseline(LayoutRun *r,int hz) {
  int f,jumped=0;
  initialize(r,0);
  for(f=0;f<hz*30;f++) {
    unsigned input=RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN;
    int request=0,n;
    if(!jumped&&r->loop.player.grounded&&r->loop.player.x>=1542.0f&&
       r->loop.player.x<1600.0f) {
      input|=RIKITU_INPUT_JUMP;
      request=1;
    }
    n=observe_frame(r,hz,input);
    if(request&&n>0) {
      /* One input edge consumes one jump even when 30Hz advances two ticks. */
      jumped=1;r->jumps++;
    }
    if(jumped&&r->loop.player.x>=2830.0f&&on_ground(&r->loop.player))break;
  }
  assert(jumped&&r->jumps==1u);
  assert(r->loop.player.x>=2830.0f&&r->loop.player.x<2960.0f);
  assert(on_ground(&r->loop.player));
  assert(r->scene.stage.fragments[0]&&r->scene.stage.fragment_count==1);
  assert(r->scene.reached_checkpoints==1&&r->scene.active_checkpoint==2050.0f);
  assert(r->scene.hearts>0&&r->scene.damage_count==0);
  assert(r->telemetry.new_fragments==1u&&r->telemetry.new_checkpoints==1u);
  assert(r->telemetry.leftward_distance_px==0.0f);
  assert(r->telemetry.observed_ticks==r->ticks);
  assert(r->telemetry.observed_frames==r->frames);
}
static void prototype_branch(LayoutRun *r,int hz) {
  int f,was_grounded;
  initialize(r,1);
  r->phase=0; /* normal spawn to required first fragment */
  for(f=0;f<hz*30;f++) {
    unsigned input=RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN;
    int request=0,n;
    was_grounded=r->loop.player.grounded;
    if(r->phase==1&&r->jumps==0u&&r->loop.player.grounded&&
       r->loop.player.x>=1542.0f&&r->loop.player.x<1600.0f) {
      input|=RIKITU_INPUT_JUMP;request=1;
    }
    if(r->phase==4)input=RIKITU_INPUT_LEFT|RIKITU_INPUT_RUN;
    if(r->phase==6)input=RIKITU_INPUT_LEFT|RIKITU_INPUT_RUN;
    if(r->phase==3&&r->jumps==1u&&r->loop.player.grounded&&
       r->loop.player.x>=2200.0f&&r->loop.player.x<2240.0f) {
      input|=RIKITU_INPUT_JUMP;request=1;
    }
    if(r->phase==5&&r->jumps==2u&&r->loop.player.grounded&&
       r->loop.player.x>=2300.0f&&r->loop.player.x<=2320.0f) {
      input|=RIKITU_INPUT_JUMP;request=1;
    }
    n=observe_frame(r,hz,input);
    if(request&&n>0)r->jumps++;
    switch(r->phase) {
      case 0:
        if(r->loop.player.x>=1542.0f)r->phase=1;
        break;
      case 1:
        if(r->jumps==1u&&r->loop.player.x>=1800.0f&&on_ground(&r->loop.player))
          r->phase=2;
        break;
      case 2:
        if(r->loop.player.x>=2200.0f&&r->scene.reached_checkpoints==1) {
          assert(r->scene.stage.fragments[0]&&r->scene.stage.fragment_count==1);
          r->checkpoint_time=r->loop.active_seconds;
          r->phase=3;
        }
        break;
      case 3:
        if(!was_grounded&&r->loop.player.grounded&&
           r->loop.player.x>=2300.0f&&r->loop.player.x<2440.0f&&
           close_to(r->loop.player.y,347.0f,0.15f)) {
          r->platform_a_time=r->loop.active_seconds;
          r->phase=4;
        }
        break;
      case 4:
        if(r->loop.player.x<=2310.0f&&r->loop.player.grounded&&
           close_to(r->loop.player.y,347.0f,0.15f))r->phase=5;
        break;
      case 5:
        if(!was_grounded&&r->loop.player.grounded&&
           r->loop.player.x>=2460.0f&&r->loop.player.x<2600.0f&&
           close_to(r->loop.player.y,277.0f,0.15f)) {
          r->platform_b_time=r->loop.active_seconds;
          r->phase=6;
        }
        break;
      case 6:
        if(r->loop.player.x<=2470.0f&&r->loop.player.grounded&&
           close_to(r->loop.player.y,277.0f,0.15f))r->phase=7;
        break;
      case 7:
        if(r->loop.player.x>=2620.0f&&!r->loop.player.grounded)r->phase=8;
        break;
      case 8:
        if(r->loop.player.x>=2630.0f&&r->loop.player.grounded&&
           close_to(r->loop.player.y,345.0f,0.15f))r->phase=9;
        break;
      case 9:
        if(r->loop.player.x>=2830.0f&&on_ground(&r->loop.player)) {
          r->rejoin_time=r->loop.active_seconds;
          r->phase=10;
        }
        break;
      default:
        assert(0&&"invalid prototype route phase");
    }
    if(r->phase==10)break;
  }
  printf("Stage171 layout %dHz: phase=%d x=%.1f y=%.1f coins=%d tiers=%.2f,%.2f rejoin=%.2f left=%.1f jumps=%u fragment=%d checkpoint=%d damage=%d active=%.2f\n",
         hz,r->phase,r->loop.player.x,r->loop.player.y,r->scene.stage.coin_count,
         r->platform_a_time,r->platform_b_time,r->rejoin_time,
         r->telemetry.leftward_distance_px,r->jumps,
         r->scene.stage.fragment_count,r->scene.reached_checkpoints,
         r->scene.damage_count,r->loop.active_seconds);
  fflush(stdout);
  assert(r->phase==10);
  assert(r->jumps==3u);
  assert(r->checkpoint_time>0.0f&&r->platform_a_time>r->checkpoint_time);
  assert(r->platform_b_time>r->platform_a_time&&r->rejoin_time>r->platform_b_time);
  assert(r->loop.player.x>=2830.0f&&r->loop.player.x<2960.0f);
  assert(on_ground(&r->loop.player));
  assert(r->scene.stage.fragments[0]&&r->scene.stage.fragment_count==1);
  assert(r->scene.reached_checkpoints==1&&r->scene.active_checkpoint==2050.0f);
  assert(r->scene.hearts>0&&r->scene.damage_count==0);
  assert(r->scene.stage.coins[47]&&r->scene.stage.coins[48]&&r->scene.stage.coins[49]);
  assert(r->scene.stage.coin_count>=3&&r->scene.stage.coin_count<90);
  assert(r->telemetry.new_coins==(unsigned)r->scene.stage.coin_count);
  assert(r->telemetry.new_fragments==1u&&r->telemetry.new_checkpoints==1u);
  assert(r->telemetry.leftward_distance_px>60.0f&&
         r->telemetry.leftward_distance_px<110.0f);
  assert(r->telemetry.large_left_discontinuities==0u);
  assert(r->telemetry.observed_ticks==r->ticks);
  assert(r->telemetry.observed_frames==r->frames);
}
int main(void) {
  LayoutRun d30,d60,d120,p30,p60,p120;
  direct_baseline(&d30,30);direct_baseline(&d60,60);direct_baseline(&d120,120);
  prototype_branch(&p30,30);prototype_branch(&p60,60);prototype_branch(&p120,120);
  assert(close_to(d30.loop.active_seconds,d60.loop.active_seconds,0.08f));
  assert(close_to(d60.loop.active_seconds,d120.loop.active_seconds,0.08f));
  assert(close_to(p30.loop.active_seconds,p60.loop.active_seconds,0.15f));
  assert(close_to(p60.loop.active_seconds,p120.loop.active_seconds,0.15f));
  assert(close_to(p30.telemetry.leftward_distance_px,p60.telemetry.leftward_distance_px,12.0f));
  assert(close_to(p60.telemetry.leftward_distance_px,p120.telemetry.leftward_distance_px,12.0f));
  assert(p30.scene.stage.coin_count>=d30.scene.stage.coin_count+3);
  assert(p60.scene.stage.coin_count==p30.scene.stage.coin_count);
  assert(p120.scene.stage.coin_count==p30.scene.stage.coin_count);
  assert(p30.loop.active_seconds>d30.loop.active_seconds+0.15f);
  assert(p30.loop.active_seconds<d30.loop.active_seconds+1.5f);
  assert(p60.loop.active_seconds>d60.loop.active_seconds+0.15f);
  assert(p60.loop.active_seconds<d60.loop.active_seconds+1.5f);
  assert(p120.loop.active_seconds>d120.loop.active_seconds+0.15f);
  assert(p120.loop.active_seconds<d120.loop.active_seconds+1.5f);
  puts("PASS Stage171: disposable two-tier branch fixture is reachable, rewards optional exploration, and returns safely; not production geometry or human pacing");
  return 0;
}
