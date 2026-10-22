#include "rikitu_save.h"
#include "rikitu_slot_read.h"
#include <stdio.h>
#include <string.h>
/* Read one independently checksummed fixed-size slot. */
static int read_slot(const char *path,uint8_t out[RIKITU_SAVE_BYTES]){
  return rikitu_read_fixed_slot(path,out,RIKITU_SAVE_BYTES);
}
static int valid(const uint8_t p[RIKITU_SAVE_BYTES],uint32_t *gen){
  RikituScene s;RikituLoop l;
  return rikitu_save_decode(&s,&l,p,RIKITU_SAVE_BYTES,gen);
}
/* Wrap-safe comparison for monotonically incrementing slot generations. */
static int newer(uint32_t a,uint32_t b){
  return a!=b && (uint32_t)(a-b)<0x80000000u;
}
int rikitu_save_load_pair(const char *a,const char *b,
                          RikituScene *s,RikituLoop *l){
  uint8_t x[RIKITU_SAVE_BYTES],y[RIKITU_SAVE_BYTES];
  uint32_t gx=0,gy=0;
  int rx,ry,vx,vy;
  if(!a||!b||!s||!l)return 0;
  rx=read_slot(a,x);
  if(rx==RIKITU_SLOT_READ_ERROR)return 0;
  ry=read_slot(b,y);
  if(ry==RIKITU_SLOT_READ_ERROR)return 0;
  vx=rx==RIKITU_SLOT_READ_OK&&valid(x,&gx);
  vy=ry==RIKITU_SLOT_READ_OK&&valid(y,&gy);
  if(!vx&&!vy)return 0;
  if(vy&&(!vx||newer(gy,gx)))
    return rikitu_save_decode(s,l,y,sizeof(y),NULL);
  return rikitu_save_decode(s,l,x,sizeof(x),NULL);
}
int rikitu_save_store_pair(const char *a,const char *b,
                           const RikituScene *s,const RikituLoop *l){
  uint8_t x[RIKITU_SAVE_BYTES],y[RIKITU_SAVE_BYTES],data[RIKITU_SAVE_BYTES];
  uint32_t gx=0,gy=0,generation=1;
  int rx,ry,vx,vy;
  const char *target;
  FILE *f;
  size_t written;
  if(!a||!b||!s||!l||strcmp(a,b)==0)return 0;
  rx=read_slot(a,x);
  if(rx==RIKITU_SLOT_READ_ERROR)return 0;
  ry=read_slot(b,y);
  if(ry==RIKITU_SLOT_READ_ERROR)return 0;
  vx=rx==RIKITU_SLOT_READ_OK&&valid(x,&gx);
  vy=ry==RIKITU_SLOT_READ_OK&&valid(y,&gy);
  if(vx&&vy){
    if(newer(gy,gx)){generation=gy+1;target=a;}
    else {generation=gx+1;target=b;}
  }else if(vx){generation=gx+1;target=b;}
  else if(vy){generation=gy+1;target=a;}
  else target=a;
  if(generation==0)generation=1;
  if(!rikitu_save_encode(data,s,l,generation))return 0;
  f=fopen(target,"wb");
  if(!f)return 0;
  written=fwrite(data,1,sizeof(data),f);
  if(written!=sizeof(data)||fflush(f)!=0){
    (void)fclose(f);return 0;
  }
  if(fclose(f)!=0)return 0;
  return 1;
}
