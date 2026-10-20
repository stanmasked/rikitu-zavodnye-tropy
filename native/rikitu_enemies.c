#include "rikitu_enemies.h"
#include <math.h>
static float bounded(float v,float a,float b) {
  return v<a?a:(v>b?b:v);
}
RikituEnemyEvent rikitu_enemy_tick(RikituEnemy *e,RikituPlayer *p,
                                   float old_bottom,float dt) {
  if(!e||!p||!isfinite(dt)||dt<=0.0f||!isfinite(old_bottom)||
     !isfinite(e->x)||!isfinite(e->y)||!isfinite(e->w)||!isfinite(e->h)||
     !isfinite(e->a)||!isfinite(e->b)||e->a>e->b||
     e->w<=0.0f||e->h<=0.0f||(e->dir!=-1&&e->dir!=1)||
     !isfinite(p->x)||!isfinite(p->y)||!isfinite(p->w)||
     !isfinite(p->h)||!isfinite(p->vy)||p->w<=0||p->h<=0)
    return RIKITU_ENEMY_INVALID;
  if(e->dead)return RIKITU_ENEMY_NONE;
  e->x+=e->dir*43.0f*dt;
  if(e->x<e->a||e->x>e->b) {
    e->x=bounded(e->x,e->a,e->b);
    e->dir=-e->dir;
  }
  {
    const RikituRect pr={p->x,p->y,p->w,p->h};
    const RikituRect er={e->x,e->y,e->w,e->h};
    if(!rikitu_box_hit(pr,er))return RIKITU_ENEMY_NONE;
  }
  if(rikitu_stomp_from_above(old_bottom,p->y+p->h,e->y,p->vy>=0.0f)) {
    e->dead=1;
    p->vy=-360.0f;
    return RIKITU_ENEMY_STOMP;
  }
  return RIKITU_ENEMY_CONTACT;
}
