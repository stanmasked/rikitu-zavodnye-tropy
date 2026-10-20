/* ROM-free, geometry-only graphical smoke scene for Nintendo Switch.
 * Uses the official switchbrew framebuffer API; no character art or game ROMs.
 */
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <switch.h>
#include "rikitu_flower.h"
#include "rikitu_mill.h"
#include "rikitu_save.h"

#define MAP01_A "sdmc:/switch/RIKITU/map01-a.rks"
#define MAP01_B "sdmc:/switch/RIKITU/map01-b.rks"

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
  int sd_mounted=0,save_ready=0,have_last=0;
  float save_elapsed=0.0f;
  uint8_t last_saved[28]={0};
  uint8_t now[RIKITU_SAVE_BYTES];
  rikitu_flower_init(&scene);
  memset(&game,0,sizeof(game));
  game.player=scene.player;
  if(R_SUCCEEDED(fsdevMountSdmc())){
    sd_mounted=1;
    (void)mkdir("sdmc:/switch",0777);
    if(mkdir("sdmc:/switch/RIKITU",0777)==0 || errno==EEXIST)
      save_ready=1;
  }
  if(save_ready){
    (void)rikitu_save_load_pair(MAP01_A,MAP01_B,&scene,&game);
    if(rikitu_save_encode(now,&scene,&game,1)){
      memcpy(last_saved,now,sizeof(last_saved));have_last=1;
    }
  }
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
    if(pressed & HidNpadButton_Plus){
      if(save_ready && rikitu_save_encode(now,&scene,&game,1) &&
         (!have_last || memcmp(now,last_saved,sizeof(last_saved))!=0))
        (void)rikitu_save_store_pair(MAP01_A,MAP01_B,&scene,&game);
      break;
    }
    /* Y switches between source-parity MAP01 and experimental MAP02.
     * Only MAP01 is persisted. MAP02 never touches MAP01 save files. */
    if(pressed & HidNpadButton_Y){
      if(scene.level_id==1){
        if(save_ready && rikitu_save_encode(now,&scene,&game,1))
          (void)rikitu_save_store_pair(MAP01_A,MAP01_B,&scene,&game);
        rikitu_mill_init(&scene);
      }else{
        rikitu_flower_init(&scene);
        if(save_ready)
          (void)rikitu_save_load_pair(MAP01_A,MAP01_B,&scene,&game);
      }
      memset(&game,0,sizeof(game));
      game.player=scene.player;
      save_elapsed=0.0f;
      have_last=0;
    }
    if(pressed & HidNpadButton_X){
      if(scene.level_id==2)rikitu_mill_init(&scene);
      else rikitu_flower_init(&scene);
      memset(&game,0,sizeof(game));
      game.player=scene.player;
      have_last=0;save_elapsed=0.0f;
      if(save_ready && scene.level_id==1){
        (void)remove(MAP01_A);(void)remove(MAP01_B);
      }
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
    if(save_ready && rikitu_save_encode(now,&scene,&game,1)){
      /* Ignore elapsed time for change detection: save at progress events,
       * not on each 60-Hz frame. Time is recorded in the chosen snapshot. */
      const int dirty=!have_last ||
        memcmp(now,last_saved,sizeof(last_saved))!=0;
      save_elapsed+=wall_seconds;
      if(dirty && save_elapsed>=2.0f){
        if(rikitu_save_store_pair(MAP01_A,MAP01_B,&scene,&game)){
          memcpy(last_saved,now,sizeof(last_saved));
          have_last=1;
        }
        save_elapsed=0.0f;
      }
    }
    /* A short yielding sleep reduces a busy-loop, but it does not define
     * simulation speed: measured clock deltas do. */
    svcSleepThread(1000000LL);
  }
  framebufferClose(&fb);
  if(sd_mounted)fsdevUnmountDevice("sdmc");
  return error;
}
