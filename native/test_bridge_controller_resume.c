/* Stage75: MAP04 authentic spawn and 60 Hz button input -> enemy
 * contact -> checkpoint -> first touch lever -> RKL3 restart and fallback.
 * No player position edits / teleports. Host replay, not physical Switch.
 */
#include "rikitu_bridge.h"
#include "rikitu_demo.h"
#include "rikitu_level_save.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define A "/tmp/rikitu-stage75-bridge-a.rkl"
#define B "/tmp/rikitu-stage75-bridge-b.rkl"
#define RIGHT_RUN (RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN)

static void fresh(RikituScene *scene,RikituLoop *loop){
  rikitu_bridge_init(scene);
  memset(loop,0,sizeof(*loop));
  loop->player=scene->player;
}
static void tick(RikituScene *scene,RikituLoop *loop,unsigned buttons){
  assert(rikitu_demo_advance(scene,loop,RIKITU_FIXED_DT,buttons)==1);
}
static uint32_t generation(const char *path){
  uint8_t bytes[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t value=0;
  FILE *f=fopen(path,"rb");
  assert(f);
  assert(fread(bytes,1,sizeof(bytes),f)==sizeof(bytes));
  assert(fclose(f)==0);
  assert(rikitu_level_save_inspect(bytes,sizeof(bytes),&value));
  return value;
}
int main(void){
  RikituScene scene,loaded,backup;
  RikituLoop loop,reboot,old_loop;
  RikituCampaign campaign,wrong;
  float checkpoint,elapsed;
  int frame=0,jump_coin=0,jump_fragment=0,jump_gap=0;
  int damage,hearts,coins,fragments,old_coins,old_fragments;
  FILE *f;
  (void)remove(A);(void)remove(B);
  assert(rikitu_campaign_init(&campaign,4));
  campaign.wallet=55;
  fresh(&scene,&loop);
  assert(scene.level_id==4 && loop.player.x==105.0f);
  assert(scene.lever_count==3 && scene.levers_on==0);
  assert(scene.stage.boss_required && scene.boss.hp==3);

  /* From the original spawn: start jump reaches the first coin, a
   * separate jump picks up fragment 1, and a third crosses the gap.
   * The first patrol contacts the player through real enemy physics. */
  while(scene.reached_checkpoints==0 && frame<1600){
    unsigned buttons=RIGHT_RUN;
    if(!jump_coin && loop.player.grounded && loop.player.x>109.0f){
      buttons|=RIKITU_INPUT_JUMP;jump_coin=1;
    }else if(!jump_fragment && jump_coin && loop.player.grounded &&
             loop.player.x>1400.0f && loop.player.x<1510.0f){
      buttons|=RIKITU_INPUT_JUMP;jump_fragment=1;
    }else if(!jump_gap && jump_fragment && loop.player.grounded &&
             loop.player.x>1660.0f && loop.player.x<1780.0f){
      buttons|=RIKITU_INPUT_JUMP;jump_gap=1;
    }
    tick(&scene,&loop,buttons);
    frame++;
  }
  if(scene.reached_checkpoints!=1 || !jump_coin ||
     !jump_fragment || !jump_gap ||
     !scene.stage.coins[0] || !scene.stage.fragments[0]){
    fprintf(stderr,"MAP04 route: frame=%d x=%.2f cp=%d "
            "jumps=%d%d%d coin=%u fragment=%u damage=%d\n",
            frame,(double)loop.player.x,scene.reached_checkpoints,
            jump_coin,jump_fragment,jump_gap,
            (unsigned)scene.stage.coins[0],
            (unsigned)scene.stage.fragments[0],scene.damage_count);
    return 1;
  }
  assert(frame>400 && frame<700);
  assert(scene.active_checkpoint==scene.checkpoints[0]);
  assert(scene.active_checkpoint==2100.0f);
  assert(scene.levers_on==0 && !scene.levers[0].on);
  /* Enemy contact before the checkpoint is part of this route,
   * unlike artificial assignments to hearts/enemy bitsets. */
  assert(scene.damage_count>=1 && scene.hearts<scene.max_hearts);
  assert(!scene.enemies[0].dead);
  checkpoint=scene.active_checkpoint;
  old_coins=scene.stage.coin_count;
  old_fragments=scene.stage.fragment_count;
  assert(rikitu_level_save_store_pair(A,B,&scene,&loop,&campaign));
  assert(generation(A)==1);

  /* Walk to the switch at x=2260. MAP04 levers are triggered by player
   * overlap; they are not buttons and are not mandatory gate keys. */
  for(frame=0;scene.levers_on==0 && frame<100;frame++){
    tick(&scene,&loop,RIGHT_RUN);
  }
  if(!scene.levers[0].on || scene.levers_on!=1 ||
     scene.reached_checkpoints!=1){
    fprintf(stderr,"MAP04 lever not reached: frame=%d x=%.2f on=%d\n",
            frame,(double)loop.player.x,scene.levers_on);
    return 2;
  }
  assert(loop.player.x>2200.0f && loop.player.x<2330.0f);
  for(frame=0;frame<18;frame++)tick(&scene,&loop,RIGHT_RUN);
  assert(scene.levers_on==1 && scene.levers[0].on);
  assert(scene.stage.coins[0] && scene.stage.fragments[0]);
  assert(scene.boss.hp==3 && scene.boss.alive);
  coins=scene.stage.coin_count;
  fragments=scene.stage.fragment_count;
  hearts=scene.hearts;
  damage=scene.damage_count;
  elapsed=loop.active_seconds;
  assert(rikitu_level_save_store_pair(A,B,&scene,&loop,&campaign));
  assert(generation(B)==2);

  fresh(&loaded,&reboot);
  assert(rikitu_level_save_load_pair(A,B,&loaded,&reboot,&campaign));
  assert(loaded.active_checkpoint==checkpoint &&
         loaded.reached_checkpoints==1);
  assert(loaded.levers_on==1 && loaded.levers[0].on);
  assert(loaded.stage.coins[0] && loaded.stage.fragments[0]);
  assert(loaded.stage.coin_count==coins &&
         loaded.stage.fragment_count==fragments);
  assert(loaded.hearts==hearts && loaded.damage_count==damage);
  assert(!loaded.enemies[0].dead);
  assert(loaded.boss.hp==3 && loaded.boss.alive);
  assert(reboot.player.x==checkpoint);
  assert(reboot.active_seconds>0.0f &&
         reboot.active_seconds<=elapsed+0.002f);
  assert(loaded.invulnerability_seconds>0.0f);

  /* Retouching an already ON lever does not increment the count again. */
  for(frame=0;frame<60;frame++)tick(&loaded,&reboot,RIGHT_RUN);
  assert(reboot.player.x>checkpoint+200.0f);
  assert(loaded.levers_on==1 && loaded.levers[0].on);
  assert(loaded.stage.coins[0] && loaded.stage.fragments[0]);
  assert(loaded.reached_checkpoints==1);

  /* Wrong campaign must not mutate a clean level scene. */
  wrong=campaign;wrong.wallet++;
  fresh(&backup,&old_loop);
  assert(!rikitu_level_save_load_pair(A,B,&backup,&old_loop,&wrong));
  assert(backup.levers_on==0 && backup.stage.coin_count==0 &&
         backup.reached_checkpoints==0);

  /* Newer generation B is corrupted; A correctly restores the same
   * checkpoint but the switch is OFF. Replay input toggles it again. */
  f=fopen(B,"r+b");assert(f);
  assert(fseek(f,14,SEEK_SET)==0 && fputc(255,f)!=EOF);
  assert(fclose(f)==0);
  fresh(&backup,&old_loop);
  assert(rikitu_level_save_load_pair(A,B,&backup,&old_loop,&campaign));
  assert(backup.reached_checkpoints==1 &&
         backup.active_checkpoint==checkpoint);
  assert(backup.levers_on==0 && !backup.levers[0].on);
  assert(backup.stage.coin_count==old_coins &&
         backup.stage.fragment_count==old_fragments);
  assert(backup.stage.coins[0] && backup.stage.fragments[0]);
  assert(old_loop.player.x==checkpoint);
  for(frame=0;backup.levers_on==0 && frame<90;frame++){
    tick(&backup,&old_loop,RIGHT_RUN);
  }
  assert(backup.levers_on==1 && backup.levers[0].on);
  assert(backup.reached_checkpoints==1);
  (void)remove(A);(void)remove(B);
  puts("PASS MAP04 button route -> real enemy contact -> fragment "
       "-> checkpoint -> touch lever -> RKL3 reboot and CRC rollback");
  return 0;
}
