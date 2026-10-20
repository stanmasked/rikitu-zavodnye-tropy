#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void init(RikituScene *scene,RikituLoop *loop) {
  rikitu_demo_init(scene);
  memset(loop,0,sizeof(*loop));
  loop->player=scene->player;
}
static int closef(float a,float b,float epsilon){
  return fabsf(a-b)<epsilon;
}
int main(void) {
  RikituScene a,b,c;
  RikituLoop la,lb,lc;
  int i,na=0,nb=0,nc=0;
  float xbefore,elapsed;
  init(&a,&la);
  init(&b,&lb);
  init(&c,&lc);
  /* The same 60-Hz game results from 30, 60 or 120 rendered frames/sec. */
  for(i=0;i<30;i++)na+=rikitu_demo_advance(&a,&la,1.0f/30.0f,
                                          RIKITU_INPUT_RIGHT);
  for(i=0;i<60;i++)nb+=rikitu_demo_advance(&b,&lb,1.0f/60.0f,
                                          RIKITU_INPUT_RIGHT);
  for(i=0;i<120;i++)nc+=rikitu_demo_advance(&c,&lc,1.0f/120.0f,
                                           RIKITU_INPUT_RIGHT);
  assert(na==60 && nb==60 && nc==60);
  assert(closef(a.player.x,b.player.x,0.05f));
  assert(closef(b.player.x,c.player.x,0.05f));
  assert(closef(a.player.x,345.0f,0.1f));
  assert(a.stage.coin_count==b.stage.coin_count);
  assert(b.stage.coin_count==c.stage.coin_count);
  assert(closef(la.active_seconds,1.0f,0.001f));
  /* A 500ms stall never advances more than six 60-Hz ticks (100ms). */
  init(&a,&la);
  assert(rikitu_demo_advance(&a,&la,0.5f,RIKITU_INPUT_RIGHT)==6);
  assert(closef(la.active_seconds,0.1f,0.001f));
  assert(closef(a.player.x,142.5f,0.05f));
  /* Both conveyor and item collection occur on EACH simulated tick. */
  init(&a,&la);
  la.player.x=880.0f;la.player.y=497.0f;la.player.grounded=1;
  assert(rikitu_demo_advance(&a,&la,1.0f/30.0f,0)==2);
  assert(closef(a.player.x,880.0f+2.0f*85.0f/60.0f,0.05f));
  /* Avoid collection occurring only on the final render tick:
   * start in the first coin hitbox and check immediate collection. */
  init(&a,&la);
  la.player.x=201.0f;la.player.y=497.0f;la.player.grounded=1;
  assert(rikitu_demo_advance(&a,&la,1.0f/60.0f,0)==1);
  assert(a.stage.coin_count==1);
  assert(rikitu_demo_advance(&a,&la,1.0f/30.0f,0)==2);
  assert(a.stage.coin_count==1);
  /* Jump and gravity are applied inside a fixed tick, not display refresh. */
  init(&a,&la);
  assert(rikitu_demo_advance(&a,&la,1.0f/30.0f,RIKITU_INPUT_JUMP)==2);
  assert(a.player.vy<0.0f && !a.player.grounded);
  /* Pausing neither changes position nor advances active time, and
   * resuming never runs a backlog accumulated while paused. */
  init(&a,&la);
  (void)rikitu_demo_advance(&a,&la,1.0f/60.0f,RIKITU_INPUT_RIGHT);
  xbefore=a.player.x;elapsed=la.active_seconds;
  la.paused=1;
  assert(rikitu_demo_advance(&a,&la,1.0f,RIKITU_INPUT_RIGHT)==0);
  assert(a.paused && closef(a.player.x,xbefore,0.001f));
  assert(closef(la.active_seconds,elapsed,0.001f));
  assert(closef(la.accumulator_seconds,0.0f,0.001f));
  la.paused=0;
  assert(rikitu_demo_advance(&a,&la,0.0f,RIKITU_INPUT_RIGHT)==0);
  assert(closef(a.player.x,xbefore,0.001f));
  assert(rikitu_demo_advance(&a,&la,1.0f/60.0f,RIKITU_INPUT_RIGHT)==1);
  /* Gate is one-shot: a complete stage stops all further simulation. */
  init(&a,&la);
  for(i=0;i<3;i++)a.stage.fragments[i]=1;
  a.stage.fragment_count=3;
  la.player.x=a.gate_x-20.0f;
  la.player.y=497.0f;
  la.player.grounded=1;
  assert(rikitu_demo_advance(&a,&la,1.0f/60.0f,0)==1);
  assert(a.stage.completed);
  xbefore=a.player.x;elapsed=la.active_seconds;
  assert(rikitu_demo_advance(&a,&la,0.1f,RIKITU_INPUT_RIGHT)==0);
  assert(closef(a.player.x,xbefore,0.001f));
  assert(closef(la.active_seconds,elapsed,0.001f));
  assert(rikitu_demo_advance(NULL,&la,0.01f,0)==-1);
  assert(rikitu_demo_advance(&a,NULL,0.01f,0)==-1);
  assert(rikitu_demo_advance(&a,&la,-0.01f,0)==-1);
  assert(rikitu_demo_advance(&a,&la,NAN,0)==-1);
  puts("PASS measured fixed-step: 30/60/120 Hz, stall cap, surfaces, pickups, jump, pause and completion");
  return 0;
}
