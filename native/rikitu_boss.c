#include "rikitu_boss.h"
#include <math.h>
RikituBossEvent rikitu_boss_tick(RikituBoss *b,RikituPlayer *p,
                                  float old_bottom,float dt) {
  if(!b||!p||!isfinite(dt)||dt<=0.0f||!isfinite(old_bottom)||
     !isfinite(b->x)||!isfinite(b->y)||!isfinite(b->w)||!isfinite(b->h)||
     !isfinite(b->center)||!isfinite(b->hit_seconds)||
     b->w<=0.0f||b->h<=0.0f||b->hit_seconds<0.0f||
     (b->dir!=-1&&b->dir!=1)||b->hp<0||b->hp>3||
     (b->alive!=0&&b->alive!=1)||
     (b->alive&&b->hp==0)||(!b->alive&&b->hp!=0)||
     !isfinite(p->x)||!isfinite(p->y)||!isfinite(p->w)||
     !isfinite(p->h)||!isfinite(p->vy)||p->w<=0.0f||p->h<=0.0f)
    return RIKITU_BOSS_INVALID;
  if(!b->alive)return RIKITU_BOSS_NONE;
  b->hit_seconds=fmaxf(0.0f,b->hit_seconds-dt);
  b->x+=b->dir*48.0f*dt;
  if(b->x<b->center-80.0f||b->x>b->center+80.0f)b->dir=-b->dir;
  {
    const RikituRect pr={p->x,p->y,p->w,p->h};
    const RikituRect br={b->x,b->y,b->w,b->h};
    if(!rikitu_box_hit(pr,br))return RIKITU_BOSS_NONE;
  }
  if(b->hit_seconds>0.0f)return RIKITU_BOSS_NONE;
  if(rikitu_stomp_from_above(old_bottom,p->y+p->h,b->y,p->vy>=0.0f)){
    b->hp--;
    b->hit_seconds=0.85f;
    p->vy=-490.0f;
    p->y=b->y-p->h-1.0f;
    if(b->hp==0){
      b->alive=0;
      return RIKITU_BOSS_DEFEATED;
    }
    return RIKITU_BOSS_HURT;
  }
  return RIKITU_BOSS_CONTACT;
}
