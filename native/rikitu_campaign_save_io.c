#include "rikitu_campaign_save.h"
#include <stdio.h>
#include <string.h>
/* Restrict this new writer to .rkc paths, guarding legacy MAP01 .rks.
 * Rejected paths are never opened, even if the record itself is valid. */
static int campaign_path(const char *p){
  size_t n;
  if(!p)return 0;
  n=strlen(p);
  return n>4 && strcmp(p+n-4,".rkc")==0;
}
static int read_slot(const char *path,
                     uint8_t p[RIKITU_CAMPAIGN_SAVE_BYTES]){
  FILE *f;
  int extra;
  size_t bytes;
  if(!path||!p)return 0;
  f=fopen(path,"rb");
  if(!f)return 0;
  bytes=fread(p,1,RIKITU_CAMPAIGN_SAVE_BYTES,f);
  extra=fgetc(f);
  if(fclose(f)!=0)return 0;
  return bytes==RIKITU_CAMPAIGN_SAVE_BYTES && extra==EOF;
}
static int check(const uint8_t p[RIKITU_CAMPAIGN_SAVE_BYTES],
                 uint32_t *generation){
  RikituCampaign c;
  return rikitu_campaign_save_decode(
      &c,p,RIKITU_CAMPAIGN_SAVE_BYTES,generation);
}
static int newer(uint32_t a,uint32_t b){
  return a!=b && (uint32_t)(a-b)<0x80000000u;
}
int rikitu_campaign_save_load_pair(
    const char *a,const char *b,RikituCampaign *campaign){
  uint8_t x[RIKITU_CAMPAIGN_SAVE_BYTES],y[RIKITU_CAMPAIGN_SAVE_BYTES];
  uint32_t gx=0,gy=0;
  int vx,vy;
  if(!campaign_path(a)||!campaign_path(b)||!campaign||
     strcmp(a,b)==0)return 0;
  vx=read_slot(a,x)&&check(x,&gx);
  vy=read_slot(b,y)&&check(y,&gy);
  if(!vx&&!vy)return 0;
  if(vy&&(!vx||newer(gy,gx)))
    return rikitu_campaign_save_decode(
        campaign,y,RIKITU_CAMPAIGN_SAVE_BYTES,NULL);
  return rikitu_campaign_save_decode(
      campaign,x,RIKITU_CAMPAIGN_SAVE_BYTES,NULL);
}
int rikitu_campaign_save_store_pair(
    const char *a,const char *b,const RikituCampaign *campaign){
  uint8_t x[RIKITU_CAMPAIGN_SAVE_BYTES],y[RIKITU_CAMPAIGN_SAVE_BYTES];
  uint8_t data[RIKITU_CAMPAIGN_SAVE_BYTES];
  uint32_t gx=0,gy=0,gen=1;
  int vx,vy;
  const char *target;
  FILE *f;
  size_t n;
  if(!campaign_path(a)||!campaign_path(b)||!campaign||
     strcmp(a,b)==0||!rikitu_campaign_valid(campaign))return 0;
  vx=read_slot(a,x)&&check(x,&gx);
  vy=read_slot(b,y)&&check(y,&gy);
  if(vx&&vy){
    if(newer(gy,gx)){gen=gy+1;target=a;}
    else{gen=gx+1;target=b;}
  }else if(vx){gen=gx+1;target=b;}
  else if(vy){gen=gy+1;target=a;}
  else target=a;
  if(!gen)gen=1;
  if(!rikitu_campaign_save_encode(data,campaign,gen))return 0;
  f=fopen(target,"wb");
  if(!f)return 0;
  n=fwrite(data,1,sizeof(data),f);
  if(n!=sizeof(data)||fflush(f)!=0){(void)fclose(f);return 0;}
  return fclose(f)==0;
}
