#include "rikitu_level_save.h"
#include <stdio.h>
#include <string.h>
static int allowed_path(const char *path){
  size_t n;
  if(!path)return 0;
  n=strlen(path);
  return n>4&&strcmp(path+n-4,".rkl")==0;
}
static int read_slot(const char *path,uint8_t out[RIKITU_LEVEL_SAVE_BYTES]){
  FILE *f;
  size_t n;
  int extra;
  if(!allowed_path(path)||!out)return 0;
  f=fopen(path,"rb");
  if(!f)return 0;
  n=fread(out,1,RIKITU_LEVEL_SAVE_BYTES,f);
  extra=fgetc(f);
  if(fclose(f)!=0)return 0;
  return n==RIKITU_LEVEL_SAVE_BYTES&&extra==EOF;
}
static int newer(uint32_t x,uint32_t y){
  return x!=y&&(uint32_t)(x-y)<0x80000000u;
}
int rikitu_level_save_load_pair(const char *a,const char *b,
                                 RikituScene *scene,RikituLoop *loop,
                                 const RikituCampaign *campaign){
  uint8_t x[RIKITU_LEVEL_SAVE_BYTES],y[RIKITU_LEVEL_SAVE_BYTES];
  RikituScene sx,sy;
  RikituLoop lx,ly;
  uint32_t gx=0,gy=0;
  int vx=0,vy=0;
  if(!allowed_path(a)||!allowed_path(b)||strcmp(a,b)==0||
     !scene||!loop||!campaign)return 0;
  sx=*scene;sy=*scene;lx=*loop;ly=*loop;
  if(read_slot(a,x))
    vx=rikitu_level_save_decode(&sx,&lx,campaign,x,sizeof(x),&gx);
  if(read_slot(b,y))
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
  int vx=0,vy=0;
  const char *path;
  FILE *f;
  size_t n;
  if(!allowed_path(a)||!allowed_path(b)||strcmp(a,b)==0||
     !scene||!loop||!campaign)return 0;
  if(read_slot(a,x))
    vx=rikitu_level_save_inspect(x,sizeof(x),&gx);
  if(read_slot(b,y))
    vy=rikitu_level_save_inspect(y,sizeof(y),&gy);
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
