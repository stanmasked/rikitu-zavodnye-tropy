/* Stage106: test-only Switch guest for sibling-file rename over existing SDMC slot.
 * Three independent NeXium executions, same SD root:
 *   seed    : write OLD A and STALE B, staged-commit NEW over existing B
 *   advance : detect OLD A/NEW B, staged-commit LATEST over existing A
 *   verify  : check LATEST A/NEW B unchanged (cold guest read only)
 *
 * All files named stage106-*.bin, NEVER RKS1/RKC2/RKL3 game saves.
 * Host verifies exact SD files and 4 green guest checks in every phase.
 * Emulator success does NOT establish physical Switch/FAT power-loss safety.
 */
#include <switch.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "rikitu_staged_slot.h"

#define SLOT_A "sdmc:/switch/RIKITU/stage106-a.bin"
#define SLOT_B "sdmc:/switch/RIKITU/stage106-b.bin"
#define BYTES 32u

static void pattern(uint8_t out[BYTES],uint8_t seed){
  size_t i;
  for(i=0;i<BYTES;i++)out[i]=(uint8_t)(seed+(uint8_t)i);
}
static int get_file(const char *path,uint8_t out[BYTES]){
  FILE *f=fopen(path,"rb");
  int extra,closed;
  size_t count;
  if(!f)return 0;
  count=fread(out,1,BYTES,f);
  extra=fgetc(f);
  closed=fclose(f)==0;
  return count==BYTES && extra==EOF && closed;
}
static int write_seed(const char *path,const uint8_t data[BYTES]){
  FILE *f=fopen(path,"wb");
  int closed;
  size_t written;
  if(!f)return 0;
  written=fwrite(data,1,BYTES,f);
  if(written!=BYTES || fflush(f)!=0){(void)fclose(f);return 0;}
  closed=fclose(f)==0;
  return closed;
}
static int matches(const char *path,const uint8_t data[BYTES]){
  uint8_t actual[BYTES];
  return get_file(path,actual) && memcmp(actual,data,BYTES)==0;
}
static void marker(uint32_t *frame,int stride,unsigned bits,int phase){
  int i,x,y;
  for(i=0;i<4;i++){
    const uint32_t c=(bits&(1u<<i))?0xff22df22u:0xff3030dfu;
    const int left=280+i*200;
    for(y=118;y<174;y++)
      for(x=left;x<left+64;x++)
        frame[(size_t)y*(size_t)stride+(size_t)x]=c;
  }
  { /* Three visually distinct independent guest states. */
    const uint32_t c=phase==1?0xff22bbeeU:
                      phase==2?0xffeebbaaU:
                      phase==3?0xffaa33eeU:0xff3030dfU;
    for(y=208;y<270;y++)
      for(x=1090;x<1154;x++)
        frame[(size_t)y*(size_t)stride+(size_t)x]=c;
  }
}
int main(void){
  Framebuffer fb;
  PadState pad;
  uint8_t old[BYTES],stale[BYTES],newer[BYTES],latest[BYTES];
  unsigned mask=0;
  int phase=0,sd_mounted=0;
  Result mount=0;
  int initial_a,initial_b;
  pattern(old,17);pattern(stale,70);pattern(newer,120);pattern(latest,180);
  if(!fsdevGetDeviceFileSystem("sdmc")){
    mount=fsdevMountSdmc();
    if(R_SUCCEEDED(mount))sd_mounted=1;
  }
  if(R_SUCCEEDED(mount)){
    mask|=1u;
    if((mkdir("sdmc:/switch",0777)==0||errno==EEXIST)&&
       (mkdir("sdmc:/switch/RIKITU",0777)==0||errno==EEXIST)){
      mask|=2u;
      initial_a=matches(SLOT_A,old);
      initial_b=matches(SLOT_B,newer);
      if(!initial_a&&!initial_b){
        /* Only seed on a genuinely empty root, not a foreign file. */
        struct stat sa,sb;
        if(stat(SLOT_A,&sa)!=0 && errno==ENOENT &&
           stat(SLOT_B,&sb)!=0 && errno==ENOENT){
          phase=1;
          if(write_seed(SLOT_A,old)&&write_seed(SLOT_B,stale)&&
             matches(SLOT_A,old)&&matches(SLOT_B,stale)){
            mask|=4u;
            if(rikitu_staged_slot_commit(SLOT_B,newer,BYTES)&&
               matches(SLOT_A,old)&&matches(SLOT_B,newer))
              mask|=8u;
          }
        }
      }else if(initial_a&&initial_b){
        phase=2;
        mask|=4u; /* Came from an earlier, independent guest. */
        if(rikitu_staged_slot_commit(SLOT_A,latest,BYTES)&&
           matches(SLOT_A,latest)&&matches(SLOT_B,newer))
          mask|=8u;
      }else if(matches(SLOT_A,latest)&&matches(SLOT_B,newer)){
        phase=3;
        mask|=4u;
        if(matches(SLOT_A,latest)&&matches(SLOT_B,newer))
          mask|=8u;
      }
    }
  }
  framebufferCreate(&fb,nwindowGetDefault(),1280,720,
                    PIXEL_FORMAT_RGBA_8888,2);
  framebufferMakeLinear(&fb);
  padConfigureInput(1,HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&pad);
  while(appletMainLoop()){
    u32 stride=0;
    int x,y;
    uint32_t *frame;
    padUpdate(&pad);
    if(padGetButtonsDown(&pad)&HidNpadButton_Plus)break;
    frame=(uint32_t *)framebufferBegin(&fb,&stride);
    if(!frame || stride%4u!=0)break;
    for(y=0;y<720;y++)
      for(x=0;x<1280;x++)
        frame[(size_t)y*(size_t)(stride/4u)+(size_t)x]=0xff222433u;
    marker(frame,(int)(stride/4u),mask,phase);
    framebufferEnd(&fb);
    svcSleepThread(16000000LL);
  }
  framebufferClose(&fb);
  if(sd_mounted)fsdevUnmountDevice("sdmc");
  return mask==15u?0:1;
}
