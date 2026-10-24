/* Stage160: ordinary-input traversal of MAP01's first sector.
 * No teleports, coordinate writes, save manipulation, or private assets.
 * Not a full level run and not human pacing evidence.
 */
#include "rikitu_flower.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct SectorRun {
  RikituScene scene;
  RikituLoop loop;
  int jump_attempts, ticks, frames;
  float at_checkpoint_seconds;
} SectorRun;

static int closef_(float a,float b,float tolerance) {
  return isfinite(a)&&isfinite(b)&&fabsf(a-b)<=tolerance;
}

static void sector_run(SectorRun *r,int render_hz) {
  int i;
  int crossed=0;
  assert(render_hz==30||render_hz==60||render_hz==120);
  memset(r,0,sizeof(*r));
  rikitu_flower_init(&r->scene);
  r->loop.player=r->scene.player;
  r->at_checkpoint_seconds=-1.0f;
  /* Normal RUN movement; press JUMP on the first rise before the
   * 75-unit ground gap x1660..1735, near the fragment at x1530.
   * Input is chosen based only on position observed from the engine.
   * Position is NEVER assigned again after scene initialization. */
  for(i=0;i<render_hz*12;i++) {
    unsigned input=RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN;
    int n;
    if(r->jump_attempts==0 &&
       r->loop.player.grounded &&
       r->loop.player.x>=1542.0f && r->loop.player.x<1600.0f){
      input|=RIKITU_INPUT_JUMP;
      r->jump_attempts++;
    }
    n=rikitu_demo_advance(&r->scene,&r->loop,
                          1.0f/(float)render_hz,input);
    assert(n>=0);
    r->ticks+=n;
    r->frames++;
    if(!crossed&&r->scene.reached_checkpoints>=1){
      crossed=1;
      r->at_checkpoint_seconds=r->loop.active_seconds;
    }
    if(crossed&&r->loop.player.x>=2100.0f)break;
  }
  printf("Stage160 MAP01 %dHz: x=%.1f fragments=%d cp=%d jumps=%d damage=%d time=%.2f\n",
         render_hz,r->loop.player.x,r->scene.stage.fragment_count,
         r->scene.reached_checkpoints,r->jump_attempts,
         r->scene.damage_count,r->loop.active_seconds);
  fflush(stdout);
  assert(r->jump_attempts==1);
  assert(r->scene.level_id==1);
  assert(r->scene.stage.fragments[0]==1);
  assert(r->scene.stage.fragment_count>=1);
  assert(r->scene.reached_checkpoints>=1);
  assert(r->scene.active_checkpoint==2050.0f);
  assert(r->loop.player.x>=2100.0f);
  assert(r->at_checkpoint_seconds>0.0f &&
         r->at_checkpoint_seconds<12.0f);
  assert(r->scene.hearts>0);
  assert(!r->scene.stage.completed);
  assert(r->loop.active_seconds<12.0f);
}

int main(void) {
  SectorRun a,b,c;
  sector_run(&a,30);
  sector_run(&b,60);
  sector_run(&c,120);
  assert(closef_(a.at_checkpoint_seconds,b.at_checkpoint_seconds,0.06f));
  assert(closef_(b.at_checkpoint_seconds,c.at_checkpoint_seconds,0.06f));
  assert(closef_(a.loop.player.x,b.loop.player.x,6.0f));
  assert(closef_(b.loop.player.x,c.loop.player.x,6.0f));
  assert(a.scene.stage.fragments[0]&&b.scene.stage.fragments[0]&&
         c.scene.stage.fragments[0]);
  puts("PASS Stage160: MAP01 first fragment + first checkpoint through ordinary native input at 30/60/120Hz");
  return 0;
}
