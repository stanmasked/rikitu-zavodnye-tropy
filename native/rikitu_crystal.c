#include "rikitu_crystal.h"
#include "rikitu_crystal_data.h"
#include <string.h>
void rikitu_crystal_init(RikituScene *s){
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=RIKITU_CRYSTAL_WIDTH;
  s->gate_x=RIKITU_CRYSTAL_GATE;
  s->level_id=11;
  s->solid_count=RIKITU_CRYSTAL_SOLIDS;
  s->coin_total=RIKITU_CRYSTAL_COINS;
  s->launcher_count=RIKITU_CRYSTAL_SPRINGS;
  s->checkpoint_total=3;
  s->enemy_count=RIKITU_CRYSTAL_ENEMIES;
  s->hearts=RIKITU_STARTING_HEARTS;
  s->max_hearts=RIKITU_STARTING_HEARTS;
  s->active_checkpoint=105.0f;
  memcpy(s->solids,rikitu_crystal_solids,sizeof(rikitu_crystal_solids));
  memcpy(s->fragments,rikitu_crystal_fragments,sizeof(rikitu_crystal_fragments));
  memcpy(s->coins,rikitu_crystal_coins,sizeof(rikitu_crystal_coins));
  memcpy(s->checkpoints,rikitu_crystal_checkpoints,sizeof(rikitu_crystal_checkpoints));
  memcpy(s->enemies,rikitu_crystal_enemies,sizeof(rikitu_crystal_enemies));
  memcpy(s->launchers,rikitu_crystal_springs,sizeof(rikitu_crystal_springs));
  rikitu_stage_init(&s->stage,0);
  rikitu_player_init(&s->player,105.0f,413.0f);
  s->player.grounded=0;
}
