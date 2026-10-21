/* Stage72: each MAP02..MAP16 uses the real C99 engine for pickups,
 * checkpoint crossing, two RKL3 writes, restart and repeated pickup ticks.
 * Controlled spawn positions deliberately isolate save/reboot semantics.
 * This is not a controller-only playthrough or physical Switch SD test.
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
#include "rikitu_demo.h"
#include "rikitu_level_save.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SLOT_A "/tmp/rikitu-stage72-allmaps-a.rkl"
#define SLOT_B "/tmp/rikitu-stage72-allmaps-b.rkl"

typedef void (*SceneInit)(RikituScene *);
static const SceneInit scenes[16]={
  rikitu_flower_init,rikitu_mill_init,rikitu_grove_init,rikitu_bridge_init,
  rikitu_workshop_init,rikitu_delivery_init,rikitu_clock_init,rikitu_orders_init,
  rikitu_mist_init,rikitu_cloud_init,rikitu_crystal_init,rikitu_mine_init,
  rikitu_gardens_init,rikitu_foundry_init,rikitu_gallery_init,rikitu_heart_init
};

static void reset_scene(int level,RikituScene *s,RikituLoop *loop){
  scenes[level-1](s);
  memset(loop,0,sizeof(*loop));
  loop->player=s->player;
}
static void tick_at(RikituScene *s,RikituLoop *loop,float x,float y){
  loop->player.x=x;
  loop->player.y=y;
  loop->player.vx=0.0f;
  loop->player.vy=0.0f;
  loop->player.grounded=0;
  assert(rikitu_demo_advance(s,loop,RIKITU_FIXED_DT,0)==1);
}
static void collect_coin(RikituScene *s,RikituLoop *loop,int index){
  tick_at(s,loop,s->coins[index].x-5.0f,s->coins[index].y-3.0f);
}
static void collect_fragment(RikituScene *s,RikituLoop *loop,int index){
  tick_at(s,loop,s->fragments[index].x,s->fragments[index].y-10.0f);
}
int main(void){
  RikituScene scene,loaded;
  RikituLoop loop,restarted;
  RikituCampaign campaign,foreign;
  int level;
  assert(sizeof(scenes)/sizeof(scenes[0])==16u);
  for(level=2;level<=16;level++){
    int old_coins,old_fragments,coins,fragments,checkpoint,boss_hp,hearts;
    float checkpoint_x;
    FILE *f;
    (void)remove(SLOT_A);(void)remove(SLOT_B);
    assert(rikitu_campaign_init(&campaign,level));
    campaign.wallet=level*7;
    reset_scene(level,&scene,&loop);
    assert(scene.level_id==level && scene.coin_total==90);
    assert(scene.checkpoint_total==3);
    /* Actual game ticks award the objects and activate the checkpoint. */
    collect_coin(&scene,&loop,0);
    assert(scene.stage.coins[0] && scene.stage.coin_count>=1);
    collect_fragment(&scene,&loop,0);
    assert(scene.stage.fragments[0] && scene.stage.fragment_count>=1);
    tick_at(&scene,&loop,scene.checkpoints[0]+8.0f,413.0f);
    assert(scene.reached_checkpoints>=1);
    assert(!scene.stage.completed);
    old_coins=scene.stage.coin_count;
    old_fragments=scene.stage.fragment_count;
    assert(rikitu_level_save_store_pair(SLOT_A,SLOT_B,
                                         &scene,&loop,&campaign));
    /* Another genuine pickup tick produces the newer RKL3 generation. */
    collect_coin(&scene,&loop,1);
    assert(scene.stage.coins[1]);
    assert(rikitu_level_save_store_pair(SLOT_A,SLOT_B,
                                         &scene,&loop,&campaign));
    coins=scene.stage.coin_count;
    fragments=scene.stage.fragment_count;
    checkpoint=scene.reached_checkpoints;
    checkpoint_x=scene.active_checkpoint;
    boss_hp=scene.boss.hp;
    hearts=scene.hearts;
    reset_scene(level,&loaded,&restarted);
    assert(rikitu_level_save_load_pair(SLOT_A,SLOT_B,
                                        &loaded,&restarted,&campaign));
    assert(loaded.stage.coin_count==coins &&
           loaded.stage.fragment_count==fragments);
    assert(loaded.stage.coins[0] && loaded.stage.coins[1]);
    assert(loaded.stage.fragments[0]);
    assert(loaded.reached_checkpoints==checkpoint);
    assert(loaded.active_checkpoint==checkpoint_x);
    assert(loaded.boss.hp==boss_hp && loaded.hearts==hearts);
    assert(restarted.player.x==checkpoint_x);
    assert(loaded.invulnerability_seconds>0.0f);
    collect_coin(&loaded,&restarted,0);
    collect_coin(&loaded,&restarted,1);
    collect_fragment(&loaded,&restarted,0);
    assert(loaded.stage.coin_count==coins &&
           loaded.stage.fragment_count==fragments);
    assert(!loaded.stage.completed);

    /* A wallet for another campaign cannot mutate a fresh scene. */
    foreign=campaign;foreign.wallet++;
    reset_scene(level,&loaded,&restarted);
    assert(!rikitu_level_save_load_pair(SLOT_A,SLOT_B,
                                         &loaded,&restarted,&foreign));
    assert(loaded.stage.coin_count==0 &&
           loaded.stage.fragment_count==0 &&
           loaded.reached_checkpoints==0);
    /* Nor can the save leak to a neighboring chapter. */
    foreign=campaign;foreign.level=level==16?2:level+1;
    reset_scene(foreign.level,&loaded,&restarted);
    assert(!rikitu_level_save_load_pair(SLOT_A,SLOT_B,
                                         &loaded,&restarted,&foreign));
    assert(loaded.level_id==foreign.level &&
           loaded.stage.coin_count==0 &&
           loaded.reached_checkpoints==0);

    /* Corrupt newer B and confirm older checkpoint A survives. */
    f=fopen(SLOT_B,"r+b");assert(f);
    assert(fseek(f,14,SEEK_SET)==0 && fputc(255,f)!=EOF);
    assert(fclose(f)==0);
    reset_scene(level,&loaded,&restarted);
    assert(rikitu_level_save_load_pair(SLOT_A,SLOT_B,
                                        &loaded,&restarted,&campaign));
    assert(loaded.stage.coin_count==old_coins &&
           loaded.stage.fragment_count==old_fragments);
    assert(loaded.stage.coins[0] && loaded.stage.fragments[0]);
    assert(loaded.reached_checkpoints>=1);
    printf("PASS MAP%02d RKL3 game-loop pickups / checkpoint / reboot / replay / fallback\n",
           level);
  }
  (void)remove(SLOT_A);(void)remove(SLOT_B);
  puts("PASS 15 MAP02-MAP16 real-loop checkpoint replay cases");
  return 0;
}
