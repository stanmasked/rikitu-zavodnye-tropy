#include "rikitu_delivery.h"
#include "rikitu_delivery_data.h"
#include <string.h>
void rikitu_delivery_init(RikituScene *s){
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=RIKITU_DELIVERY_WIDTH;
  s->gate_x=RIKITU_DELIVERY_GATE;
  s->level_id=6;
  s->solid_count=RIKITU_DELIVERY_SOLIDS;
  s->coin_total=RIKITU_DELIVERY_COINS;
  s->belt_count=RIKITU_DELIVERY_BELTS;
  s->launcher_count=RIKITU_DELIVERY_VENTS;
  s->checkpoint_total=3;
  s->enemy_count=RIKITU_DELIVERY_ENEMIES;
  s->hearts=RIKITU_STARTING_HEARTS;
  s->max_hearts=RIKITU_STARTING_HEARTS;
  s->active_checkpoint=105.0f;
  memcpy(s->solids,rikitu_delivery_solids,sizeof(rikitu_delivery_solids));
  memcpy(s->fragments,rikitu_delivery_fragments,sizeof(rikitu_delivery_fragments));
  memcpy(s->coins,rikitu_delivery_coins,sizeof(rikitu_delivery_coins));
  memcpy(s->checkpoints,rikitu_delivery_checkpoints,sizeof(rikitu_delivery_checkpoints));
  memcpy(s->enemies,rikitu_delivery_enemies,sizeof(rikitu_delivery_enemies));
  memcpy(s->belts,rikitu_delivery_belts,sizeof(rikitu_delivery_belts));
  memcpy(s->launchers,rikitu_delivery_vents,sizeof(rikitu_delivery_vents));
  rikitu_stage_init(&s->stage,1);
  s->boss=(RikituBoss){RIKITU_DELIVERY_BOSS_X,399.0f,72.0f,57.0f,RIKITU_DELIVERY_BOSS_X,0.0f,3,1,1};
  rikitu_player_init(&s->player,105.0f,413.0f);
  s->player.grounded=0;
}
