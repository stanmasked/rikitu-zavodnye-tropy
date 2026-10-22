/* Stage109: a fully valid *or unavailable* .pending path is not a live save.
 *
 * Native Linux-host only, experimental staged helper + unmodified real
 * RKS1/RKC2/RKL3 encoders and loaders. For both A/B orientations:
 *  - a CRC-valid higher-gen4 *.pending never wins over live gen2; staging
 *    a verified gen3 replaces it and leaves the gen2 backup untouched.
 *  - if the pending path is an existing directory, stage gen3 must fail
 *    without altering either live file; after removing the obstruction,
 *    retry succeeds and the live loader selects gen3.
 *
 * This does not test simultaneous writers, FAT/SDMC power failure or fsync.
 */
#define _POSIX_C_SOURCE 200809L
#include "rikitu_flower.h"
#include "rikitu_bridge.h"
#include "rikitu_save.h"
#include "rikitu_campaign_save.h"
#include "rikitu_level_save.h"
#include "rikitu_staged_slot.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum { FAMILY_MAP=0,FAMILY_CAMPAIGN=1,FAMILY_LEVEL=2 };
enum { VALID_PENDING=0,BLOCKED_PENDING=1 };

static size_t record_size(int family){
  return family==FAMILY_MAP?RIKITU_SAVE_BYTES:
         family==FAMILY_CAMPAIGN?RIKITU_CAMPAIGN_SAVE_BYTES:
         (size_t)RIKITU_LEVEL_SAVE_BYTES;
}
static void scene_state(int family,int stage,
                        RikituScene *s,RikituLoop *l){
  int i;
  if(family==FAMILY_MAP)rikitu_flower_init(s);
  else rikitu_bridge_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
  if(stage>0){
    s->reached_checkpoints=stage;
    s->active_checkpoint=s->checkpoints[stage-1];
    s->hearts=3-stage;
    l->active_seconds=(float)(stage*20);
    if(family==FAMILY_LEVEL){
      s->stage.coin_count=stage;
      for(i=0;i<stage;i++)s->stage.coins[i]=1;
    }
  }
}
static void encoded(int family,int generation,uint8_t data[RIKITU_LEVEL_SAVE_BYTES]){
  RikituScene s;
  RikituLoop l;
  RikituCampaign c;
  int stage=generation==1?0:generation==2?1:2;
  assert(rikitu_campaign_init(&c,4));
  c.wallet=157;
  if(family==FAMILY_CAMPAIGN){
    c.wallet=157+stage+(generation==4?1:0);
    assert(rikitu_campaign_save_encode(data,&c,(uint32_t)generation));
  }else{
    scene_state(family,stage,&s,&l);
    if(generation==4)s.hearts=2; /* distinguish gen4 */
    if(family==FAMILY_MAP){
      assert(rikitu_save_encode(data,&s,&l,(uint32_t)generation));
    }else{
      assert(rikitu_level_save_encode(data,&s,&l,&c,(uint32_t)generation));
    }
  }
}
static void assert_load(int family,const char *a,const char *b,int generation){
  RikituScene s;
  RikituLoop l;
  RikituCampaign c;
  int stage=generation==2?1:2;
  if(family==FAMILY_CAMPAIGN){
    memset(&c,0,sizeof(c));
    assert(rikitu_campaign_save_load_pair(a,b,&c));
    assert(c.level==4 && c.wallet==157+stage);
  }else{
    scene_state(family,0,&s,&l);
    if(family==FAMILY_MAP){
      assert(rikitu_save_load_pair(a,b,&s,&l));
    }else{
      assert(rikitu_campaign_init(&c,4));
      c.wallet=157;
      assert(rikitu_level_save_load_pair(a,b,&s,&l,&c));
      assert(s.stage.coin_count==stage);
      assert(s.stage.coins[stage-1]==1);
    }
    assert(s.reached_checkpoints==stage && s.hearts==3-stage);
  }
}
static void write_record(const char *path,const uint8_t *bytes,size_t n){
  FILE *f=fopen(path,"wb");
  assert(f);
  assert(fwrite(bytes,1,n,f)==n && fflush(f)==0);
  assert(fclose(f)==0);
}
static void assert_record(const char *path,const uint8_t *expected,size_t n){
  uint8_t actual[RIKITU_LEVEL_SAVE_BYTES];
  FILE *f=fopen(path,"rb");
  assert(f && n<=sizeof(actual));
  assert(fread(actual,1,n,f)==n && fgetc(f)==EOF);
  assert(fclose(f)==0);
  assert(memcmp(actual,expected,n)==0);
}
static void pending_name(char out[240],const char *live_path){
  int n=snprintf(out,240,"%s.pending",live_path);
  assert(n>0 && n<240);
}
static void assert_no_pending(const char *path){
  struct stat s;
  assert(stat(path,&s)!=0 && errno==ENOENT);
}
static void run_case(int family,int backup_b,int mode){
  const char *ext=family==FAMILY_MAP?"rks":
                  family==FAMILY_CAMPAIGN?"rkc":"rkl";
  char a[240],b[240],pending[240];
  const char *older,*newer;
  uint8_t gen1[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t gen2[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t gen3[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t gen4[RIKITU_LEVEL_SAVE_BYTES];
  size_t n=record_size(family);
  assert(snprintf(a,sizeof(a),"/tmp/rikitu-stage109-%s-a.%s",ext,ext)>0);
  assert(snprintf(b,sizeof(b),"/tmp/rikitu-stage109-%s-b.%s",ext,ext)>0);
  older=backup_b?b:a;
  newer=backup_b?a:b;
  pending_name(pending,older);
  (void)remove(pending);(void)rmdir(pending);
  (void)remove(a);(void)remove(b);
  encoded(family,1,gen1);
  encoded(family,2,gen2);
  encoded(family,3,gen3);
  encoded(family,4,gen4);
  write_record(older,gen1,n);
  write_record(newer,gen2,n);
  if(mode==VALID_PENDING){
    write_record(pending,gen4,n);
    assert_record(pending,gen4,n);
    /* The real loader sees only live A/B, not uncommitted gen4. */
    assert_load(family,a,b,2);
    assert_record(older,gen1,n);
    assert_record(newer,gen2,n);
    assert(rikitu_staged_slot_commit(older,gen3,n));
    assert_no_pending(pending);
    assert_record(older,gen3,n);
    assert_record(newer,gen2,n);
    assert_load(family,a,b,3);
  }else{
    assert(mkdir(pending,0700)==0);
    assert(!rikitu_staged_slot_commit(older,gen3,n));
    { struct stat st; assert(stat(pending,&st)==0 && S_ISDIR(st.st_mode)); }
    assert_record(older,gen1,n);
    assert_record(newer,gen2,n);
    assert_load(family,a,b,2);
    assert(rmdir(pending)==0);
    assert(rikitu_staged_slot_commit(older,gen3,n));
    assert_no_pending(pending);
    assert_record(older,gen3,n);
    assert_record(newer,gen2,n);
    assert_load(family,a,b,3);
  }
  (void)remove(a);(void)remove(b);
  puts(mode==VALID_PENDING?
       "PASS Stage109 uncommitted complete record ignored, replaced safely":
       "PASS Stage109 blocked pending preserved both slots, retry succeeded");
}
int main(void){
  int family,backup_b,mode;
  for(family=FAMILY_MAP;family<=FAMILY_LEVEL;family++)
    for(backup_b=0;backup_b<=1;backup_b++)
      for(mode=VALID_PENDING;mode<=BLOCKED_PENDING;mode++)
        run_case(family,backup_b,mode);
  puts("PASS Stage109: 12 pending isolation and guarded retry scenarios");
  return 0;
}
