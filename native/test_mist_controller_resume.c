/* Stage80: MAP09 Misty Pass from the authentic scene spawn.
 * Replays 60Hz RIGHT/RUN/JUMP and one mid-air neutral input frame;
 * verifies the -690 gust with ZERO involuntary horizontal drift,
 * patrol damage, fragment, first gap, and RKL3 pre/post-checkpoint
 * rollback. Never modifies player X/Y to fake a playthrough.
 * Host input-mask replay, NOT physical Joy-Con or SD power-loss proof.
 */
#include "rikitu_mist.h"
#include "rikitu_demo.h"
#include "rikitu_level_save.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define SLOT_A "/tmp/rikitu-stage80-mist-a.rkl"
#define SLOT_B "/tmp/rikitu-stage80-mist-b.rkl"
#define RUN_RIGHT (RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN)

static void fresh(RikituScene *scene,RikituLoop *loop){
  rikitu_mist_init(scene);
  memset(loop,0,sizeof(*loop));
  loop->player=scene->player;
}
static void tick(RikituScene *scene,RikituLoop *loop,unsigned input){
  assert(rikitu_demo_advance(scene,loop,RIKITU_FIXED_DT,input)==1);
}
static uint32_t generation(const char *path){
  uint8_t raw[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t gen=0;
  FILE *f=fopen(path,"rb");
  assert(f);
  assert(fread(raw,1,sizeof(raw),f)==sizeof(raw));
  assert(fclose(f)==0);
  assert(rikitu_level_save_inspect(raw,sizeof(raw),&gen));
  return gen;
}
int main(void){
  RikituScene scene,loaded,backup;
  RikituLoop loop,reboot,old_loop;
  RikituCampaign campaign,wrong;
  int frames=0,start_jump=0,fragment_jump=0,gap_jump=0;
  int gust_count=0,neutral_frames=0;
  int coins_a,fragments_a,hearts_a,damage_a;
  int coins_b,fragments_b,hearts_b,damage_b;
  float elapsed_b;
  FILE *file;
  (void)remove(SLOT_A);
  (void)remove(SLOT_B);
  assert(rikitu_campaign_init(&campaign,9));
  campaign.wallet=90;
  fresh(&scene,&loop);
  assert(scene.level_id==9 && loop.player.x==105.0f);
  assert(scene.launcher_count==3 && scene.belt_count==0);
  assert(scene.launchers[0].kind==RIKITU_LAUNCH_GUST);
  assert(scene.launchers[0].x==1240.0f);
  assert(scene.checkpoints[0]==2280.0f);
  assert(!scene.stage.boss_required && !scene.boss.alive);

  /* Route stops at x~2100 to capture an actual pre-checkpoint state.
   * Neutral input on the first airborne tick after the gust proves
   * there is no forced sideways drift from the wind launcher. */
  while(loop.player.x<2100.0f && frames<1200){
    const float before_x=loop.player.x;
    unsigned input=RUN_RIGHT;
    if(!start_jump && loop.player.grounded && loop.player.x>109.0f){
      input|=RIKITU_INPUT_JUMP;
      start_jump=1;
    }else if(!fragment_jump && loop.player.grounded &&
             loop.player.x>1570.0f && loop.player.x<1620.0f){
      input|=RIKITU_INPUT_JUMP;
      fragment_jump=1;
    }else if(!gap_jump && fragment_jump && loop.player.grounded &&
             loop.player.x>1780.0f && loop.player.x<1880.0f){
      input|=RIKITU_INPUT_JUMP;
      gap_jump=1;
    }
    if(gust_count==1 && !neutral_frames &&
       !loop.player.grounded && loop.player.vy < -500.0f){
      input=0u;
      neutral_frames++;
    }
    tick(&scene,&loop,input);
    if(input==0u && fabsf(loop.player.x-before_x)>0.005f){
      fprintf(stderr,"MAP09 gust caused horizontal drift %.5f\n",
              (double)(loop.player.x-before_x));
      return 1;
    }
    if(before_x>1100.0f && before_x<1330.0f &&
       !loop.player.grounded &&
       fabsf(loop.player.vy+690.0f)<0.01f){
      const float expected=(input & RIKITU_INPUT_RIGHT)?
                            265.0f*RIKITU_FIXED_DT:0.0f;
      if(fabsf((loop.player.x-before_x)-expected)>0.03f){
        fprintf(stderr,"MAP09 gust impulse altered run speed\n");
        return 2;
      }
      gust_count++;
    }
    frames++;
  }
  if(frames>=1200 || !start_jump || !fragment_jump || !gap_jump ||
     gust_count!=1 || neutral_frames!=1 ||
     !scene.stage.coins[0] || !scene.stage.fragments[0] ||
     scene.reached_checkpoints!=0){
    fprintf(stderr,"MAP09 route frame=%d x=%.2f jumps=%d%d%d "
            "gust=%d neutral=%d coin=%u frag=%u cp=%d\n",
            frames,(double)loop.player.x,start_jump,fragment_jump,
            gap_jump,gust_count,neutral_frames,
            (unsigned)scene.stage.coins[0],
            (unsigned)scene.stage.fragments[0],
            scene.reached_checkpoints);
    return 3;
  }
  assert(frames>400 && frames<530);
  assert(scene.active_checkpoint==105.0f);
  assert(scene.damage_count>=1 && scene.hearts<scene.max_hearts);
  assert(scene.stage.coin_count>0 && scene.stage.fragment_count==1);
  coins_a=scene.stage.coin_count;
  fragments_a=scene.stage.fragment_count;
  hearts_a=scene.hearts;
  damage_a=scene.damage_count;
  assert(rikitu_level_save_store_pair(SLOT_A,SLOT_B,
                                      &scene,&loop,&campaign));
  assert(generation(SLOT_A)==1u);

  /* Continue with genuine RIGHT/RUN input until checkpoint 1. */
  for(frames=0;scene.reached_checkpoints==0 && frames<110;frames++)
    tick(&scene,&loop,RUN_RIGHT);
  if(scene.reached_checkpoints!=1 ||
     scene.active_checkpoint!=2280.0f){
    fprintf(stderr,"MAP09 checkpoint failed x=%.2f cp=%d\n",
            (double)loop.player.x,scene.reached_checkpoints);
    return 4;
  }
  for(frames=0;frames<20;frames++)tick(&scene,&loop,RUN_RIGHT);
  assert(scene.reached_checkpoints==1);
  assert(scene.stage.coins[0] && scene.stage.fragments[0]);
  coins_b=scene.stage.coin_count;
  fragments_b=scene.stage.fragment_count;
  hearts_b=scene.hearts;
  damage_b=scene.damage_count;
  elapsed_b=loop.active_seconds;
  assert(rikitu_level_save_store_pair(SLOT_A,SLOT_B,
                                      &scene,&loop,&campaign));
  assert(generation(SLOT_B)==2u);

  fresh(&loaded,&reboot);
  assert(rikitu_level_save_load_pair(SLOT_A,SLOT_B,
                                      &loaded,&reboot,&campaign));
  assert(loaded.reached_checkpoints==1 &&
         loaded.active_checkpoint==2280.0f);
  assert(reboot.player.x==2280.0f &&
         loaded.player.x==2280.0f);
  assert(loaded.stage.coin_count==coins_b &&
         loaded.stage.fragment_count==fragments_b);
  assert(loaded.stage.coins[0] && loaded.stage.fragments[0]);
  assert(loaded.hearts==hearts_b && loaded.damage_count==damage_b);
  assert(!loaded.stage.boss_required && !loaded.stage.completed);
  assert(loaded.invulnerability_seconds>0.0f);
  assert(reboot.active_seconds>0.0f &&
         reboot.active_seconds<=elapsed_b+0.002f);
  for(frames=0;frames<25;frames++)tick(&loaded,&reboot,RUN_RIGHT);
  assert(reboot.player.x>2280.0f+100.0f);
  assert(loaded.stage.coins[0] && loaded.stage.fragments[0]);

  wrong=campaign;wrong.wallet++;
  fresh(&backup,&old_loop);
  assert(!rikitu_level_save_load_pair(SLOT_A,SLOT_B,
                                       &backup,&old_loop,&wrong));
  assert(backup.stage.coin_count==0 &&
         backup.reached_checkpoints==0);

  /* Corrupt B's CRC-covered payload. A must restore the earlier
   * PRE-CHECKPOINT generation and its genuine collected items. */
  file=fopen(SLOT_B,"r+b");assert(file);
  assert(fseek(file,14,SEEK_SET)==0 &&
         fputc(255,file)!=EOF);
  assert(fclose(file)==0);
  fresh(&backup,&old_loop);
  assert(rikitu_level_save_load_pair(SLOT_A,SLOT_B,
                                      &backup,&old_loop,&campaign));
  assert(backup.reached_checkpoints==0 &&
         backup.active_checkpoint==105.0f);
  assert(old_loop.player.x==105.0f);
  assert(backup.stage.coin_count==coins_a &&
         backup.stage.fragment_count==fragments_a);
  assert(backup.stage.coins[0] && backup.stage.fragments[0]);
  assert(backup.hearts==hearts_a &&
         backup.damage_count==damage_a);
  for(frames=0;frames<10;frames++)tick(&backup,&old_loop,RUN_RIGHT);
  assert(old_loop.player.x>105.0f);
  assert(backup.stage.coin_count==coins_a &&
         backup.stage.fragment_count==fragments_a);
  (void)remove(SLOT_A);
  (void)remove(SLOT_B);
  puts("PASS MAP09 gust (-690, zero drift), input path, patrol, "
       "fragment, checkpoint and RKL3 pre/post checkpoint fallback");
  return 0;
}
