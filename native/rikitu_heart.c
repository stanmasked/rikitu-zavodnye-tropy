#include "rikitu_heart.h"
#include "rikitu_heart_data.h"
#include <string.h>
void rikitu_heart_init(RikituScene *s){
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=RIKITU_HEART_WIDTH;
  s->gate_x=RIKITU_HEART_GATE;
  s->level_id=16;
  s->solid_count=RIKITU_HEART_SOLIDS;
  s->coin_total=RIKITU_HEART_COINS;
  s->belt_count=RIKITU_HEART_BELTS;
  s->launcher_count=RIKITU_HEART_LAUNCHERS;
  s->checkpoint_total=3;
  s->enemy_count=RIKITU_HEART_ENEMIES;
  s->hearts=RIKITU_STARTING_HEARTS;
  s->max_hearts=RIKITU_STARTING_HEARTS;
  s->active_checkpoint=105.0f;
  memcpy(s->solids,rikitu_heart_solids,sizeof(rikitu_heart_solids));
  memcpy(s->fragments,rikitu_heart_fragments,sizeof(rikitu_heart_fragments));
  memcpy(s->coins,rikitu_heart_coins,sizeof(rikitu_heart_coins));
  memcpy(s->checkpoints,rikitu_heart_checkpoints,sizeof(rikitu_heart_checkpoints));
  memcpy(s->enemies,rikitu_heart_enemies,sizeof(rikitu_heart_enemies));
  memcpy(s->belts,rikitu_heart_belts,sizeof(rikitu_heart_belts));
  memcpy(s->launchers,rikitu_heart_launchers,sizeof(rikitu_heart_launchers));
  rikitu_stage_init(&s->stage,1);
  s->boss=(RikituBoss){RIKITU_HEART_BOSS_X,399.0f,72.0f,57.0f,
                       RIKITU_HEART_BOSS_X,0.0f,3,1,1};
  rikitu_player_init(&s->player,105.0f,413.0f);
  s->player.grounded=0;
}
