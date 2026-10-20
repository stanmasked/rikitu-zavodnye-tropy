#include "rikitu_orders.h"
#include "rikitu_orders_data.h"
#include <string.h>
void rikitu_orders_init(RikituScene *s){
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=RIKITU_ORDERS_WIDTH;
  s->gate_x=RIKITU_ORDERS_GATE;
  s->level_id=8;
  s->solid_count=RIKITU_ORDERS_SOLIDS;
  s->coin_total=RIKITU_ORDERS_COINS;
  s->belt_count=RIKITU_ORDERS_BELTS;
  s->lever_count=RIKITU_ORDERS_SWITCHES;
  s->checkpoint_total=3;
  s->enemy_count=RIKITU_ORDERS_ENEMIES;
  s->hearts=RIKITU_STARTING_HEARTS;
  s->max_hearts=RIKITU_STARTING_HEARTS;
  s->active_checkpoint=105.0f;
  memcpy(s->solids,rikitu_orders_solids,sizeof(rikitu_orders_solids));
  memcpy(s->fragments,rikitu_orders_fragments,sizeof(rikitu_orders_fragments));
  memcpy(s->coins,rikitu_orders_coins,sizeof(rikitu_orders_coins));
  memcpy(s->checkpoints,rikitu_orders_checkpoints,sizeof(rikitu_orders_checkpoints));
  memcpy(s->enemies,rikitu_orders_enemies,sizeof(rikitu_orders_enemies));
  memcpy(s->belts,rikitu_orders_belts,sizeof(rikitu_orders_belts));
  memcpy(s->levers,rikitu_orders_levers,sizeof(rikitu_orders_levers));
  rikitu_stage_init(&s->stage,1);
  s->boss=(RikituBoss){RIKITU_ORDERS_BOSS_X,399.0f,72.0f,57.0f,RIKITU_ORDERS_BOSS_X,0.0f,3,1,1};
  rikitu_player_init(&s->player,105.0f,413.0f);
  s->player.grounded=0;
}
