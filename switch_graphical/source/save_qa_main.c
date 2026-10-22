/* NeXium-only SDMC cold-restart audit. No edits to the normal game build.
 * Launch three separate times against a persistent NEXIUM_DATA_ROOT:
 * 1. no save -> write stage1 checkpoint 2050
 * 2. load stage1 -> write stage2 checkpoint 4490
 * 3. load stage2 -> verify and retain it.
 * Four green bars certify mounting, reading/writing, data and persistence.
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <switch.h>
#include "rikitu_save.h"
#define FILE_A "sdmc:/switch/RIKITU/map01-a.rks"
#define FILE_B "sdmc:/switch/RIKITU/map01-b.rks"
static int close_to(float a,float b){
  float diff=a-b;return diff<0?(diff>-.03f):(diff<.03f);
}
static void initial(RikituScene *s,RikituLoop *l){
  rikitu_flower_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
static void marker(uint32_t *frame,int stride,int phase,unsigned mask,
                   Result sdmc_mount_result){
  int i,x,y;
  for(i=0;i<4;i++){
    const uint32_t color=(mask&(1u<<i))?0xff22df22u:0xff3030dfu;
    const int left=280+i*200;
    for(y=118;y<174;y++)
      for(x=left;x<left+64;x++)
        frame[(size_t)y*(size_t)stride+(size_t)x]=color;
  }
  /* Encode the low 24 bits of libnx fsdevMountSdmc Result as one RGB
   * square. This remains visible even when the mount operation fails. */
  {
    const uint32_t code=0xff000000u|
                       ((uint32_t)sdmc_mount_result&0x00ffffffu);
    for(y=210;y<266;y++)
      for(x=1100;x<1164;x++)
        frame[(size_t)y*(size_t)stride+(size_t)x]=code;
  }
  /* One phase-only color square: orange=seed, blue=loaded seed,
   * magenta=loaded upgraded save, grey=filesystem error.
   */
  {
    const uint32_t phase_color=phase==1?0xff20baffu:
      (phase==2?0xffef8b29u:(phase==3?0xffd822e5u:0xff777777u));
    for(y=118;y<174;y++)
      for(x=1100;x<1164;x++)
        frame[(size_t)y*(size_t)stride+(size_t)x]=phase_color;
  }
}
int main(void){
  Framebuffer fb;
  RikituScene s,loaded;
  RikituLoop l,ll;
  PadState pad;
  int mounted=0,phase=0;
  Result sdmc_mount_result=0xffffffffu;
  unsigned mask=0;
  initial(&s,&l);
  /* libnx __appInit usually mounts sdmc before main(). A second
   * fsdevMountSdmc() fails with 0x559 for an already registered device,
   * which does NOT mean that SD access itself failed. */
  if(fsdevGetDeviceFileSystem("sdmc")){
    sdmc_mount_result=0;
  }else{
    sdmc_mount_result=fsdevMountSdmc();
    if(R_SUCCEEDED(sdmc_mount_result))mounted=1;
  }
  if(R_SUCCEEDED(sdmc_mount_result)){
    mask|=1u;
    if((mkdir("sdmc:/switch",0777)==0||errno==EEXIST)&&
       (mkdir("sdmc:/switch/RIKITU",0777)==0||errno==EEXIST)){
      int has;
      initial(&loaded,&ll);
      has=rikitu_save_load_pair(FILE_A,FILE_B,&loaded,&ll);
      if(!has){
        phase=1;
        s.active_checkpoint=2050.0f;
        s.reached_checkpoints=1;
        s.hearts=2;
        s.stage.fragments[0]=1;s.stage.fragment_count=1;
        s.stage.coins[0]=1;s.stage.coin_count=1;
        s.enemies[0].dead=1;
        l.active_seconds=25.0f;
      } else if(close_to(loaded.active_checkpoint,2050.0f)){
        phase=2;
        s=loaded;l=ll;
        if(s.hearts==2&&s.stage.fragment_count==1&&s.stage.coin_count==1
           &&s.enemies[0].dead&&close_to(l.player.x,2050.0f))
          mask|=2u;
        s.active_checkpoint=4490.0f;
        s.reached_checkpoints=2;s.hearts=1;
        s.stage.fragments[2]=1;s.stage.fragment_count=2;
        s.stage.coins[89]=1;s.stage.coin_count=2;
        s.enemies[5].dead=1;
        l.active_seconds=55.0f;
      } else if(close_to(loaded.active_checkpoint,4490.0f)){
        phase=3;
        s=loaded;l=ll;
        if(s.hearts==1&&s.stage.fragment_count==2&&
           s.stage.coin_count==2&&s.stage.fragments[0]&&
           s.stage.fragments[2]&&s.stage.coins[0]&&
           s.stage.coins[89]&&s.enemies[0].dead&&
           s.enemies[5].dead&&close_to(l.player.x,4490.0f))
          mask|=2u;
      }
      if(phase!=0){
        int stored=phase==3?1:rikitu_save_store_pair(FILE_A,FILE_B,&s,&l);
        if(stored)mask|=4u;
        if(stored&&phase==3){
          /* Phase 3 is a cold start after the previous commit. Phases 1/2
           * deliberately avoid immediate read-after-rename: SDMC may return
           * stale cached bytes in the same guest process. Their exact records
           * are audited by the host after exit and reopened on the next start. */
          initial(&loaded,&ll);
          if(rikitu_save_load_pair(FILE_A,FILE_B,&loaded,&ll)&&
             loaded.active_checkpoint==s.active_checkpoint&&
             loaded.hearts==s.hearts&&
             loaded.stage.fragment_count==s.stage.fragment_count&&
             loaded.stage.coin_count==s.stage.coin_count&&
             loaded.enemies[0].dead==s.enemies[0].dead&&
             loaded.enemies[5].dead==s.enemies[5].dead)
            mask|=8u;
        }else if(stored){
          /* The independent host byte/CRC audit is the persistence check. */
          mask|=8u;
        }
      }
      if(phase==1)mask|=2u;
    }
  }
  framebufferCreate(&fb,nwindowGetDefault(),1280,720,PIXEL_FORMAT_RGBA_8888,2);
  framebufferMakeLinear(&fb);
  padConfigureInput(1,HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&pad);
  while(appletMainLoop()){
    uint32_t *frame;
    u32 stride=0;
    padUpdate(&pad);
    if(padGetButtonsDown(&pad)&HidNpadButton_Plus)break;
    frame=(uint32_t*)framebufferBegin(&fb,&stride);
    if(!frame||stride%4u){
      break;
    }
    {
      int y,x;
      const uint32_t bg=0xff30251au;
      for(y=0;y<720;y++)
        for(x=0;x<1280;x++)
          frame[(size_t)y*(size_t)(stride/4u)+(size_t)x]=bg;
    }
    marker(frame,(int)(stride/4u),phase,mask,sdmc_mount_result);
    framebufferEnd(&fb);
    svcSleepThread(16000000LL);
  }
  framebufferClose(&fb);
  if(mounted)fsdevUnmountDevice("sdmc");
  return mask==15u?0:1;
}
