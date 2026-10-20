#include "rikitu_rules.h"
#include <stdio.h>
int main(void) {
  const int coins[]={0,119,120,159,160,199,200,239,240,720};
  int stage,bought,i;
  RikituRect a={0,0,10,10},b={9,9,1,1},edge={10,0,2,2};
  printf("box %d %d\n",rikitu_box_hit(a,b),rikitu_box_hit(a,edge));
  printf("gate %d %d %d\n",rikitu_can_exit(2,0,1),
         rikitu_can_exit(3,1,0),rikitu_can_exit(3,1,1));
  printf("jump %d %d %d\n",rikitu_can_jump(0,0.0f),
         rikitu_can_jump(0,0.01f),rikitu_can_jump(1,0.0f));
  printf("stomp %d %d %d\n",rikitu_stomp_from_above(95,102,100,1),
         rikitu_stomp_from_above(113,120,100,1),
         rikitu_stomp_from_above(95,102,100,0));
  for(stage=0;stage<=18;stage++) {
    printf("unlock %d %d %d\n",stage,rikitu_is_shop_stage(stage),
           rikitu_unlocked_upgrades(stage));
    for(bought=-1;bought<=4;bought++)for(i=0;i<10;i++){
      RikituHeartOffer o=rikitu_next_heart_offer(stage,bought,coins[i]);
      RikituHeartPurchase p=rikitu_buy_heart(stage,bought,coins[i]);
      printf("offer %d %d %d %d %d %d %d %d %d %d %d %d\n",
             stage,bought,coins[i],o.available,(int)o.reason,o.price,
             o.hearts_after,o.unlocked,p.success,p.purchased_count,
             p.coins,p.hearts);
    }
  }
  return 0;
}
