#define _POSIX_C_SOURCE 200809L
#include "rikitu_staged_slot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

/* Test-only prototype: same-directory staging + exact-byte readback
 * before replacing target via rename(). If ANY preparatory step fails,
 * both production slot paths remain byte-identical. No fsync/dir-fsync:
 * this does NOT certify FAT/SDMC crash durability or rename atomicity.
 * The game does not yet call this helper.
 */
int rikitu_staged_slot_commit(const char *target,const uint8_t *record,size_t n){
  const char suffix[]=".pending";
  char *tmp=NULL;
  uint8_t *readback=NULL;
  size_t name_len,read_n;
  int ok=0,extra,close_ok,opened_tmp=0;
  FILE *f=NULL;
  if(!target||!target[0]||!record||!n||n>4096u)return 0;
  name_len=strlen(target);
  if(name_len>4095u-sizeof(suffix))return 0;
  tmp=(char*)malloc(name_len+sizeof(suffix));
  readback=(uint8_t*)malloc(n);
  if(!tmp||!readback)goto done;
  memcpy(tmp,target,name_len);
  memcpy(tmp+name_len,suffix,sizeof(suffix));
  /* On POSIX hosts, a pre-existing .pending symlink must not be
   * followed: truncating its referent could destroy the only valid save.
   * O_NOFOLLOW checks this atomically with open, unlike an lstat check.
   * Non-POSIX/libnx builds retain the prior experimental fallback until
   * their filesystem exposes a tested equivalent. */
#if defined(O_NOFOLLOW)
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
  f=fopen(tmp,"rb");
  if(!f)goto done;
  read_n=fread(readback,1,n,f);
  extra=fgetc(f);
  close_ok=fclose(f)==0;f=NULL;
  if(!close_ok||read_n!=n||extra!=EOF||memcmp(readback,record,n)!=0)goto done;
  /* No unlink-first fallback: on a filesystem that rejects replace,
   * fail without destroying the existing target or other backup. */
  if(rename(tmp,target)!=0)goto done;
  ok=1;
done:
  if(f)(void)fclose(f);
  /* Never delete a pre-existing directory/path when fopen failed. */
  if(!ok&&tmp&&opened_tmp)(void)remove(tmp);
  free(readback);
  free(tmp);
  return ok;
}
