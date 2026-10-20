#include "rikitu_workshop.h"
#include "rikitu_workshop_data.h"
#include <string.h>
void rikitu_workshop_init(RikituScene *s){
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=RIKITU_WORKSHOP_WIDTH;
  s->gate_x=RIKITU_WORKSHOP_GATE;
  s->level_id=5;
  s->solid_count=RIKITU_WORKSHOP_SOLIDS;
  s->coin_total=RIKITU_WORKSHOP_COINS;
  s->belt_count=RIKITU_WORKSHOP_BELTS;
  s->checkpoint_total=3;
  s->enemy_count=RIKITU_WORKSHOP_ENEMIES;
  s->hearts=RIKITU_STARTING_HEARTS;
  s->max_hearts=RIKITU_STARTING_HEARTS;
  s->active_checkpoint=105.0f;
  memcpy(s->solids,rikitu_workshop_solids,sizeof(rikitu_workshop_solids));
  memcpy(s->fragments,rikitu_workshop_fragments,sizeof(rikitu_workshop_fragments));
  memcpy(s->coins,rikitu_workshop_coins,sizeof(rikitu_workshop_coins));
  memcpy(s->checkpoints,rikitu_workshop_checkpoints,sizeof(rikitu_workshop_checkpoints));
  memcpy(s->enemies,rikitu_workshop_enemies,sizeof(rikitu_workshop_enemies));
  memcpy(s->belts,rikitu_workshop_belts,sizeof(rikitu_workshop_belts));
  rikitu_stage_init(&s->stage,0);
  rikitu_player_init(&s->player,105.0f,413.0f);
  s->player.grounded=0;
}
