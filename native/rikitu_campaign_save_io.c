#include "rikitu_campaign_save.h"
#include "rikitu_slot_read.h"
#include "rikitu_staged_pair.h"
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
  return rikitu_read_fixed_slot(path,p,RIKITU_CAMPAIGN_SAVE_BYTES);
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
  int rx,ry,vx,vy;
  if(!campaign_path(a)||!campaign_path(b)||!campaign||
     strcmp(a,b)==0)return 0;
  rx=read_slot(a,x);
  if(rx==RIKITU_SLOT_READ_ERROR)return 0;
  ry=read_slot(b,y);
  if(ry==RIKITU_SLOT_READ_ERROR)return 0;
  vx=rx==RIKITU_SLOT_READ_OK&&check(x,&gx);
  vy=ry==RIKITU_SLOT_READ_OK&&check(y,&gy);
  if(!vx&&!vy)return 0;
  if(vy&&(!vx||newer(gy,gx)))
    return rikitu_campaign_save_decode(
        campaign,y,RIKITU_CAMPAIGN_SAVE_BYTES,NULL);
  return rikitu_campaign_save_decode(
      campaign,x,RIKITU_CAMPAIGN_SAVE_BYTES,NULL);
}
static int staged_campaign_validate(const uint8_t *record,size_t size,
                                     void *context,uint32_t *generation){
  RikituCampaign decoded;
  (void)context;
  if(size!=RIKITU_CAMPAIGN_SAVE_BYTES)return 0;
  return rikitu_campaign_save_decode(&decoded,record,size,generation);
}
static int staged_campaign_encode(uint8_t *record,size_t size,
                                  uint32_t generation,void *context){
  const RikituCampaign *campaign=(const RikituCampaign*)context;
  if(size!=RIKITU_CAMPAIGN_SAVE_BYTES||!campaign)return 0;
  return rikitu_campaign_save_encode(record,campaign,generation);
}
int rikitu_campaign_save_store_pair(
    const char *a,const char *b,const RikituCampaign *campaign){
  if(!campaign_path(a)||!campaign_path(b)||!campaign||strcmp(a,b)==0||
     !rikitu_campaign_valid(campaign))return 0;
  return rikitu_staged_pair_commit_checked(a,b,RIKITU_CAMPAIGN_SAVE_BYTES,
                                   staged_campaign_validate,
                                   staged_campaign_encode,
                                   (void*)campaign,NULL);
}
