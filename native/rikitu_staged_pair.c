#define _POSIX_C_SOURCE 200809L
#include "rikitu_staged_pair.h"
#include "rikitu_staged_slot.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__SWITCH__)
#include <switch.h>
#include "rikitu_slot_read.h"
#endif
#if defined(__linux__)
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

/* Linux uses an advisory pair lock and canonical POSIX paths. The Switch
 * backend serializes callers within this process with a libnx mutex and
 * restricts writes to the game's SDMC save directory. Neither backend
 * claims physical SDMC power-loss durability; the Switch mutex is not a
 * cross-process lock. Other targets fail closed.
 */
#if defined(__linux__)
/* Return 1 for codec-valid, 0 for absent/invalid regular records,
 * and -1 for filesystem/path I/O errors. A symlink, FIFO, device or
 * directory must never be mistaken for a harmless absent backup. */
static int slot_generation(const char *path,uint8_t *buffer,size_t n,
                           RikituStagedValidate validate,
                           void *context,uint32_t *generation){
  FILE *f;
  struct stat st;
  size_t got;
  int fd,tail,read_error,closed;
  fd=open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);
  if(fd<0)return errno==ENOENT?0:-1;
  if(fstat(fd,&st)!=0||!S_ISREG(st.st_mode)){
    (void)close(fd);
    return -1;
  }
  f=fdopen(fd,"rb");
  if(!f){
    (void)close(fd);
    return -1;
  }
  got=fread(buffer,1,n,f);
  tail=fgetc(f);
  read_error=ferror(f);
  closed=fclose(f)==0;
  if(read_error||!closed)return -1;
  if(got!=n||tail!=EOF)return 0;
  return validate(buffer,n,context,generation)?1:0;
}
#endif

static int newer(uint32_t a,uint32_t b){
  return a!=b && (uint32_t)(a-b)<0x80000000u;
}

#if defined(__SWITCH__)
/* Switch SDMC does not expose the Linux flock/O_NOFOLLOW contract used by
 * the host backend. Keep this backend deliberately narrow: only canonical
 * save paths under the game's own directory are accepted, and all pair
 * selection/commit calls in this process share one statically initialized
 * libnx mutex. */
static Mutex g_switch_pair_lock = 0;

static int switch_slot_path_ok(const char *path){
  static const char prefix[]="sdmc:/switch/RIKITU/";
  const char *p,*slash,*suffix;
  size_t n;
  if(!path||strncmp(path,prefix,sizeof(prefix)-1u)!=0)return 0;
  n=strlen(path);
  if(n>1024u||n<=sizeof(prefix)-1u)return 0;
  if(strchr(path,'\\')||strstr(path,"//"))return 0;
  p=path+sizeof(prefix)-1u;
  /* Every production slot is a direct child of RIKITU/. Reject nested
   * components so an SDMC symlink directory cannot redirect a save outside
   * the private save folder on host-backed emulator filesystems. */
  if(strchr(p,'/'))return 0;
  while(*p){
    size_t segment_len;
    slash=strchr(p,'/');
    segment_len=slash?(size_t)(slash-p):strlen(p);
    if(segment_len==0||
       (segment_len==1u&&p[0]=='.')||
       (segment_len==2u&&p[0]=='.'&&p[1]=='.'))return 0;
    if(!slash)break;
    p=slash+1;
    if(!*p)return 0;
  }
  suffix=strrchr(path,'.');
  if(!suffix|| (strcmp(suffix,".rks")!=0&&
                strcmp(suffix,".rkc")!=0&&
                strcmp(suffix,".rkl")!=0))return 0;
  return 1;
}

static int switch_slot_generation(const char *path,uint8_t *buffer,size_t n,
                                  RikituStagedValidate validate,
                                  void *context,uint32_t *generation){
  int read_result=rikitu_read_fixed_slot(path,buffer,n);
  if(read_result==RIKITU_SLOT_READ_ERROR)return -1;
  if(read_result!=RIKITU_SLOT_READ_OK)return 0;
  return validate(buffer,n,context,generation)?1:0;
}
#endif

