/* Stage105: inject five errors into a separately staged write.
 *
 * This does not alter or wrap game save codecs: it exercises a new
 * optional host-only prototype. Neither staging nor rename is yet used
 * by the shipping game. POSIX rename semantics do not prove FAT/SDMC
 * hardware or power-cut atomicity.
 */
#define _POSIX_C_SOURCE 200809L
#include "rikitu_staged_slot.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

typedef enum {
  F_NONE=0,F_SHORT=1,F_FLUSH=2,F_CLOSE=3,F_BAD_READ=4,F_RENAME=5
} Fault;
static Fault fault=F_NONE;
static unsigned injections=0;
extern size_t __real_fwrite(const void *,size_t,size_t,FILE *);
extern int __real_fflush(FILE *);
extern int __real_fclose(FILE *);
extern size_t __real_fread(void *,size_t,size_t,FILE *);
extern int __real_rename(const char *,const char *);

size_t __wrap_fwrite(const void *d,size_t s,size_t n,FILE *f){
  if(fault!=F_SHORT)return __real_fwrite(d,s,n,f);
  assert(++injections==1 && s==1 && n>3);
  assert(__real_fwrite(d,1,n/2,f)==n/2);
  return n/2;
}
int __wrap_fflush(FILE *f){
  if(fault!=F_FLUSH)return __real_fflush(f);
  assert(++injections==1);
  assert(__real_fflush(f)==0);
  errno=EIO;return EOF;
}
int __wrap_fclose(FILE *f){
  int fd,flags,rc;
  if(fault!=F_CLOSE)return __real_fclose(f);
  fd=fileno(f);assert(fd>=0);
  flags=fcntl(fd,F_GETFL);assert(flags>=0);
  if((flags&O_ACCMODE)==O_RDONLY)return __real_fclose(f);
  assert(++injections==1);
  rc=__real_fclose(f);assert(rc==0);
  errno=EIO;return EOF;
}
size_t __wrap_fread(void *d,size_t s,size_t n,FILE *f){
  size_t r=__real_fread(d,s,n,f);
  if(fault!=F_BAD_READ)return r;
  assert(++injections==1 && r==n && n>1);
  ((uint8_t *)d)[0]^=1; /* Exact-byte readback must reject mismatch. */
  return r;
}
int __wrap_rename(const char *from,const char *to){
  if(fault!=F_RENAME)return __real_rename(from,to);
  assert(++injections==1);
  errno=EIO;return -1;
}
static void read_bytes(const char *p,uint8_t *d,size_t n){
  FILE *f=fopen(p,"rb");assert(f);
  assert(fread(d,1,n,f)==n && fgetc(f)==EOF && fclose(f)==0);
}
static void seed(const char *p,const uint8_t *b,size_t n){
  FILE *f=fopen(p,"wb");assert(f);
  assert(fwrite(b,1,n,f)==n && fclose(f)==0);
}
static void test(Fault mode,int backup_b,size_t n){
  const char *a="/tmp/rikitu-stage105-a.rks";
  const char *b="/tmp/rikitu-stage105-b.rks";
  const char *target=backup_b?a:b;
  const char *backup=backup_b?b:a;
  uint8_t old[64],previous_target[64],next[64],read[64];
  size_t i;
  assert(fault==F_NONE);
  (void)remove(a);(void)remove(b);
  for(i=0;i<n;i++){
    old[i]=(uint8_t)(17+i);
    previous_target[i]=(uint8_t)(120+i);
    next[i]=(uint8_t)(80+i);
  }
  /* Unlike a missing destination, both slots already contain a
   * distinct valid-size record; failures must preserve BOTH. */
  seed(backup,old,n);
  seed(target,previous_target,n);
  fault=mode;injections=0;
  assert(!rikitu_staged_slot_commit(target,next,n));
  fault=F_NONE;
  assert(injections==1);
  read_bytes(backup,read,n);assert(memcmp(read,old,n)==0);
  read_bytes(target,read,n);assert(memcmp(read,previous_target,n)==0);
  /* A retry after each fault MUST be able to promote the new bytes. */
  assert(rikitu_staged_slot_commit(target,next,n));
  read_bytes(backup,read,n);assert(memcmp(read,old,n)==0);
  read_bytes(target,read,n);assert(memcmp(read,next,n)==0);
  (void)remove(a);(void)remove(b);
  printf("PASS Stage105 fault=%d backup=%c record_size=%zu\n",
         (int)mode,backup_b?'B':'A',n);
}
int main(void){
  Fault mode;
  int backup_b;
  const size_t sizes[]={24,32,64};
  size_t k;
  for(mode=F_SHORT;mode<=F_RENAME;mode=(Fault)(mode+1))
    for(backup_b=0;backup_b<2;backup_b++)
      for(k=0;k<3;k++)test(mode,backup_b,sizes[k]);
  puts("PASS Stage105: 30 optional staged-writer fault+retry tests");
  return 0;
}
