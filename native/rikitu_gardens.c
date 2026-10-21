#include "rikitu_gardens.h"
#include "rikitu_gardens_data.h"
#include <string.h>
void rikitu_gardens_init(RikituScene *s){
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=RIKITU_GARDENS_WIDTH;
  s->gate_x=RIKITU_GARDENS_GATE;
  s->level_id=13;
  s->solid_count=RIKITU_GARDENS_SOLIDS;
  s->coin_total=RIKITU_GARDENS_COINS;
  s->launcher_count=RIKITU_GARDENS_MUSHROOMS;
  s->lever_count=RIKITU_GARDENS_SWITCHES;
  s->checkpoint_total=3;
  s->enemy_count=RIKITU_GARDENS_ENEMIES;
  s->hearts=RIKITU_STARTING_HEARTS;
  s->max_hearts=RIKITU_STARTING_HEARTS;
  s->active_checkpoint=105.0f;
  memcpy(s->solids,rikitu_gardens_solids,sizeof(rikitu_gardens_solids));
  memcpy(s->fragments,rikitu_gardens_fragments,sizeof(rikitu_gardens_fragments));
  memcpy(s->coins,rikitu_gardens_coins,sizeof(rikitu_gardens_coins));
  memcpy(s->checkpoints,rikitu_gardens_checkpoints,sizeof(rikitu_gardens_checkpoints));
  memcpy(s->enemies,rikitu_gardens_enemies,sizeof(rikitu_gardens_enemies));
  memcpy(s->launchers,rikitu_gardens_mushrooms,sizeof(rikitu_gardens_mushrooms));
  memcpy(s->levers,rikitu_gardens_levers,sizeof(rikitu_gardens_levers));
  rikitu_stage_init(&s->stage,0);
  rikitu_player_init(&s->player,105.0f,413.0f);
  s->player.grounded=0;
}
