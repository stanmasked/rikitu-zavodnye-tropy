#include "rikitu_foundry.h"
#include "rikitu_foundry_data.h"
#include <string.h>
void rikitu_foundry_init(RikituScene *s){
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=RIKITU_FOUNDRY_WIDTH;
  s->gate_x=RIKITU_FOUNDRY_GATE;
  s->level_id=14;
  s->solid_count=RIKITU_FOUNDRY_SOLIDS;
  s->coin_total=RIKITU_FOUNDRY_COINS;
  s->belt_count=RIKITU_FOUNDRY_BELTS;
  s->launcher_count=RIKITU_FOUNDRY_VENTS;
  s->lever_count=RIKITU_FOUNDRY_SWITCHES;
  s->checkpoint_total=3;
  s->enemy_count=RIKITU_FOUNDRY_ENEMIES;
  s->hearts=RIKITU_STARTING_HEARTS;
  s->max_hearts=RIKITU_STARTING_HEARTS;
  s->active_checkpoint=105.0f;
  memcpy(s->solids,rikitu_foundry_solids,sizeof(rikitu_foundry_solids));
  memcpy(s->fragments,rikitu_foundry_fragments,sizeof(rikitu_foundry_fragments));
  memcpy(s->coins,rikitu_foundry_coins,sizeof(rikitu_foundry_coins));
  memcpy(s->checkpoints,rikitu_foundry_checkpoints,sizeof(rikitu_foundry_checkpoints));
  memcpy(s->enemies,rikitu_foundry_enemies,sizeof(rikitu_foundry_enemies));
  memcpy(s->belts,rikitu_foundry_belts,sizeof(rikitu_foundry_belts));
  memcpy(s->launchers,rikitu_foundry_vents,sizeof(rikitu_foundry_vents));
  memcpy(s->levers,rikitu_foundry_levers,sizeof(rikitu_foundry_levers));
  rikitu_stage_init(&s->stage,1);
  s->boss=(RikituBoss){RIKITU_FOUNDRY_BOSS_X,399.0f,72.0f,57.0f,
                       RIKITU_FOUNDRY_BOSS_X,0.0f,3,1,1};
  rikitu_player_init(&s->player,105.0f,413.0f);
  s->player.grounded=0;
}
