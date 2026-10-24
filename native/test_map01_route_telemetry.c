/* Stage165: cross-check MAP01 telemetry on scripted full physics-input route.
 * Never teleport or directly change state after canonical scene initialization.
 * This is an optimized automation policy, NOT a natural player-duration test.
 */
#include "rikitu_flower.h"
#include "rikitu_map01_telemetry.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct RouteResult {
  RikituScene scene;
  RikituLoop loop;
  RikituMap01Telemetry telemetry;
  float checkpoint_seconds[3], completed_seconds;
  unsigned ticks, frames, jumps, expected_pause_frames;
  int paused_once;
} RouteResult;

static int close_(float a,float b,float eps){
  return isfinite(a)&&isfinite(b)&&fabsf(a-b)<=eps;
}
static void run(RouteResult *r,int hz){
  /* SCHEDULE AT SHARED 60Hz TICK BOUNDARIES, not at render-frame
   * positions. Odd-tick button edges cannot exist in the 30Hz sampler
   * (which executes two fixed steps per rendered frame). Keeping these
   * triggers at even ticks makes the SAME controller-input timeline
   * executable at 30, 60 and 120 display Hz. Physics remains unchanged. */
  static const unsigned jump_on_tick[]={
    118u,326u,508u,638u,698u,790u,
    934u,1046u,1196u,1260u,1522u,1572u
  };
  const unsigned planned=(unsigned)(sizeof(jump_on_tick)/sizeof(jump_on_tick[0]));
  int prev_checkpoint=0;
  int i;
  assert(hz==30||hz==60||hz==120);
  memset(r,0,sizeof(*r));
  rikitu_flower_init(&r->scene);
  r->loop.player=r->scene.player;
  assert(rikitu_map01_telemetry_start(&r->telemetry,&r->scene,&r->loop));
  for(i=0;i<3;i++)r->checkpoint_seconds[i]=-1.0f;
  r->completed_seconds=-1.0f;
  for(i=0;i<hz*90;i++){
    unsigned input=RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN;
    int n,pressed=0;
    /* Pause the actual simulation loop for 2 wall seconds, after the
     * second checkpoint and on the same even physics-tick boundary.
     * No character, pickup, scene-counter or timer mutations allowed. */
    if(!r->paused_once&&r->ticks>=1180u){
      const float before=r->loop.active_seconds;
      const float x_before=r->loop.player.x;
      const int fragments_before=r->scene.stage.fragment_count;
      const int coins_before=r->scene.stage.coin_count;
      int j;
      r->loop.paused=1;
      for(j=0;j<2*hz;j++){
        const int paused_ticks=rikitu_demo_advance(&r->scene,&r->loop,
                             1.0f/(float)hz,RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN);
        assert(paused_ticks==0);
        assert(rikitu_map01_telemetry_observe(&r->telemetry,
                             &r->scene,&r->loop,paused_ticks));
        r->frames++;
      }
      assert(r->loop.active_seconds==before);
      assert(r->loop.player.x==x_before);
      assert(r->scene.stage.fragment_count==fragments_before);
      assert(r->scene.stage.coin_count==coins_before);
      r->expected_pause_frames=(unsigned)(2*hz);
      r->loop.paused=0;
      r->paused_once=1;
    }
    if(r->jumps<planned&&r->loop.player.grounded&&
       r->ticks>=jump_on_tick[r->jumps]){
      input|=RIKITU_INPUT_JUMP;
      pressed=1;
    }
    n=rikitu_demo_advance(&r->scene,&r->loop,1.0f/(float)hz,input);
    assert(n>=0);
    assert(rikitu_map01_telemetry_observe(&r->telemetry,
                                          &r->scene,&r->loop,n));
    r->frames++;
    r->ticks+=(unsigned)n;
    if(pressed&&n>0)r->jumps++;
    if(r->scene.reached_checkpoints>prev_checkpoint){
      int j;
      for(j=prev_checkpoint;j<r->scene.reached_checkpoints&&j<3;j++)
        r->checkpoint_seconds[j]=r->loop.active_seconds;
      prev_checkpoint=r->scene.reached_checkpoints;
    }
    if(r->scene.stage.completed){
      r->completed_seconds=r->loop.active_seconds;
      break;
    }
  }
  printf("Stage165 MAP01 %dHz: x=%.1f frags=%d mask=%u%u%u checkpoint=%d jumps=%u damage=%d coins=%d complete=%d active=%.2f splits=%.2f,%.2f,%.2f\n",
         hz,r->loop.player.x,r->scene.stage.fragment_count,
         (unsigned)r->scene.stage.fragments[0],
         (unsigned)r->scene.stage.fragments[1],
         (unsigned)r->scene.stage.fragments[2],
         r->scene.reached_checkpoints,r->jumps,r->scene.damage_count,
         r->scene.stage.coin_count,r->scene.stage.completed,
         r->loop.active_seconds,r->checkpoint_seconds[0],
         r->checkpoint_seconds[1],r->checkpoint_seconds[2]);
  fflush(stdout);
  assert(r->scene.level_id==1);
  assert(r->scene.stage.fragment_count==3);
  assert(r->scene.stage.fragments[0]&&r->scene.stage.fragments[1]&&
         r->scene.stage.fragments[2]);
  assert(r->scene.reached_checkpoints==3);
  assert(r->scene.active_checkpoint==6150.0f);
  assert(r->scene.stage.completed&&rikitu_stage_can_finish(&r->scene.stage));
  assert(r->loop.player.x>7700.0f);
  assert(r->completed_seconds>0.0f&&r->completed_seconds<90.0f);
  assert(r->scene.hearts>0);
  /* These are measured on every normal physics frame, not injected. */
  assert(r->paused_once);
  assert(r->telemetry.initialized);
  assert(r->telemetry.observed_ticks==r->ticks);
  assert(r->telemetry.observed_frames==r->frames);
  assert(r->telemetry.paused_frames==r->expected_pause_frames);
  assert(r->telemetry.new_fragments==3u);
  assert(r->telemetry.new_checkpoints==3u);
  assert(r->telemetry.new_coins==(unsigned)r->scene.stage.coin_count);
  assert(r->telemetry.new_coins==27u);
  assert(r->telemetry.new_damage==(unsigned)r->scene.damage_count);
  assert(r->telemetry.new_damage==0u);
  assert(r->telemetry.completions==1u);
  assert(close_(r->telemetry.last_active_seconds,
                r->loop.active_seconds,0.02f));
  assert(r->checkpoint_seconds[0]>0.0f);
  assert(r->checkpoint_seconds[1]>r->checkpoint_seconds[0]);
  assert(r->checkpoint_seconds[2]>r->checkpoint_seconds[1]);
  assert(r->completed_seconds>r->checkpoint_seconds[2]);
  {
    unsigned j;
    for(j=0;j<3;j++)
      assert(close_(r->telemetry.checkpoint_active_seconds[j],
                    r->checkpoint_seconds[j],0.02f));
  }
}
int main(void){
  RouteResult a,b,c;
  int i;
  run(&a,30);
  run(&b,60);
  run(&c,120);
  for(i=0;i<3;i++){
    assert(close_(a.checkpoint_seconds[i],b.checkpoint_seconds[i],0.06f));
    assert(close_(b.checkpoint_seconds[i],c.checkpoint_seconds[i],0.06f));
  }
  assert(close_(a.completed_seconds,b.completed_seconds,0.06f));
  assert(close_(b.completed_seconds,c.completed_seconds,0.06f));
  assert(a.scene.stage.coin_count==b.scene.stage.coin_count);
  assert(b.scene.stage.coin_count==c.scene.stage.coin_count);
  assert(a.telemetry.new_coins==b.telemetry.new_coins);
  assert(b.telemetry.new_coins==c.telemetry.new_coins);
  assert(a.telemetry.new_fragments==b.telemetry.new_fragments);
  assert(b.telemetry.new_fragments==c.telemetry.new_fragments);
  assert(a.telemetry.new_checkpoints==b.telemetry.new_checkpoints);
  assert(b.telemetry.new_checkpoints==c.telemetry.new_checkpoints);
  assert(a.telemetry.completions==b.telemetry.completions);
  assert(b.telemetry.completions==c.telemetry.completions);
  assert(a.scene.damage_count==b.scene.damage_count);
  assert(b.scene.damage_count==c.scene.damage_count);
  puts("PASS Stage165: full MAP01 native input telemetry parity, 3 fragments, 3 checkpoints, 27 coins, 1 completion, 2s pause excluded; NOT human pacing");
  return 0;
}
