#include "rikitu_loop.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int near(float a,float b) { return fabsf(a-b)<0.015f; }
int main(void) {
  RikituPlayer p;
  RikituRect floor={0.0f,100.0f,1000.0f,300.0f};
  RikituLoop loop;
  float before,elapsed;
  int i,n;
  rikitu_player_init(&p,50.0f,57.0f);
  assert(p.w==33.0f && p.h==43.0f && p.facing==1);
  p.grounded=1;
  assert(rikitu_player_step(&p,RIKITU_INPUT_RIGHT,RIKITU_FIXED_DT,
                            1000.0f,&floor,1));
  assert(near(p.x,53.75f) && near(p.y,57.0f) && p.grounded);
  assert(rikitu_player_step(&p,RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN,
                            RIKITU_FIXED_DT,1000.0f,&floor,1));
  assert(near(p.vx,265.0f));
  assert(rikitu_player_step(&p,RIKITU_INPUT_RIGHT|RIKITU_INPUT_LEFT,
                            RIKITU_FIXED_DT,1000.0f,&floor,1));
  assert(near(p.vx,0.0f));
  assert(rikitu_player_step(&p,RIKITU_INPUT_JUMP,RIKITU_FIXED_DT,
                            1000.0f,&floor,1));
  assert(p.y<57.0f && p.vy<0.0f && !p.grounded);
  before=p.vy;
  assert(rikitu_player_step(&p,RIKITU_INPUT_JUMP,RIKITU_FIXED_DT,
                            1000.0f,&floor,1));
  assert(p.vy>before && p.vy<0.0f);
  for(i=0;i<100;i++)
    assert(rikitu_player_step(&p,0,RIKITU_FIXED_DT,1000.0f,&floor,1));
  assert(p.grounded && near(p.y,57.0f));
  /* Buffered jump executes on the step after landing. */
  rikitu_player_init(&p,40.0f,47.0f); p.vy=250.0f;
  assert(rikitu_player_step(&p,RIKITU_INPUT_JUMP,RIKITU_FIXED_DT,
                            1000.0f,&floor,1));
  for(i=0;i<9 && !p.grounded;i++)
    assert(rikitu_player_step(&p,RIKITU_INPUT_JUMP,RIKITU_FIXED_DT,
                              1000.0f,&floor,1));
  assert(p.grounded && p.jump_buffer_seconds>0.0f);
  assert(rikitu_player_step(&p,RIKITU_INPUT_JUMP,RIKITU_FIXED_DT,
                            1000.0f,&floor,1));
  assert(!p.grounded && p.vy<0.0f);
  rikitu_player_init(&p,0.0f,57.0f); p.grounded=1;
  assert(rikitu_player_step(&p,RIKITU_INPUT_LEFT,RIKITU_FIXED_DT,
                            1000.0f,&floor,1));
  assert(near(p.x,0.0f));
  p.x=966.0f;
  assert(rikitu_player_step(&p,RIKITU_INPUT_RIGHT,RIKITU_FIXED_DT,
                            1000.0f,&floor,1));
  assert(near(p.x,967.0f));
  memset(&loop,0,sizeof(loop));
  rikitu_player_init(&loop.player,50.0f,57.0f); loop.player.grounded=1;
  n=rikitu_loop_advance(&loop,0.101f,RIKITU_INPUT_RIGHT,
                         1000.0f,&floor,1);
  assert(n==6 && near(loop.active_seconds,0.10f));
  loop.paused=1;before=loop.player.x;elapsed=loop.active_seconds;
  assert(rikitu_loop_advance(&loop,5.0f,RIKITU_INPUT_RIGHT,
                             1000.0f,&floor,1)==0);
  assert(near(loop.active_seconds,elapsed) && near(loop.player.x,before));
  loop.paused=0;
  assert(rikitu_loop_advance(&loop,RIKITU_FIXED_DT,RIKITU_INPUT_RIGHT,
                             1000.0f,&floor,1)>=1);
  assert(loop.player.x>before && loop.active_seconds>elapsed);
  assert(rikitu_loop_advance(&loop,-1.0f,0,1000.0f,&floor,1)==-1);
  assert(!rikitu_player_step(NULL,0,RIKITU_FIXED_DT,1000.0f,&floor,1));
  assert(!rikitu_player_step(&p,0,0.0f,1000.0f,&floor,1));
  assert(!rikitu_player_step(&p,0,RIKITU_FIXED_DT,1000.0f,NULL,1));
  printf("PASS C99 fixed-step controls: walk/run, jump/buffer, landing, pause, bounds and invalid input\n");
  return 0;
}
