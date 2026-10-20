#include "rikitu_flower.h"
#include "rikitu_flower_data.h"
#include <string.h>
void rikitu_flower_init(RikituScene *s) {
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=RIKITU_FLOWER_WORLD_WIDTH;
  s->gate_x=RIKITU_FLOWER_GATE;
  s->level_id=1;
  s->solid_count=RIKITU_FLOWER_SOLID_COUNT;
  s->coin_total=RIKITU_FLOWER_COIN_COUNT;
  s->checkpoint_total=RIKITU_FLOWER_CP_COUNT;
  s->active_checkpoint=105.0f;
  memcpy(s->solids,rikitu_flower_solids,sizeof(rikitu_flower_solids));
  memcpy(s->fragments,rikitu_flower_fragments,sizeof(rikitu_flower_fragments));
  memcpy(s->coins,rikitu_flower_coins,sizeof(rikitu_flower_coins));
  memcpy(s->checkpoints,rikitu_flower_checkpoints,sizeof(rikitu_flower_checkpoints));
  rikitu_stage_init(&s->stage,0);
  rikitu_player_init(&s->player,105.0f,456.0f-43.0f);
  /* The browser initializes airborne. First physics tick grounds the player. */
  s->player.grounded=0;
}
