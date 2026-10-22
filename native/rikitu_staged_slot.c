#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L
#include "rikitu_staged_slot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#if defined(__linux__)
int rikitu_staged_slot_path_is_canonical(const char *path){
  const char *slash,*base;
  char *parent=NULL,*resolved=NULL,*canonical=NULL;
  size_t parent_len,base_len,total;
  int ok=0;
  if(!path||path[0]!='/'||strlen(path)>4095u)return 0;
  slash=strrchr(path,'/');
  if(!slash)return 0;
  base=slash+1;
  if(!base[0]||strcmp(base,".")==0||strcmp(base,"..")==0)return 0;
  parent_len=slash==path?1u:(size_t)(slash-path);
  parent=(char*)malloc(parent_len+1u);
  if(!parent)goto done;
  memcpy(parent,path,parent_len);
  parent[parent_len]='\0';
  resolved=realpath(parent,NULL);
  if(!resolved)goto done;
  base_len=strlen(base);
  total=strlen(resolved)+base_len+2u;
  canonical=(char*)malloc(total);
  if(!canonical)goto done;
  if(strcmp(resolved,"/")==0)
    (void)snprintf(canonical,total,"/%s",base);
  else
    (void)snprintf(canonical,total,"%s/%s",resolved,base);
  ok=strcmp(path,canonical)==0;
done:
  free(parent);free(resolved);free(canonical);
  return ok;
}
#endif

/* Same-directory staging + exact-byte readback used by the production
 * pair transaction. If any preparatory step fails, the existing target and
 * its backup slot remain untouched. No fsync/dir-fsync: this does NOT certify
 * FAT/SDMC crash durability or rename atomicity. Linux additionally guards
 * pending/target inodes; the Switch path is serialized by the pair API's
 * in-process mutex and remains subject to the SDMC filesystem contract.
 */
