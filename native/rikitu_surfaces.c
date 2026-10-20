#include "rikitu_surfaces.h"
#include <math.h>
static float bounded(float v, float lo, float hi) {
  return v<lo ? lo : (v>hi ? hi : v);
}
static int hit_launcher(const RikituPlayer *p, const RikituLauncher *l) {
  float left, right;
  switch(l->kind) {
    case RIKITU_LAUNCH_STEAM: left=l->x-18.0f;right=l->x+52.0f;break;
    case RIKITU_LAUNCH_CLOCK: left=l->x-16.0f;right=l->x+54.0f;break;
    case RIKITU_LAUNCH_GUST: left=l->x-24.0f;right=l->x+66.0f;break;
    case RIKITU_LAUNCH_CRYSTAL: left=l->x-20.0f;right=l->x+60.0f;break;
    case RIKITU_LAUNCH_MUSHROOM:
      left=l->x-20.0f;right=l->x+64.0f;
      if(fabsf(p->y+p->h-l->y)>=7.0f) return 0;
      break;
    default:return 0;
  }
  return p->x+p->w>left && p->x<right;
}
int rikitu_apply_surfaces(RikituPlayer *p, float dt, float world_width,
                          const RikituConveyor *belts, size_t belts_len,
                          const RikituLauncher *launchers,size_t launchers_len) {
  size_t i;
  int kind;
  if(!p || !isfinite(dt) || dt<0.0f || !isfinite(world_width) ||
     world_width<p->w || (belts_len&&!belts) || (launchers_len&&!launchers))
    return -1;
  if(!p->grounded) return RIKITU_LAUNCH_NONE;
  /* The browser checks only horizontal belt overlap, in level array order. */
  for(i=0;i<belts_len;i++) {
    const RikituConveyor *b=&belts[i];
    if(!isfinite(b->x) || !isfinite(b->width) || !isfinite(b->speed))
      return -1;
    if(p->x+p->w>b->x && p->x<b->x+b->width) {
      p->x=bounded(p->x+b->speed*dt,0.0f,world_width-p->w);
      break;
    }
  }
  /* The original script checks each launcher family in a separate pass.
   * Preserve family priority even if input array order is interleaved. */
  for(kind=RIKITU_LAUNCH_STEAM;kind<=RIKITU_LAUNCH_MUSHROOM;kind++) {
    for(i=0;i<launchers_len;i++) {
      const RikituLauncher *l=&launchers[i];
      if(l->kind<RIKITU_LAUNCH_STEAM ||
         l->kind>RIKITU_LAUNCH_MUSHROOM ||
         !isfinite(l->x) || !isfinite(l->y)) return -1;
      if((int)l->kind!=kind || !hit_launcher(p,l)) continue;
      switch(kind) {
        case RIKITU_LAUNCH_STEAM:p->vy=-705.0f;break;
        case RIKITU_LAUNCH_CLOCK:p->vy=-675.0f;break;
        case RIKITU_LAUNCH_GUST:
        case RIKITU_LAUNCH_CRYSTAL:p->vy=-690.0f;break;
        case RIKITU_LAUNCH_MUSHROOM:p->vy=-730.0f;break;
        default:break;
      }
      p->grounded=0;
      return kind;
    }
  }
  return RIKITU_LAUNCH_NONE;
}
