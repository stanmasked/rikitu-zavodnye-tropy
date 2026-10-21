#include "rikitu_campaign.h"
#include <string.h>
#define TOTAL_LEVELS 16
#define MAX_COIN_WALLET (TOTAL_LEVELS*RIKITU_STAGE_COINS)
int rikitu_campaign_init(RikituCampaign *c,int level){
  if(!c||level<1||level>TOTAL_LEVELS)return 0;
  memset(c,0,sizeof(*c));
  c->level=level;c->mode=RIKITU_CAMPAIGN_PLAYING;
  c->max_hearts=RIKITU_STARTING_HEARTS;
  return 1;
}
int rikitu_campaign_valid(const RikituCampaign *c){
  if(!c||c->level<1||c->level>TOTAL_LEVELS||
     c->wallet<0||c->wallet>MAX_COIN_WALLET||
     c->purchased_hearts<0||
     c->purchased_hearts>RIKITU_MAX_HEARTS-RIKITU_STARTING_HEARTS||
     c->max_hearts!=RIKITU_STARTING_HEARTS+c->purchased_hearts)return 0;
  switch(c->mode){
    case RIKITU_CAMPAIGN_PLAYING:return 1;
    case RIKITU_CAMPAIGN_BETWEEN:return c->level%2==1;
    case RIKITU_CAMPAIGN_SHOP:return rikitu_is_shop_stage(c->level);
    case RIKITU_CAMPAIGN_VICTORY:return c->level==TOTAL_LEVELS;
    default:return 0;
  }
}
int rikitu_campaign_finish(RikituCampaign *c,const RikituStage *stage){
  RikituNextMode next;
  if(!rikitu_campaign_valid(c)||!stage||
     c->mode!=RIKITU_CAMPAIGN_PLAYING||!stage->completed||
     stage->fragment_count!=RIKITU_STAGE_FRAGMENTS||
     !rikitu_stage_can_finish(stage)||
     stage->coin_count<0||stage->coin_count>RIKITU_STAGE_COINS||
     c->wallet>MAX_COIN_WALLET-stage->coin_count)return 0;
  next=rikitu_stage_next_mode(c->level);
  if(next!=RIKITU_NEXT_BETWEEN&&next!=RIKITU_NEXT_SHOP)return 0;
  c->wallet+=stage->coin_count;
  c->mode=next==RIKITU_NEXT_SHOP?RIKITU_CAMPAIGN_SHOP:RIKITU_CAMPAIGN_BETWEEN;
  return 1;
}
int rikitu_campaign_continue(RikituCampaign *c){
  if(!rikitu_campaign_valid(c))return 0;
  if(c->mode==RIKITU_CAMPAIGN_BETWEEN){
    if(c->level==TOTAL_LEVELS)return 0;
    c->level++;c->mode=RIKITU_CAMPAIGN_PLAYING;return 1;
  }
  if(c->mode==RIKITU_CAMPAIGN_SHOP){
    if(rikitu_stage_after_shop(c->level)==RIKITU_NEXT_VICTORY){
      c->mode=RIKITU_CAMPAIGN_VICTORY;return 2;
    }
    c->level++;c->mode=RIKITU_CAMPAIGN_PLAYING;return 1;
  }
  return 0;
}
RikituHeartOffer rikitu_campaign_offer(const RikituCampaign *c){
  if(!rikitu_campaign_valid(c)||c->mode!=RIKITU_CAMPAIGN_SHOP)
    return rikitu_next_heart_offer(0,0,0);
  return rikitu_next_heart_offer(c->level,c->purchased_hearts,c->wallet);
}
RikituHeartPurchase rikitu_campaign_buy(RikituCampaign *c){
  RikituHeartPurchase result={0,0,0,0};
  if(!rikitu_campaign_valid(c)||c->mode!=RIKITU_CAMPAIGN_SHOP)return result;
  result=rikitu_buy_heart(c->level,c->purchased_hearts,c->wallet);
  if(result.success){
    c->wallet=result.coins;
    c->purchased_hearts=result.purchased_count;
    c->max_hearts=result.hearts;
  }
  return result;
}
