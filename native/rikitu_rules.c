#include "rikitu_rules.h"
static const int unlock_stages[4] = {2, 6, 10, 14};
static const int prices[4] = {120, 160, 200, 240};
int rikitu_box_hit(RikituRect a, RikituRect b) {
  return a.x < b.x+b.w && a.x+a.w > b.x &&
         a.y < b.y+b.h && a.y+a.h > b.y;
}
int rikitu_can_exit(int fragments, int boss_required, int boss_defeated) {
  return fragments >= RIKITU_REQUIRED_FRAGMENTS &&
         (!boss_required || boss_defeated);
}
int rikitu_can_jump(int grounded, float coyote_seconds) {
  return grounded || coyote_seconds > 0.0f;
}
int rikitu_stomp_from_above(float prev_bottom, float now_bottom,
                            float target_top, int falling) {
  return falling && prev_bottom <= target_top+12.0f &&
         now_bottom >= target_top;
}
int rikitu_heart_after_damage(int hearts) {
  return hearts-1 > 0 ? hearts-1 : 0;
}
int rikitu_is_shop_stage(int stage) {
  return stage >= 2 && stage <= 16 && stage%2 == 0;
}
int rikitu_unlocked_upgrades(int cleared_stage) {
  int i, n=0;
  for(i=0;i<4;i++)if(cleared_stage >= unlock_stages[i])n++;
  return n;
}
RikituHeartOffer rikitu_next_heart_offer(int stage, int bought, int coins) {
  RikituHeartOffer offer={0,0,0,0,RIKITU_OFFER_NOT_IN_SHOP_OR_MAXED};
  int limit=rikitu_unlocked_upgrades(stage);
  if(!rikitu_is_shop_stage(stage) || bought < 0 || bought >= 4)
    return offer;
  if(bought >= limit) {
    offer.reason=RIKITU_OFFER_NOT_UNLOCKED;
    return offer;
  }
  offer.price=prices[bought];
  offer.hearts_after=RIKITU_STARTING_HEARTS+bought+1;
  offer.unlocked=limit;
  offer.available=coins >= offer.price;
  offer.reason=offer.available ? RIKITU_OFFER_READY :
                 RIKITU_OFFER_INSUFFICIENT_COINS;
  return offer;
}
RikituHeartPurchase rikitu_buy_heart(int stage, int bought, int coins) {
  RikituHeartOffer offer=rikitu_next_heart_offer(stage,bought,coins);
  RikituHeartPurchase result={0,bought,coins,RIKITU_STARTING_HEARTS+bought};
  if(!offer.available)return result;
  result.success=1;
  result.purchased_count=bought+1;
  result.coins=coins-offer.price;
  result.hearts=offer.hearts_after;
  return result;
}
