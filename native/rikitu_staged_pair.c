#define _POSIX_C_SOURCE 200809L
#include "rikitu_staged_pair.h"
#include "rikitu_staged_slot.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

/* This is a Linux-host proof of pair-wide sequencing, not a Switch
 * filesystem guarantee. A future libnx implementation must NOT fall
 * back to unlocked pair selection. Advisory flock protects cooperating
 * writers that use this API and stable canonical slot paths only.
 */
#if defined(__linux__)
static int slot_generation(const char *path,uint8_t *buffer,size_t n,
                           RikituStagedValidate validate,
                           void *context,uint32_t *generation){
  FILE *f=fopen(path,"rb");
  size_t got;
  int tail,closed;
  if(!f)return 0;
  got=fread(buffer,1,n,f);
  tail=fgetc(f);
  closed=fclose(f)==0;
  if(got!=n||tail!=EOF||!closed)return 0;
  return validate(buffer,n,context,generation);
}
static int newer(uint32_t a,uint32_t b){
  return a!=b && (uint32_t)(a-b)<0x80000000u;
}
#endif
int rikitu_staged_pair_commit(
  const char *a,const char *b,size_t n,
  RikituStagedValidate validate,RikituStagedEncode encode,
  void *context,uint32_t *committed_generation){
#if defined(__linux__)
  const char suffix[]=".pair.lock";
  const char *base;
  char *lock_path=NULL;
  uint8_t *x=NULL,*y=NULL,*out=NULL;
  size_t len;
  uint32_t ga=0,gb=0,gen=1;
  const char *target=NULL;
  int fd=-1,va,vb,ok=0;
  if(!a||!b||!a[0]||!b[0]||strcmp(a,b)==0||
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
  fd=open(lock_path,O_RDWR|O_CREAT|O_NOFOLLOW,0600);
  if(fd<0||flock(fd,LOCK_EX)!=0)goto done;
  /* Selection is performed AFTER the lock, not on a stale earlier read.
   * Do not consider any .pending file a valid generation. */
  va=slot_generation(a,x,n,validate,context,&ga);
  vb=slot_generation(b,y,n,validate,context,&gb);
  if(va&&vb){
    if(newer(gb,ga)){gen=gb+1u;target=a;}
    else{gen=ga+1u;target=b;}
  }else if(va){gen=ga+1u;target=b;}
  else if(vb){gen=gb+1u;target=a;}
  else target=a;
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
#else
  (void)a;(void)b;(void)n;(void)validate;(void)encode;
  (void)context;(void)committed_generation;
  /* Fail closed until a target-platform pair lock is independently proven. */
  return 0;
#endif
}
