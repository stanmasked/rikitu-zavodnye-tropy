#include "rikitu_stage.h"
#include <math.h>
#include <string.h>
static int rect_valid(RikituRect r){
  return isfinite(r.x)&&isfinite(r.y)&&isfinite(r.w)&&isfinite(r.h)&&
         r.w>0.0f&&r.h>0.0f;
}
void rikitu_stage_init(RikituStage *s,int boss_required){
  if(!s)return;
  memset(s,0,sizeof(*s));s->boss_required=!!boss_required;
  s->boss_defeated=!s->boss_required;
}
int rikitu_stage_pickups(RikituStage *s,RikituRect player,
                         const RikituPoint fragments[RIKITU_STAGE_FRAGMENTS],
                         const RikituPoint *coins,size_t coin_total){
  size_t i;int newly=0;
  if(!s||!fragments||!rect_valid(player)||coin_total>RIKITU_STAGE_COINS||
     (coin_total&&!coins))return -1;
  for(i=0;i<RIKITU_STAGE_FRAGMENTS;i++)
    if(!isfinite(fragments[i].x)||!isfinite(fragments[i].y))return -1;
  for(i=0;i<coin_total;i++)
    if(!isfinite(coins[i].x)||!isfinite(coins[i].y))return -1;
  for(i=0;i<RIKITU_STAGE_FRAGMENTS;i++){
    const RikituRect pickup={fragments[i].x-20.0f,
         fragments[i].y-30.0f,65.0f,72.0f};
    if(!s->fragments[i]&&rikitu_box_hit(player,pickup)){
      s->fragments[i]=1;s->fragment_count++;newly++;
    }
  }
  for(i=0;i<coin_total;i++){
    const RikituRect pickup={coins[i].x-12.0f,
         coins[i].y-12.0f,24.0f,24.0f};
    if(!s->coins[i]&&rikitu_box_hit(player,pickup)){
      s->coins[i]=1;s->coin_count++;newly++;
    }
  }
  return newly;
}
int rikitu_stage_can_finish(const RikituStage *s){
  return s&&rikitu_can_exit(s->fragment_count,s->boss_required,s->boss_defeated);
}
RikituGateResult rikitu_stage_touch_gate(RikituStage *s,
                                        RikituRect player,float gate_x){
  if(!s||!rect_valid(player)||!isfinite(gate_x))return RIKITU_GATE_INVALID;
  if(s->completed)return RIKITU_GATE_ALREADY_COMPLETED;
  if(!(player.x+player.w>gate_x-30.0f))return RIKITU_GATE_NOT_REACHED;
  if(!rikitu_stage_can_finish(s))return RIKITU_GATE_BLOCKED;
  s->completed=1;return RIKITU_GATE_COMPLETED;
}
RikituNextMode rikitu_stage_next_mode(int stage_number){
  if(stage_number<1||stage_number>16)return 0;
  return rikitu_is_shop_stage(stage_number)?
         RIKITU_NEXT_SHOP:RIKITU_NEXT_BETWEEN;
}
RikituNextMode rikitu_stage_after_shop(int stage_number){
  if(!rikitu_is_shop_stage(stage_number))return 0;
  return stage_number==16?RIKITU_NEXT_VICTORY:RIKITU_NEXT_BETWEEN;
}
