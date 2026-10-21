#include "rikitu_mine.h"
#include "rikitu_mine_data.h"
#include <string.h>
void rikitu_mine_init(RikituScene *s){
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=RIKITU_MINE_WIDTH;
  s->gate_x=RIKITU_MINE_GATE;
  s->level_id=12;
  s->solid_count=RIKITU_MINE_SOLIDS;
  s->coin_total=RIKITU_MINE_COINS;
  s->belt_count=RIKITU_MINE_BELTS;
  s->launcher_count=RIKITU_MINE_VENTS;
  s->lever_count=RIKITU_MINE_SWITCHES;
  s->checkpoint_total=3;
  s->enemy_count=RIKITU_MINE_ENEMIES;
  s->hearts=RIKITU_STARTING_HEARTS;
  s->max_hearts=RIKITU_STARTING_HEARTS;
  s->active_checkpoint=105.0f;
  memcpy(s->solids,rikitu_mine_solids,sizeof(rikitu_mine_solids));
  memcpy(s->fragments,rikitu_mine_fragments,sizeof(rikitu_mine_fragments));
  memcpy(s->coins,rikitu_mine_coins,sizeof(rikitu_mine_coins));
  memcpy(s->checkpoints,rikitu_mine_checkpoints,sizeof(rikitu_mine_checkpoints));
  memcpy(s->enemies,rikitu_mine_enemies,sizeof(rikitu_mine_enemies));
  memcpy(s->belts,rikitu_mine_belts,sizeof(rikitu_mine_belts));
  memcpy(s->launchers,rikitu_mine_vents,sizeof(rikitu_mine_vents));
  memcpy(s->levers,rikitu_mine_levers,sizeof(rikitu_mine_levers));
  rikitu_stage_init(&s->stage,1);
  s->boss=(RikituBoss){RIKITU_MINE_BOSS_X,399.0f,72.0f,57.0f,
                       RIKITU_MINE_BOSS_X,0.0f,3,1,1};
  rikitu_player_init(&s->player,105.0f,413.0f);
  s->player.grounded=0;
}
