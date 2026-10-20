/* ROM-free, geometry-only graphical smoke scene for Nintendo Switch.
 * Uses the official switchbrew framebuffer API; no character art or game ROMs.
 */
#include <stdint.h>
#include <string.h>
#include <switch.h>
#include "rikitu_demo.h"

static unsigned input_mask(u64 held) {
  unsigned mask=0;
  if(held & (HidNpadButton_Left|HidNpadButton_StickLLeft))
    mask|=RIKITU_INPUT_LEFT;
  if(held & (HidNpadButton_Right|HidNpadButton_StickLRight))
    mask|=RIKITU_INPUT_RIGHT;
  if(held & (HidNpadButton_A|HidNpadButton_B))
    mask|=RIKITU_INPUT_JUMP;
  if(held & (HidNpadButton_R|HidNpadButton_ZR))
    mask|=RIKITU_INPUT_RUN;
  return mask;
}
int main(void) {
  PadState pad;
  Framebuffer fb;
  RikituScene scene;
  RikituLoop game;
  int error=0;
  rikitu_demo_init(&scene);
  memset(&game,0,sizeof(game));
  game.player=scene.player;
  framebufferCreate(&fb,nwindowGetDefault(),RIKITU_SCENE_WIDTH,
                    RIKITU_SCENE_HEIGHT,PIXEL_FORMAT_RGBA_8888,2);
  framebufferMakeLinear(&fb);
  padConfigureInput(1,HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&pad);
  while(appletMainLoop()) {
    int ticks;
    u64 held,pressed;
    u32 stride=0;
    uint32_t *frame;
    padUpdate(&pad);
    held=padGetButtons(&pad);
    pressed=padGetButtonsDown(&pad);
    if(pressed & HidNpadButton_Plus)break;
    if(pressed & HidNpadButton_X){
      rikitu_demo_init(&scene);
      memset(&game,0,sizeof(game));
      game.player=scene.player;
    }
    if(pressed & HidNpadButton_Minus)game.paused=!game.paused;
    scene.paused=game.paused;
    /* A fixed nominal 60-Hz step: display-time calibration remains pending. */
    ticks=rikitu_loop_advance(&game,RIKITU_FIXED_DT,input_mask(held),
             scene.world_width,scene.solids,RIKITU_DEMO_SOLIDS);
    if(ticks<0){error=1;break;}
    if(ticks>0&&!game.paused&&!scene.stage.completed) {
      /* This diagnostic normally executes one simulation tick per frame.
       * Proper per-tick integration of surfaces will follow later. */
      if(rikitu_apply_surfaces(&game.player,RIKITU_FIXED_DT,
            scene.world_width,scene.belts,RIKITU_DEMO_BELTS,
            scene.launchers,RIKITU_DEMO_LAUNCHERS)<0) {
        error=2;break;
      }
      scene.player=game.player;
      if(rikitu_stage_pickups(&scene.stage,(RikituRect){
             game.player.x,game.player.y,game.player.w,game.player.h},
             scene.fragments,scene.coins,RIKITU_DEMO_COINS)<0) {
        error=3;break;
      }
      (void)rikitu_stage_touch_gate(&scene.stage,(RikituRect){
              game.player.x,game.player.y,game.player.w,game.player.h},
              scene.gate_x);
    } else {
      scene.player=game.player;
    }
    frame=(uint32_t*)framebufferBegin(&fb,&stride);
    if(!frame || stride%sizeof(uint32_t)!=0 ||
       !rikitu_draw_scene(frame,RIKITU_SCENE_WIDTH,RIKITU_SCENE_HEIGHT,
                          (int)(stride/sizeof(uint32_t)),&scene)) {
      error=4;break;
    }
    framebufferEnd(&fb);
    svcSleepThread(16666667LL);
  }
  framebufferClose(&fb);
  return error;
}