static int pair_commit_internal(
  const char *a,const char *b,size_t n,
  RikituStagedValidate validate,RikituStagedEncode encode,
  void *context,int enforce_expected,uint32_t expected_generation,
  uint32_t *committed_generation){
#if defined(__linux__)
  const char suffix[]=".pair.lock";
  const char *base;
  char *lock_path=NULL;
  uint8_t *x=NULL,*y=NULL,*out=NULL;
  size_t len;
  uint32_t ga=0,gb=0,gen=1,latest=0;
  const char *target=NULL;
  int fd=-1,va,vb,ok=0;
  if(!a||!b||!a[0]||!b[0]||strcmp(a,b)==0||
     !rikitu_staged_slot_path_is_canonical(a)||
     !rikitu_staged_slot_path_is_canonical(b)||
     !validate||!encode||!n||n>4096u)return 0;
  /* Lexicographic ordering ensures the same lock when caller swaps A/B.
   * Paths must identify the same files without aliases/symlink redirection. */
  base=strcmp(a,b)<0?a:b;
  len=strlen(base);
  if(len>4095u-sizeof(suffix))return 0;
  lock_path=(char*)malloc(len+sizeof(suffix));
  x=(uint8_t*)malloc(n);
  y=(uint8_t*)malloc(n);
  out=(uint8_t*)malloc(n);
  if(!lock_path||!x||!y||!out)goto done;
  memcpy(lock_path,base,len);
  memcpy(lock_path+len,suffix,sizeof(suffix));
  fd=open(lock_path,O_RDWR|O_CREAT|O_NOFOLLOW|O_NONBLOCK,0600);
  if(fd<0)goto done;
  /* Do not accept a special file or hard-linked live record as the
   * stable pair-wide lock; never unlink a trusted lock after use. */
  {
    struct stat lock_stat;
    if(fstat(fd,&lock_stat)!=0||
       !S_ISREG(lock_stat.st_mode)||lock_stat.st_nlink!=1)goto done;
  }
  if(flock(fd,LOCK_EX)!=0)goto done;
  /* An open descriptor can remain single-linked even if the lock path
   * was renamed away and replaced during flock contention. Check that
   * the current directory entry STILL names our locked inode. */
  {
    struct stat locked_stat,path_stat;
    if(fstat(fd,&locked_stat)!=0||
       lstat(lock_path,&path_stat)!=0||
       !S_ISREG(locked_stat.st_mode)||locked_stat.st_nlink!=1||
       !S_ISREG(path_stat.st_mode)||path_stat.st_nlink!=1||
       locked_stat.st_dev!=path_stat.st_dev||
       locked_stat.st_ino!=path_stat.st_ino)goto done;
  }
  /* Two different names must not be hard links to one live slot:
   * that would defeat the two-independent-copies recovery contract. */
  {
    struct stat stat_a,stat_b;
    if(lstat(a,&stat_a)==0&&lstat(b,&stat_b)==0&&
       S_ISREG(stat_a.st_mode)&&S_ISREG(stat_b.st_mode)&&
       stat_a.st_dev==stat_b.st_dev&&stat_a.st_ino==stat_b.st_ino)
      goto done;
  }
  /* Selection is performed AFTER the lock, not on a stale earlier read.
   * Do not consider any .pending file a valid generation. */
  va=slot_generation(a,x,n,validate,context,&ga);
  if(va<0)goto done;
  vb=slot_generation(b,y,n,validate,context,&gb);
  if(vb<0)goto done;
  /* A guarded writer cannot safely select the winner of split-brain
   * records with the SAME generation but DIFFERENT contents, nor can
   * serial arithmetic order generations exactly 2^31 apart. Leave
   * both slots untouched and require explicit caller reconciliation.
   * Preserve the legacy experimental unguarded API's selection. */
  if(enforce_expected&&va&&vb&&
     ((ga==gb&&memcmp(x,y,n)!=0)||
      (uint32_t)(ga-gb)==0x80000000u))goto done;
  if(va&&vb){
    if(newer(gb,ga)){latest=gb;gen=gb+1u;target=a;}
    else{latest=ga;gen=ga+1u;target=b;}
  }else if(va){latest=ga;gen=ga+1u;target=b;}
  else if(vb){latest=gb;gen=gb+1u;target=a;}
  else target=a;
  /* A stale writer must not promote an old gameplay snapshot to a
   * higher generation. Compare while the pair lock is held, BEFORE
   * invoking encode() or ever touching .pending / a live slot. */
  if(enforce_expected&&latest!=expected_generation)goto done;
  if(!gen)gen=1;
  if(!encode(out,n,gen,context))goto done;
  if(!rikitu_staged_slot_commit(target,out,n))goto done;
  if(committed_generation)*committed_generation=gen;
  ok=1;
done:
  /* Never unlink the pair lock: removing it risks split-lock inodes. */
  if(fd>=0){(void)flock(fd,LOCK_UN);(void)close(fd);}
  free(lock_path);free(x);free(y);free(out);
  return ok;
#elif defined(__SWITCH__)
  uint8_t *x=NULL,*y=NULL,*out=NULL;
  uint32_t ga=0,gb=0,gen=1,latest=0;
  const char *target=NULL;
  const char *ext_a,*ext_b;
  int va=0,vb=0,ok=0,locked=0;
  if(!a||!b||!a[0]||!b[0]||strcmp(a,b)==0||
     !switch_slot_path_ok(a)||!switch_slot_path_ok(b)||
     !validate||!encode||!n||n>4096u)return 0;
  ext_a=strrchr(a,'.');ext_b=strrchr(b,'.');
  if(!ext_a||!ext_b||strcmp(ext_a,ext_b)!=0)return 0;
  x=(uint8_t*)malloc(n);
  y=(uint8_t*)malloc(n);
  out=(uint8_t*)malloc(n);
  if(!x||!y||!out)goto switch_done;
  mutexLock(&g_switch_pair_lock);
  locked=1;
  /* Inspect both live slots under the same in-process lock used for commit.
   * A path/read error is distinct from a missing or invalid record. */
  va=switch_slot_generation(a,x,n,validate,context,&ga);
  if(va<0)goto switch_done;
  vb=switch_slot_generation(b,y,n,validate,context,&gb);
  if(vb<0)goto switch_done;
  if(enforce_expected&&va&&vb&&
     ((ga==gb&&memcmp(x,y,n)!=0)||
      (uint32_t)(ga-gb)==0x80000000u))goto switch_done;
  if(va&&vb){
    if(newer(gb,ga)){latest=gb;gen=gb+1u;target=a;}
    else{latest=ga;gen=ga+1u;target=b;}
  }else if(va){latest=ga;gen=ga+1u;target=b;}
  else if(vb){latest=gb;gen=gb+1u;target=a;}
  else target=a;
  if(enforce_expected&&latest!=expected_generation)goto switch_done;
  if(!gen)gen=1;
  if(!encode(out,n,gen,context))goto switch_done;
  if(!rikitu_staged_slot_commit(target,out,n))goto switch_done;
  if(committed_generation)*committed_generation=gen;
  ok=1;
switch_done:
  if(locked)mutexUnlock(&g_switch_pair_lock);
  free(x);free(y);free(out);
  return ok;
#else
  (void)a;(void)b;(void)n;(void)validate;(void)encode;
  (void)context;(void)committed_generation;
  (void)enforce_expected;(void)expected_generation;
  /* Fail closed on unverified platforms. */
  return 0;
#endif
}

int rikitu_staged_pair_commit(
  const char *a,const char *b,size_t n,
  RikituStagedValidate validate,RikituStagedEncode encode,
  void *context,uint32_t *committed_generation){
  return pair_commit_internal(a,b,n,validate,encode,context,
                              0,0,committed_generation);
}
int rikitu_staged_pair_commit_if_current(
  const char *a,const char *b,size_t n,
  RikituStagedValidate validate,RikituStagedEncode encode,
  void *context,uint32_t expected_generation,
  uint32_t *committed_generation){
  return pair_commit_internal(a,b,n,validate,encode,context,
                              1,expected_generation,committed_generation);
}
