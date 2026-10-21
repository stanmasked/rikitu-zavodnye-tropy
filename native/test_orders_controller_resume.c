/* Stage79: MAP08 Orders Depot input-only route from the real spawn.
 * Checks real conveyor, enemy contact, fragment, first touch lever,
 * checkpoint and two-generation RKL3 save with CRC fallback.
 * No direct player coordinate manipulation. Host replay, not HID/SD.
 */
#include "rikitu_orders.h"
#include "rikitu_demo.h"
#include "rikitu_level_save.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define SLOT_A "/tmp/rikitu-stage79-orders-a.rkl"
#define SLOT_B "/tmp/rikitu-stage79-orders-b.rkl"
#define RUN_RIGHT (RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN)
#define RUN_LEFT (RIKITU_INPUT_LEFT|RIKITU_INPUT_RUN)

static void fresh(RikituScene *scene,RikituLoop *loop){
  rikitu_orders_init(scene);
  memset(loop,0,sizeof(*loop));
  loop->player=scene->player;
}
static void tick(RikituScene *scene,RikituLoop *loop,unsigned buttons){
  assert(rikitu_demo_advance(scene,loop,RIKITU_FIXED_DT,buttons)==1);
}
static uint32_t generation(const char *path){
  uint8_t buf[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t gen=0;
  FILE *file=fopen(path,"rb");
  assert(file);
  assert(fread(buf,1,sizeof(buf),file)==sizeof(buf));
  assert(fclose(file)==0);
  assert(rikitu_level_save_inspect(buf,sizeof(buf),&gen));
  return gen;
}
int main(void){
  RikituScene scene,loaded,backup;
  RikituLoop loop,reboot,old_loop;
  RikituCampaign campaign,foreign;
  int frame=0,jump_coin=0,jump_fragment=0,jump_gap=0;
  int belt_frames=0,coins_a,fragments_a,hearts_a,damage_a;
  int coins_b,fragments_b,hearts_b,damage_b;
  float elapsed_b;
  FILE *file;
  (void)remove(SLOT_A);
  (void)remove(SLOT_B);
  assert(rikitu_campaign_init(&campaign,8));
  campaign.wallet=88;
  fresh(&scene,&loop);
  assert(scene.level_id==8 && loop.player.x==105.0f);
  assert(scene.belt_count==3 && scene.belts[0].speed==45.0f);
  assert(scene.lever_count==3 && scene.levers_on==0);
  assert(scene.checkpoints[0]==2380.0f);
  assert(scene.stage.boss_required && scene.boss.hp==3);

  /* Save generation A just BEFORE the first touch lever.
   * All travel uses a 60Hz player input mask, not X/Y teleports. */
  while(loop.player.x<2040.0f && frame<1200){
    const float before_x=loop.player.x;
    const float before_y=loop.player.y;
    unsigned buttons=RUN_RIGHT;
    if(!jump_coin && loop.player.grounded && loop.player.x>109.0f){
      buttons|=RIKITU_INPUT_JUMP;
      jump_coin=1;
    }else if(!jump_fragment && jump_coin &&
             loop.player.grounded &&
             loop.player.x>1610.0f && loop.player.x<1685.0f){
      buttons|=RIKITU_INPUT_JUMP;
      jump_fragment=1;
    }else if(!jump_gap && jump_fragment &&
             loop.player.grounded &&
             loop.player.x>1810.0f && loop.player.x<1885.0f){
      buttons|=RIKITU_INPUT_JUMP;
      jump_gap=1;
    }
    tick(&scene,&loop,buttons);
    if(before_x>945.0f && before_x<1040.0f &&
       fabsf(before_y-413.0f)<0.01f && loop.player.grounded){
      const float actual=loop.player.x-before_x;
      const float expected=(265.0f+45.0f)*RIKITU_FIXED_DT;
      if(fabsf(actual-expected)>0.03f){
        fprintf(stderr,"MAP08 belt dx=%.4f expected=%.4f\n",
                (double)actual,(double)expected);
        return 1;
      }
      belt_frames++;
    }
    frame++;
  }
  if(frame>=1200 || !jump_coin || !jump_fragment || !jump_gap ||
     belt_frames<10 || !scene.stage.coins[0] ||
     !scene.stage.fragments[0] || scene.reached_checkpoints!=0 ||
     scene.levers_on!=0){
    fprintf(stderr,"MAP08 pre-lever route frame=%d x=%.2f "
            "jump=%d%d%d belt=%d coin=%u frag=%u cp=%d levers=%d\n",
            frame,(double)loop.player.x,jump_coin,jump_fragment,
            jump_gap,belt_frames,(unsigned)scene.stage.coins[0],
            (unsigned)scene.stage.fragments[0],
            scene.reached_checkpoints,scene.levers_on);
    return 2;
  }
  assert(frame>380 && frame<490);
  assert(scene.damage_count>=1 && scene.hearts<scene.max_hearts);
  assert(scene.active_checkpoint==105.0f);
  assert(!scene.levers[0].on && scene.boss.hp==3);
  coins_a=scene.stage.coin_count;
  fragments_a=scene.stage.fragment_count;
  hearts_a=scene.hearts;
  damage_a=scene.damage_count;
  assert(rikitu_level_save_store_pair(SLOT_A,SLOT_B,
                                      &scene,&loop,&campaign));
  assert(generation(SLOT_A)==1);

  /* Cross the x=2120 touch lever, then reach checkpoint x=2380. */
  for(frame=0;scene.reached_checkpoints==0 && frame<130;frame++){
    tick(&scene,&loop,RUN_RIGHT);
  }
  if(scene.reached_checkpoints!=1 || scene.levers_on!=1 ||
     !scene.levers[0].on){
    fprintf(stderr,"MAP08 lever/checkpoint frame=%d x=%.2f "
            "checkpoint=%d levers=%d\n",
            frame,(double)loop.player.x,
            scene.reached_checkpoints,scene.levers_on);
    return 3;
  }
  assert(frame>50 && frame<110);
  assert(scene.active_checkpoint==2380.0f);
  assert(scene.stage.coins[0] && scene.stage.fragments[0]);
  assert(scene.boss.alive && scene.boss.hp==3);
  for(frame=0;frame<18;frame++)tick(&scene,&loop,RUN_RIGHT);
  assert(scene.levers_on==1 && scene.reached_checkpoints==1);
  coins_b=scene.stage.coin_count;
  fragments_b=scene.stage.fragment_count;
  hearts_b=scene.hearts;
  damage_b=scene.damage_count;
  elapsed_b=loop.active_seconds;
  assert(rikitu_level_save_store_pair(SLOT_A,SLOT_B,
                                      &scene,&loop,&campaign));
  assert(generation(SLOT_B)==2);

  /* Load the newer slot in a truly fresh MAP08 scene. */
  fresh(&loaded,&reboot);
  assert(rikitu_level_save_load_pair(SLOT_A,SLOT_B,
                                      &loaded,&reboot,&campaign));
  assert(loaded.active_checkpoint==2380.0f &&
         loaded.reached_checkpoints==1);
  assert(loaded.levers_on==1 && loaded.levers[0].on);
  assert(reboot.player.x==2380.0f && loaded.player.x==2380.0f);
  assert(loaded.stage.coins[0] && loaded.stage.fragments[0]);
  assert(loaded.stage.coin_count==coins_b &&
         loaded.stage.fragment_count==fragments_b);
  assert(loaded.hearts==hearts_b && loaded.damage_count==damage_b);
  assert(loaded.boss.hp==3 && loaded.boss.alive &&
         !loaded.stage.boss_defeated);
  assert(loaded.invulnerability_seconds>0.0f);
  assert(reboot.active_seconds>0.0f &&
         reboot.active_seconds<=elapsed_b+0.002f);

  /* Revisit lever 1 with LEFT then RIGHT inputs. It stays counted ONCE. */
  for(frame=0;frame<72;frame++)tick(&loaded,&reboot,RUN_LEFT);
  assert(reboot.player.x<2120.0f);
  assert(loaded.levers_on==1 && loaded.levers[0].on);
  for(frame=0;frame<73;frame++)tick(&loaded,&reboot,RUN_RIGHT);
  assert(reboot.player.x>2300.0f);
  assert(loaded.levers_on==1 && loaded.levers[0].on);
  assert(loaded.stage.coins[0] && loaded.stage.fragments[0]);

  /* Reject a different campaign wallet without touching fresh state. */
  foreign=campaign;foreign.wallet++;
  fresh(&backup,&old_loop);
  assert(!rikitu_level_save_load_pair(SLOT_A,SLOT_B,
                                       &backup,&old_loop,&foreign));
  assert(backup.reached_checkpoints==0 &&
         backup.levers_on==0 && backup.stage.coin_count==0);

  /* Corrupt the latest CRC-protected record and recover the older
   * pre-lever generation A: checkpoint 0, lever OFF, pickups intact. */
  file=fopen(SLOT_B,"r+b");
  assert(file);
  assert(fseek(file,14,SEEK_SET)==0 && fputc(255,file)!=EOF);
  assert(fclose(file)==0);
  fresh(&backup,&old_loop);
  assert(rikitu_level_save_load_pair(SLOT_A,SLOT_B,
                                      &backup,&old_loop,&campaign));
  assert(backup.reached_checkpoints==0 &&
         backup.active_checkpoint==105.0f);
  assert(backup.levers_on==0 && !backup.levers[0].on);
  assert(backup.stage.coin_count==coins_a &&
         backup.stage.fragment_count==fragments_a);
  assert(backup.stage.coins[0] && backup.stage.fragments[0]);
  assert(backup.hearts==hearts_a && backup.damage_count==damage_a);
  assert(old_loop.player.x==105.0f);
  assert(backup.boss.hp==3 && backup.boss.alive);
  (void)remove(SLOT_A);
  (void)remove(SLOT_B);
  puts("PASS MAP08 controller conveyor + patrol + fragment + touch "
       "lever + checkpoint + RKL3 rollback");
  return 0;
}
