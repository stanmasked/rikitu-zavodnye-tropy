#include "rikitu_gallery.h"
#include "rikitu_gallery_data.h"
#include <string.h>
void rikitu_gallery_init(RikituScene *s){
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=RIKITU_GALLERY_WIDTH;
  s->gate_x=RIKITU_GALLERY_GATE;
  s->level_id=15;
  s->solid_count=RIKITU_GALLERY_SOLIDS;
  s->coin_total=RIKITU_GALLERY_COINS;
  s->launcher_count=RIKITU_GALLERY_CLOCK_PADS;
  s->lever_count=RIKITU_GALLERY_SWITCHES;
  s->checkpoint_total=3;
  s->enemy_count=RIKITU_GALLERY_ENEMIES;
  s->hearts=RIKITU_STARTING_HEARTS;
  s->max_hearts=RIKITU_STARTING_HEARTS;
  s->active_checkpoint=105.0f;
  memcpy(s->solids,rikitu_gallery_solids,sizeof(rikitu_gallery_solids));
  memcpy(s->fragments,rikitu_gallery_fragments,sizeof(rikitu_gallery_fragments));
  memcpy(s->coins,rikitu_gallery_coins,sizeof(rikitu_gallery_coins));
  memcpy(s->checkpoints,rikitu_gallery_checkpoints,sizeof(rikitu_gallery_checkpoints));
  memcpy(s->enemies,rikitu_gallery_enemies,sizeof(rikitu_gallery_enemies));
  memcpy(s->launchers,rikitu_gallery_clock_pads,sizeof(rikitu_gallery_clock_pads));
  memcpy(s->levers,rikitu_gallery_levers,sizeof(rikitu_gallery_levers));
  rikitu_stage_init(&s->stage,0);
  rikitu_player_init(&s->player,105.0f,413.0f);
  s->player.grounded=0;
}
