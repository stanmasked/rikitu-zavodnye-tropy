/* Stage170: candidate optional elevated pickup branch on real MAP01 geometry.
 * No map edits, teleports, direct coordinate writes or synthetic pickups.
 * The direct path is the baseline; the branch enters a raised platform,
 * collects its three coins, backtracks on the platform, then safely
 * rejoins the ground path before the first enemy patrol.
 */
#include "rikitu_flower.h"
#include "rikitu_map01_telemetry.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct BranchRun {
  RikituScene scene;
  RikituLoop loop;
  RikituMap01Telemetry telemetry;
  unsigned ticks, frames, jumps;
  int phase;
  float platform_land_seconds, return_seconds, rejoin_seconds;
} BranchRun;

static int close_to(float a,float b,float tolerance) {
  return isfinite(a)&&isfinite(b)&&fabsf(a-b)<=tolerance;
}
static int on_ground(const RikituPlayer *p) {
  return p->grounded && fabsf(p->y-413.0f)<0.15f;
}
static void init_run(BranchRun *r) {
  memset(r,0,sizeof(*r));
  rikitu_flower_init(&r->scene);
  r->loop.player=r->scene.player;
  assert(rikitu_map01_telemetry_start(&r->telemetry,&r->scene,&r->loop));
}
static void direct_path(BranchRun *r,int hz) {
  int f;
  init_run(r);
  for(f=0;f<hz*8;f++) {
    const unsigned input=RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN;
    int n=rikitu_demo_advance(&r->scene,&r->loop,1.0f/(float)hz,input);
    assert(n>=0);
    assert(rikitu_map01_telemetry_observe(&r->telemetry,&r->scene,&r->loop,n));
    r->ticks+=(unsigned)n;r->frames++;
    if(r->loop.player.x>=730.0f&&on_ground(&r->loop.player))break;
  }
  assert(r->loop.player.x>=730.0f&&r->loop.player.x<790.0f);
  assert(on_ground(&r->loop.player));
  assert(r->scene.hearts>0);
  assert(r->scene.stage.coin_count==0);
  assert(r->telemetry.new_coins==0u);
  assert(r->telemetry.leftward_distance_px==0.0f);
  assert(r->telemetry.observed_ticks==r->ticks);
  assert(r->telemetry.observed_frames==r->frames);
}
static void elevated_branch(BranchRun *r,int hz) {
  int f;
  int was_grounded;
  init_run(r);
  r->phase=0; /* approach platform */
  for(f=0;f<hz*10;f++) {
    unsigned input=RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN;
    int n,jump_requested=0;
    was_grounded=r->loop.player.grounded;
    if(r->phase==1&&r->jumps==0u&&r->loop.player.grounded&&
       r->loop.player.x>=420.0f&&r->loop.player.x<460.0f) {
      input|=RIKITU_INPUT_JUMP;
      jump_requested=1;
    }
    if(r->phase==2)input=RIKITU_INPUT_LEFT|RIKITU_INPUT_RUN;
    n=rikitu_demo_advance(&r->scene,&r->loop,1.0f/(float)hz,input);
    assert(n>=0);
    assert(rikitu_map01_telemetry_observe(&r->telemetry,&r->scene,&r->loop,n));
    r->ticks+=(unsigned)n;r->frames++;
    if(jump_requested&&n>0)r->jumps++;
    switch(r->phase) {
      case 0:
        if(r->loop.player.x>=420.0f)r->phase=1;
        break;
      case 1:
        if(!was_grounded&&r->loop.player.grounded&&
           r->loop.player.x>=490.0f&&r->loop.player.x<675.0f&&
           close_to(r->loop.player.y,347.0f,0.15f)) {
          r->platform_land_seconds=r->loop.active_seconds;
          r->phase=2;
        }
        break;
      case 2:
        if(r->loop.player.x<=500.0f&&r->loop.player.grounded&&
           close_to(r->loop.player.y,347.0f,0.15f)) {
          r->return_seconds=r->loop.active_seconds;
          r->phase=3;
        }
        break;
      case 3:
        if(r->loop.player.x>=700.0f&&!r->loop.player.grounded)
          r->phase=4; /* left the raised platform; falling is native physics */
        break;
      case 4:
        if(r->loop.player.x>=730.0f&&on_ground(&r->loop.player)) {
          r->rejoin_seconds=r->loop.active_seconds;
          r->phase=5;
        }
        break;
      default:
        assert(0&&"unexpected branch phase");
    }
    if(r->phase==5)break;
  }
  printf("Stage170 MAP01 %dHz: phase=%d x=%.1f y=%.1f grounded=%d coins=%d elevated=%u%u%u left=%.1f discontinuities=%u active=%.2f land=%.2f turn=%.2f rejoin=%.2f damage=%d\n",
         hz,r->phase,r->loop.player.x,r->loop.player.y,r->loop.player.grounded,
         r->scene.stage.coin_count,(unsigned)r->scene.stage.coins[47],
         (unsigned)r->scene.stage.coins[48],(unsigned)r->scene.stage.coins[49],
         r->telemetry.leftward_distance_px,r->telemetry.large_left_discontinuities,
         r->loop.active_seconds,r->platform_land_seconds,r->return_seconds,
         r->rejoin_seconds,r->scene.damage_count);
  fflush(stdout);
  assert(r->phase==5);
  assert(r->jumps==1u);
  assert(r->platform_land_seconds>0.0f);
  assert(r->return_seconds>r->platform_land_seconds);
  assert(r->rejoin_seconds>r->return_seconds);
  assert(r->loop.player.x>=730.0f&&r->loop.player.x<800.0f);
  assert(on_ground(&r->loop.player));
  assert(r->scene.hearts>0);
  /* Elevated coins are existing optional content, not new requirements. */
  assert(r->scene.stage.coins[47]&&r->scene.stage.coins[48]&&
         r->scene.stage.coins[49]);
  assert(r->scene.stage.coins[4]); /* coin at x=740 collected on the descent */
  assert(r->scene.stage.coin_count>=4&&r->scene.stage.coin_count<90);
  assert(r->telemetry.new_coins==(unsigned)r->scene.stage.coin_count);
  assert(r->telemetry.leftward_distance_px>80.0f&&
         r->telemetry.leftward_distance_px<120.0f);
  assert(r->telemetry.leftward_frames>0u);
  assert(r->telemetry.large_left_discontinuities==0u);
  assert(r->telemetry.new_fragments==0u&&r->telemetry.new_checkpoints==0u);
  assert(r->telemetry.observed_ticks==r->ticks);
  assert(r->telemetry.observed_frames==r->frames);
}
int main(void) {
  BranchRun direct30,direct60,direct120,branch30,branch60,branch120;
  direct_path(&direct30,30);direct_path(&direct60,60);direct_path(&direct120,120);
  elevated_branch(&branch30,30);elevated_branch(&branch60,60);elevated_branch(&branch120,120);
  assert(close_to(direct30.loop.active_seconds,direct60.loop.active_seconds,0.08f));
  assert(close_to(direct60.loop.active_seconds,direct120.loop.active_seconds,0.08f));
  assert(close_to(branch30.loop.active_seconds,branch60.loop.active_seconds,0.12f));
  assert(close_to(branch60.loop.active_seconds,branch120.loop.active_seconds,0.12f));
  assert(branch30.scene.stage.coin_count>=direct30.scene.stage.coin_count+3);
  assert(branch60.scene.stage.coin_count==branch30.scene.stage.coin_count);
  assert(branch120.scene.stage.coin_count==branch30.scene.stage.coin_count);
  assert(branch30.loop.active_seconds>direct30.loop.active_seconds+0.4f);
  assert(branch30.loop.active_seconds<direct30.loop.active_seconds+1.8f);
  assert(branch60.loop.active_seconds>direct60.loop.active_seconds+0.4f);
  assert(branch60.loop.active_seconds<direct60.loop.active_seconds+1.8f);
  assert(branch120.loop.active_seconds>direct120.loop.active_seconds+0.4f);
  assert(branch120.loop.active_seconds<direct120.loop.active_seconds+1.8f);
  puts("PASS Stage170: optional raised MAP01 branch collects four-plus existing coins, returns to ground by real input, and preserves canonical geometry; not human pacing evidence");
  return 0;
}
