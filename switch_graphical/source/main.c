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
#include "rikitu_grove.h"
#include "rikitu_bridge.h"
#include "rikitu_workshop.h"
#include "rikitu_delivery.h"
#include "rikitu_clock.h"
#include "rikitu_orders.h"
#include "rikitu_mist.h"
#include "rikitu_cloud.h"
#include "rikitu_crystal.h"
#include "rikitu_mine.h"
#include "rikitu_gardens.h"
#include "rikitu_foundry.h"
#include "rikitu_gallery.h"
#include "rikitu_heart.h"
#include "rikitu_save.h"
#include "rikitu_campaign.h"
#include "rikitu_campaign_save.h"
#include "rikitu_level_save.h"
#include "rikitu_map01_telemetry.h"

#define MAP01_A "sdmc:/switch/RIKITU/map01-a.rks"
#define MAP01_B "sdmc:/switch/RIKITU/map01-b.rks"
#define CAMPAIGN_A "sdmc:/switch/RIKITU/campaign-a.rkc"
#define CAMPAIGN_B "sdmc:/switch/RIKITU/campaign-b.rkc"
#define LEVEL_A "sdmc:/switch/RIKITU/level-a.rkl"
#define LEVEL_B "sdmc:/switch/RIKITU/level-b.rkl"

