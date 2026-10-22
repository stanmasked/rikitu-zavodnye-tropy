#include "rikitu_campaign_save.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define A "/tmp/rikitu-campaign-slot-test-a.rkc"
#define B "/tmp/rikitu-campaign-slot-test-b.rkc"
#define LEGACY "/tmp/rikitu-map01-save-test-a.rks"
static void fill(RikituCampaign *c){
  assert(rikitu_campaign_init(c,2));
  c->mode=RIKITU_CAMPAIGN_SHOP;
  c->wallet=180;
}
int main(void){
  RikituCampaign c,d,untouched;
  uint8_t bytes[RIKITU_CAMPAIGN_SAVE_BYTES];
  uint8_t scratch[RIKITU_CAMPAIGN_SAVE_BYTES];
  uint32_t generation=0;
  FILE *f;size_t i;
  (void)remove(A);(void)remove(B);
  fill(&c);
  /* A misconfigured new writer cannot truncate an existing MAP01 RKS1. */
  f=fopen(LEGACY,"wb");assert(f);
  assert(fwrite("RKS1LEGACY",1,10,f)==10 && fclose(f)==0);
  assert(!rikitu_campaign_save_store_pair(LEGACY,B,&c));
  assert(!rikitu_campaign_save_load_pair(LEGACY,B,&d));
  f=fopen(LEGACY,"rb");assert(f);
  assert(fread(scratch,1,10,f)==10 && fclose(f)==0);
  assert(memcmp(scratch,"RKS1LEGACY",10)==0);
  (void)remove(LEGACY);
  assert(!rikitu_campaign_save_encode(NULL,&c,1));
  assert(!rikitu_campaign_save_encode(bytes,&c,0));
  assert(rikitu_campaign_save_encode(bytes,&c,12));
  assert(bytes[0]=='R' && bytes[1]=='K' && bytes[2]=='C' &&
         bytes[3]=='2' && bytes[4]==2 && bytes[5]==2);
  memset(&d,0x6a,sizeof(d));
  assert(rikitu_campaign_save_decode(
      &d,bytes,sizeof(bytes),&generation));
  assert(generation==12 && d.level==2 &&
         d.mode==RIKITU_CAMPAIGN_SHOP && d.wallet==180);
  assert(d.max_hearts==3 && d.purchased_hearts==0);
  assert(!rikitu_campaign_save_decode(
      &d,bytes,sizeof(bytes)-1,NULL));
  assert(!rikitu_campaign_save_decode(
      &d,bytes,sizeof(bytes)+1,NULL));
  memcpy(scratch,bytes,sizeof(scratch));scratch[11]=1;
  assert(!rikitu_campaign_save_decode(&d,scratch,sizeof(scratch),NULL));
  memcpy(scratch,bytes,sizeof(scratch));scratch[25]=1;
  assert(!rikitu_campaign_save_decode(&d,scratch,sizeof(scratch),NULL));
  memcpy(scratch,bytes,sizeof(scratch));scratch[12]^=1;
  assert(!rikitu_campaign_save_decode(&d,scratch,sizeof(scratch),NULL));
  memcpy(scratch,bytes,sizeof(scratch));scratch[0]='S';
  assert(!rikitu_campaign_save_decode(&d,scratch,sizeof(scratch),NULL));
  memcpy(scratch,bytes,sizeof(scratch));scratch[4]=1;
  assert(!rikitu_campaign_save_decode(&d,scratch,sizeof(scratch),NULL));
  assert(d.wallet==180 && d.level==2);
  assert(!rikitu_campaign_save_load_pair(A,B,&d));
  assert(!rikitu_campaign_save_store_pair(A,A,&c));
  assert(rikitu_campaign_save_store_pair(A,B,&c));
  assert(rikitu_campaign_save_load_pair(A,B,&d)&&d.wallet==180);
  /* Duplicate paths must not masquerade as a two-slot recovery pair, and
   * rejection must leave the caller's campaign byte-for-byte untouched. */
  memset(&d,0x5c,sizeof(d));
  memcpy(&untouched,&d,sizeof(d));
  assert(!rikitu_campaign_save_load_pair(A,A,&d));
  assert(memcmp(&d,&untouched,sizeof(d))==0);
  /* A valid Moki purchase persists earned wallet/hearts in slot B. */
  assert(rikitu_campaign_buy(&c).success);
  assert(c.wallet==60 && c.purchased_hearts==1 && c.max_hearts==4);
  assert(rikitu_campaign_save_store_pair(A,B,&c));
  assert(rikitu_campaign_save_load_pair(A,B,&d));
  assert(d.wallet==60 && d.max_hearts==4);
  assert(rikitu_campaign_continue(&c)==1 && c.level==3);
  assert(rikitu_campaign_save_store_pair(A,B,&c));
  assert(rikitu_campaign_save_load_pair(A,B,&d) && d.level==3);
  /* Deliberately break the newest slot A(gen3); fallback to B(gen2). */
  f=fopen(A,"r+b");assert(f);
  assert(fseek(f,14,SEEK_SET)==0 && fputc(17,f)!=EOF);
  assert(fclose(f)==0);
  assert(rikitu_campaign_save_load_pair(A,B,&d));
  assert(d.level==2 && d.mode==RIKITU_CAMPAIGN_SHOP &&
         d.wallet==60 && d.max_hearts==4);
  /* Torn write also must not damage the remaining valid checkpoint. */
  f=fopen(A,"wb");assert(f);
  for(i=0;i<5;i++)assert(fputc('X',f)!=EOF);
  assert(fclose(f)==0);
  assert(rikitu_campaign_save_load_pair(A,B,&d)&&d.level==2);
  assert(!rikitu_campaign_save_store_pair(A,B,NULL));
  assert(!rikitu_campaign_save_load_pair(A,A,&d));
  /* Load never accepts legacy RKS1: the formats have distinct magic. */
  memcpy(scratch,bytes,sizeof(scratch));
  scratch[2]='S';scratch[3]='1';
  assert(!rikitu_campaign_save_decode(&d,scratch,sizeof(scratch),NULL));
  (void)remove(A);(void)remove(B);
  assert(!rikitu_campaign_save_load_pair(A,B,&d));
  assert(d.level==2 && d.wallet==60);
  /* Final mode is independently valid and preserved verbatim. */
  assert(rikitu_campaign_init(&c,16));
  c.mode=RIKITU_CAMPAIGN_VICTORY;c.wallet=720;
  c.max_hearts=7;c.purchased_hearts=4;
  assert(rikitu_campaign_save_encode(bytes,&c,0xfffffffeu));
  assert(rikitu_campaign_save_decode(
       &d,bytes,sizeof(bytes),&generation));
  assert(generation==0xfffffffeu && d.level==16 &&
         d.mode==RIKITU_CAMPAIGN_VICTORY && d.max_hearts==7);
  puts("PASS RKC2 campaign: CRC, two-slot fallback, torn write, RKS1 isolation, 16-stage metadata");
  return 0;
}
