#include "rikitu_draw.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
enum { W=1280,H=720 };
int main(void){
  RikituCampaign c;
  RikituHeartOffer offer;
  uint32_t *pixels=(uint32_t*)calloc((size_t)W*H,sizeof(uint32_t));
  assert(pixels && rikitu_campaign_init(&c,2));
  offer=rikitu_campaign_offer(&c);
  assert(rikitu_draw_campaign_overlay(pixels,W,H,W,&c,&offer)==1);
  assert(pixels[(size_t)(H/2)*W+(W/2)]==0u);
  c.mode=RIKITU_CAMPAIGN_SHOP;c.wallet=180;
  offer=rikitu_campaign_offer(&c);
  assert(offer.available && offer.price==120);
  assert(rikitu_draw_campaign_overlay(pixels,W,H,W,&c,&offer)==1);
  assert(pixels[(size_t)(H/2)*W+(W/2)]!=0u);
  c.mode=RIKITU_CAMPAIGN_BETWEEN;
  assert(rikitu_draw_campaign_overlay(pixels,W,H,W,&c,&offer)==1);
  c.mode=RIKITU_CAMPAIGN_VICTORY;
  assert(rikitu_draw_campaign_overlay(pixels,W,H,W,&c,&offer)==1);
  assert(!rikitu_draw_campaign_overlay(NULL,W,H,W,&c,&offer));
  free(pixels);
  puts("PASS campaign shop/intermission/victory overlays on 1280x720 framebuffer");
  return 0;
}
