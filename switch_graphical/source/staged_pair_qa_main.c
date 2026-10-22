/* Stage131: exercise the production RKS1/RKC2/RKL3 save-pair APIs.
 * QA filenames only; never touches the shipping game saves.
 * This verifies staged writes and fail-closed pair selection under NeXium,
 * not physical Switch power-loss durability.
 */
#include <switch.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rikitu_save.h"
#include "rikitu_campaign_save.h"
#include "rikitu_level_save.h"
#include "rikitu_flower.h"
#include "rikitu_bridge.h"

enum { MAP=0,CAMPAIGN=1,LEVEL=2 };
static const char *const paths[3][2]={
  {"sdmc:/switch/RIKITU/stage131-map-a.rks",
   "sdmc:/switch/RIKITU/stage131-map-b.rks"},
  {"sdmc:/switch/RIKITU/stage131-campaign-a.rkc",
   "sdmc:/switch/RIKITU/stage131-campaign-b.rkc"},
  {"sdmc:/switch/RIKITU/stage131-level-a.rkl",
   "sdmc:/switch/RIKITU/stage131-level-b.rkl"}
};
static const char *const holds[3]={
  "sdmc:/switch/RIKITU/stage131-map-a.hold",
  "sdmc:/switch/RIKITU/stage131-campaign-a.hold",
  "sdmc:/switch/RIKITU/stage131-level-a.hold"
};
static const char *const marker="sdmc:/switch/RIKITU/stage131-phase.txt";
static const char *const escaped_path="sdmc:/switch/RIKITU/../stage131-escape.rks";
static const char *const escaped_target="sdmc:/switch/stage131-escape.rks";
static const char *const nested_dir="sdmc:/switch/RIKITU/nested";
static const char *const nested_path="sdmc:/switch/RIKITU/nested/stage131-escape.rks";
static const char *const wrong_ext_a="sdmc:/switch/RIKITU/stage131-map-mislabel-a.rkc";
static const char *const wrong_ext_b="sdmc:/switch/RIKITU/stage131-map-mislabel-b.rkc";

