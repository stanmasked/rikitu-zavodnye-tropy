/* Stage114: optimistic expected-generation compare under A/B pair lock.
 *
 * Three authentic save codecs x two A/B orientations x three histories:
 *   - gen1/gen2 -> gen3; stale gen2 writer rejected; fresh gen3 -> gen4
 *   - wrap UINT32_MAX/gen1; stale expected UINT32_MAX rejected;
 *     expected gen1 writes gen2 into older slot
 *   - no valid records; expected 0 creates gen1; duplicate expected 0
 *     rejected; fresh expected gen1 writes gen2.
 *
 * Rejected encoders are NEVER CALLED, .pending is not created, and both
 * live files stay byte-exact. Real production loaders verify outcomes.
 * Linux-only test of experimental pair writer. No shipping save I/O change.
 */
#define _POSIX_C_SOURCE 200809L
#include "rikitu_staged_pair.h"
#include "rikitu_save.h"
#include "rikitu_campaign_save.h"
#include "rikitu_level_save.h"
#include "rikitu_flower.h"
#include "rikitu_bridge.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#if defined(__linux__)
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <time.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum { MAP=0,CAMPAIGN=1,LEVEL=2,STANDARD=0,WRAP=1,EMPTY=2 };
typedef struct {
  int family,phase,encode_calls;
  RikituCampaign campaign;
} Context;
static size_t bytes_for(int f){
  return f==MAP?RIKITU_SAVE_BYTES:
         f==CAMPAIGN?RIKITU_CAMPAIGN_SAVE_BYTES:RIKITU_LEVEL_SAVE_BYTES;
}
static void create_scene(int f,int phase,RikituScene *s,RikituLoop *l){
  int i;
  if(f==MAP)rikitu_flower_init(s);else rikitu_bridge_init(s);
  memset(l,0,sizeof(*l));l->player=s->player;
  if(phase){
    s->reached_checkpoints=phase;
    s->active_checkpoint=s->checkpoints[phase-1];
    s->hearts=3-phase;
    if(f==LEVEL){
      s->stage.coin_count=phase;
      for(i=0;i<phase;i++)s->stage.coins[i]=1;
    }
  }
}
static int record_encode(uint8_t *out,size_t n,uint32_t gen,void *opaque){
  Context *ctx=(Context*)opaque;
  RikituScene s;
  RikituLoop l;
  RikituCampaign c=ctx->campaign;
  ctx->encode_calls++;
  if(n!=bytes_for(ctx->family))return 0;
  if(ctx->family==CAMPAIGN){
    c.wallet=157+ctx->phase;
    return rikitu_campaign_save_encode(out,&c,gen);
  }
  create_scene(ctx->family,ctx->phase,&s,&l);
  if(ctx->family==MAP)return rikitu_save_encode(out,&s,&l,gen);
  c.wallet=157;
  return rikitu_level_save_encode(out,&s,&l,&c,gen);
}
static int record_validate(const uint8_t *bytes,size_t n,
                           void *opaque,uint32_t *gen){
  Context *ctx=(Context*)opaque;
  RikituScene s;
  RikituLoop l;
  RikituCampaign c=ctx->campaign;
  if(ctx->family==MAP){
    create_scene(MAP,0,&s,&l);
    return rikitu_save_decode(&s,&l,bytes,n,gen);
  }
  if(ctx->family==CAMPAIGN)
    return rikitu_campaign_save_decode(&c,bytes,n,gen);
  create_scene(LEVEL,0,&s,&l);
  return rikitu_level_save_decode(&s,&l,&c,bytes,n,gen);
}
static void generate(Context *ctx,int phase,uint32_t gen,uint8_t *out){
  int before=ctx->phase;
  ctx->phase=phase;
  assert(record_encode(out,bytes_for(ctx->family),gen,ctx));
  ctx->phase=before;
}
static void write_bytes(const char *path,const uint8_t *bytes,size_t n){
  FILE *f=fopen(path,"wb");
  assert(f);
  assert(fwrite(bytes,1,n,f)==n&&fflush(f)==0);
  assert(fclose(f)==0);
}
static void assert_bytes(const char *path,const uint8_t *expected,size_t n){
  FILE *f=fopen(path,"rb");
  uint8_t got[RIKITU_LEVEL_SAVE_BYTES];
  assert(f&&n<=sizeof(got));
  assert(fread(got,1,n,f)==n&&fgetc(f)==EOF);
  assert(fclose(f)==0);
  assert(memcmp(expected,got,n)==0);
}
static void check_loader(Context *ctx,const char *a,const char *b,int phase){
  RikituScene s;
  RikituLoop l;
  RikituCampaign c=ctx->campaign;
  if(ctx->family==CAMPAIGN){
    assert(rikitu_campaign_save_load_pair(a,b,&c));
    assert(c.level==4&&c.wallet==157+phase);
  }else{
    create_scene(ctx->family,0,&s,&l);
    if(ctx->family==MAP)
      assert(rikitu_save_load_pair(a,b,&s,&l));
    else{
      assert(rikitu_level_save_load_pair(a,b,&s,&l,&c));
      assert(s.stage.coin_count==phase);
    }
    assert(s.reached_checkpoints==phase&&s.hearts==3-phase);
  }
}
static void create_paths(int f,char *a,char *b){
  const char *ext=f==MAP?"rks":f==CAMPAIGN?"rkc":"rkl";
  int count=snprintf(a,200,"/tmp/rikitu-stage114-%s-a.%s",ext,ext);
  assert(count>0&&count<200);
  count=snprintf(b,200,"/tmp/rikitu-stage114-%s-b.%s",ext,ext);
  assert(count>0&&count<200);
}
static void clean_path(const char *path){
  char s[240];
  int count;
  (void)remove(path);
  count=snprintf(s,sizeof(s),"%s.pending",path);
  assert(count>0&&count<(int)sizeof(s));(void)remove(s);
  count=snprintf(s,sizeof(s),"%s.pending.lock",path);
  assert(count>0&&count<(int)sizeof(s));(void)remove(s);
  count=snprintf(s,sizeof(s),"%s.pair.lock",path);
  assert(count>0&&count<(int)sizeof(s));(void)remove(s);
}
static void no_pending(const char *a,const char *b){
  struct stat st;
  char path[240];
  int n=snprintf(path,sizeof(path),"%s.pending",a);
  assert(n>0&&n<(int)sizeof(path));
  assert(lstat(path,&st)!=0&&errno==ENOENT);
  n=snprintf(path,sizeof(path),"%s.pending",b);
  assert(n>0&&n<(int)sizeof(path));
  assert(lstat(path,&st)!=0&&errno==ENOENT);
}
static void reject_stale(Context *ctx,const char *a,const char *b,
                         uint32_t stale_generation){
  uint32_t reported=0xf00df00du;
  int called=ctx->encode_calls;
  /* Reversal tests canonical lock identity in the same operation. */
  assert(!rikitu_staged_pair_commit_if_current(
    b,a,bytes_for(ctx->family),record_validate,record_encode,ctx,
    stale_generation,&reported));
  assert(reported==0xf00df00du);
  assert(ctx->encode_calls==called);
  no_pending(a,b);
}
static void run_case(int family,int initial_b,int scenario){
  char a[200],b[200];
  const char *newer=initial_b?b:a;
  const char *older=initial_b?a:b;
  uint8_t old[RIKITU_LEVEL_SAVE_BYTES],recent[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t next[RIKITU_LEVEL_SAVE_BYTES],last[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t reported=0;
  Context ctx;
  size_t n=bytes_for(family);
  memset(&ctx,0,sizeof(ctx));
  ctx.family=family;
  assert(rikitu_campaign_init(&ctx.campaign,4));
  ctx.campaign.wallet=157;
  create_paths(family,a,b);
  clean_path(a);clean_path(b);
  if(scenario==STANDARD){
    generate(&ctx,0,1,old);
    generate(&ctx,1,2,recent);
    generate(&ctx,2,3,next);
    generate(&ctx,1,4,last);
    write_bytes(older,old,n);
    write_bytes(newer,recent,n);
    ctx.phase=2;
    assert(rikitu_staged_pair_commit_if_current(
      a,b,n,record_validate,record_encode,&ctx,2,&reported));
    assert(reported==3);
    assert_bytes(older,next,n);
    assert_bytes(newer,recent,n);
    check_loader(&ctx,a,b,2);
    ctx.phase=0; /* stale progress must NEVER supersede gen3. */
    reject_stale(&ctx,a,b,2);
    assert_bytes(older,next,n);
    assert_bytes(newer,recent,n);
    ctx.phase=1;
    assert(rikitu_staged_pair_commit_if_current(
      a,b,n,record_validate,record_encode,&ctx,3,&reported));
    assert(reported==4);
    assert_bytes(older,next,n);
    assert_bytes(newer,last,n);
    check_loader(&ctx,a,b,1);
  }else if(scenario==WRAP){
    generate(&ctx,0,UINT32_MAX,old);
    generate(&ctx,1,1,recent);
    generate(&ctx,2,2,next);
    write_bytes(older,old,n);
    write_bytes(newer,recent,n);
    ctx.phase=0;
    reject_stale(&ctx,a,b,UINT32_MAX);
    assert_bytes(older,old,n);
    assert_bytes(newer,recent,n);
    ctx.phase=2;
    assert(rikitu_staged_pair_commit_if_current(
      a,b,n,record_validate,record_encode,&ctx,1,&reported));
    assert(reported==2);
    assert_bytes(older,next,n);
    assert_bytes(newer,recent,n);
    check_loader(&ctx,a,b,2);
  }else{
    ctx.phase=0;
    generate(&ctx,0,1,old);
    generate(&ctx,1,2,recent);
    /* First write is gen1 even when zero valid slots exist. */
    assert(rikitu_staged_pair_commit_if_current(
      a,b,n,record_validate,record_encode,&ctx,0,&reported));
    assert(reported==1);
    /* With empty pair, the staged implementation initially chooses A. */
    assert_bytes(a,old,n);
    ctx.phase=2;
    reject_stale(&ctx,a,b,0);
    assert_bytes(a,old,n);
    ctx.phase=1;
    assert(rikitu_staged_pair_commit_if_current(
      a,b,n,record_validate,record_encode,&ctx,1,&reported));
    assert(reported==2);
    assert_bytes(b,recent,n);
    check_loader(&ctx,a,b,1);
  }
  no_pending(a,b);
  clean_path(a);clean_path(b);
  printf("PASS Stage114 family=%d newer=%c case=%d guarded generations\n",
         family,initial_b?'B':'A',scenario);
}
/* Stage115: two valid A/B records can be inconclusive even when the
 * caller's expected generation equals one of their decoded generations.
 * Fail closed on conflicting equal-generation payloads and serial numbers
 * exactly 2^31 apart; allow byte-identical duplicates. */
enum { SPLIT=0, HALFWAY=1, DUPLICATE=2 };
static void run_ambiguity_case(int family,int reversed,int scenario){
  char a[200],b[200];
  const char *first,*second;
  uint8_t one[RIKITU_LEVEL_SAVE_BYTES],two[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t successor[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t ga=0,gb=0,reported=0xf00df00du;
  Context ctx;
  int called;
  size_t n=bytes_for(family);
  memset(&ctx,0,sizeof(ctx));
  ctx.family=family;
  assert(rikitu_campaign_init(&ctx.campaign,4));
  ctx.campaign.wallet=157;
  create_paths(family,a,b);
  clean_path(a);clean_path(b);
  generate(&ctx,0,8u,one);
  if(scenario==HALFWAY)
    generate(&ctx,1,0x80000008u,two);
  else if(scenario==SPLIT)
    generate(&ctx,1,8u,two);
  else
    memcpy(two,one,n);
  generate(&ctx,2,9u,successor);
  write_bytes(a,one,n);
  write_bytes(b,two,n);
  /* Both slots independently pass their authentic CRC/semantic decoder. */
  assert(record_validate(one,n,&ctx,&ga));
  assert(record_validate(two,n,&ctx,&gb));
  assert(ga==8u);
  assert(gb==(scenario==HALFWAY?0x80000008u:8u));
  first=reversed?b:a;
  second=reversed?a:b;
  ctx.phase=2;
  called=ctx.encode_calls;
  if(scenario==DUPLICATE){
    assert(rikitu_staged_pair_commit_if_current(
      first,second,n,record_validate,record_encode,&ctx,
      8u,&reported));
    assert(reported==9u&&ctx.encode_calls==called+1);
    assert_bytes(first,one,n);
    assert_bytes(second,successor,n);
    check_loader(&ctx,a,b,2);
  }else{
    assert(!rikitu_staged_pair_commit_if_current(
      first,second,n,record_validate,record_encode,&ctx,
      8u,&reported));
    assert(reported==0xf00df00du&&ctx.encode_calls==called);
    assert_bytes(a,one,n);
    assert_bytes(b,two,n);
  }
  no_pending(a,b);
  clean_path(a);clean_path(b);
  printf("PASS Stage115 family=%d reversed=%d ambiguity=%d\n",
         family,reversed,scenario);
}
/* Stage116: deny live slot symlinks, directories, and FIFOs, instead of
 * silently classifying read/path errors as an absent or corrupt backup.
 * After explicit cleanup, ordinary guarded saves must still succeed. */
enum { SLOT_LINK=0, SLOT_DIRECTORY=1, SLOT_FIFO=2 };
static void run_path_guard_case(int family,int bad_b,int kind){
  char a[200],b[200];
  const char *bad,*good;
  uint8_t old[RIKITU_LEVEL_SAVE_BYTES],recent[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t successor[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t reported=0xf00df00du;
  struct stat st;
  Context ctx;
  int called;
  size_t n=bytes_for(family);
  memset(&ctx,0,sizeof(ctx));
  ctx.family=family;
  assert(rikitu_campaign_init(&ctx.campaign,4));
  ctx.campaign.wallet=157;
  create_paths(family,a,b);
  clean_path(a);clean_path(b);
  bad=bad_b?b:a;
  good=bad_b?a:b;
  generate(&ctx,0,1u,old);
  generate(&ctx,1,2u,recent);
  generate(&ctx,2,3u,successor);
  write_bytes(good,recent,n);
  if(kind==SLOT_LINK)assert(symlink(good,bad)==0);
  else if(kind==SLOT_DIRECTORY)assert(mkdir(bad,0700)==0);
  else assert(mkfifo(bad,0600)==0);
  assert(lstat(bad,&st)==0);
  ctx.phase=2;
  called=ctx.encode_calls;
  assert(!rikitu_staged_pair_commit_if_current(
    b,a,n,record_validate,record_encode,&ctx,2u,&reported));
  assert(reported==0xf00df00du);
  assert(ctx.encode_calls==called);
  assert_bytes(good,recent,n);
  assert(lstat(bad,&st)==0);
  if(kind==SLOT_LINK)assert(S_ISLNK(st.st_mode));
  else if(kind==SLOT_DIRECTORY)assert(S_ISDIR(st.st_mode));
  else assert(S_ISFIFO(st.st_mode));
  no_pending(a,b);
  if(kind==SLOT_DIRECTORY)assert(rmdir(bad)==0);
  else assert(unlink(bad)==0);
  write_bytes(bad,old,n);
  reported=0;
  assert(rikitu_staged_pair_commit_if_current(
    a,b,n,record_validate,record_encode,&ctx,2u,&reported));
  assert(reported==3u);
  assert_bytes(good,recent,n);
  assert_bytes(bad,successor,n);
  check_loader(&ctx,a,b,2);
  no_pending(a,b);
  clean_path(a);clean_path(b);
  printf("PASS Stage116 family=%d unsafe=%c kind=%d\n",
         family,bad_b?'B':'A',kind);
}
/* Stage117: an existing hard link at a temporary/lock path must never
 * allow O_TRUNC to clobber a live backup. Reject odd inode types too,
 * preserve the original bytes, then recover after explicit cleanup. */
enum {
  PENDING_HARDLINK=0,PENDING_LOCK_HARDLINK=1,PAIR_LOCK_HARDLINK=2,
  PENDING_FIFO=3,PENDING_DIRECTORY=4,PAIR_LOCK_FIFO=5
};
static void run_staging_inode_case(int family,int newer_b,int kind){
  char a[200],b[200],obstruction[240];
  const char *recent_path,*target,*source;
  uint8_t old[RIKITU_LEVEL_SAVE_BYTES],recent[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t successor[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t reported=0xf00df00du;
  Context ctx;
  struct stat st;
  int called,len;
  size_t n=bytes_for(family);
  memset(&ctx,0,sizeof(ctx));
  ctx.family=family;
  assert(rikitu_campaign_init(&ctx.campaign,4));
  ctx.campaign.wallet=157;
  create_paths(family,a,b);
  clean_path(a);clean_path(b);
  recent_path=newer_b?b:a;
  target=newer_b?a:b;
  generate(&ctx,0,1u,old);
  generate(&ctx,1,2u,recent);
  generate(&ctx,2,3u,successor);
  write_bytes(target,old,n);
  write_bytes(recent_path,recent,n);
  source=(kind==PAIR_LOCK_HARDLINK||kind==PAIR_LOCK_FIFO)?a:target;
  len=snprintf(obstruction,sizeof(obstruction),"%s%s",source,
      (kind==PENDING_HARDLINK||kind==PENDING_FIFO||
       kind==PENDING_DIRECTORY)?".pending":
      kind==PENDING_LOCK_HARDLINK?".pending.lock":".pair.lock");
  assert(len>0&&len<(int)sizeof(obstruction));
  if(kind<=PAIR_LOCK_HARDLINK)
    assert(link(recent_path,obstruction)==0);
  else if(kind==PENDING_DIRECTORY)
    assert(mkdir(obstruction,0700)==0);
  else
    assert(mkfifo(obstruction,0600)==0);
  ctx.phase=2;
  called=ctx.encode_calls;
  assert(!rikitu_staged_pair_commit_if_current(
      b,a,n,record_validate,record_encode,&ctx,2u,&reported));
  assert(reported==0xf00df00du);
  /* Pair lock rejects before encoding. Target staging guards reject
   * after the codec has produced in-memory bytes, but before commit. */
  assert(ctx.encode_calls==called+
    ((kind==PAIR_LOCK_HARDLINK||kind==PAIR_LOCK_FIFO)?0:1));
  assert_bytes(target,old,n);
  assert_bytes(recent_path,recent,n);
  assert(lstat(obstruction,&st)==0);
  if(kind<=PAIR_LOCK_HARDLINK){
    assert(S_ISREG(st.st_mode));
    assert(st.st_nlink>=2);
    assert_bytes(obstruction,recent,n);
  }else if(kind==PENDING_DIRECTORY){
    assert(S_ISDIR(st.st_mode));
  }else assert(S_ISFIFO(st.st_mode));
  if(kind==PENDING_DIRECTORY)assert(rmdir(obstruction)==0);
  else assert(unlink(obstruction)==0);
  no_pending(a,b);
  reported=0;
  assert(rikitu_staged_pair_commit_if_current(
      a,b,n,record_validate,record_encode,&ctx,2u,&reported));
  assert(reported==3u);
  assert_bytes(target,successor,n);
  assert_bytes(recent_path,recent,n);
  check_loader(&ctx,a,b,2);
  no_pending(a,b);
  clean_path(a);clean_path(b);
  printf("PASS Stage117 family=%d newer=%c kind=%d\n",
         family,newer_b?'B':'A',kind);
}
/* Stage117 positive recovery: a left-over ordinary .pending file is
 * safe to truncate in place after inode inspection. */
static void run_stale_pending_regular_case(int family,int newer_b){
  char a[200],b[200],pending[240];
  const char *recent_path,*target;
  uint8_t old[RIKITU_LEVEL_SAVE_BYTES],recent[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t successor[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t reported=0;
  Context ctx;
  int len;
  size_t n=bytes_for(family);
  memset(&ctx,0,sizeof(ctx));
  ctx.family=family;
  assert(rikitu_campaign_init(&ctx.campaign,4));
  ctx.campaign.wallet=157;
  create_paths(family,a,b);
  clean_path(a);clean_path(b);
  recent_path=newer_b?b:a;
  target=newer_b?a:b;
  generate(&ctx,0,1u,old);
  generate(&ctx,1,2u,recent);
  generate(&ctx,2,3u,successor);
  write_bytes(target,old,n);
  write_bytes(recent_path,recent,n);
  len=snprintf(pending,sizeof(pending),"%s.pending",target);
  assert(len>0&&len<(int)sizeof(pending));
  write_bytes(pending,(const uint8_t*)"incomplete",10u);
  ctx.phase=2;
  assert(rikitu_staged_pair_commit_if_current(
      a,b,n,record_validate,record_encode,&ctx,2u,&reported));
  assert(reported==3u);
  assert_bytes(target,successor,n);
  assert_bytes(recent_path,recent,n);
  check_loader(&ctx,a,b,2);
  no_pending(a,b);
  clean_path(a);clean_path(b);
  printf("PASS Stage117 recovery family=%d newer=%c\n",
         family,newer_b?'B':'A');
}
/* Stage118: reject non-regular stage/lock paths and preserve them.
 * lstat-based no_pending() also detects dangling .pending symlinks. */
enum { PENDING_LINK=0,PENDING_DANGLING_LINK=1,
       PENDING_LOCK_FIFO=2,PAIR_LOCK_DIRECTORY=3 };
static void run_extra_obstruction_case(int family,int newer_b,int kind){
  char a[200],b[200],obstruction[240],missing[240];
  const char *recent_path,*target,*base,*suffix;
  uint8_t old[RIKITU_LEVEL_SAVE_BYTES],recent[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t successor[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t reported=0xf00df00du;
  Context ctx;
  struct stat st;
  int called,len;
  size_t n=bytes_for(family);
  memset(&ctx,0,sizeof(ctx));
  ctx.family=family;
  assert(rikitu_campaign_init(&ctx.campaign,4));
  ctx.campaign.wallet=157;
  create_paths(family,a,b);
  clean_path(a);clean_path(b);
  recent_path=newer_b?b:a;
  target=newer_b?a:b;
  generate(&ctx,0,1u,old);
  generate(&ctx,1,2u,recent);
  generate(&ctx,2,3u,successor);
  write_bytes(target,old,n);
  write_bytes(recent_path,recent,n);
  base=kind==PAIR_LOCK_DIRECTORY?a:target;
  suffix=kind<=PENDING_DANGLING_LINK?".pending":
         kind==PENDING_LOCK_FIFO?".pending.lock":".pair.lock";
  len=snprintf(obstruction,sizeof(obstruction),"%s%s",base,suffix);
  assert(len>0&&len<(int)sizeof(obstruction));
  if(kind==PENDING_LINK)assert(symlink(recent_path,obstruction)==0);
  else if(kind==PENDING_DANGLING_LINK){
    len=snprintf(missing,sizeof(missing),"%s.absent",target);
    assert(len>0&&len<(int)sizeof(missing));
    assert(lstat(missing,&st)!=0&&errno==ENOENT);
    assert(symlink(missing,obstruction)==0);
  }else if(kind==PENDING_LOCK_FIFO)
    assert(mkfifo(obstruction,0600)==0);
  else assert(mkdir(obstruction,0700)==0);
  ctx.phase=2;
  called=ctx.encode_calls;
  assert(!rikitu_staged_pair_commit_if_current(
      b,a,n,record_validate,record_encode,&ctx,2u,&reported));
  assert(reported==0xf00df00du);
  /* Unsafe .pair.lock rejects pre-encode; all .pending-related
   * obstructions reject in the later staged writer. */
  assert(ctx.encode_calls==called+(kind==PAIR_LOCK_DIRECTORY?0:1));
  assert_bytes(target,old,n);
  assert_bytes(recent_path,recent,n);
  assert(lstat(obstruction,&st)==0);
  if(kind<=PENDING_DANGLING_LINK)assert(S_ISLNK(st.st_mode));
  else if(kind==PENDING_LOCK_FIFO)assert(S_ISFIFO(st.st_mode));
  else assert(S_ISDIR(st.st_mode));
  if(kind==PAIR_LOCK_DIRECTORY)assert(rmdir(obstruction)==0);
  else assert(unlink(obstruction)==0);
  no_pending(a,b);
  reported=0;
  assert(rikitu_staged_pair_commit_if_current(
      a,b,n,record_validate,record_encode,&ctx,2u,&reported));
  assert(reported==3u);
  assert_bytes(target,successor,n);
  assert_bytes(recent_path,recent,n);
  check_loader(&ctx,a,b,2);
  no_pending(a,b);
  clean_path(a);clean_path(b);
  printf("PASS Stage118 family=%d newer=%c kind=%d\n",
         family,newer_b?'B':'A',kind);
}
#if defined(__linux__)
/* Stage119: hold an actual Linux flock in the parent, wait until a
 * child has opened the SAME inode, then rename the pathname and place
 * a fresh inode at its original name. The child must refuse to write
 * when flock finally succeeds on its now-stale descriptor. */
static int child_has_inode(pid_t pid,const struct stat *expected){
  char dirname[80],path[160];
  DIR *d;
  struct dirent *e;
  struct stat st;
  int found=0,len;
  len=snprintf(dirname,sizeof(dirname),"/proc/%ld/fd",(long)pid);
  assert(len>0&&len<(int)sizeof(dirname));
  d=opendir(dirname);
  if(!d)return 0;
  while((e=readdir(d))!=NULL){
    if(e->d_name[0]=='.')continue;
    len=snprintf(path,sizeof(path),"%s/%s",dirname,e->d_name);
    if(len<0||len>=(int)sizeof(path))continue;
    if(stat(path,&st)==0&&st.st_dev==expected->st_dev&&
       st.st_ino==expected->st_ino){found=1;break;}
  }
  assert(closedir(d)==0);
  return found;
}
static void run_lock_swap_case(int family,int newer_b,int pair_lock){
  char a[200],b[200],lock_path[240],rotated[280];
  const char *newest,*target,*base;
  uint8_t old[RIKITU_LEVEL_SAVE_BYTES],recent[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t successor[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t reported=0;
  Context ctx;
  struct stat locked_stat;
  struct timespec interval={0,1000000L};
  int held,replace_fd,ready[2],len,found=0,status,attempt;
  char signal_byte;
  pid_t child;
  size_t n=bytes_for(family);
  memset(&ctx,0,sizeof(ctx));
  ctx.family=family;
  assert(rikitu_campaign_init(&ctx.campaign,4));
  ctx.campaign.wallet=157;
  create_paths(family,a,b);
  clean_path(a);clean_path(b);
  newest=newer_b?b:a;
  target=newer_b?a:b;
  generate(&ctx,0,1u,old);
  generate(&ctx,1,2u,recent);
  generate(&ctx,2,3u,successor);
  write_bytes(target,old,n);
  write_bytes(newest,recent,n);
  base=pair_lock?a:target;
  len=snprintf(lock_path,sizeof(lock_path),"%s%s",base,
               pair_lock?".pair.lock":".pending.lock");
  assert(len>0&&len<(int)sizeof(lock_path));
  len=snprintf(rotated,sizeof(rotated),"%s.oldinode",lock_path);
  assert(len>0&&len<(int)sizeof(rotated));
  (void)unlink(rotated);
  held=open(lock_path,O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
  assert(held>=0);
  assert(fstat(held,&locked_stat)==0);
  assert(flock(held,LOCK_EX)==0);
  assert(pipe(ready)==0);
  child=fork();
  assert(child>=0);
  if(child==0){
    int result,before;
    uint32_t child_reported=0xf00df00du;
    /* Close the inherited OFD *before* reporting readiness. Otherwise
     * /proc scanning could see this inherited descriptor rather than
     * the new descriptor that is really blocked on flock. */
    (void)close(held);
    (void)close(ready[0]);
    if(write(ready[1],"R",1)!=1)_exit(10);
    (void)close(ready[1]);
    ctx.phase=2;
    before=ctx.encode_calls;
    result=rikitu_staged_pair_commit_if_current(
      b,a,n,record_validate,record_encode,&ctx,2u,&child_reported);
    if(result||child_reported!=0xf00df00du||
       ctx.encode_calls!=before+(pair_lock?0:1))_exit(11);
    _exit(0);
  }
  (void)close(ready[1]);
  assert(read(ready[0],&signal_byte,1)==1&&signal_byte=='R');
  (void)close(ready[0]);
  for(attempt=0;attempt<3000;attempt++){
    if(child_has_inode(child,&locked_stat)){found=1;break;}
    (void)nanosleep(&interval,NULL);
  }
  if(!found){
    (void)kill(child,SIGKILL);
    (void)waitpid(child,&status,0);
    assert(found&&"child never opened the blocked lock inode");
  }
  assert(rename(lock_path,rotated)==0);
  replace_fd=open(lock_path,O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
  assert(replace_fd>=0);
  assert(close(replace_fd)==0);
  assert(flock(held,LOCK_UN)==0);
  assert(close(held)==0);
  assert(waitpid(child,&status,0)==child);
  assert(WIFEXITED(status)&&WEXITSTATUS(status)==0);
  assert_bytes(target,old,n);
  assert_bytes(newest,recent,n);
  no_pending(a,b);
  /* Explicit operator cleanup after child exit permits a normal
   * guarded save; production code never unlinks its lock files. */
  assert(unlink(lock_path)==0);
  assert(unlink(rotated)==0);
  ctx.phase=2;
  assert(rikitu_staged_pair_commit_if_current(
    a,b,n,record_validate,record_encode,&ctx,2u,&reported));
  assert(reported==3u);
  assert_bytes(target,successor,n);
  assert_bytes(newest,recent,n);
  check_loader(&ctx,a,b,2);
  no_pending(a,b);
  clean_path(a);clean_path(b);
  printf("PASS Stage119 family=%d newer=%c lock=%s\n",
         family,newer_b?'B':'A',pair_lock?"pair":"pending");
}
#endif
/* Stage120: reject alternate spellings of a slot path and reject A/B
 * entries that are hard links to the same inode. These violate the
 * two-slot stable-path contract and must fail before codec encoding. */
enum { DOT_ALIAS=0,PARENT_SYMLINK_ALIAS=1 };
static void run_noncanonical_path_case(int family,int alias_b,int kind){
  char a[200],b[200],alias_path[260],alias_dir[100],lock_path[240];
  const char *base_path,*leaf;
  const char *arg_a,*arg_b;
  uint8_t old[RIKITU_LEVEL_SAVE_BYTES],recent[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t successor[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t reported=0xf00df00du;
  Context ctx;
  struct stat st;
  int len,called;
  size_t n=bytes_for(family);
  memset(&ctx,0,sizeof(ctx));
  ctx.family=family;
  assert(rikitu_campaign_init(&ctx.campaign,4));
  ctx.campaign.wallet=157;
  create_paths(family,a,b);
  clean_path(a);clean_path(b);
  generate(&ctx,0,1u,old);
  generate(&ctx,1,2u,recent);
  generate(&ctx,2,3u,successor);
  write_bytes(a,old,n);
  write_bytes(b,recent,n);
  base_path=alias_b?b:a;
  leaf=strrchr(base_path,'/');
  assert(leaf&&leaf[1]);
  if(kind==DOT_ALIAS){
    len=snprintf(alias_path,sizeof(alias_path),"/tmp/./%s",leaf+1);
    assert(len>0&&len<(int)sizeof(alias_path));
  }else{
    (void)unlink("/tmp/rikitu-stage120-parent-alias");
    assert(symlink("/tmp","/tmp/rikitu-stage120-parent-alias")==0);
    len=snprintf(alias_dir,sizeof(alias_dir),
                 "/tmp/rikitu-stage120-parent-alias");
    assert(len>0&&len<(int)sizeof(alias_dir));
    len=snprintf(alias_path,sizeof(alias_path),"%s/%s",alias_dir,leaf+1);
    assert(len>0&&len<(int)sizeof(alias_path));
  }
  arg_a=alias_b?a:alias_path;
  arg_b=alias_b?alias_path:b;
  len=snprintf(lock_path,sizeof(lock_path),"%s.pair.lock",a);
  assert(len>0&&len<(int)sizeof(lock_path));
  ctx.phase=2;
  called=ctx.encode_calls;
  assert(!rikitu_staged_pair_commit_if_current(
      arg_a,arg_b,n,record_validate,record_encode,&ctx,2u,&reported));
  assert(reported==0xf00df00du&&ctx.encode_calls==called);
  assert_bytes(a,old,n);
  assert_bytes(b,recent,n);
  no_pending(a,b);
  assert(lstat(lock_path,&st)!=0&&errno==ENOENT);
  if(kind==PARENT_SYMLINK_ALIAS)
    assert(unlink("/tmp/rikitu-stage120-parent-alias")==0);
  reported=0;
  assert(rikitu_staged_pair_commit_if_current(
      a,b,n,record_validate,record_encode,&ctx,2u,&reported));
  assert(reported==3u);
  assert_bytes(a,successor,n);
  assert_bytes(b,recent,n);
  check_loader(&ctx,a,b,2);
  no_pending(a,b);
  clean_path(a);clean_path(b);
  printf("PASS Stage120 alias family=%d slot=%c kind=%d\n",
         family,alias_b?'B':'A',kind);
}
static void run_shared_inode_case(int family){
  char a[200],b[200];
  uint8_t old[RIKITU_LEVEL_SAVE_BYTES],recent[RIKITU_LEVEL_SAVE_BYTES];
  uint8_t successor[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t reported=0xf00df00du;
  Context ctx;
  struct stat sa,sb;
  int called;
  size_t n=bytes_for(family);
  memset(&ctx,0,sizeof(ctx));
  ctx.family=family;
  assert(rikitu_campaign_init(&ctx.campaign,4));
  ctx.campaign.wallet=157;
  create_paths(family,a,b);
  clean_path(a);clean_path(b);
  generate(&ctx,0,1u,old);
  generate(&ctx,1,2u,recent);
  generate(&ctx,2,3u,successor);
  write_bytes(a,old,n);
  assert(link(a,b)==0);
  assert(lstat(a,&sa)==0&&lstat(b,&sb)==0);
  assert(sa.st_dev==sb.st_dev&&sa.st_ino==sb.st_ino&&sa.st_nlink>=2);
  ctx.phase=2;
  called=ctx.encode_calls;
  assert(!rikitu_staged_pair_commit_if_current(
      a,b,n,record_validate,record_encode,&ctx,1u,&reported));
  assert(reported==0xf00df00du&&ctx.encode_calls==called);
  assert_bytes(a,old,n);
  assert_bytes(b,old,n);
  no_pending(a,b);
  assert(unlink(b)==0);
  write_bytes(b,recent,n);
  reported=0;
  assert(rikitu_staged_pair_commit_if_current(
      a,b,n,record_validate,record_encode,&ctx,2u,&reported));
  assert(reported==3u);
  assert_bytes(a,successor,n);
  assert_bytes(b,recent,n);
  check_loader(&ctx,a,b,2);
  no_pending(a,b);
  clean_path(a);clean_path(b);
  printf("PASS Stage120 shared-inode family=%d\n",family);
}
int main(void){
  int f,b,s;
  for(f=MAP;f<=LEVEL;f++)
    for(b=0;b<2;b++)
      for(s=STANDARD;s<=EMPTY;s++)
        run_case(f,b,s);
  puts("PASS Stage114 18 stale-snapshot conflict/write protection cases");
  for(f=MAP;f<=LEVEL;f++)
    for(b=0;b<2;b++)
      for(s=SPLIT;s<=DUPLICATE;s++)
        run_ambiguity_case(f,b,s);
  puts("PASS Stage115 18 ambiguous-generation fail-closed/duplicate cases");
  for(f=MAP;f<=LEVEL;f++)
    for(b=0;b<2;b++)
      for(s=SLOT_LINK;s<=SLOT_FIFO;s++)
        run_path_guard_case(f,b,s);
  puts("PASS Stage116 18 unsafe-slot path refusal and recovery cases");
  for(f=MAP;f<=LEVEL;f++)
    for(b=0;b<2;b++)
      for(s=PENDING_HARDLINK;s<=PAIR_LOCK_FIFO;s++)
        run_staging_inode_case(f,b,s);
  puts("PASS Stage117 36 staging/lock inode refusal and recovery cases");
  for(f=MAP;f<=LEVEL;f++)
    for(b=0;b<2;b++)
      run_stale_pending_regular_case(f,b);
  puts("PASS Stage117 6 ordinary stale .pending recovery cases");
  for(f=MAP;f<=LEVEL;f++)
    for(b=0;b<2;b++)
      for(s=PENDING_LINK;s<=PAIR_LOCK_DIRECTORY;s++)
        run_extra_obstruction_case(f,b,s);
  puts("PASS Stage118 24 symlink/FIFO/directory obstruction cases");
#if defined(__linux__)
  for(f=MAP;f<=LEVEL;f++)
    for(b=0;b<2;b++)
      for(s=0;s<2;s++)
        run_lock_swap_case(f,b,s);
  puts("PASS Stage119 12 real two-process lock-path replacement cases");
#endif
  for(f=MAP;f<=LEVEL;f++)
    for(b=0;b<2;b++)
      for(s=DOT_ALIAS;s<=PARENT_SYMLINK_ALIAS;s++)
        run_noncanonical_path_case(f,b,s);
  puts("PASS Stage120 12 noncanonical path-alias rejection/recovery cases");
  for(f=MAP;f<=LEVEL;f++)
    run_shared_inode_case(f);
  puts("PASS Stage120 3 shared-inode A/B rejection/recovery cases");
  return 0;
}
