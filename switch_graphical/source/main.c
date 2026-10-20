/* ROM-free, geometry-only graphical smoke scene for Nintendo Switch.
 * Uses the official switchbrew framebuffer API; no character art or game ROMs.
 */
#include <stdint.h>
#include <string.h>
#include <switch.h>
#include "rikitu_flower.h"

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
  u64 last_tick;
  int error=0;
  rikitu_flower_init(&scene);
  memset(&game,0,sizeof(game));
  game.player=scene.player;
  framebufferCreate(&fb,nwindowGetDefault(),RIKITU_SCENE_WIDTH,
                    RIKITU_SCENE_HEIGHT,PIXEL_FORMAT_RGBA_8888,2);
  framebufferMakeLinear(&fb);
  padConfigureInput(1,HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&pad);
  last_tick=armGetSystemTick();
  while(appletMainLoop()) {
    const u64 tick=armGetSystemTick();
    const float wall_seconds=(float)((double)armTicksToNs(tick-last_tick)*1e-9);
    u64 held,pressed;
    u32 stride=0;
    uint32_t *frame;
    last_tick=tick;
    padUpdate(&pad);
    held=padGetButtons(&pad);
    pressed=padGetButtonsDown(&pad);
    if(pressed & HidNpadButton_Plus)break;
    if(pressed & HidNpadButton_X){
      rikitu_flower_init(&scene);
      memset(&game,0,sizeof(game));
      game.player=scene.player;
    }
    if(pressed & HidNpadButton_Minus)game.paused=!game.paused;
    /* The accumulator caps catch-up at 100ms/six ticks, pauses do not
     * accumulate time, and conveyors/pickups now run on EVERY tick. */
    if(rikitu_demo_advance(&scene,&game,wall_seconds,input_mask(held))<0){
      error=1;break;
    }
    frame=(uint32_t*)framebufferBegin(&fb,&stride);
    if(!frame || stride%sizeof(uint32_t)!=0 ||
       !rikitu_draw_scene(frame,RIKITU_SCENE_WIDTH,RIKITU_SCENE_HEIGHT,
                          (int)(stride/sizeof(uint32_t)),&scene)){
      error=2;break;
    }
    framebufferEnd(&fb);
    /* A short yielding sleep reduces a busy-loop, but it does not define
     * simulation speed: measured clock deltas do. */
    svcSleepThread(1000000LL);
  }
  framebufferClose(&fb);
  return error;
}
