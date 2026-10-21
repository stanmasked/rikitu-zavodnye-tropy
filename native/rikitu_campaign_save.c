#include "rikitu_campaign_save.h"
#include <string.h>
static void put32(uint8_t *p,uint32_t x){
  unsigned i;
  for(i=0;i<4;i++)p[i]=(uint8_t)(x>>(8u*i));
}
static uint32_t get32(const uint8_t *p){
  return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
         ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint32_t checksum(const uint8_t *p,size_t n){
  uint32_t c=0xffffffffu;
  size_t i;unsigned j;
  for(i=0;i<n;i++){
    c^=p[i];
    for(j=0;j<8;j++)
      c=(c>>1)^((c&1u)?0xedb88320u:0u);
  }
  return ~c;
}
int rikitu_campaign_save_encode(
    uint8_t out[RIKITU_CAMPAIGN_SAVE_BYTES],
    const RikituCampaign *campaign,uint32_t generation){
  if(!out||!campaign||!generation||!rikitu_campaign_valid(campaign))
    return 0;
  memset(out,0,RIKITU_CAMPAIGN_SAVE_BYTES);
  out[0]='R';out[1]='K';out[2]='C';out[3]='2';
  out[4]=2; /* schema version */
  out[5]=(uint8_t)campaign->level;
  out[6]=(uint8_t)campaign->mode;
  out[7]=(uint8_t)campaign->purchased_hearts;
  out[8]=(uint8_t)campaign->max_hearts;
  /* Reserved 9..11 and 20..27 must remain zero in this version. */
  put32(out+12,(uint32_t)campaign->wallet);
  put32(out+16,generation);
  put32(out+28,checksum(out,28));
  return 1;
}
int rikitu_campaign_save_decode(
    RikituCampaign *campaign,const uint8_t *p,size_t size,
    uint32_t *generation){
  RikituCampaign temp;
  uint32_t gen;
  size_t i;
  if(!campaign||!p||size!=RIKITU_CAMPAIGN_SAVE_BYTES||
     p[0]!='R'||p[1]!='K'||p[2]!='C'||p[3]!='2'||p[4]!=2)
    return 0;
  for(i=9;i<=11;i++)if(p[i])return 0;
  for(i=20;i<=27;i++)if(p[i])return 0;
  if(get32(p+28)!=checksum(p,28))return 0;
  gen=get32(p+16);
  if(!gen)return 0;
  temp.level=(int)p[5];
  temp.mode=(RikituCampaignMode)p[6];
  temp.purchased_hearts=(int)p[7];
  temp.max_hearts=(int)p[8];
  if(get32(p+12)>1440u)return 0;
  temp.wallet=(int)get32(p+12);
  if(!rikitu_campaign_valid(&temp))return 0;
  *campaign=temp;
  if(generation)*generation=gen;
  return 1;
}
