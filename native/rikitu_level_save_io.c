#include "rikitu_level_save.h"
#include "rikitu_slot_read.h"
#include "rikitu_staged_pair.h"
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
typedef struct RikituLevelWriteContext {
  const RikituScene *scene;
  const RikituLoop *loop;
  const RikituCampaign *campaign;
} RikituLevelWriteContext;
static int staged_level_validate(const uint8_t *record,size_t size,
                                 void *context,uint32_t *generation){
  const RikituLevelWriteContext *write_context=
      (const RikituLevelWriteContext*)context;
  RikituScene candidate;
  RikituLoop candidate_loop;
  if(size!=RIKITU_LEVEL_SAVE_BYTES||!write_context||
     !write_context->scene||!write_context->campaign)return 0;
  /* Inspecting only the CRC/header is not enough for slot selection:
   * a CRC-valid record with impossible boss/lever state could otherwise
   * outrank the last fully restorable checkpoint. Decode into a scratch
   * copy of the current level geometry, clearing only the fresh-scene
   * counters required by the decoder. The decoder replaces saved state
   * in this copy and leaves the live scene untouched. */
  candidate=*write_context->scene;
  candidate.stage.coin_count=0;
  candidate.stage.fragment_count=0;
  candidate.reached_checkpoints=0;
  candidate.damage_count=0;
  memset(&candidate_loop,0,sizeof(candidate_loop));
  return rikitu_level_save_decode(&candidate,&candidate_loop,
                                  write_context->campaign,record,size,
                                  generation)&&
         belongs_to_campaign(record,write_context->campaign);
}
static int staged_level_encode(uint8_t *record,size_t size,
                               uint32_t generation,void *context){
  const RikituLevelWriteContext *write_context=
      (const RikituLevelWriteContext*)context;
  if(size!=RIKITU_LEVEL_SAVE_BYTES||!write_context||
     !write_context->scene||!write_context->loop||
     !write_context->campaign)return 0;
  return rikitu_level_save_encode(record,write_context->scene,
                                  write_context->loop,
                                  write_context->campaign,generation);
}
int rikitu_level_save_store_pair(const char *a,const char *b,
                                  const RikituScene *scene,
                                  const RikituLoop *loop,
                                  const RikituCampaign *campaign){
  RikituLevelWriteContext context;
  if(!allowed_path(a)||!allowed_path(b)||strcmp(a,b)==0||
     !scene||!loop||!campaign)return 0;
  context.scene=scene;
  context.loop=loop;
  context.campaign=campaign;
  return rikitu_staged_pair_commit_checked(a,b,RIKITU_LEVEL_SAVE_BYTES,
                                   staged_level_validate,
                                   staged_level_encode,&context,NULL);
}
