#include "rikitu_save.h"
#include "rikitu_slot_read.h"
#include "rikitu_staged_pair.h"
#include <stdio.h>
#include <string.h>
/* RKS1 is the map/checkpoint format; never let this codec overwrite
 * an RKC2 campaign or RKL3 level slot through a misrouted caller. */
static int save_path(const char *path){
  size_t n;
  if(!path)return 0;
  n=strlen(path);
  return n>4&&strcmp(path+n-4,".rks")==0;
}
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
  if(!save_path(a)||!save_path(b)||strcmp(a,b)==0||!s||!l)return 0;
  rx=read_slot(a,x);
  if(rx==RIKITU_SLOT_READ_ERROR)return 0;
  ry=read_slot(b,y);
  if(ry==RIKITU_SLOT_READ_ERROR)return 0;
  vx=rx==RIKITU_SLOT_READ_OK&&valid(x,&gx);
  vy=ry==RIKITU_SLOT_READ_OK&&valid(y,&gy);
  if(!vx&&!vy)return 0;
  /* Divergent equal-generation records and half-range serials have no safe
   * automatic winner. Return before decoding into caller-owned outputs. */
  if(vx&&vy&&((gx==gy&&memcmp(x,y,sizeof(x))!=0)||
               (uint32_t)(gx-gy)==0x80000000u))return 0;
  if(vy&&(!vx||newer(gy,gx)))
    return rikitu_save_decode(s,l,y,sizeof(y),NULL);
  return rikitu_save_decode(s,l,x,sizeof(x),NULL);
}
typedef struct RikituSaveWriteContext {
  const RikituScene *scene;
  const RikituLoop *loop;
} RikituSaveWriteContext;
static int staged_save_validate(const uint8_t *record,size_t size,
                                void *context,uint32_t *generation){
  RikituScene scene;
  RikituLoop loop;
  (void)context;
  if(size!=RIKITU_SAVE_BYTES)return 0;
  return rikitu_save_decode(&scene,&loop,record,size,generation);
}
static int staged_save_encode(uint8_t *record,size_t size,uint32_t generation,
                              void *context){
  const RikituSaveWriteContext *write_context=
      (const RikituSaveWriteContext*)context;
  if(size!=RIKITU_SAVE_BYTES||!write_context||
     !write_context->scene||!write_context->loop)return 0;
  return rikitu_save_encode(record,write_context->scene,
                            write_context->loop,generation);
}
int rikitu_save_store_pair(const char *a,const char *b,
                           const RikituScene *s,const RikituLoop *l){
  RikituSaveWriteContext context;
  if(!save_path(a)||!save_path(b)||!s||!l||strcmp(a,b)==0)return 0;
  context.scene=s;
  context.loop=l;
  return rikitu_staged_pair_commit_checked(a,b,RIKITU_SAVE_BYTES,
                                   staged_save_validate,staged_save_encode,
                                   &context,NULL);
}
