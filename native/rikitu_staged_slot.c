#include "rikitu_staged_slot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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
  int ok=0,extra,close_ok;
  FILE *f=NULL;
  if(!target||!target[0]||!record||!n||n>4096u)return 0;
  name_len=strlen(target);
  if(name_len>4095u-sizeof(suffix))return 0;
  tmp=(char*)malloc(name_len+sizeof(suffix));
  readback=(uint8_t*)malloc(n);
  if(!tmp||!readback)goto done;
  memcpy(tmp,target,name_len);
  memcpy(tmp+name_len,suffix,sizeof(suffix));
  f=fopen(tmp,"wb");
  if(!f)goto done;
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
  if(!ok&&tmp)(void)remove(tmp);
  free(readback);
  free(tmp);
  return ok;
}
