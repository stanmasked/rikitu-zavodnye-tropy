#include "rikitu_bridge.h"
#include "rikitu_bridge_data.h"
#include <string.h>
void rikitu_bridge_init(RikituScene *s){
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=RIKITU_BRIDGE_WIDTH;
  s->gate_x=RIKITU_BRIDGE_GATE;
  s->level_id=4;
  s->solid_count=RIKITU_BRIDGE_SOLIDS;
  s->coin_total=RIKITU_BRIDGE_COINS;
  s->checkpoint_total=3;
  s->enemy_count=RIKITU_BRIDGE_ENEMIES;
  s->lever_count=RIKITU_BRIDGE_SWITCHES;
  s->hearts=RIKITU_STARTING_HEARTS;
  s->max_hearts=RIKITU_STARTING_HEARTS;
  s->active_checkpoint=105.0f;
  memcpy(s->solids,rikitu_bridge_solids,sizeof(rikitu_bridge_solids));
  memcpy(s->fragments,rikitu_bridge_fragments,sizeof(rikitu_bridge_fragments));
  memcpy(s->coins,rikitu_bridge_coins,sizeof(rikitu_bridge_coins));
  memcpy(s->checkpoints,rikitu_bridge_checkpoints,sizeof(rikitu_bridge_checkpoints));
  memcpy(s->enemies,rikitu_bridge_enemies,sizeof(rikitu_bridge_enemies));
  memcpy(s->levers,rikitu_bridge_levers,sizeof(rikitu_bridge_levers));
  rikitu_stage_init(&s->stage,1);
  s->boss=(RikituBoss){8290.0f,399.0f,72.0f,57.0f,8290.0f,0.0f,3,1,1};
  rikitu_player_init(&s->player,105.0f,413.0f);
  s->player.grounded=0;
}
