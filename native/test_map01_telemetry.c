#include "rikitu_map01_telemetry.h"
#include "rikitu_flower.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void fresh(RikituScene *s,RikituLoop *l) {
  rikitu_flower_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
int main(void) {
  RikituScene s;
  RikituLoop l;
  RikituMap01Telemetry m,old;
  int i,ticks;
  fresh(&s,&l);
  assert(!rikitu_map01_telemetry_start(NULL,&s,&l));
  assert(rikitu_map01_telemetry_start(&m,&s,&l));
  assert(m.initialized&&m.observed_frames==0&&m.new_coins==0);
  for(i=0;i<3;i++)assert(m.checkpoint_active_seconds[i]<0.0f);
  l.paused=1;
  ticks=rikitu_demo_advance(&s,&l,0.1f,RIKITU_INPUT_RIGHT);
  assert(ticks==0);
  assert(rikitu_map01_telemetry_observe(&m,&s,&l,ticks));
  assert(m.paused_frames==1&&m.observed_ticks==0&&l.active_seconds==0.0f);
  l.paused=0;
  for(i=0;i<18;i++) {
    ticks=rikitu_demo_advance(&s,&l,RIKITU_FIXED_DT,
                              RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN|RIKITU_INPUT_JUMP);
    assert(ticks==1);
    assert(rikitu_map01_telemetry_observe(&m,&s,&l,ticks));
  }
  assert(m.observed_ticks==18&&m.observed_frames==19);
  assert(m.new_coins>0&&m.new_coins==(unsigned)s.stage.coin_count);
  assert(fabsf(l.active_seconds-18.0f*RIKITU_FIXED_DT)<0.01f);
  /* Tests below inject synthetic state changes, NOT player-route evidence. */
  s.reached_checkpoints=2;
  s.stage.fragment_count=2;
  s.stage.fragments[0]=s.stage.fragments[1]=1;
  s.damage_count+=1;
  assert(rikitu_map01_telemetry_observe(&m,&s,&l,0));
  assert(m.new_checkpoints==2&&m.new_fragments==2&&m.new_damage==1);
  assert(m.checkpoint_active_seconds[0]==l.active_seconds);
  assert(m.checkpoint_active_seconds[1]==l.active_seconds);
  assert(m.checkpoint_active_seconds[2]<0.0f);
  old=m;
  assert(!rikitu_map01_telemetry_observe(&m,&s,&l,7));
  assert(memcmp(&old,&m,sizeof(m))==0);
  s.stage.fragment_count=1;
  assert(!rikitu_map01_telemetry_observe(&m,&s,&l,0));
  assert(memcmp(&old,&m,sizeof(m))==0);
  s.stage.fragment_count=3;
  s.stage.fragments[2]=1;
  s.stage.completed=1;
  assert(rikitu_map01_telemetry_observe(&m,&s,&l,0));
  assert(rikitu_map01_telemetry_observe(&m,&s,&l,0));
  assert(m.completions==1);
  /* Saved/loaded state is an independent baseline, not a new pickup. */
  assert(rikitu_map01_telemetry_start(&m,&s,&l));
  assert(m.new_fragments==0&&m.new_checkpoints==0&&m.completions==0);
  assert(m.checkpoint_active_seconds[0]<0.0f);
  s.level_id=2;
  old=m;
  assert(!rikitu_map01_telemetry_observe(&m,&s,&l,0));
  assert(!rikitu_map01_telemetry_start(&m,&s,&l));
  assert(memcmp(&old,&m,sizeof(m))==0);
  puts("PASS MAP01 observational telemetry: pause, fixed ticks, events, split baseline, invalid-state fail-closed");
  return 0;
}
