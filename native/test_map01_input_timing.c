/* Stage159: input-driven timing sanity on the real MAP01 native layout.
 * No position edits, teleport API, graphics, private art or save data.
 * This is NOT a successful full-level playthrough or 6–8 minute proof.
 */
#include "rikitu_flower.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct InputRun {
  RikituScene scene;
  RikituLoop loop;
  int ticks;
} InputRun;

static void begin(InputRun *r){
  rikitu_flower_init(&r->scene);
  memset(&r->loop,0,sizeof(r->loop));
  r->loop.player=r->scene.player;
  r->ticks=0;
}
static int close_to(float a,float b,float tolerance){
  return isfinite(a)&&isfinite(b)&&fabsf(a-b)<=tolerance;
}
static void frames(InputRun *r,int frame_count,int hz,unsigned buttons){
  int i;
  for(i=0;i<frame_count;i++){
    const int n=rikitu_demo_advance(&r->scene,&r->loop,
                                   1.0f/(float)hz,buttons);
    assert(n>=0);
    r->ticks+=n;
  }
}
static void deterministic_one_second(void){
  InputRun at30,at60,at120,walk;
  const float initial=105.0f;
  begin(&at30);begin(&at60);begin(&at120);begin(&walk);
  assert(at30.scene.level_id==1 && at30.scene.world_width==7900.0f);
  frames(&at30,30,30,RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN);
  frames(&at60,60,60,RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN);
  frames(&at120,120,120,RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN);
  frames(&walk,60,60,RIKITU_INPUT_RIGHT);
  assert(at30.ticks==60 && at60.ticks==60 && at120.ticks==60);
  assert(walk.ticks==60);
  assert(close_to(at30.loop.active_seconds,1.0f,0.001f));
  assert(close_to(at120.loop.active_seconds,1.0f,0.001f));
  assert(close_to(at30.scene.player.x,at60.scene.player.x,0.05f));
  assert(close_to(at60.scene.player.x,at120.scene.player.x,0.05f));
  assert(close_to(at60.scene.player.x,initial+265.0f,0.3f));
  assert(close_to(walk.scene.player.x,initial+225.0f,0.3f));
  assert(at60.scene.stage.coin_count==at120.scene.stage.coin_count);
  assert(at60.scene.hearts==at120.scene.hearts);
  assert(at60.scene.reached_checkpoints==0 && !at60.scene.stage.completed);
  printf("MAP01 input-only 1s: walk=%.1f, run=%.1f, coins=%u\n",
         walk.scene.player.x-initial,at60.scene.player.x-initial,
         at60.scene.stage.coin_count);
}
static void pause_does_not_add_timing(void){
  InputRun r;
  float x,seconds;
  begin(&r);
  frames(&r,60,60,RIKITU_INPUT_RIGHT);
  x=r.scene.player.x;
  seconds=r.loop.active_seconds;
  r.loop.paused=1;
  frames(&r,180,60,RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN);
  assert(close_to(r.scene.player.x,x,0.001f));
  assert(close_to(r.loop.active_seconds,seconds,0.001f));
  r.loop.paused=0;
  frames(&r,60,60,RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN);
  assert(close_to(r.loop.active_seconds,seconds+1.0f,0.001f));
  assert(r.scene.player.x>x+200.0f);
  assert(r.scene.level_id==1);
}
static void genuine_jump_inputs_are_applied(void){
  InputRun r;
  float grounded_y,elapsed;
  begin(&r);
  /* Move onto a real surface with a full second of right input. */
  frames(&r,60,60,RIKITU_INPUT_RIGHT);
  assert(r.scene.player.grounded);
  grounded_y=r.scene.player.y;
  elapsed=r.loop.active_seconds;
  frames(&r,1,60,RIKITU_INPUT_RIGHT|RIKITU_INPUT_JUMP);
  frames(&r,12,60,RIKITU_INPUT_RIGHT);
  assert(r.scene.player.y<grounded_y-20.0f);
  assert(!r.scene.player.grounded);
  assert(r.scene.player.vy<0.0f);
  assert(close_to(r.loop.active_seconds,elapsed+13.0f/60.0f,0.001f));
  assert(r.scene.stage.fragment_count==0);
  assert(!r.scene.stage.completed);
}
int main(void){
  deterministic_one_second();
  pause_does_not_add_timing();
  genuine_jump_inputs_are_applied();
  puts("PASS Stage159: MAP01 native real input steps, 30/60/120 Hz, run/walk, pause, jump; NOT full-level timing");
  return 0;
}
