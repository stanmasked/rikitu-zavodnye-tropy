/* Stage161: deterministic ordinary-button input on real MAP01 geometry.
 * Exercise mandatory fragments 1+2 and checkpoints 1+2 without teleports.
 * Scripted test policy is not a human player or proof of 6–8 min pacing.
 */
#include "rikitu_flower.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct SectorRoute {
  RikituScene scene;
  RikituLoop loop;
  unsigned jumps, ticks, frames;
  float checkpoint_seconds[2];
} SectorRoute;
static int near(float a,float b,float tolerance) {
  return isfinite(a)&&isfinite(b)&&fabsf(a-b)<=tolerance;
}
static void play(SectorRoute *r,int hz) {
  static const float jump_at[]={
    620.0f,1542.0f,2340.0f,2920.0f,3180.0f,3590.0f,4230.0f
  };
  const unsigned planned=(unsigned)(sizeof(jump_at)/sizeof(jump_at[0]));
  int prev_checkpoint=0;
  int frame;
  assert(hz==30||hz==60||hz==120);
  memset(r,0,sizeof(*r));
  rikitu_flower_init(&r->scene);
  r->loop.player=r->scene.player;
  r->checkpoint_seconds[0]=r->checkpoint_seconds[1]=-1.0f;
  for(frame=0;frame<hz*40;frame++) {
    unsigned input=RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN;
    int pressed=0;
    int n;
    if(r->jumps<planned&&r->loop.player.grounded&&
       r->loop.player.x>=jump_at[r->jumps]){
      input|=RIKITU_INPUT_JUMP;
      pressed=1;
    }
    n=rikitu_demo_advance(&r->scene,&r->loop,1.0f/(float)hz,input);
    assert(n>=0);
    r->ticks+=(unsigned)n;
    r->frames++;
    /* Holding jump through zero-tick display frames is essential at 120Hz. */
    if(pressed&&n>0)r->jumps++;
    if(r->scene.reached_checkpoints>prev_checkpoint){
      int i;
      for(i=prev_checkpoint;i<r->scene.reached_checkpoints&&i<2;i++)
        r->checkpoint_seconds[i]=r->loop.active_seconds;
      prev_checkpoint=r->scene.reached_checkpoints;
    }
    if(r->scene.reached_checkpoints>=2&&r->loop.player.x>=4520.0f)break;
  }
  printf("Stage161 MAP01 %dHz: x=%.1f frags=%d mask=%u%u%u cp=%d jumps=%u damage=%d active=%.2f cpt=%.2f,%.2f\n",
         hz,r->loop.player.x,r->scene.stage.fragment_count,
         (unsigned)r->scene.stage.fragments[0],
         (unsigned)r->scene.stage.fragments[1],
         (unsigned)r->scene.stage.fragments[2],
         r->scene.reached_checkpoints,r->jumps,r->scene.damage_count,
         r->loop.active_seconds,r->checkpoint_seconds[0],
         r->checkpoint_seconds[1]);
  fflush(stdout);
  assert(r->scene.stage.fragments[0]&&r->scene.stage.fragments[1]);
  assert(r->scene.stage.fragment_count>=2);
  assert(r->scene.reached_checkpoints>=2);
  assert(r->scene.active_checkpoint==4490.0f);
  assert(r->loop.player.x>=4520.0f);
  assert(r->checkpoint_seconds[0]>0.0f);
  assert(r->checkpoint_seconds[1]>r->checkpoint_seconds[0]);
  assert(r->scene.hearts>0&&!r->scene.stage.completed);
  assert(r->loop.active_seconds<40.0f);
}
int main(void) {
  SectorRoute at30,at60,at120;
  play(&at30,30);
  play(&at60,60);
  play(&at120,120);
  assert(near(at30.checkpoint_seconds[0],at60.checkpoint_seconds[0],0.06f));
  assert(near(at60.checkpoint_seconds[0],at120.checkpoint_seconds[0],0.06f));
  assert(near(at30.checkpoint_seconds[1],at60.checkpoint_seconds[1],0.06f));
  assert(near(at60.checkpoint_seconds[1],at120.checkpoint_seconds[1],0.06f));
  assert(near(at30.loop.player.x,at60.loop.player.x,6.0f));
  assert(near(at60.loop.player.x,at120.loop.player.x,6.0f));
  puts("PASS Stage161: first two Flower Trail fragments/checkpoints through true physics input, not a full-level timing audit");
  return 0;
}
