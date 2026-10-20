/* QA-only libnx app: verify actual portable MAP01 battle code in NeXium.
 * Green=PASS, red=FAIL; four pixel bars read from real emulated screen.
 * This is a separate NRO and cannot change the ordinary game build.
 */
#include <stdint.h>
#include <switch.h>
#include "rikitu_flower.h"
#include "rikitu_combat_guest_qa.h"
#include "rikitu_mill_guest_qa.h"

static void overlay_result(uint32_t *frame,int stride_pixels,unsigned bits,int top) {
  int i,y,x;
  for(i=0;i<4;i++) {
    const uint32_t color=(bits&(1u<<i))?0xff22df22u:0xff3030dfu;
    const int left=280+i*200; /* avoid NeXium's x<240 diagnostic HUD */
    for(y=top;y<top+56;y++)
      for(x=left;x<left+64;x++)
        frame[(size_t)y*(size_t)stride_pixels+(size_t)x]=color;
  }
}
int main(void) {
  Framebuffer fb;
  PadState pad;
  RikituScene scene;
  const unsigned bits=rikitu_guest_combat_checks();
  const unsigned mill_bits=rikitu_guest_mill_checks();
  int error=0;
  rikitu_flower_init(&scene);
  framebufferCreate(&fb,nwindowGetDefault(),RIKITU_SCENE_WIDTH,
                    RIKITU_SCENE_HEIGHT,PIXEL_FORMAT_RGBA_8888,2);
  framebufferMakeLinear(&fb);
  padConfigureInput(1,HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&pad);
  while(appletMainLoop()) {
    u32 stride=0;
    uint32_t *frame;
    padUpdate(&pad);
    if(padGetButtonsDown(&pad)&HidNpadButton_Plus)break;
    frame=(uint32_t*)framebufferBegin(&fb,&stride);
    if(!frame || stride%4u!=0 ||
       !rikitu_draw_scene(frame,RIKITU_SCENE_WIDTH,RIKITU_SCENE_HEIGHT,
                          (int)(stride/4u),&scene)) {
      error=2;
      break;
    }
    overlay_result(frame,(int)(stride/4u),bits,118);
    overlay_result(frame,(int)(stride/4u),mill_bits,238);
    framebufferEnd(&fb);
    svcSleepThread(16000000LL);
  }
  framebufferClose(&fb);
  return error?error:(bits==RIKITU_QA_ALL &&
                       mill_bits==RIKITU_MILL_QA_ALL?0:1);
}