/* Keep debug map selection and campaign progression on the same table. */
static void (*const stage_init[16])(RikituScene *)={
  rikitu_flower_init,rikitu_mill_init,rikitu_grove_init,rikitu_bridge_init,
  rikitu_workshop_init,rikitu_delivery_init,rikitu_clock_init,rikitu_orders_init,
  rikitu_mist_init,rikitu_cloud_init,rikitu_crystal_init,rikitu_mine_init,
  rikitu_gardens_init,rikitu_foundry_init,rikitu_gallery_init,rikitu_heart_init
};
static void begin_stage(RikituScene *scene,RikituLoop *game,
                        const RikituCampaign *campaign){
  stage_init[campaign->level-1](scene);
  memset(game,0,sizeof(*game));
  game->player=scene->player;
  scene->max_hearts=campaign->max_hearts;
  scene->hearts=scene->max_hearts;
}

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
  RikituCampaign campaign;
  RikituMap01Telemetry map01_metrics;
  int map01_metrics_active=0;
  int campaign_debug=0;
  u64 last_tick;
  int error=0;
  int sd_mounted=0,save_ready=0,have_last=0;
  float save_elapsed=0.0f;
  uint8_t last_saved[28]={0};
  uint8_t now[RIKITU_SAVE_BYTES];
  uint8_t level_now[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t level_last[RIKITU_LEVEL_SAVE_BYTES]={0};
  int level_have_last=0;
  float level_save_elapsed=0.0f;
  (void)rikitu_campaign_init(&campaign,1);
  begin_stage(&scene,&game,&campaign);
  map01_metrics_active=0;
  /* libnx's default __appInit mounts sdmc before entering main().
   * Calling fsdevMountSdmc twice can return libnx 0x559 because the
   * devoptab already has a device called "sdmc". Reuse that valid
   * registration, and only unmount if this process created it here. */
  {
    const int already_mounted=fsdevGetDeviceFileSystem("sdmc")!=NULL;
    if(already_mounted || R_SUCCEEDED(fsdevMountSdmc())){
      if(!already_mounted)sd_mounted=1;
      (void)mkdir("sdmc:/switch",0777);
      if(mkdir("sdmc:/switch/RIKITU",0777)==0 || errno==EEXIST)
        save_ready=1;
    }
  }
  if(save_ready){
    /* RKC2 is a different record family and does not decode old MAP01 RKS1.
     * The current chapter restarts at its beginning after loading campaign
     * metadata; only MAP01 retains its legacy intra-level checkpoint. */
    if(rikitu_campaign_save_load_pair(CAMPAIGN_A,CAMPAIGN_B,&campaign))
      begin_stage(&scene,&game,&campaign);
      map01_metrics_active=0;
    if(rikitu_campaign_uses_legacy_map01_checkpoint(&campaign) &&
       campaign.mode==RIKITU_CAMPAIGN_PLAYING){
      (void)rikitu_save_load_pair(MAP01_A,MAP01_B,&scene,&game);
      if(rikitu_save_encode(now,&scene,&game,1)){
        memcpy(last_saved,now,sizeof(last_saved));have_last=1;
      }
    }else if(campaign.level>1 &&
             campaign.mode==RIKITU_CAMPAIGN_PLAYING){
      if(rikitu_level_save_load_pair(LEVEL_A,LEVEL_B,
                                     &scene,&game,&campaign) &&
         rikitu_level_save_encode(level_last,&scene,&game,&campaign,1))
        level_have_last=1;
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
      if(save_ready && !campaign_debug)
        (void)rikitu_campaign_save_store_pair(CAMPAIGN_A,CAMPAIGN_B,&campaign);
      if(save_ready && !campaign_debug &&
         campaign.mode==RIKITU_CAMPAIGN_PLAYING &&
         campaign.level>1 && !scene.stage.completed)
        (void)rikitu_level_save_store_pair(LEVEL_A,LEVEL_B,
                                           &scene,&game,&campaign);
      if(save_ready && !campaign_debug &&
         rikitu_campaign_uses_legacy_map01_checkpoint(&campaign) &&
         scene.level_id==1 &&
         rikitu_save_encode(now,&scene,&game,1) &&
         (!have_last || memcmp(now,last_saved,sizeof(last_saved))!=0))
        (void)rikitu_save_store_pair(MAP01_A,MAP01_B,&scene,&game);
      break;
    }
    /* Y/X start a disposable diagnostic run. They NEVER write, delete,
     * or implicitly restore the user's permanent RKS1 or RKC2 files.
     * Closing the debug session preserves the real campaign unchanged. */
    if(pressed & HidNpadButton_Y){
      campaign_debug=1;
      const int selected=scene.level_id==16?1:scene.level_id+1;
      (void)rikitu_campaign_init(&campaign,selected);
      begin_stage(&scene,&game,&campaign);
      map01_metrics_active=0;
      save_elapsed=0.0f;have_last=0;
      level_save_elapsed=0.0f;level_have_last=0;
    }else if(pressed & HidNpadButton_X){
      campaign_debug=1;
      const int selected=scene.level_id;
      (void)rikitu_campaign_init(&campaign,selected);
      begin_stage(&scene,&game,&campaign);
      map01_metrics_active=0;
      save_elapsed=0.0f;have_last=0;
      level_save_elapsed=0.0f;level_have_last=0;
    }else if(campaign.mode==RIKITU_CAMPAIGN_SHOP){
      if(pressed & HidNpadButton_A){
        const RikituHeartPurchase bought=rikitu_campaign_buy(&campaign);
        if(bought.success && save_ready && !campaign_debug)
          (void)rikitu_campaign_save_store_pair(CAMPAIGN_A,CAMPAIGN_B,&campaign);
        /* The upgrade remains available in the next stage. */
      }else if(pressed & HidNpadButton_B){
        const int transition=rikitu_campaign_continue(&campaign);
        if(transition==1){
          begin_stage(&scene,&game,&campaign);
          map01_metrics_active=0;
          save_elapsed=0.0f;have_last=0;
        level_save_elapsed=0.0f;level_have_last=0;
        }
        if(transition && save_ready && !campaign_debug)
          (void)rikitu_campaign_save_store_pair(CAMPAIGN_A,CAMPAIGN_B,&campaign);
      }
    }else if(campaign.mode==RIKITU_CAMPAIGN_BETWEEN &&
             (pressed & HidNpadButton_A)){
      if(rikitu_campaign_continue(&campaign)==1){
        begin_stage(&scene,&game,&campaign);
        map01_metrics_active=0;
        save_elapsed=0.0f;have_last=0;
        level_save_elapsed=0.0f;level_have_last=0;
        if(save_ready && !campaign_debug)
          (void)rikitu_campaign_save_store_pair(CAMPAIGN_A,CAMPAIGN_B,&campaign);
      }
    }
    if(campaign.mode==RIKITU_CAMPAIGN_PLAYING){
      if(pressed & HidNpadButton_Minus)game.paused=!game.paused;
      /* Observational diagnostics only. Never interrupt gameplay or saves. */
      if(scene.level_id==1 && !map01_metrics_active)
        map01_metrics_active=rikitu_map01_telemetry_start(
                               &map01_metrics,&scene,&game);
      else if(scene.level_id!=1)map01_metrics_active=0;
      {
        const int simulated_ticks=rikitu_demo_advance(
                                     &scene,&game,wall_seconds,input_mask(held));
        if(simulated_ticks<0){error=1;break;}
        if(map01_metrics_active &&
           !rikitu_map01_telemetry_observe(
                    &map01_metrics,&scene,&game,simulated_ticks))
          map01_metrics_active=0;
      }
      if(scene.stage.completed){
        if(!rikitu_campaign_finish(&campaign,&scene.stage)){
          error=3;break;
        }
        if(save_ready && !campaign_debug)
          (void)rikitu_campaign_save_store_pair(CAMPAIGN_A,CAMPAIGN_B,&campaign);
        if(scene.level_id==1 &&
           rikitu_campaign_uses_legacy_map01_checkpoint(&campaign) &&
           save_ready && !campaign_debug)
          (void)rikitu_save_store_pair(MAP01_A,MAP01_B,&scene,&game);
      }
    }
    frame=(uint32_t*)framebufferBegin(&fb,&stride);
    if(!frame || stride%sizeof(uint32_t)!=0 ||
       !rikitu_draw_scene(frame,RIKITU_SCENE_WIDTH,RIKITU_SCENE_HEIGHT,
                          (int)(stride/sizeof(uint32_t)),&scene)){
      error=2;break;
    }
    if(campaign.mode!=RIKITU_CAMPAIGN_PLAYING){
      const RikituHeartOffer offer=rikitu_campaign_offer(&campaign);
      if(!rikitu_draw_campaign_overlay(frame,RIKITU_SCENE_WIDTH,
               RIKITU_SCENE_HEIGHT,(int)(stride/sizeof(uint32_t)),
               &campaign,&offer)){
        error=4;break;
      }
    }
    framebufferEnd(&fb);
    if(campaign.mode==RIKITU_CAMPAIGN_PLAYING &&
       rikitu_campaign_uses_legacy_map01_checkpoint(&campaign) &&
       scene.level_id==1 &&
       save_ready && !campaign_debug &&
       rikitu_save_encode(now,&scene,&game,1)){
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
    /* RKL3 stores only *changed* MAP02-16 progress. Ignore time and
     * generation bytes when determining dirty state; coin/checkpoint/enemy/
     * boss/lever/health changes are represented by bytes 0..27,40..41.
     * Crash recovery uses alternating .rkl slots, never RKS1 or RKC2. */
    if(campaign.mode==RIKITU_CAMPAIGN_PLAYING &&
       campaign.level>1 && !campaign_debug && save_ready &&
       !scene.stage.completed &&
       rikitu_level_save_encode(level_now,&scene,&game,&campaign,1)){
      const int dirty=!level_have_last ||
          memcmp(level_now,level_last,28)!=0 ||
          level_now[40]!=level_last[40] ||
          level_now[41]!=level_last[41];
      level_save_elapsed+=wall_seconds;
      if(dirty && level_save_elapsed>=2.0f){
        if(rikitu_level_save_store_pair(LEVEL_A,LEVEL_B,
                                        &scene,&game,&campaign)){
          memcpy(level_last,level_now,sizeof(level_last));
          level_have_last=1;
        }
        level_save_elapsed=0.0f;
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
