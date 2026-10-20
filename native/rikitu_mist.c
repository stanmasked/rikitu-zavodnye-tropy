#include "rikitu_mist.h"
#include "rikitu_mist_data.h"
#include <string.h>
void rikitu_mist_init(RikituScene *s){
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=RIKITU_MIST_WIDTH;
  s->gate_x=RIKITU_MIST_GATE;
  s->level_id=9;
  s->solid_count=RIKITU_MIST_SOLIDS;
  s->coin_total=RIKITU_MIST_COINS;
  s->launcher_count=RIKITU_MIST_GUSTS;
  s->checkpoint_total=3;
  s->enemy_count=RIKITU_MIST_ENEMIES;
  s->hearts=RIKITU_STARTING_HEARTS;
  s->max_hearts=RIKITU_STARTING_HEARTS;
  s->active_checkpoint=105.0f;
  memcpy(s->solids,rikitu_mist_solids,sizeof(rikitu_mist_solids));
  memcpy(s->fragments,rikitu_mist_fragments,sizeof(rikitu_mist_fragments));
  memcpy(s->coins,rikitu_mist_coins,sizeof(rikitu_mist_coins));
  memcpy(s->checkpoints,rikitu_mist_checkpoints,sizeof(rikitu_mist_checkpoints));
  memcpy(s->enemies,rikitu_mist_enemies,sizeof(rikitu_mist_enemies));
  memcpy(s->launchers,rikitu_mist_gusts,sizeof(rikitu_mist_gusts));
  rikitu_stage_init(&s->stage,0);
  rikitu_player_init(&s->player,105.0f,413.0f);
  s->player.grounded=0;
}
