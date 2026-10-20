#include "rikitu_mill.h"
#include "rikitu_mill_data.h"
#include <string.h>
void rikitu_mill_init(RikituScene *s){
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=RIKITU_MILL_WIDTH;
  s->gate_x=RIKITU_MILL_GATE;
  s->level_id=2;
  s->solid_count=RIKITU_MILL_SOLIDS;
  s->coin_total=RIKITU_MILL_COINS;
  s->checkpoint_total=3;
  s->enemy_count=RIKITU_MILL_ENEMIES;
  s->hearts=RIKITU_STARTING_HEARTS;
  s->max_hearts=RIKITU_STARTING_HEARTS;
  s->active_checkpoint=105.0f;
  memcpy(s->solids,rikitu_mill_solids,sizeof(rikitu_mill_solids));
  memcpy(s->fragments,rikitu_mill_fragments,sizeof(rikitu_mill_fragments));
  memcpy(s->coins,rikitu_mill_coins,sizeof(rikitu_mill_coins));
  memcpy(s->checkpoints,rikitu_mill_checkpoints,sizeof(rikitu_mill_checkpoints));
  memcpy(s->enemies,rikitu_mill_enemy_spawns,sizeof(rikitu_mill_enemy_spawns));
  rikitu_stage_init(&s->stage,1);
  s->boss=(RikituBoss){7910.0f,399.0f,72.0f,57.0f,7910.0f,0.0f,3,1,1};
  rikitu_player_init(&s->player,105.0f,413.0f);
  s->player.grounded=0;
}
