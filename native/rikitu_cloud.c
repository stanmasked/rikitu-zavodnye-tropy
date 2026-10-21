#include "rikitu_cloud.h"
#include "rikitu_cloud_data.h"
#include <string.h>
void rikitu_cloud_init(RikituScene *s){
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=RIKITU_CLOUD_WIDTH;
  s->gate_x=RIKITU_CLOUD_GATE;
  s->level_id=10;
  s->solid_count=RIKITU_CLOUD_SOLIDS;
  s->coin_total=RIKITU_CLOUD_COINS;
  s->belt_count=RIKITU_CLOUD_BELTS;
  s->launcher_count=RIKITU_CLOUD_GUSTS;
  s->checkpoint_total=3;
  s->enemy_count=RIKITU_CLOUD_ENEMIES;
  s->hearts=RIKITU_STARTING_HEARTS;
  s->max_hearts=RIKITU_STARTING_HEARTS;
  s->active_checkpoint=105.0f;
  memcpy(s->solids,rikitu_cloud_solids,sizeof(rikitu_cloud_solids));
  memcpy(s->fragments,rikitu_cloud_fragments,sizeof(rikitu_cloud_fragments));
  memcpy(s->coins,rikitu_cloud_coins,sizeof(rikitu_cloud_coins));
  memcpy(s->checkpoints,rikitu_cloud_checkpoints,sizeof(rikitu_cloud_checkpoints));
  memcpy(s->enemies,rikitu_cloud_enemies,sizeof(rikitu_cloud_enemies));
  memcpy(s->belts,rikitu_cloud_belts,sizeof(rikitu_cloud_belts));
  memcpy(s->launchers,rikitu_cloud_gusts,sizeof(rikitu_cloud_gusts));
  rikitu_stage_init(&s->stage,1);
  s->boss=(RikituBoss){RIKITU_CLOUD_BOSS_X,399.0f,72.0f,57.0f,
                       RIKITU_CLOUD_BOSS_X,0.0f,3,1,1};
  rikitu_player_init(&s->player,105.0f,413.0f);
  s->player.grounded=0;
}
