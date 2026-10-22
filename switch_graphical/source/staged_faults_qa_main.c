/* Stage126: private SDMC fault-boundary guest with genuine RKS1/RKC2/RKL3
 * records. Six independent cold starts verify incomplete pending writes,
 * directory obstructions, recovery and untouched live slots.
 * IMPORTANT: stage126-* filenames only; no shipping game save writes.
 * This probes emulator rename/readback, NOT Switch power-cut durability.
 */
#include <switch.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rikitu_staged_slot.h"
#include "rikitu_save.h"
#include "rikitu_campaign_save.h"
#include "rikitu_level_save.h"
#include "rikitu_bridge.h"
#include "rikitu_flower.h"

enum { MAP=0,CAMPAIGN=1,LEVEL=2 };
static const char *const paths[3][2]={
  {"sdmc:/switch/RIKITU/stage126-map-a.rks",
   "sdmc:/switch/RIKITU/stage126-map-b.rks"},
  {"sdmc:/switch/RIKITU/stage126-campaign-a.rkc",
   "sdmc:/switch/RIKITU/stage126-campaign-b.rkc"},
  {"sdmc:/switch/RIKITU/stage126-level-a.rkl",
   "sdmc:/switch/RIKITU/stage126-level-b.rkl"}
};
static size_t record_size(int f){
  return f==MAP?RIKITU_SAVE_BYTES:
         f==CAMPAIGN?RIKITU_CAMPAIGN_SAVE_BYTES:RIKITU_LEVEL_SAVE_BYTES;
}
static int make_record(int f,int step,uint8_t out[RIKITU_LEVEL_SAVE_BYTES]){
  RikituCampaign c;
  RikituScene s;
  RikituLoop l;
  int i;
  if(!rikitu_campaign_init(&c,4))return 0;
  c.wallet=157;
  if(f==CAMPAIGN){
    c.wallet+=step;
    return rikitu_campaign_save_encode(out,&c,(uint32_t)(step+1));
  }
  if(f==MAP)rikitu_flower_init(&s);
  else rikitu_bridge_init(&s);
  memset(&l,0,sizeof(l));
  l.player=s.player;
  if(step){
    s.reached_checkpoints=step;
    s.active_checkpoint=s.checkpoints[step-1];
    s.hearts=step>=2?1:3-step;
    if(f==LEVEL){
      s.stage.coin_count=step;
      for(i=0;i<step;i++)s.stage.coins[i]=1;
    }
  }
  if(f==MAP)return rikitu_save_encode(out,&s,&l,(uint32_t)(step+1));
  return rikitu_level_save_encode(out,&s,&l,&c,(uint32_t)(step+1));
}
static int match_record(int f,int slot,int step){
  uint8_t expected[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t actual[RIKITU_LEVEL_SAVE_BYTES];
  FILE *file;
  int extra,closed,valid;
  uint32_t generation=0;
  size_t n=record_size(f),count;
  RikituCampaign c;
  RikituScene s;
  RikituLoop l;
  if(!make_record(f,step,expected))return 0;
  file=fopen(paths[f][slot],"rb");
  if(!file)return 0;
  count=fread(actual,1,n,file);
  extra=fgetc(file);
  valid=ferror(file)==0;
  closed=fclose(file)==0;
  if(!closed||!valid||count!=n||extra!=EOF||
     memcmp(actual,expected,n)!=0)return 0;
  if(!rikitu_campaign_init(&c,4))return 0;
  c.wallet=157;
  if(f==CAMPAIGN)
    valid=rikitu_campaign_save_decode(&c,actual,n,&generation);
  else{
    if(f==MAP)rikitu_flower_init(&s);
    else rikitu_bridge_init(&s);
    memset(&l,0,sizeof(l));
    l.player=s.player;
    if(f==MAP)valid=rikitu_save_decode(&s,&l,actual,n,&generation);
    else valid=rikitu_level_save_decode(
      &s,&l,&c,actual,n,&generation);
  }
  return valid&&generation==(uint32_t)(step+1);
}
static int commit_record(int f,int slot,int step){
  uint8_t data[RIKITU_LEVEL_SAVE_BYTES];
  return make_record(f,step,data)&&
         rikitu_staged_slot_commit(paths[f][slot],data,record_size(f));
}
static int all_absent(void){
  struct stat st;
  int f,b;
  for(f=MAP;f<=LEVEL;f++)
    for(b=0;b<2;b++)
      if(stat(paths[f][b],&st)==0||errno!=ENOENT)return 0;
  return 1;
}
static int pair_state(int a_step,int b_step){
  int f;
  for(f=MAP;f<=LEVEL;f++)
    if(!match_record(f,0,a_step)||!match_record(f,1,b_step))
      return 0;
  return 1;
}
static int pending_path(int f,int slot,char out[160]){
  int len=snprintf(out,160,"%s.pending",paths[f][slot]);
  return len>0&&len<160;
}
/* state 0: absent; 1: intentionally half-written gen3 A; 2: directory. */
static int pending_state(int slot,int state){
  int f;
  for(f=MAP;f<=LEVEL;f++){
    char path[160];
    struct stat st;
    if(!pending_path(f,slot,path))return 0;
    if(stat(path,&st)!=0){
      if(state==0&&errno==ENOENT)continue;
      return 0;
    }
    if(state==0)return 0;
    if(state==2){
      if(!S_ISDIR(st.st_mode))return 0;
    }else if(state==1){
      uint8_t expected[RIKITU_LEVEL_SAVE_BYTES],actual[RIKITU_LEVEL_SAVE_BYTES];
      size_t half=record_size(f)/2u;
      FILE *file;
      int extra,err,closed;
      size_t count;
      if(!S_ISREG(st.st_mode)||!make_record(f,2,expected))return 0;
      file=fopen(path,"rb");
      if(!file)return 0;
      count=fread(actual,1,half,file);
      extra=fgetc(file);
      err=ferror(file);
      closed=fclose(file)==0;
      if(!closed||err||count!=half||extra!=EOF||
         memcmp(expected,actual,half)!=0)return 0;
    }else return 0;
  }
  return 1;
}
static int create_partial(void){
  int f;
  for(f=MAP;f<=LEVEL;f++){
    char path[160];
    uint8_t data[RIKITU_LEVEL_SAVE_BYTES];
    size_t half=record_size(f)/2u;
    FILE *file;
    int closed;
    if(!pending_path(f,0,path)||!make_record(f,2,data))return 0;
    file=fopen(path,"wb");
    if(!file)return 0;
    if(fwrite(data,1,half,file)!=half||fflush(file)!=0){
      (void)fclose(file);return 0;
    }
    closed=fclose(file)==0;
    if(!closed)return 0;
  }
  return 1;
}
static int commit_after_partial(void){
  int f;
  for(f=MAP;f<=LEVEL;f++)
    if(!commit_record(f,0,2))return 0;
  return 1;
}
static int obstruct_and_reject(void){
  int f;
  for(f=MAP;f<=LEVEL;f++){
    char path[160];
    /* Pre-create a directory at the precise target's .pending name;
     * real staged helper must refuse and preserve both existing slots. */
    if(!pending_path(f,1,path)||mkdir(path,0700)!=0)return 0;
    if(commit_record(f,1,3))return 0;
  }
  return 1;
}
static int remove_obstruction_and_commit(void){
  int f;
  for(f=MAP;f<=LEVEL;f++){
    char path[160];
    if(!pending_path(f,1,path)||rmdir(path)!=0||
       !commit_record(f,1,3))return 0;
  }
  return 1;
}
static void draw_checks(uint32_t *pixels,int stride,unsigned bits,int phase){
  int i,x,y;
  for(i=0;i<4;i++){
    const uint32_t color=(bits&(1u<<i))?0xff22df22u:0xff3030dfu;
    const int left=280+i*200;
    for(y=118;y<174;y++)
      for(x=left;x<left+64;x++)
        pixels[(size_t)y*(size_t)stride+(size_t)x]=color;
  }
  {
    const uint32_t color=phase==1?0xff22bbeeU:
      phase==2?0xffeebbaaU:phase==3?0xffaa33eeU:
      phase==4?0xff20ba80U:phase==5?0xffbb88ccU:
      phase==6?0xffaabb22U:0xff3030dfU;
    for(y=208;y<270;y++)
      for(x=1090;x<1154;x++)
        pixels[(size_t)y*(size_t)stride+(size_t)x]=color;
  }
}
int main(void){
  Framebuffer fb;
  PadState pad;
  Result mount=0;
  int sd_mounted=0,phase=0,f;
  unsigned mask=0;
  if(!fsdevGetDeviceFileSystem("sdmc")){
    mount=fsdevMountSdmc();
    if(R_SUCCEEDED(mount))sd_mounted=1;
  }
  if(R_SUCCEEDED(mount)){
    mask|=1u;
    if((mkdir("sdmc:/switch",0777)==0||errno==EEXIST)&&
       (mkdir("sdmc:/switch/RIKITU",0777)==0||errno==EEXIST)){
      mask|=2u;
      if(all_absent())phase=1;
      else if(pair_state(0,1)&&pending_state(0,0)&&
              pending_state(1,0))phase=2;
      else if(pair_state(0,1)&&pending_state(0,1)&&
              pending_state(1,0))phase=3;
      else if(pair_state(2,1)&&pending_state(0,0)&&
              pending_state(1,0))phase=4;
      else if(pair_state(2,1)&&pending_state(0,0)&&
              pending_state(1,2))phase=5;
      else if(pair_state(2,3)&&pending_state(0,0)&&
              pending_state(1,0))phase=6;
      if(phase){
        int ok=1;
        mask|=4u;
        if(phase==1){
          for(f=MAP;f<=LEVEL;f++)
            if(!commit_record(f,0,0)||!commit_record(f,1,0)||
               !commit_record(f,1,1)){ok=0;break;}
        }else if(phase==2)ok=create_partial();
        else if(phase==3)ok=commit_after_partial();
        else if(phase==4)ok=obstruct_and_reject();
        else if(phase==5)ok=remove_obstruction_and_commit();
        /* Host checks exact live bytes and the pending obstruction once
         * each guest has exited; next cold start re-opens the files. */
        if(ok)mask|=8u;
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
    for(y=0;y<720;y++)
      for(x=0;x<1280;x++)
        frame[(size_t)y*(size_t)(stride/4u)+(size_t)x]=0xff222433u;
    draw_checks(frame,(int)(stride/4u),mask,phase);
    framebufferEnd(&fb);
    svcSleepThread(16000000LL);
  }
  framebufferClose(&fb);
  if(sd_mounted)fsdevUnmountDevice("sdmc");
  return mask==15u?0:1;
}
