/* Stage74: MAP03 original spawn to first checkpoint under 60Hz button
 * input, including the real mushroom launcher, jump, collectibles,
 * RKL3 generations and CRC fallback. No player-coordinate teleports.
 * Portable host replay; not a physical Switch controller or SD test.
 */
#include "rikitu_grove.h"
#include "rikitu_demo.h"
#include "rikitu_level_save.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define A "/tmp/rikitu-stage74-grove-a.rkl"
#define B "/tmp/rikitu-stage74-grove-b.rkl"
#define RIGHT_RUN (RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN)

static void fresh(RikituScene *scene,RikituLoop *loop){
  rikitu_grove_init(scene);
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
  int frame=0,jump_coin=0,jump_fragment=0,jump_gap=0,mushroom=0;
  int coin_count,fragment_count,hearts,damage;
  int first_coin_count,first_fragment_count;
  float checkpoint;
  float elapsed_before;
  FILE *f;
  (void)remove(A);(void)remove(B);
  assert(rikitu_campaign_init(&campaign,3));
  campaign.wallet=42;
  fresh(&scene,&loop);
  assert(scene.level_id==3 && loop.player.x==105.0f);
  while(scene.reached_checkpoints==0 && frame<1600){
    unsigned buttons=RIGHT_RUN;
    if(!jump_coin && loop.player.grounded && loop.player.x>109.0f){
      buttons|=RIKITU_INPUT_JUMP;jump_coin=1;
    }else if(!jump_fragment && jump_coin &&
             loop.player.grounded &&
             loop.player.x>1400.0f && loop.player.x<1510.0f){
      buttons|=RIKITU_INPUT_JUMP;jump_fragment=1;
    }else if(!jump_gap && jump_fragment &&
             loop.player.grounded &&
             loop.player.x>1590.0f && loop.player.x<1690.0f){
      buttons|=RIKITU_INPUT_JUMP;jump_gap=1;
    }
    tick(&scene,&loop,buttons);
    if(loop.player.x>880.0f && loop.player.x<1010.0f &&
       loop.player.vy< -700.0f)mushroom=1;
    frame++;
  }
  if(scene.reached_checkpoints!=1 || !jump_coin ||
     !jump_fragment || !jump_gap || !mushroom ||
     !scene.stage.coins[0] || !scene.stage.fragments[0]){
    fprintf(stderr,"MAP03 route frames=%d x=%.2f checkpoint=%d "
            "jump=%d%d%d mushroom=%d coin=%u frag=%u\n",
            frame,(double)loop.player.x,scene.reached_checkpoints,
            jump_coin,jump_fragment,jump_gap,mushroom,
            (unsigned)scene.stage.coins[0],
            (unsigned)scene.stage.fragments[0]);
    return 1;
  }
  assert(frame>350 && frame<700);
  assert(scene.active_checkpoint==scene.checkpoints[0]);
  assert(loop.player.x>=scene.active_checkpoint);
  checkpoint=scene.active_checkpoint;
  first_coin_count=scene.stage.coin_count;
  first_fragment_count=scene.stage.fragment_count;
  assert(first_coin_count>0 && first_fragment_count>0);
  assert(rikitu_level_save_store_pair(A,B,&scene,&loop,&campaign));
  assert(generation(A)==1);

  /* Keep moving via input before writing generation 2; the old slot
   * must remain recoverable if the second write is damaged. */
  for(frame=0;frame<30;frame++)tick(&scene,&loop,RIGHT_RUN);
  assert(scene.reached_checkpoints==1);
  assert(loop.player.x>checkpoint+100.0f);
  coin_count=scene.stage.coin_count;
  fragment_count=scene.stage.fragment_count;
  hearts=scene.hearts;
  damage=scene.damage_count;
  elapsed_before=loop.active_seconds;
  assert(rikitu_level_save_store_pair(A,B,&scene,&loop,&campaign));
  assert(generation(B)==2);

  fresh(&loaded,&reboot);
  assert(rikitu_level_save_load_pair(A,B,&loaded,&reboot,&campaign));
  assert(loaded.reached_checkpoints==1 &&
         loaded.active_checkpoint==checkpoint);
  assert(reboot.player.x==checkpoint &&
         loaded.player.x==checkpoint);
  assert(loaded.stage.coins[0] && loaded.stage.fragments[0]);
  assert(loaded.stage.coin_count==coin_count &&
         loaded.stage.fragment_count==fragment_count);
  assert(loaded.hearts==hearts && loaded.damage_count==damage);
  assert(reboot.active_seconds>0.0f &&
         reboot.active_seconds<=elapsed_before+0.002f);
  /* Initial 1 second invulnerability protects the restarted scene. */
  assert(loaded.invulnerability_seconds>0.0f);
  for(frame=0;frame<22;frame++)tick(&loaded,&reboot,RIGHT_RUN);
  assert(reboot.player.x>checkpoint+75.0f);
  assert(loaded.reached_checkpoints==1);
  assert(loaded.stage.coins[0] && loaded.stage.fragments[0]);

  wrong=campaign;wrong.wallet++;
  fresh(&backup,&old_loop);
  assert(!rikitu_level_save_load_pair(A,B,&backup,&old_loop,&wrong));
  assert(backup.stage.coin_count==0 &&
         backup.reached_checkpoints==0);

  /* Corrupt B's CRC-protected payload, reload generation 1 from A. */
  f=fopen(B,"r+b");assert(f);
  assert(fseek(f,14,SEEK_SET)==0 && fputc(255,f)!=EOF);
  assert(fclose(f)==0);
  fresh(&backup,&old_loop);
  assert(rikitu_level_save_load_pair(A,B,&backup,&old_loop,&campaign));
  assert(backup.reached_checkpoints==1 &&
         backup.active_checkpoint==checkpoint);
  assert(backup.stage.coin_count==first_coin_count &&
         backup.stage.fragment_count==first_fragment_count);
  assert(backup.stage.coins[0] && backup.stage.fragments[0]);
  assert(old_loop.player.x==checkpoint);
  (void)remove(A);(void)remove(B);
  puts("PASS MAP03 controller route, mushroom bounce, checkpoint, "
       "RKL3 resume and older-slot recovery");
  return 0;
}
