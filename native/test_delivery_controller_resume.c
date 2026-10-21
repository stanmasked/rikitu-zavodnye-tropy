/* Stage77: MAP06 authentic spawn, 60 Hz button input, +42 conveyor,
 * -705 steam launch, first fragment, checkpoint and two-slot RKL3.
 * No direct player-position changes. This is host input replay,
 * not a physical Switch HID / SD power-loss test.
 */
#include "rikitu_delivery.h"
#include "rikitu_demo.h"
#include "rikitu_level_save.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define A "/tmp/rikitu-stage77-delivery-a.rkl"
#define B "/tmp/rikitu-stage77-delivery-b.rkl"
#define RIGHT_RUN (RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN)

static void fresh(RikituScene *scene,RikituLoop *loop){
  rikitu_delivery_init(scene);
  memset(loop,0,sizeof(*loop));
  loop->player=scene->player;
}
static void tick(RikituScene *scene,RikituLoop *loop,unsigned input){
  assert(rikitu_demo_advance(scene,loop,RIKITU_FIXED_DT,input)==1);
}
static uint32_t inspect(const char *path){
  uint8_t bytes[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t gen=0;
  FILE *f=fopen(path,"rb");
  assert(f);
  assert(fread(bytes,1,sizeof(bytes),f)==sizeof(bytes));
  assert(fclose(f)==0);
  assert(rikitu_level_save_inspect(bytes,sizeof(bytes),&gen));
  return gen;
}

int main(void){
  RikituScene scene,loaded,backup;
  RikituLoop loop,reboot,old_loop;
  RikituCampaign campaign,wrong;
  int frames=0,jump_coin=0,jump_fragment=0,jump_gap=0;
  int belt_ticks=0,steam_ticks=0;
  int old_coins,old_fragments,coins,fragments,hearts,damage;
  float checkpoint,active_time;
  FILE *f;
  (void)remove(A);(void)remove(B);
  assert(rikitu_campaign_init(&campaign,6));
  campaign.wallet=66;
  fresh(&scene,&loop);
  assert(scene.level_id==6 && loop.player.x==105.0f);
  assert(scene.belt_count==3 && scene.belts[0].speed==42.0f);
  assert(scene.launcher_count==3 &&
         scene.launchers[0].kind==RIKITU_LAUNCH_STEAM);
  assert(scene.stage.boss_required && scene.boss.hp==3);

  /* Authentic movement from spawn. The first steam vent automatically
   * launches the grounded player. Later jumps collect fragment #1
   * and cross the 1870..1940 floor gap. */
  while(scene.reached_checkpoints==0 && frames<1250){
    const float before_x=loop.player.x;
    const float before_y=loop.player.y;
    unsigned buttons=RIGHT_RUN;
    if(!jump_coin && loop.player.grounded && loop.player.x>109.0f){
      buttons|=RIKITU_INPUT_JUMP;jump_coin=1;
    }else if(!jump_fragment && jump_coin &&
             loop.player.grounded &&
             loop.player.x>1590.0f && loop.player.x<1655.0f){
      buttons|=RIKITU_INPUT_JUMP;jump_fragment=1;
    }else if(!jump_gap && jump_fragment &&
             loop.player.grounded &&
             loop.player.x>1810.0f && loop.player.x<1880.0f){
      buttons|=RIKITU_INPUT_JUMP;jump_gap=1;
    }
    tick(&scene,&loop,buttons);

    /* On level ground, the conveyor adds exactly 42 units per second
     * to the player's button-controlled 265 units per second. */
    if(before_x>620.0f && before_x<750.0f &&
       fabsf(before_y-413.0f)<0.01f && loop.player.grounded){
      const float dx=loop.player.x-before_x;
      const float expected=(265.0f+42.0f)*RIKITU_FIXED_DT;
      if(fabsf(dx-expected)>0.02f){
        fprintf(stderr,"MAP06 belt dx %.4f expected %.4f\n",
                (double)dx,(double)expected);
        return 1;
      }
      belt_ticks++;
    }
    if(before_x>1260.0f && before_x<1430.0f &&
       fabsf(loop.player.vy+705.0f)<0.01f &&
       !loop.player.grounded)steam_ticks++;
    frames++;
  }
  if(scene.reached_checkpoints!=1 || !jump_coin ||
     !jump_fragment || !jump_gap || belt_ticks<10 ||
     steam_ticks!=1 || !scene.stage.coins[0] ||
     !scene.stage.fragments[0]){
    fprintf(stderr,"MAP06 route frames=%d x=%.2f checkpoint=%d "
            "jumps=%d%d%d belt=%d steam=%d coin=%u frag=%u\n",
            frames,(double)loop.player.x,scene.reached_checkpoints,
            jump_coin,jump_fragment,jump_gap,belt_ticks,steam_ticks,
            (unsigned)scene.stage.coins[0],
            (unsigned)scene.stage.fragments[0]);
    return 2;
  }
  assert(frames>400 && frames<700);
  assert(scene.active_checkpoint==scene.checkpoints[0] &&
         scene.active_checkpoint==2300.0f);
  assert(scene.damage_count>=1 && scene.hearts<scene.max_hearts);
  assert(scene.stage.fragment_count>=1 &&
         scene.stage.coin_count>0);
  assert(scene.boss.hp==3 && scene.boss.alive &&
         !scene.stage.boss_defeated);
  checkpoint=scene.active_checkpoint;
  old_coins=scene.stage.coin_count;
  old_fragments=scene.stage.fragment_count;
  assert(rikitu_level_save_store_pair(A,B,&scene,&loop,&campaign));
  assert(inspect(A)==1);

  /* Generate a second, newer save using actual rightward gameplay. */
  for(frames=0;frames<30;frames++)tick(&scene,&loop,RIGHT_RUN);
  assert(scene.reached_checkpoints==1 &&
         loop.player.x>checkpoint+100.0f);
  coins=scene.stage.coin_count;
  fragments=scene.stage.fragment_count;
  hearts=scene.hearts;
  damage=scene.damage_count;
  active_time=loop.active_seconds;
  assert(rikitu_level_save_store_pair(A,B,&scene,&loop,&campaign));
  assert(inspect(B)==2);

  fresh(&loaded,&reboot);
  assert(rikitu_level_save_load_pair(A,B,&loaded,&reboot,&campaign));
  assert(loaded.reached_checkpoints==1 &&
         loaded.active_checkpoint==checkpoint);
  assert(reboot.player.x==checkpoint &&
         loaded.player.x==checkpoint);
  assert(loaded.stage.coins[0] && loaded.stage.fragments[0]);
  assert(loaded.stage.coin_count==coins &&
         loaded.stage.fragment_count==fragments);
  assert(loaded.hearts==hearts &&
         loaded.damage_count==damage);
  assert(loaded.boss.hp==3 && loaded.boss.alive);
  assert(loaded.invulnerability_seconds>0.0f);
  assert(reboot.active_seconds>0.0f &&
         reboot.active_seconds<=active_time+0.002f);
  for(frames=0;frames<28;frames++)tick(&loaded,&reboot,RIGHT_RUN);
  assert(reboot.player.x>checkpoint+100.0f);
  assert(loaded.stage.coins[0] && loaded.stage.fragments[0]);
  assert(loaded.reached_checkpoints==1);

  wrong=campaign;wrong.wallet++;
  fresh(&backup,&old_loop);
  assert(!rikitu_level_save_load_pair(A,B,&backup,&old_loop,&wrong));
  assert(backup.reached_checkpoints==0 &&
         backup.stage.coin_count==0);

  /* Corrupt generation 2; fallback must retain the older checkpoint
   * without inventing player motion or awarding collected pickups. */
  f=fopen(B,"r+b");assert(f);
  assert(fseek(f,14,SEEK_SET)==0 && fputc(255,f)!=EOF);
  assert(fclose(f)==0);
  fresh(&backup,&old_loop);
  assert(rikitu_level_save_load_pair(A,B,&backup,&old_loop,&campaign));
  assert(backup.active_checkpoint==checkpoint &&
         backup.reached_checkpoints==1);
  assert(backup.stage.coin_count==old_coins &&
         backup.stage.fragment_count==old_fragments);
  assert(backup.stage.coins[0] && backup.stage.fragments[0]);
  assert(old_loop.player.x==checkpoint);
  assert(backup.boss.hp==3 && backup.boss.alive);
  (void)remove(A);(void)remove(B);
  puts("PASS MAP06 controller route, first belt + steam, fragment, "
       "checkpoint, RKL3 cold reload and fallback");
  return 0;
}
