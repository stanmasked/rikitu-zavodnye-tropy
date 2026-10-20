#include "rikitu_loop.h"
#include <math.h>
static float clampf(float value, float low, float high) {
  return value < low ? low : (value > high ? high : value);
}
static float decrementf(float value, float delta) {
  return value > delta ? value - delta : 0.0f;
}
void rikitu_player_init(RikituPlayer *p, float x, float y) {
  if (!p) return;
  p->x=x; p->y=y; p->vx=0.0f; p->vy=0.0f;
  p->w=33.0f; p->h=43.0f;
  p->coyote_seconds=0.0f; p->jump_buffer_seconds=0.0f;
  p->grounded=0; p->previous_jump=0; p->facing=1;
}
int rikitu_player_step(RikituPlayer *p, unsigned input, float dt,
                       float world_width, const RikituRect *solids,
                       size_t solid_count) {
  int left, right, jump;
  size_t i;
  float old_bottom;
  if (!p || !isfinite(dt) || dt<=0.0f || !isfinite(world_width) ||
      world_width < p->w || (solid_count && !solids)) return 0;
  left=(input & RIKITU_INPUT_LEFT)!=0;
  right=(input & RIKITU_INPUT_RIGHT)!=0;
  jump=(input & RIKITU_INPUT_JUMP)!=0;
  p->vx=(float)(right-left)*((input & RIKITU_INPUT_RUN) ? 265.0f : 225.0f);
  if (p->vx) p->facing=p->vx>0.0f ? 1 : -1;
  p->jump_buffer_seconds=(jump && !p->previous_jump) ? 0.12f :
                         decrementf(p->jump_buffer_seconds,dt);
  p->previous_jump=jump;
  p->coyote_seconds=p->grounded ? 0.115f :
                    decrementf(p->coyote_seconds,dt);
  if (p->jump_buffer_seconds>0.0f &&
      rikitu_can_jump(p->grounded,p->coyote_seconds)) {
    p->vy=-625.0f; p->grounded=0;
    p->coyote_seconds=0.0f; p->jump_buffer_seconds=0.0f;
  }
  old_bottom=p->y+p->h;
  p->x=clampf(p->x+p->vx*dt,0.0f,world_width-p->w);
  p->vy=p->vy+1580.0f*dt;
  if (p->vy>1050.0f) p->vy=1050.0f;
  p->y+=p->vy*dt;
  p->grounded=0;
  for (i=0;i<solid_count;i++) {
    const RikituRect *b=&solids[i];
    if (p->x+p->w<=b->x+3.0f || p->x>=b->x+b->w-3.0f) continue;
    if (p->vy>=0.0f && old_bottom<=b->y+10.0f &&
        p->y+p->h>=b->y && p->y<=b->y) {
      p->y=b->y-p->h; p->vy=0.0f; p->grounded=1;
    }
  }
  return 1;
}
int rikitu_loop_advance(RikituLoop *loop, float frame_seconds,
                        unsigned input, float world_width,
                        const RikituRect *solids, size_t solid_count) {
  int ticks=0;
  if (!loop || !isfinite(frame_seconds) || frame_seconds<0.0f ||
      !isfinite(world_width) || world_width<loop->player.w ||
      (solid_count && !solids)) return -1;
  if (loop->paused) return 0;
  if (frame_seconds>0.10f) frame_seconds=0.10f;
  loop->accumulator_seconds+=frame_seconds;
  while (loop->accumulator_seconds>=RIKITU_FIXED_DT && ticks<6) {
    if (!rikitu_player_step(&loop->player,input,RIKITU_FIXED_DT,
                            world_width,solids,solid_count)) return -1;
    loop->accumulator_seconds-=RIKITU_FIXED_DT;
    loop->active_seconds+=RIKITU_FIXED_DT;
    ticks++;
  }
  return ticks;
}
