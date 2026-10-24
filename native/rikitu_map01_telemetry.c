#include "rikitu_map01_telemetry.h"
#include <limits.h>
#include <math.h>
#include <string.h>
static int valid_sample(const RikituScene *s,const RikituLoop *l) {
  if(!s||!l||s->level_id!=1||s->checkpoint_total!=3 ||
     s->coin_total>RIKITU_STAGE_COINS||
     s->stage.fragment_count<0||
     s->stage.fragment_count>RIKITU_STAGE_FRAGMENTS||
     s->stage.coin_count<0||(size_t)s->stage.coin_count>s->coin_total||
     s->reached_checkpoints<0||
     (size_t)s->reached_checkpoints>s->checkpoint_total||
     s->damage_count<0||
     (s->stage.completed!=0&&s->stage.completed!=1)||
     !isfinite(l->active_seconds)||l->active_seconds<0.0f)
    return 0;
  return 1;
}
int rikitu_map01_telemetry_start(RikituMap01Telemetry *m,
                                 const RikituScene *s,const RikituLoop *l) {
  unsigned i;
  if(!m||!valid_sample(s,l))return 0;
  memset(m,0,sizeof(*m));
  for(i=0;i<RIKITU_MAX_CHECKPOINTS;i++)
    m->checkpoint_active_seconds[i]=-1.0f;
  m->last_fragments=s->stage.fragment_count;
  m->last_coins=s->stage.coin_count;
  m->last_checkpoints=s->reached_checkpoints;
  m->last_damage=s->damage_count;
  m->last_completed=s->stage.completed;
  m->last_active_seconds=l->active_seconds;
  m->initialized=1;
  return 1;
}
int rikitu_map01_telemetry_observe(RikituMap01Telemetry *m,
                                   const RikituScene *s,const RikituLoop *l,
                                   int ticks) {
  RikituMap01Telemetry next;
  int cp;
  if(!m||!m->initialized||!valid_sample(s,l)||ticks<0||ticks>6||
     (l->paused&&ticks!=0)||
     /* Float accumulation at long playtimes can lose a fraction of a tick. */
     l->active_seconds+0.02f<m->last_active_seconds||
     s->stage.fragment_count<m->last_fragments||
     s->stage.coin_count<m->last_coins||
     s->reached_checkpoints<m->last_checkpoints||
     s->damage_count<m->last_damage||
     s->stage.completed<m->last_completed||
     m->observed_frames==UINT_MAX||
     (unsigned)ticks>UINT_MAX-m->observed_ticks||
     (l->paused&&m->paused_frames==UINT_MAX))return 0;
  next=*m;
  next.observed_frames++;
  next.observed_ticks+=(unsigned)ticks;
  if(l->paused)next.paused_frames++;
  next.new_fragments+=(unsigned)(s->stage.fragment_count-m->last_fragments);
  next.new_coins+=(unsigned)(s->stage.coin_count-m->last_coins);
  next.new_checkpoints+=(unsigned)(s->reached_checkpoints-m->last_checkpoints);
  next.new_damage+=(unsigned)(s->damage_count-m->last_damage);
  if(s->stage.completed&&!m->last_completed)next.completions++;
  for(cp=m->last_checkpoints;cp<s->reached_checkpoints;cp++)
    next.checkpoint_active_seconds[cp]=l->active_seconds;
  next.last_fragments=s->stage.fragment_count;
  next.last_coins=s->stage.coin_count;
  next.last_checkpoints=s->reached_checkpoints;
  next.last_damage=s->damage_count;
  next.last_completed=s->stage.completed;
  next.last_active_seconds=l->active_seconds;
  *m=next;
  return 1;
}
