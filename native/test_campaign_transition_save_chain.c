/* Stage94: all sixteen native map scenes -> real stage APIs -> mandatory
 * between/shop phases -> persisted RKC2 round trips -> actual next scene.
 *
 * This is a focused campaign integration fixture, NOT a controller-only
 * platforming route: coordinates are supplied to the genuine pickup/gate
 * collision APIs so every chapter transition can be exercised quickly.
 * Stage81..91 separately verify actual controller-only movement/routes.
 */
#include "rikitu_flower.h"
#include "rikitu_mill.h"
#include "rikitu_grove.h"
#include "rikitu_bridge.h"
#include "rikitu_workshop.h"
#include "rikitu_delivery.h"
#include "rikitu_clock.h"
#include "rikitu_orders.h"
#include "rikitu_mist.h"
#include "rikitu_cloud.h"
#include "rikitu_crystal.h"
#include "rikitu_mine.h"
#include "rikitu_gardens.h"
#include "rikitu_foundry.h"
#include "rikitu_gallery.h"
#include "rikitu_heart.h"
#include "rikitu_campaign_save.h"
#include "rikitu_draw.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SLOT_A "/tmp/rikitu-stage94-chain-a.rkc"
#define SLOT_B "/tmp/rikitu-stage94-chain-b.rkc"
enum { W=1280,H=720 };
typedef void (*SceneInit)(RikituScene *);
static const SceneInit scenes[16]={
  rikitu_flower_init,rikitu_mill_init,rikitu_grove_init,rikitu_bridge_init,
  rikitu_workshop_init,rikitu_delivery_init,rikitu_clock_init,rikitu_orders_init,
  rikitu_mist_init,rikitu_cloud_init,rikitu_crystal_init,rikitu_mine_init,
  rikitu_gardens_init,rikitu_foundry_init,rikitu_gallery_init,rikitu_heart_init
};
static void begin(RikituScene *s,RikituLoop *loop,const RikituCampaign *c){
  assert(rikitu_campaign_valid(c));
  assert(c->mode==RIKITU_CAMPAIGN_PLAYING);
  scenes[c->level-1](s);
  memset(loop,0,sizeof(*loop));
  loop->player=s->player;
  s->hearts=s->max_hearts=c->max_hearts;
  assert(s->level_id==c->level && s->hearts==c->max_hearts);
  assert(s->stage.fragment_count==0 && !s->stage.completed);
}
static void save_reload(RikituCampaign *c){
  RikituCampaign loaded;
  memset(&loaded,0xa5,sizeof(loaded));
  assert(rikitu_campaign_save_store_pair(SLOT_A,SLOT_B,c));
  assert(rikitu_campaign_save_load_pair(SLOT_A,SLOT_B,&loaded));
  assert(loaded.level==c->level && loaded.mode==c->mode);
  assert(loaded.wallet==c->wallet);
  assert(loaded.max_hearts==c->max_hearts &&
         loaded.purchased_hearts==c->purchased_hearts);
  *c=loaded;
}
static void render_phase(uint32_t *pixels,const RikituCampaign *c){
  const RikituHeartOffer offer=rikitu_campaign_offer(c);
  const size_t center=(size_t)(H/2)*W+(W/2);
  memset(pixels,0,(size_t)W*H*sizeof(uint32_t));
  assert(rikitu_draw_campaign_overlay(pixels,W,H,W,c,&offer));
  assert(pixels[center]!=0u);
}
static void finish_actual_stage(RikituScene *s){
  const RikituRect gate={s->gate_x-25.0f,413.0f,33.0f,43.0f};
  int i;
  assert(!rikitu_stage_can_finish(&s->stage));
  assert(rikitu_stage_touch_gate(&s->stage,gate,s->gate_x)
         ==RIKITU_GATE_BLOCKED);
  /* The actual map's ninety coins and three fragments are collected via
   * their real collision API (fixture coordinates, no state flag writes). */
  assert(s->coin_total==90 && s->enemy_count==6);
  for(i=0;i<90;i++){
    const RikituRect at={s->coins[i].x-5.0f,s->coins[i].y-5.0f,25.0f,25.0f};
    assert(rikitu_stage_pickups(&s->stage,at,s->fragments,
                                s->coins,s->coin_total)>=1);
  }
  for(i=0;i<3;i++){
    const RikituRect at={
      s->fragments[i].x,s->fragments[i].y,33.0f,43.0f
    };
    assert(rikitu_stage_pickups(&s->stage,at,s->fragments,
                                s->coins,s->coin_total)>=1);
  }
  assert(s->stage.fragment_count==3 && s->stage.coin_count==90);
  if(s->stage.boss_required){
    int hit;
    assert(s->boss.alive && s->boss.hp==3);
    assert(rikitu_stage_touch_gate(&s->stage,gate,s->gate_x)
           ==RIKITU_GATE_BLOCKED);
    for(hit=0;hit<3;hit++){
      RikituPlayer p=s->player;
      RikituBossEvent e;
      p.x=s->boss.x+1.0f;
      p.y=s->boss.y-p.h+2.0f;
      p.vy=330.0f;
      e=rikitu_boss_tick(&s->boss,&p,s->boss.y-1.0f,
                         RIKITU_FIXED_DT);
      assert(e==(hit==2?RIKITU_BOSS_DEFEATED:RIKITU_BOSS_HURT));
      if(hit<2){
        int frame;
        for(frame=0;frame<54;frame++){
          p.x=s->boss.center+1000.0f;
          p.y=s->boss.y-p.h+2.0f;
          p.vy=0.0f;
          assert(rikitu_boss_tick(&s->boss,&p,s->boss.y-1.0f,
                                  RIKITU_FIXED_DT)==RIKITU_BOSS_NONE);
        }
      }
    }
    assert(!s->boss.alive && s->boss.hp==0);
    /* Mirror normal game-loop bookkeeping of RIKITU_BOSS_DEFEATED. */
    s->stage.boss_defeated=1;
  }
  assert(rikitu_stage_can_finish(&s->stage));
  assert(rikitu_stage_touch_gate(&s->stage,gate,s->gate_x)
         ==RIKITU_GATE_COMPLETED);
  assert(s->stage.completed);
}
int main(void){
  RikituCampaign c;
  RikituScene s;
  RikituLoop loop;
  uint32_t *pixels=(uint32_t*)calloc((size_t)W*H,sizeof(uint32_t));
  int level,shops=0,between=0,purchases=0;
  assert(pixels);
  (void)remove(SLOT_A);(void)remove(SLOT_B);
  assert(rikitu_campaign_init(&c,1));
  assert(sizeof(scenes)/sizeof(scenes[0])==16u);
  for(level=1;level<=16;level++){
    const int last_hearts=c.max_hearts;
    const int before_wallet=c.wallet;
    int change;
    assert(c.mode==RIKITU_CAMPAIGN_PLAYING && c.level==level);
    begin(&s,&loop,&c);
    assert(s.max_hearts==last_hearts);
    assert(!rikitu_campaign_finish(&c,&s.stage));
    finish_actual_stage(&s);
    assert(rikitu_campaign_finish(&c,&s.stage));
    assert(!rikitu_campaign_finish(&c,&s.stage));
    assert(c.wallet==before_wallet+90);
    assert(c.mode==(level%2?RIKITU_CAMPAIGN_BETWEEN:
                                 RIKITU_CAMPAIGN_SHOP));
    /* This round trip models closing the NRO immediately after the exit.
     * The next operation resumes the BETWEEN/SHOP overlay, not gameplay. */
    save_reload(&c);
    render_phase(pixels,&c);
    assert(!rikitu_campaign_finish(&c,&s.stage));
    if(level%2){
      between++;
      assert(!rikitu_campaign_buy(&c).success);
    }else{
      const RikituHeartOffer offer=rikitu_campaign_offer(&c);
      shops++;
      if(level==2||level==6||level==10||level==14){
        const RikituHeartPurchase buy=rikitu_campaign_buy(&c);
        assert(offer.available);
        assert(buy.success);
        purchases++;
        assert(c.purchased_hearts==purchases);
        assert(c.max_hearts==last_hearts+1);
        /* Restart while still in Moki's shop after buying a heart. */
        save_reload(&c);
        assert(c.mode==RIKITU_CAMPAIGN_SHOP);
        render_phase(pixels,&c);
        assert(!rikitu_campaign_buy(&c).success);
      }else{
        assert(!offer.available && !rikitu_campaign_buy(&c).success);
      }
    }
    change=rikitu_campaign_continue(&c);
    assert(change==(level==16?2:1));
    assert(c.mode==(level==16?RIKITU_CAMPAIGN_VICTORY:
                             RIKITU_CAMPAIGN_PLAYING));
    assert(c.level==(level==16?16:level+1));
    /* After pressing A or B to continue, the newest slot restores exactly
     * the next chapter (or VICTORY). Stale old-level RKC2 can't win. */
    save_reload(&c);
    if(level==16){
      render_phase(pixels,&c);
      assert(!rikitu_campaign_continue(&c));
    }else{
      begin(&s,&loop,&c);
      assert(s.level_id==level+1 && s.hearts==c.max_hearts);
      assert(s.stage.fragment_count==0 && s.stage.coin_count==0);
      assert(!s.stage.completed);
    }
    printf("Stage94 MAP%02d -> %s: coins=%d hearts=%d saved PASS\n",
           level,level==16?"VICTORY":"NEXT",c.wallet,c.max_hearts);
  }
  assert(shops==8 && between==8 && purchases==4);
  assert(c.mode==RIKITU_CAMPAIGN_VICTORY && c.level==16);
  assert(c.max_hearts==7 && c.purchased_hearts==4);
  assert(c.wallet==1440-(120+160+200+240));
  (void)remove(SLOT_A);(void)remove(SLOT_B);
  free(pixels);
  puts("PASS Stage94 16/16 real map stage APIs -> phases -> RKC2 save/restart"
       " -> next chapter, 8 shops, 4 upgrades and victory");
  return 0;
}
