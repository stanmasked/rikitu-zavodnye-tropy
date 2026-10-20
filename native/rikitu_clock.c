#include "rikitu_clock.h"
#include "rikitu_clock_data.h"
#include <string.h>
void rikitu_clock_init(RikituScene *s){
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=RIKITU_CLOCK_WIDTH;
  s->gate_x=RIKITU_CLOCK_GATE;
  s->level_id=7;
  s->solid_count=RIKITU_CLOCK_SOLIDS;
  s->coin_total=RIKITU_CLOCK_COINS;
  s->belt_count=RIKITU_CLOCK_BELTS;
  s->launcher_count=RIKITU_CLOCK_PADS;
  s->checkpoint_total=3;
  s->enemy_count=RIKITU_CLOCK_ENEMIES;
  s->hearts=RIKITU_STARTING_HEARTS;
  s->max_hearts=RIKITU_STARTING_HEARTS;
  s->active_checkpoint=105.0f;
  memcpy(s->solids,rikitu_clock_solids,sizeof(rikitu_clock_solids));
  memcpy(s->fragments,rikitu_clock_fragments,sizeof(rikitu_clock_fragments));
  memcpy(s->coins,rikitu_clock_coins,sizeof(rikitu_clock_coins));
  memcpy(s->checkpoints,rikitu_clock_checkpoints,sizeof(rikitu_clock_checkpoints));
  memcpy(s->enemies,rikitu_clock_enemies,sizeof(rikitu_clock_enemies));
  memcpy(s->belts,rikitu_clock_belts,sizeof(rikitu_clock_belts));
  memcpy(s->launchers,rikitu_clock_pads,sizeof(rikitu_clock_pads));
  rikitu_stage_init(&s->stage,0);
  rikitu_player_init(&s->player,105.0f,413.0f);
  s->player.grounded=0;
}