int rikitu_staged_slot_commit(const char *target,const uint8_t *record,size_t n){
  const char suffix[]=".pending";
#if defined(__linux__)
  const char lock_suffix[]=".pending.lock";
  char *lock_path=NULL;
  int lock_fd=-1;
  dev_t stage_dev=0;
  ino_t stage_ino=0;
#endif
  char *tmp=NULL;
  uint8_t *readback=NULL;
  size_t name_len,read_n;
  int ok=0,extra,read_error,close_ok,opened_tmp=0;
  FILE *f=NULL;
  if(!target||!target[0]||!record||!n||n>4096u)return 0;
#if defined(__linux__)
  if(!rikitu_staged_slot_path_is_canonical(target))return 0;
#endif
  name_len=strlen(target);
  if(name_len>4095u-sizeof(suffix))return 0;
#if defined(__linux__)
  if(name_len>4095u-sizeof(lock_suffix))return 0;
#endif
  tmp=(char*)malloc(name_len+sizeof(suffix));
  readback=(uint8_t*)malloc(n);
  if(!tmp||!readback)goto done;
  memcpy(tmp,target,name_len);
  memcpy(tmp+name_len,suffix,sizeof(suffix));
#if defined(__linux__)
  /* Serialize all writers of this one target BEFORE touching .pending.
   * The lock inode is persistent: unlinking it would allow two different
   * inodes, and hence two simultaneous apparent owners. This is a
   * Linux-host protection only; Switch/libnx is not certified here. */
  lock_path=(char*)malloc(name_len+sizeof(lock_suffix));
  if(!lock_path)goto done;
  memcpy(lock_path,target,name_len);
  memcpy(lock_path+name_len,lock_suffix,sizeof(lock_suffix));
  lock_fd=open(lock_path,O_RDWR|O_CREAT|O_NOFOLLOW|O_NONBLOCK,0600);
  if(lock_fd<0)goto done;
  /* A pre-existing FIFO/device or hard link to a live save cannot
   * serve as a trusted persistent lock inode. */
  {
    struct stat lock_stat;
    if(fstat(lock_fd,&lock_stat)!=0||
       !S_ISREG(lock_stat.st_mode)||lock_stat.st_nlink!=1)goto done;
  }
  if(flock(lock_fd,LOCK_EX)!=0)goto done;
  /* Validate both the locked descriptor and its current pathname:
   * a lock renamed during contention can otherwise split writers
   * across different advisory-lock inodes. */
  {
    struct stat locked_stat,path_stat;
    if(fstat(lock_fd,&locked_stat)!=0||
       lstat(lock_path,&path_stat)!=0||
       !S_ISREG(locked_stat.st_mode)||locked_stat.st_nlink!=1||
       !S_ISREG(path_stat.st_mode)||path_stat.st_nlink!=1||
       locked_stat.st_dev!=path_stat.st_dev||
       locked_stat.st_ino!=path_stat.st_ino)goto done;
  }
#endif
  /* On POSIX hosts, a pre-existing .pending symlink must not be
   * followed: truncating its referent could destroy the only valid save.
   * O_NOFOLLOW checks this atomically with open, unlike an lstat check.
   * Non-POSIX/libnx builds retain the prior experimental fallback until
   * their filesystem exposes a tested equivalent. */
#if defined(__linux__)
  {
    struct stat stage_stat;
    /* O_TRUNC here would already destroy a live save hard-linked from
     * .pending. Validate the opened inode BEFORE truncating anything. */
    int fd=open(tmp,O_WRONLY|O_CREAT|O_NOFOLLOW|O_NONBLOCK,0600);
    if(fd<0)goto done;
    if(fstat(fd,&stage_stat)!=0||
       !S_ISREG(stage_stat.st_mode)||stage_stat.st_nlink!=1){
      (void)close(fd);
      goto done;
    }
    stage_dev=stage_stat.st_dev;
    stage_ino=stage_stat.st_ino;
    if(ftruncate(fd,0)!=0){(void)close(fd);goto done;}
    opened_tmp=1;
    f=fdopen(fd,"wb");
    if(!f){(void)close(fd);goto done;}
  }
#elif defined(O_NOFOLLOW)
  {
    int fd=open(tmp,O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW,0600);
    if(fd<0)goto done;
    opened_tmp=1;
    f=fdopen(fd,"wb");
    if(!f){(void)close(fd);goto done;}
  }
#else
  f=fopen(tmp,"wb");
  if(!f)goto done;
  opened_tmp=1;
#endif
  if(fwrite(record,1,n,f)!=n||fflush(f)!=0){
    (void)fclose(f);f=NULL;
    goto done;
  }
  close_ok=fclose(f)==0;f=NULL;
  if(!close_ok)goto done;
#if defined(__linux__)
  {
    struct stat check_stat;
    int read_fd=open(tmp,O_RDONLY|O_NOFOLLOW|O_NONBLOCK);
    if(read_fd<0)goto done;
    if(fstat(read_fd,&check_stat)!=0||
       !S_ISREG(check_stat.st_mode)||check_stat.st_nlink!=1||
       check_stat.st_dev!=stage_dev||check_stat.st_ino!=stage_ino){
      (void)close(read_fd);
      goto done;
    }
    f=fdopen(read_fd,"rb");
    if(!f){(void)close(read_fd);goto done;}
  }
#else
  f=fopen(tmp,"rb");
  if(!f)goto done;
#endif
  read_n=fread(readback,1,n,f);
  extra=fgetc(f);
  read_error=ferror(f);
  close_ok=fclose(f)==0;f=NULL;
  if(!close_ok||read_error||read_n!=n||extra!=EOF||
     memcmp(readback,record,n)!=0)goto done;
  /* Refuse a renamed/replaced pending path even if the contents have
   * independently passed the exact-byte readback. This does not make
   * a malicious post-check path swap atomic. */
#if defined(__linux__)
  {
    struct stat path_stat;
    if(lstat(tmp,&path_stat)!=0||
       !S_ISREG(path_stat.st_mode)||path_stat.st_nlink!=1||
       path_stat.st_dev!=stage_dev||path_stat.st_ino!=stage_ino)goto done;
  }
#endif
  /* No unlink-first fallback: on a filesystem that rejects replace,
   * fail without destroying the existing target or other backup. */
  if(rename(tmp,target)!=0)goto done;
  ok=1;
done:
  if(f)(void)fclose(f);
  /* If the pending pathname was swapped, do not delete the replacement
   * while cleaning up our failed write. Only unlink our opened inode. */
  if(!ok&&tmp&&opened_tmp){
#if defined(__linux__)
    struct stat path_stat;
    if(lstat(tmp,&path_stat)==0&&S_ISREG(path_stat.st_mode)&&
       path_stat.st_nlink==1&&path_stat.st_dev==stage_dev&&
       path_stat.st_ino==stage_ino)(void)unlink(tmp);
#else
    (void)remove(tmp);
#endif
  }
#if defined(__linux__)
  if(lock_fd>=0){(void)flock(lock_fd,LOCK_UN);(void)close(lock_fd);}
  /* Intentionally preserve .pending.lock for future concurrent writers. */
  free(lock_path);
#endif
  free(readback);
  free(tmp);
  return ok;
}
