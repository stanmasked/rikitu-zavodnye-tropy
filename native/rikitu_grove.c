#include "rikitu_grove.h"
#include "rikitu_grove_data.h"
#include <string.h>
void rikitu_grove_init(RikituScene *s){
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=RIKITU_GROVE_WIDTH;
  s->gate_x=RIKITU_GROVE_GATE;
  s->level_id=3;
  s->solid_count=RIKITU_GROVE_SOLIDS;
  s->coin_total=RIKITU_GROVE_COINS;
  s->checkpoint_total=3;
  s->enemy_count=RIKITU_GROVE_ENEMIES;
  s->launcher_count=RIKITU_GROVE_MUSHROOMS;
  s->hearts=RIKITU_STARTING_HEARTS;
  s->max_hearts=RIKITU_STARTING_HEARTS;
  s->active_checkpoint=105.0f;
  memcpy(s->solids,rikitu_grove_solids,sizeof(rikitu_grove_solids));
  memcpy(s->fragments,rikitu_grove_fragments,sizeof(rikitu_grove_fragments));
  memcpy(s->coins,rikitu_grove_coins,sizeof(rikitu_grove_coins));
  memcpy(s->checkpoints,rikitu_grove_checkpoints,sizeof(rikitu_grove_checkpoints));
  memcpy(s->enemies,rikitu_grove_enemies,sizeof(rikitu_grove_enemies));
  memcpy(s->launchers,rikitu_grove_mushrooms,sizeof(rikitu_grove_mushrooms));
  rikitu_stage_init(&s->stage,0);
  rikitu_player_init(&s->player,105.0f,413.0f);
  s->player.grounded=0;
}
