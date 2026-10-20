#include "rikitu_rules.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  int stage,bought,wallet;
  RikituRect a={0,0,10,10}, b={9,9,1,1}, c={10,0,5,5};
  RikituHeartOffer offer;
  RikituHeartPurchase purchase;
  assert(rikitu_box_hit(a,b) && !rikitu_box_hit(a,c));
  assert(!rikitu_can_exit(2,0,1));
  assert(rikitu_can_exit(3,0,0));
  assert(!rikitu_can_exit(3,1,0));
  assert(rikitu_can_exit(3,1,1));
  assert(rikitu_can_jump(0,0.001f) && rikitu_can_jump(1,0.0f));
  assert(!rikitu_can_jump(0,0.0f));
  assert(rikitu_stomp_from_above(95,102,100,1));
  assert(!rikitu_stomp_from_above(113,120,100,1));
  assert(!rikitu_stomp_from_above(95,102,100,0));
  assert(rikitu_heart_after_damage(1)==0);
  assert(rikitu_heart_after_damage(0)==0);
  for(stage=1;stage<=16;stage++) {
    int expected=stage>=14?4:stage>=10?3:stage>=6?2:stage>=2?1:0;
    assert(rikitu_unlocked_upgrades(stage)==expected);
    assert(rikitu_is_shop_stage(stage)==(stage%2==0));
  }
  assert(!rikitu_is_shop_stage(0) && !rikitu_is_shop_stage(18));
  offer=rikitu_next_heart_offer(2,0,119);
  assert(!offer.available && offer.price==120 &&
         offer.reason==RIKITU_OFFER_INSUFFICIENT_COINS);
  offer=rikitu_next_heart_offer(2,0,120);
  assert(offer.available && offer.hearts_after==4 && offer.unlocked==1);
  assert(rikitu_next_heart_offer(8,2,999).reason==RIKITU_OFFER_NOT_UNLOCKED);
  assert(rikitu_next_heart_offer(12,3,999).reason==RIKITU_OFFER_NOT_UNLOCKED);
  assert(rikitu_next_heart_offer(10,2,200).price==200);
  assert(rikitu_next_heart_offer(14,3,240).price==240);
  assert(rikitu_next_heart_offer(3,0,999).reason==
         RIKITU_OFFER_NOT_IN_SHOP_OR_MAXED);
  wallet=720;bought=0;
  for(stage=0;stage<4;stage++){
    purchase=rikitu_buy_heart(16,bought,wallet);
    assert(purchase.success);
    assert(purchase.purchased_count==bought+1);
    assert(purchase.hearts==RIKITU_STARTING_HEARTS+bought+1);
    bought=purchase.purchased_count;
    wallet=purchase.coins;
  }
  assert(bought==4 && wallet==0);
  purchase=rikitu_buy_heart(16,bought,wallet);
  assert(!purchase.success && purchase.hearts==RIKITU_MAX_HEARTS);
  purchase=rikitu_buy_heart(12,3,500);
  assert(!purchase.success && purchase.coins==500 && purchase.purchased_count==3);
  printf("PASS native C99 rule parity: collision, jump, gate, Moki shops and seven-heart cap\n");
  return 0;
}