static size_t record_size(int family){
  return family==MAP?RIKITU_SAVE_BYTES:
         family==CAMPAIGN?RIKITU_CAMPAIGN_SAVE_BYTES:RIKITU_LEVEL_SAVE_BYTES;
}
static int absent(const char *path){
  struct stat st;
  if(stat(path,&st)==0)return 0;
  return errno==ENOENT;
}
static int read_exact(const char *path,uint8_t *out,size_t size){
  FILE *f;
  size_t got;
  int extra,read_error,closed;
  f=fopen(path,"rb");
  if(!f)return 0;
  got=fread(out,1,size,f);
  read_error=ferror(f);
  extra=fgetc(f);
  if(ferror(f))read_error=1;
  closed=fclose(f)==0;
  return !read_error&&closed&&got==size&&extra==EOF;
}
static int bytes_equal(const char *path,const uint8_t *expected,size_t size){
  uint8_t actual[RIKITU_LEVEL_SAVE_BYTES];
  return size<=sizeof(actual)&&read_exact(path,actual,size)&&
         memcmp(actual,expected,size)==0;
}
static int write_marker(const char *state){
  FILE *f=fopen(marker,"wb");
  size_t n;
  int ok;
  if(!f)return 0;
  n=fwrite(state,1,strlen(state),f);
  ok=n==strlen(state)&&fflush(f)==0;
  if(fclose(f)!=0)ok=0;
  return ok;
}
static int read_marker(char out[64]){
  FILE *f=fopen(marker,"rb");
  size_t n;
  int extra,read_error,closed;
  if(!f)return 0;
  n=fread(out,1,63,f);
  read_error=ferror(f);
  extra=fgetc(f);
  if(ferror(f))read_error=1;
  closed=fclose(f)==0;
  if(read_error||!closed||extra!=EOF||n==0||n>=63)return 0;
  out[n]='\0';
  return 1;
}
static int all_absent(void){
  int f,b;
  char pending[128];
  if(!absent(marker)||!absent(escaped_target)||!absent(nested_dir)||
     !absent(wrong_ext_a)||!absent(wrong_ext_b))return 0;
  for(f=MAP;f<=LEVEL;f++){
    if(!absent(holds[f]))return 0;
    for(b=0;b<2;b++){
      if(!absent(paths[f][b]))return 0;
      if(snprintf(pending,sizeof(pending),"%s.pending",paths[f][b])<0||
         !absent(pending))return 0;
    }
  }
  return 1;
}
static void seed_flower(RikituScene *s,RikituLoop *l){
  rikitu_flower_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
static void seed_bridge(RikituScene *s,RikituLoop *l){
  rikitu_bridge_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
static void progress_map(RikituScene *s,RikituLoop *l,int final_step){
  s->reached_checkpoints=1;
  s->active_checkpoint=s->checkpoints[0];
  s->hearts=final_step?1:2;
  l->active_seconds=final_step?30.0f:20.0f;
}
static void progress_level(RikituScene *s,RikituLoop *l,int final_step){
  s->reached_checkpoints=1;
  s->active_checkpoint=s->checkpoints[0];
  s->hearts=final_step?1:2;
  s->stage.coin_count=1;
  s->stage.coins[0]=1;
  l->active_seconds=final_step?30.0f:20.0f;
}
static int record_generation(int family,int slot,uint32_t *generation){
  uint8_t data[RIKITU_LEVEL_SAVE_BYTES];
  size_t size=record_size(family);
  if(!read_exact(paths[family][slot],data,size))return 0;
  if(family==MAP){
    RikituScene s;RikituLoop l;
    seed_flower(&s,&l);
    return rikitu_save_decode(&s,&l,data,size,generation);
  }
  if(family==CAMPAIGN){
    RikituCampaign c;
    if(!rikitu_campaign_init(&c,4))return 0;
    return rikitu_campaign_save_decode(&c,data,size,generation);
  }
  {
    RikituCampaign c;
    RikituScene s;RikituLoop l;
    if(!rikitu_campaign_init(&c,4))return 0;
    c.wallet=157;
    seed_bridge(&s,&l);
    return rikitu_level_save_decode(&s,&l,&c,data,size,generation);
  }
}
static int pair_generation_state(int family,uint32_t gen_a,uint32_t gen_b){
  uint32_t a=0,b=0;
  return record_generation(family,0,&a)&&
         record_generation(family,1,&b)&&a==gen_a&&b==gen_b;
}
static int a1_b0_state(void){
  int f;
  uint32_t generation;
  for(f=MAP;f<=LEVEL;f++)
    if(!record_generation(f,0,&generation)||generation!=1||
       !absent(paths[f][1]))return 0;
  return 1;
}
static int a1_b2_state(void){
  int f;
  for(f=MAP;f<=LEVEL;f++)
    if(!pair_generation_state(f,1,2))return 0;
  return 1;
}
static int a3_b2_state(void){
  int f;
  for(f=MAP;f<=LEVEL;f++)
    if(!pair_generation_state(f,3,2))return 0;
  return 1;
}
static int latest_is_progress(int final_step){
  RikituScene s;RikituLoop l;RikituCampaign c;
  seed_flower(&s,&l);
  if(!rikitu_save_load_pair(paths[MAP][0],paths[MAP][1],&s,&l)||
     s.reached_checkpoints!=1||s.hearts!=(final_step?1:2))return 0;
  if(!rikitu_campaign_init(&c,4))return 0;
  if(!rikitu_campaign_save_load_pair(paths[CAMPAIGN][0],
                                     paths[CAMPAIGN][1],&c)||
     c.level!=4||c.wallet!=(final_step?159:158))return 0;
  if(!rikitu_campaign_init(&c,4))return 0;
  c.wallet=157;
  seed_bridge(&s,&l);
  if(!rikitu_level_save_load_pair(paths[LEVEL][0],paths[LEVEL][1],
                                  &s,&l,&c)||
     s.reached_checkpoints!=1||s.hearts!=(final_step?1:2)||
     s.stage.coin_count!=1||s.stage.coins[0]!=1)return 0;
  return 1;
}
static int reject_traversal_path(void){
  RikituScene s;RikituLoop l;
  seed_flower(&s,&l);
  /* Make the nested parent writable first: a rejected write must prove
   * path validation, not merely fail because its parent is missing. */
  if(mkdir(nested_dir,0777)!=0)return 0;
  if(rikitu_save_store_pair(escaped_path,paths[MAP][1],&s,&l)||
     rikitu_save_store_pair(nested_path,paths[MAP][1],&s,&l)||
     rikitu_save_store_pair(wrong_ext_a,wrong_ext_b,&s,&l)||
     !absent(escaped_target)||!absent(nested_path)||
     !absent(wrong_ext_a)||!absent(wrong_ext_b))
    return 0; /* Leave evidence behind if any guard fails. */
  return rmdir(nested_dir)==0;
}
static int seed_pairs(void){
  RikituScene s;RikituLoop l;RikituCampaign c;
  seed_flower(&s,&l);
  if(!rikitu_save_store_pair(paths[MAP][0],paths[MAP][1],&s,&l))return 0;
  if(!rikitu_campaign_init(&c,4))return 0;
  c.wallet=157;
  if(!rikitu_campaign_save_store_pair(paths[CAMPAIGN][0],
                                      paths[CAMPAIGN][1],&c))return 0;
  if(!rikitu_campaign_init(&c,4))return 0;
  c.wallet=157;
  seed_bridge(&s,&l);
  if(!rikitu_level_save_store_pair(paths[LEVEL][0],paths[LEVEL][1],
                                   &s,&l,&c))return 0;
  return write_marker("seeded");
}
static int advance_pairs(void){
  RikituScene s;RikituLoop l;RikituCampaign c;
  seed_flower(&s,&l);progress_map(&s,&l,0);
  if(!rikitu_save_store_pair(paths[MAP][0],paths[MAP][1],&s,&l))return 0;
  if(!rikitu_campaign_init(&c,4))return 0;
  c.wallet=158;
  if(!rikitu_campaign_save_store_pair(paths[CAMPAIGN][0],
                                      paths[CAMPAIGN][1],&c))return 0;
  if(!rikitu_campaign_init(&c,4))return 0;
  c.wallet=157;
  seed_bridge(&s,&l);progress_level(&s,&l,0);
  if(!rikitu_level_save_store_pair(paths[LEVEL][0],paths[LEVEL][1],
                                   &s,&l,&c))return 0;
  return write_marker("ready-to-block");
}
static int block_one(int family,const uint8_t *old_b,size_t size){
  char pending[128];
  const char *a=paths[family][0],*b=paths[family][1],*hold=holds[family];
  if(!absent(hold)||rename(a,hold)!=0)return 0;
  if(mkdir(a,0777)!=0){(void)rename(hold,a);return 0;}
  if(family==MAP){
    RikituScene s,loaded;RikituLoop l,loaded_loop;
    seed_flower(&s,&l);progress_map(&s,&l,1);
    seed_flower(&loaded,&loaded_loop);loaded.hearts=1;
    loaded_loop.active_seconds=777.0f;
    if(rikitu_save_store_pair(a,b,&s,&l)||
       rikitu_save_load_pair(a,b,&loaded,&loaded_loop)||
       loaded.hearts!=1||loaded.reached_checkpoints!=0||
       loaded_loop.active_seconds!=777.0f)goto restore_fail;
  }else if(family==CAMPAIGN){
    RikituCampaign c,loaded;
    if(!rikitu_campaign_init(&c,4)||!rikitu_campaign_init(&loaded,4))
      goto restore_fail;
    c.wallet=159;loaded.wallet=999;
    if(rikitu_campaign_save_store_pair(a,b,&c)||
       rikitu_campaign_save_load_pair(a,b,&loaded)||loaded.wallet!=999)
      goto restore_fail;
  }else{
    RikituCampaign c;RikituScene s,loaded;RikituLoop l,loaded_loop;
    if(!rikitu_campaign_init(&c,4))goto restore_fail;
    c.wallet=157;
    seed_bridge(&s,&l);progress_level(&s,&l,1);
    seed_bridge(&loaded,&loaded_loop);loaded_loop.active_seconds=777.0f;
    if(rikitu_level_save_store_pair(a,b,&s,&l,&c)||
       rikitu_level_save_load_pair(a,b,&loaded,&loaded_loop,&c)||
       loaded.hearts!=3||loaded.reached_checkpoints!=0||
       loaded_loop.active_seconds!=777.0f)goto restore_fail;
  }
  /* Avoid same-process readback through the renamed path: fsdev may cache
   * directory entries around rename. The host audits both live slots after
   * this guest exits; here we check the untouched backup slot and sidecar. */
  if(!bytes_equal(b,old_b,size)||
     snprintf(pending,sizeof(pending),"%s.pending",a)<0||
     !absent(pending))goto restore_fail;
  if(rmdir(a)!=0||rename(hold,a)!=0)return 0;
  return bytes_equal(b,old_b,size);
restore_fail:
  (void)rmdir(a);
  (void)rename(hold,a);
  return 0;
}
static int block_pairs(void){
  uint8_t b[RIKITU_LEVEL_SAVE_BYTES];
  int f;
  for(f=MAP;f<=LEVEL;f++){
    size_t n=record_size(f);
    if(!read_exact(paths[f][1],b,n)||!block_one(f,b,n))return 0;
  }
  return write_marker("ready-to-recover");
}
static int recover_pairs(void){
  RikituScene s;RikituLoop l;RikituCampaign c;
  seed_flower(&s,&l);progress_map(&s,&l,1);
  if(!rikitu_save_store_pair(paths[MAP][0],paths[MAP][1],&s,&l))return 0;
  if(!rikitu_campaign_init(&c,4))return 0;
  c.wallet=159;
  if(!rikitu_campaign_save_store_pair(paths[CAMPAIGN][0],
                                      paths[CAMPAIGN][1],&c))return 0;
  if(!rikitu_campaign_init(&c,4))return 0;
  c.wallet=157;
  seed_bridge(&s,&l);progress_level(&s,&l,1);
  if(!rikitu_level_save_store_pair(paths[LEVEL][0],paths[LEVEL][1],
                                   &s,&l,&c))return 0;
  return write_marker("ready-to-verify");
}
static int verify_pairs(void){
  return a3_b2_state()&&latest_is_progress(1)&&write_marker("verified");
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
      phase==4?0xffddbb22U:phase==5?0xff33dd88U:0xff3030dfU;
    for(y=208;y<270;y++)
      for(x=1090;x<1154;x++)
        pixels[(size_t)y*(size_t)stride+(size_t)x]=color;
  }
}
int main(void){
  Framebuffer fb;
  PadState pad;
  Result mount=0;
  int sd_mounted=0,phase=0,ok=0;
  unsigned mask=0;
  char state[64];
  if(!fsdevGetDeviceFileSystem("sdmc")){
    mount=fsdevMountSdmc();
    if(R_SUCCEEDED(mount))sd_mounted=1;
  }
  if(R_SUCCEEDED(mount)){
    mask|=1u;
    if((mkdir("sdmc:/switch",0777)==0||errno==EEXIST)&&
       (mkdir("sdmc:/switch/RIKITU",0777)==0||errno==EEXIST)){
      mask|=2u;
      if(all_absent()){
        phase=1;mask|=4u;
        ok=reject_traversal_path()&&seed_pairs();
      }else if(read_marker(state)&&strcmp(state,"seeded")==0&&a1_b0_state()){
        phase=2;mask|=4u;
        ok=reject_traversal_path()&&advance_pairs();
      }else if(read_marker(state)&&strcmp(state,"ready-to-block")==0&&
               a1_b2_state()&&latest_is_progress(0)){
        phase=3;mask|=4u;
        ok=reject_traversal_path()&&block_pairs();
      }else if(read_marker(state)&&strcmp(state,"ready-to-recover")==0&&
               a1_b2_state()){
        phase=4;mask|=4u;
        ok=reject_traversal_path()&&recover_pairs();
      }else if(read_marker(state)&&strcmp(state,"ready-to-verify")==0&&
               a3_b2_state()){
        phase=5;mask|=4u;
        ok=reject_traversal_path()&&verify_pairs();
      }
      if(ok)mask|=8u;
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
