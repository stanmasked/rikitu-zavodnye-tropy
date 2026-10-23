/* Stage96: genuine NeXium guest RKC2+RKL3 persistence with three
 * independent NRO launches sharing the same SDMC filesystem.
 * Isolated QA executable: no change to the shipping 16-map game.
 * Test fixtures are controlled (not a controller-only playthrough).
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <switch.h>
#include "rikitu_bridge.h"
#include "rikitu_campaign_save.h"
#include "rikitu_level_save.h"
#define CAMPAIGN_A "sdmc:/switch/RIKITU/campaign-a.rkc"
#define CAMPAIGN_B "sdmc:/switch/RIKITU/campaign-b.rkc"
#define LEVEL_A "sdmc:/switch/RIKITU/level-a.rkl"
#define LEVEL_B "sdmc:/switch/RIKITU/level-b.rkl"
static void fresh(RikituScene *s,RikituLoop *l,const RikituCampaign *c){
  rikitu_bridge_init(s);
  memset(l,0,sizeof(*l));l->player=s->player;
  s->max_hearts=c->max_hearts;s->hearts=s->max_hearts;
}
static void seed(RikituScene *s,RikituLoop *l){
  s->active_checkpoint=s->checkpoints[0];
  s->reached_checkpoints=1;
  s->hearts=3;
  s->stage.fragments[0]=1;s->stage.fragment_count=1;
  s->stage.coins[0]=1;s->stage.coin_count=1;
  s->enemies[0].dead=1;
  l->active_seconds=25.0f;
}
static void upgrade(RikituScene *s,RikituLoop *l){
  s->active_checkpoint=s->checkpoints[1];
  s->reached_checkpoints=2;s->hearts=2;
  s->stage.fragments[2]=1;s->stage.fragment_count=2;
  s->stage.coins[89]=1;s->stage.coin_count=2;
  s->enemies[5].dead=1;
  s->boss.hp=2;s->boss.alive=1;
  l->active_seconds=55.0f;
}
static int match_first(const RikituScene *s,const RikituLoop *l,
                       const RikituCampaign *c){
  return c->level==4&&c->mode==RIKITU_CAMPAIGN_PLAYING&&
    c->wallet==157&&c->purchased_hearts==1&&c->max_hearts==4&&
    s->level_id==4&&s->reached_checkpoints==1&&
    s->active_checkpoint==s->checkpoints[0]&&s->hearts==3&&
    s->stage.fragment_count==1&&s->stage.fragments[0]&&
    s->stage.coin_count==1&&s->stage.coins[0]&&
    s->enemies[0].dead&&s->boss.hp==3&&
    l->active_seconds>24.9f&&l->active_seconds<25.1f;
}
static int match_second(const RikituScene *s,const RikituLoop *l,
                        const RikituCampaign *c){
  return c->level==4&&c->mode==RIKITU_CAMPAIGN_PLAYING&&
    c->wallet==157&&c->purchased_hearts==1&&c->max_hearts==4&&
    s->level_id==4&&s->reached_checkpoints==2&&
    s->active_checkpoint==s->checkpoints[1]&&s->hearts==2&&
    s->stage.fragment_count==2&&s->stage.fragments[0]&&
    s->stage.fragments[2]&&s->stage.coin_count==2&&
    s->stage.coins[0]&&s->stage.coins[89]&&
    s->enemies[0].dead&&s->enemies[5].dead&&
    s->boss.hp==2&&s->boss.alive&&
    l->active_seconds>54.9f&&l->active_seconds<55.1f;
}
static void marker(uint32_t *pixels,int stride,int phase,unsigned mask){
  int i,x,y;
  for(i=0;i<4;i++){
    const uint32_t color=(mask&(1u<<i))?0xff22df22u:0xff3030dfu;
    for(y=118;y<174;y++)for(x=280+i*200;x<344+i*200;x++)
      pixels[(size_t)y*(size_t)stride+(size_t)x]=color;
  }
  {
    const uint32_t color=phase==1?0xff20baffu:
      (phase==2?0xffef8b29u:(phase==3?0xffd822e5u:0xff777777u));
    for(y=118;y<174;y++)for(x=1100;x<1164;x++)
      pixels[(size_t)y*(size_t)stride+(size_t)x]=color;
  }
}
int main(void){
  RikituCampaign campaign,loaded_campaign,checked_campaign;
  RikituScene scene,loaded_scene,checked_scene;
  RikituLoop loop,loaded_loop,checked_loop;
  Framebuffer fb;PadState pad;
  Result mount_rc=0;
  int mounted=0,phase=0;
  unsigned mask=0;
  if(rikitu_campaign_init(&campaign,4)){
    campaign.wallet=157;campaign.purchased_hearts=1;
    campaign.max_hearts=4;
  }
  fresh(&scene,&loop,&campaign);
  if(!fsdevGetDeviceFileSystem("sdmc")){
    mount_rc=fsdevMountSdmc();
    if(R_SUCCEEDED(mount_rc))mounted=1;
  }
  if(R_SUCCEEDED(mount_rc)){
    mask|=1u;
    if((mkdir("sdmc:/switch",0777)==0||errno==EEXIST)&&
       (mkdir("sdmc:/switch/RIKITU",0777)==0||errno==EEXIST)){
      int has_campaign,has_level;
      checked_campaign=campaign;
      has_campaign=rikitu_campaign_save_load_pair(
          CAMPAIGN_A,CAMPAIGN_B,&checked_campaign);
      fresh(&checked_scene,&checked_loop,&checked_campaign);
      has_level=has_campaign&&rikitu_level_save_load_pair(
          LEVEL_A,LEVEL_B,&checked_scene,&checked_loop,&checked_campaign);
      if(!has_campaign&&!has_level){
        phase=1;
        seed(&scene,&loop);
        mask|=2u;
      }else if(has_campaign&&has_level&&
               match_first(&checked_scene,&checked_loop,&checked_campaign)){
        phase=2;mask|=2u;
        campaign=checked_campaign;scene=checked_scene;loop=checked_loop;
        upgrade(&scene,&loop);
      }else if(has_campaign&&has_level&&
               match_second(&checked_scene,&checked_loop,&checked_campaign)){
        phase=3;mask|=2u;
        campaign=checked_campaign;scene=checked_scene;loop=checked_loop;
      }
      if(phase!=0){
        const int saved=phase==3?1:
          rikitu_campaign_save_store_pair(CAMPAIGN_A,CAMPAIGN_B,&campaign)&&
          rikitu_level_save_store_pair(LEVEL_A,LEVEL_B,
                                       &scene,&loop,&campaign);
        if(saved)mask|=4u;
        if(saved&&phase==3){
          /* Phase 3 is a fresh guest process after the previous rename.
           * Seed/resume persistence is verified by the host's exact-byte
           * CRC audit after exit and by the next independent guest start;
           * same-process readback can observe stale SDMC cache contents. */
          loaded_campaign=campaign;
          if(rikitu_campaign_save_load_pair(
                 CAMPAIGN_A,CAMPAIGN_B,&loaded_campaign)){
            fresh(&loaded_scene,&loaded_loop,&loaded_campaign);
            if(rikitu_level_save_load_pair(LEVEL_A,LEVEL_B,
                   &loaded_scene,&loaded_loop,&loaded_campaign)&&
               match_second(&loaded_scene,&loaded_loop,&loaded_campaign))
              mask|=8u;
          }
        }else if(saved){
          mask|=8u;
        }
      }
    }
  }
  framebufferCreate(&fb,nwindowGetDefault(),1280,720,
                    PIXEL_FORMAT_RGBA_8888,2);
  framebufferMakeLinear(&fb);
  padConfigureInput(1,HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&pad);
  while(appletMainLoop()){
    uint32_t *frame;
    u32 stride=0;
    int x,y;
    padUpdate(&pad);
    if(padGetButtonsDown(&pad)&HidNpadButton_Plus)break;
    frame=(uint32_t*)framebufferBegin(&fb,&stride);
    if(!frame||stride%4u)break;
    for(y=0;y<720;y++)for(x=0;x<1280;x++)
      frame[(size_t)y*(size_t)(stride/4u)+(size_t)x]=0xff30251au;
    marker(frame,(int)(stride/4u),phase,mask);
    framebufferEnd(&fb);
    svcSleepThread(16000000LL);
  }
  framebufferClose(&fb);
  if(mounted)fsdevUnmountDevice("sdmc");
  return mask==15u?0:1;
}
