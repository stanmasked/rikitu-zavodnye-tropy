#include "rikitu_stage.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static RikituRect player(float x,float y){
  RikituRect p={x,y,33.0f,43.0f};return p;
}
int main(void){
  RikituStage s;
  RikituPoint f[3]={{100,100},{200,100},{300,100}};
  RikituPoint c[90]={{100,100},{202,101}};
  RikituRect gate=player(845,57),p=player(90,65);
  RikituPoint bad[3]={{100,100},{NAN,100},{300,100}};
  int i;
  rikitu_stage_init(&s,0);
  assert(s.fragment_count==0&&s.coin_count==0&&s.boss_defeated);
  assert(!rikitu_stage_can_finish(&s));
  assert(rikitu_stage_pickups(&s,p,f,c,2)==2);
  assert(s.fragment_count==1&&s.coin_count==1);
  assert(rikitu_stage_pickups(&s,p,f,c,2)==0);
  assert(rikitu_stage_pickups(&s,player(190,65),f,c,2)==2);
  assert(rikitu_stage_pickups(&s,player(290,65),f,c,2)==1);
  assert(s.fragment_count==3&&s.coin_count==2);
  assert(rikitu_stage_can_finish(&s));
  assert(rikitu_stage_touch_gate(&s,player(837,57),900)==RIKITU_GATE_NOT_REACHED);
  assert(rikitu_stage_touch_gate(&s,gate,900)==RIKITU_GATE_COMPLETED);
  assert(rikitu_stage_touch_gate(&s,gate,900)==RIKITU_GATE_ALREADY_COMPLETED);
  rikitu_stage_init(&s,1);
  assert(s.boss_required&&!s.boss_defeated);
  assert(rikitu_stage_touch_gate(&s,gate,900)==RIKITU_GATE_BLOCKED);
  assert(rikitu_stage_pickups(&s,player(90,65),f,c,2)==2);
  assert(rikitu_stage_pickups(&s,player(190,65),f,c,2)==2);
  assert(rikitu_stage_pickups(&s,player(290,65),f,c,2)==1);
  assert(!rikitu_stage_can_finish(&s));
  assert(rikitu_stage_touch_gate(&s,gate,900)==RIKITU_GATE_BLOCKED);
  s.boss_defeated=1;
  assert(rikitu_stage_can_finish(&s));
  assert(rikitu_stage_touch_gate(&s,gate,900)==RIKITU_GATE_COMPLETED);
  rikitu_stage_init(&s,0);
  assert(rikitu_stage_pickups(&s,player(90,65),bad,c,2)==-1);
  assert(s.fragment_count==0&&s.coin_count==0);
  assert(rikitu_stage_pickups(&s,player(90,65),f,c,91)==-1);
  assert(rikitu_stage_pickups(&s,player(90,65),f,NULL,1)==-1);
  assert(rikitu_stage_pickups(&s,player(90,65),NULL,c,1)==-1);
  assert(rikitu_stage_pickups(&s,player(NAN,57),f,c,1)==-1);
  assert(!s.fragment_count&&!s.coin_count);
  assert(rikitu_stage_touch_gate(NULL,gate,900)==RIKITU_GATE_INVALID);
  assert(rikitu_stage_touch_gate(&s,gate,NAN)==RIKITU_GATE_INVALID);
  /* Every in-game coin must remain individually collectible, once. */
  for(i=0;i<90;i++){c[i].x=400.0f+35.0f*i;c[i].y=100.0f;}
  for(i=0;i<90;i++)
    assert(rikitu_stage_pickups(&s,player(c[i].x-10.0f,65),f,c,90)==1);
  assert(s.coin_count==90&&s.fragment_count==0);
  assert(rikitu_stage_pickups(&s,player(c[0].x-10.0f,65),f,c,90)==0);
  for(i=1;i<=16;i++){
    assert(rikitu_stage_next_mode(i)==
      ((i%2==0)?RIKITU_NEXT_SHOP:RIKITU_NEXT_BETWEEN));
    assert(rikitu_stage_after_shop(i)==
      ((i%2)?0:((i==16)?RIKITU_NEXT_VICTORY:RIKITU_NEXT_BETWEEN)));
  }
  assert(rikitu_stage_next_mode(0)==0);
  assert(rikitu_stage_next_mode(17)==0);
  assert(rikitu_stage_after_shop(17)==0);
  puts("PASS C99 stage objectives: pickup geometry, 90 unique coins, boss gate, Moki transitions");
  return 0;
}
