/* Stage110: a symlink at target.pending cannot truncate either live slot.
 * Linux/POSIX host-only: requires O_NOFOLLOW. Real RKS1/RKC2/RKL3
 * codec encoders/loaders, original A/B orientations, both symlink victims.
 * No linker hooks or fake writes; the caller checks real disk bytes.
 * This protection is NOT claimed for a platform without O_NOFOLLOW.
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
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef O_NOFOLLOW
#error "Stage110 symlink safety tests require POSIX O_NOFOLLOW"
#endif
enum { F_MAP=0,F_CAMPAIGN=1,F_LEVEL=2 };
static size_t record_size(int f){
  return f==F_MAP?RIKITU_SAVE_BYTES:f==F_CAMPAIGN?
         RIKITU_CAMPAIGN_SAVE_BYTES:RIKITU_LEVEL_SAVE_BYTES;
}
static void state(int f,int phase,RikituScene *s,RikituLoop *l){
  int i;
  if(f==F_MAP)rikitu_flower_init(s);
  else rikitu_bridge_init(s);
  memset(l,0,sizeof(*l));l->player=s->player;
  if(phase>0){
    s->reached_checkpoints=phase;
    s->active_checkpoint=s->checkpoints[phase-1];
    s->hearts=3-phase;
    l->active_seconds=(float)(20*phase);
    if(f==F_LEVEL){
      s->stage.coin_count=phase;
      for(i=0;i<phase;i++)s->stage.coins[i]=1;
    }
  }
}
static void make_record(int f,int gen,uint8_t out[RIKITU_LEVEL_SAVE_BYTES]){
  RikituScene s;
  RikituLoop l;
  RikituCampaign c;
  int phase=gen-1;
  assert(phase>=0&&phase<=2);
  assert(rikitu_campaign_init(&c,4));
  c.wallet=157+phase;
  if(f==F_CAMPAIGN){
    assert(rikitu_campaign_save_encode(out,&c,(uint32_t)gen));
  }else{
    state(f,phase,&s,&l);
    if(f==F_MAP)assert(rikitu_save_encode(out,&s,&l,(uint32_t)gen));
    else{
      c.wallet=157;
      assert(rikitu_level_save_encode(out,&s,&l,&c,(uint32_t)gen));
    }
  }
}
static void write_file(const char *p,const uint8_t *bytes,size_t n){
  FILE *f=fopen(p,"wb");
  assert(f);
  assert(fwrite(bytes,1,n,f)==n&&fflush(f)==0);
  assert(fclose(f)==0);
}
static void assert_file(const char *p,const uint8_t *bytes,size_t n){
  uint8_t read[RIKITU_LEVEL_SAVE_BYTES];
  FILE *f=fopen(p,"rb");
  assert(f&&n<=sizeof(read));
  assert(fread(read,1,n,f)==n&&fgetc(f)==EOF);
  assert(fclose(f)==0);
  assert(memcmp(bytes,read,n)==0);
}
static void assert_loaded(int family,const char *a,const char *b,int gen){
  RikituScene s;
  RikituLoop l;
  RikituCampaign c;
  int phase=gen-1;
  if(family==F_CAMPAIGN){
    memset(&c,0,sizeof(c));
    assert(rikitu_campaign_save_load_pair(a,b,&c));
    assert(c.wallet==157+phase&&c.level==4);
  }else{
    state(family,0,&s,&l);
    if(family==F_MAP)assert(rikitu_save_load_pair(a,b,&s,&l));
    else{
      assert(rikitu_campaign_init(&c,4));
      c.wallet=157;
      assert(rikitu_level_save_load_pair(a,b,&s,&l,&c));
      assert(s.stage.coin_count==phase);
    }
    assert(s.reached_checkpoints==phase&&s.hearts==3-phase);
  }
}
static void one_case(int family,int backup_b,int to_backup){
  const char *ext=family==F_MAP?"rks":family==F_CAMPAIGN?"rkc":"rkl";
  char a[240],b[240],pending[240];
  const char *backup,*target,*victim;
  uint8_t gen1[RIKITU_LEVEL_SAVE_BYTES],gen2[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t gen3[RIKITU_LEVEL_SAVE_BYTES];
  size_t n=record_size(family);
  struct stat st;
  int count;
  count=snprintf(a,sizeof(a),"/tmp/rikitu-stage110-%s-a.%s",ext,ext);
  assert(count>0&&count<(int)sizeof(a));
  count=snprintf(b,sizeof(b),"/tmp/rikitu-stage110-%s-b.%s",ext,ext);
  assert(count>0&&count<(int)sizeof(b));
  backup=backup_b?b:a;
  target=backup_b?a:b;
  victim=to_backup?backup:target;
  count=snprintf(pending,sizeof(pending),"%s.pending",target);
  assert(count>0&&count<(int)sizeof(pending));
  (void)remove(pending);(void)remove(a);(void)remove(b);
  make_record(family,1,gen1);
  make_record(family,2,gen2);
  make_record(family,3,gen3);
  write_file(target,gen1,n);
  write_file(backup,gen2,n);
  assert(symlink(victim,pending)==0);
  assert(lstat(pending,&st)==0&&S_ISLNK(st.st_mode));
  /* The writer must FAIL, neither unlink the link nor truncate victim. */
  assert(!rikitu_staged_slot_commit(target,gen3,n));
  assert(lstat(pending,&st)==0&&S_ISLNK(st.st_mode));
  assert_file(target,gen1,n);
  assert_file(backup,gen2,n);
  assert_loaded(family,a,b,2);
  assert(unlink(pending)==0);
  assert(rikitu_staged_slot_commit(target,gen3,n));
  assert(lstat(pending,&st)!=0&&errno==ENOENT);
  assert_file(target,gen3,n);
  assert_file(backup,gen2,n);
  assert_loaded(family,a,b,3);
  (void)remove(a);(void)remove(b);
  printf("PASS Stage110 %s backup=%c symlink->%s: no live truncation, retry gen3\n",
         ext,backup_b?'B':'A',to_backup?"backup":"target");
}
int main(void){
  int family,backup_b,to_backup;
  for(family=F_MAP;family<=F_LEVEL;family++)
    for(backup_b=0;backup_b<2;backup_b++)
      for(to_backup=0;to_backup<2;to_backup++)
        one_case(family,backup_b,to_backup);
  puts("PASS Stage110 12 O_NOFOLLOW symlink-protection cases");
  return 0;
}
