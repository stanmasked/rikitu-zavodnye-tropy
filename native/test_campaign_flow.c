#include "rikitu_campaign.h"
#include <assert.h>
#include <stdio.h>
static RikituStage done(int level,int coins){
  RikituStage s;
  rikitu_stage_init(&s,level%2==0);
  s.fragment_count=3;
  s.fragments[0]=1;s.fragments[1]=1;s.fragments[2]=1;
  s.coin_count=coins;s.completed=1;
  if(s.boss_required)s.boss_defeated=1;
  return s;
}
int main(void){
  RikituCampaign c;
  RikituStage stage;
  int i,shops=0,bought=0;
  assert(!rikitu_campaign_init(NULL,1));
  assert(!rikitu_campaign_init(&c,0));
  assert(!rikitu_campaign_init(&c,17));
  assert(rikitu_campaign_init(&c,1));
  assert(c.wallet==0 && c.mode==RIKITU_CAMPAIGN_PLAYING);
  for(i=1;i<=16;i++){
    RikituHeartOffer offer;
    assert(c.level==i&&c.mode==RIKITU_CAMPAIGN_PLAYING);
    stage=done(i,90);
    assert(!rikitu_campaign_buy(&c).success);
    stage.completed=0;
    assert(!rikitu_campaign_finish(&c,&stage));
    stage.completed=1;
    if(i%2==0){
      stage.boss_defeated=0;
      assert(!rikitu_campaign_finish(&c,&stage));
      stage.boss_defeated=1;
    }
    assert(rikitu_campaign_finish(&c,&stage));
    assert(!rikitu_campaign_finish(&c,&stage));
    assert(rikitu_campaign_valid(&c));
    if(i%2){
      assert(c.mode==RIKITU_CAMPAIGN_BETWEEN);
      assert(!rikitu_campaign_buy(&c).success);
      assert(rikitu_campaign_continue(&c)==1);
    }else{
      assert(c.mode==RIKITU_CAMPAIGN_SHOP);
      shops++;
      offer=rikitu_campaign_offer(&c);
      if(i==2||i==6||i==10||i==14){
        RikituHeartPurchase p;
        assert(offer.available&&offer.reason==RIKITU_OFFER_READY);
        p=rikitu_campaign_buy(&c);
        assert(p.success&&c.purchased_hearts==++bought);
        assert(c.max_hearts==RIKITU_STARTING_HEARTS+bought);
        assert(!rikitu_campaign_buy(&c).success);
      }else{
        assert(!offer.available&&!rikitu_campaign_buy(&c).success);
      }
      assert(rikitu_campaign_continue(&c)==(i==16?2:1));
    }
    assert(c.level==(i==16?16:i+1));
  }
  assert(shops==8&&bought==4);
  assert(c.mode==RIKITU_CAMPAIGN_VICTORY&&c.max_hearts==7);
  assert(c.wallet==1440-(120+160+200+240));
  assert(!rikitu_campaign_continue(&c));
  assert(!rikitu_campaign_finish(&c,&stage));
  assert(rikitu_campaign_init(&c,1));
  stage=done(1,91);
  assert(!rikitu_campaign_finish(&c,&stage)&&c.wallet==0);
  stage.coin_count=-1;
  assert(!rikitu_campaign_finish(&c,&stage));
  stage.coin_count=90;stage.fragment_count=2;
  assert(!rikitu_campaign_finish(&c,&stage));
  stage.fragment_count=3;
  assert(rikitu_campaign_finish(&c,&stage)&&c.wallet==90);
  c.max_hearts=99;
  assert(!rikitu_campaign_valid(&c));
  assert(!rikitu_campaign_continue(&c));
  assert(!rikitu_campaign_buy(&c).success);
  puts("PASS campaign: 16 stages, 8 Moki visits, 4 upgrades, final victory");
  return 0;
}
