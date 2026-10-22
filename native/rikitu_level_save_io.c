#include "rikitu_level_save.h"
#include "rikitu_slot_read.h"
#include <stdio.h>
#include <string.h>
static int allowed_path(const char *path){
  size_t n;
  if(!path)return 0;
  n=strlen(path);
  return n>4&&strcmp(path+n-4,".rkl")==0;
}
static int read_slot(const char *path,uint8_t out[RIKITU_LEVEL_SAVE_BYTES]){
  if(!allowed_path(path)||!out)return RIKITU_SLOT_READ_ERROR;
  return rikitu_read_fixed_slot(path,out,RIKITU_LEVEL_SAVE_BYTES);
}
static int newer(uint32_t x,uint32_t y){
  return x!=y&&(uint32_t)(x-y)<0x80000000u;
}
/* Never use a stale chapter/wallet record when choosing a file to
 * overwrite. It may have a much higher generation than the active
 * campaign's last good checkpoint. The loader already applies the same
 * identity requirement when actually restoring a checkpoint. */
static int belongs_to_campaign(const uint8_t p[RIKITU_LEVEL_SAVE_BYTES],
                               const RikituCampaign *c){
  const uint32_t wallet=(uint32_t)p[36]|
             ((uint32_t)p[37]<<8)|
             ((uint32_t)p[38]<<16)|
             ((uint32_t)p[39]<<24);
  return c&&rikitu_campaign_valid(c)&&
         c->mode==RIKITU_CAMPAIGN_PLAYING&&
         p[5]==(uint8_t)c->level &&
         p[8]==(uint8_t)c->max_hearts &&
         p[41]==(uint8_t)c->purchased_hearts &&
         wallet==(uint32_t)c->wallet;
}
int rikitu_level_save_load_pair(const char *a,const char *b,
                                 RikituScene *scene,RikituLoop *loop,
                                 const RikituCampaign *campaign){
  uint8_t x[RIKITU_LEVEL_SAVE_BYTES],y[RIKITU_LEVEL_SAVE_BYTES];
  RikituScene sx,sy;
  RikituLoop lx,ly;
  uint32_t gx=0,gy=0;
  int rx,ry,vx=0,vy=0;
  if(!allowed_path(a)||!allowed_path(b)||strcmp(a,b)==0||
     !scene||!loop||!campaign)return 0;
  rx=read_slot(a,x);
  if(rx==RIKITU_SLOT_READ_ERROR)return 0;
  ry=read_slot(b,y);
  if(ry==RIKITU_SLOT_READ_ERROR)return 0;
  sx=*scene;sy=*scene;lx=*loop;ly=*loop;
  if(rx==RIKITU_SLOT_READ_OK)
    vx=rikitu_level_save_decode(&sx,&lx,campaign,x,sizeof(x),&gx);
  if(ry==RIKITU_SLOT_READ_OK)
    vy=rikitu_level_save_decode(&sy,&ly,campaign,y,sizeof(y),&gy);
  if(!vx&&!vy)return 0;
  if(vy&&(!vx||newer(gy,gx))){*scene=sy;*loop=ly;}
  else{*scene=sx;*loop=lx;}
  return 1;
}
int rikitu_level_save_store_pair(const char *a,const char *b,
                                  const RikituScene *scene,
                                  const RikituLoop *loop,
                                  const RikituCampaign *campaign){
  uint8_t x[RIKITU_LEVEL_SAVE_BYTES],y[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t p[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t gx=0,gy=0,generation=1;
  int rx,ry,vx=0,vy=0;
  const char *path;
  FILE *f;
  size_t n;
  if(!allowed_path(a)||!allowed_path(b)||strcmp(a,b)==0||
     !scene||!loop||!campaign)return 0;
  rx=read_slot(a,x);
  if(rx==RIKITU_SLOT_READ_ERROR)return 0;
  ry=read_slot(b,y);
  if(ry==RIKITU_SLOT_READ_ERROR)return 0;
  if(rx==RIKITU_SLOT_READ_OK)
    vx=rikitu_level_save_inspect(x,sizeof(x),&gx) &&
       belongs_to_campaign(x,campaign);
  if(ry==RIKITU_SLOT_READ_OK)
    vy=rikitu_level_save_inspect(y,sizeof(y),&gy) &&
       belongs_to_campaign(y,campaign);
  if(vx&&vy){
    if(newer(gy,gx)){generation=gy+1;path=a;}
    else{generation=gx+1;path=b;}
  }else if(vx){generation=gx+1;path=b;}
  else if(vy){generation=gy+1;path=a;}
  else path=a;
  if(!generation)generation=1;
  if(!rikitu_level_save_encode(p,scene,loop,campaign,generation))
    return 0;
  f=fopen(path,"wb");
  if(!f)return 0;
  n=fwrite(p,1,sizeof(p),f);
  if(n!=sizeof(p)||fflush(f)!=0){(void)fclose(f);return 0;}
  return fclose(f)==0;
}
