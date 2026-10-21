/* Stage76: MAP05 player-input route from real spawn over conveyor #1,
 * first collectible fragment and checkpoint, then RKL3 cold restore
 * and corrupted-newest-slot recovery. No direct position changes.
 * Host-side fixed-step replay, not physical Joy-Con / SD validation.
 */
#include "rikitu_workshop.h"
#include "rikitu_demo.h"
#include "rikitu_level_save.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define A "/tmp/rikitu-stage76-workshop-a.rkl"
#define B "/tmp/rikitu-stage76-workshop-b.rkl"
#define RIGHT_RUN (RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN)

static void fresh(RikituScene *scene,RikituLoop *loop){
  rikitu_workshop_init(scene);
  memset(loop,0,sizeof(*loop));
  loop->player=scene->player;
}
static void tick(RikituScene *scene,RikituLoop *loop,unsigned buttons){
  assert(rikitu_demo_advance(scene,loop,RIKITU_FIXED_DT,buttons)==1);
}
static uint32_t generation(const char *path){
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
  RikituCampaign campaign,foreign;
  int frame=0,jump_coin=0,jump_fragment=0,jump_gap=0;
  int belt_frames=0,old_coins,old_fragments,coins,fragments,hearts,damage;
  float checkpoint,old_time;
  FILE *f;
  (void)remove(A);(void)remove(B);
  assert(rikitu_campaign_init(&campaign,5));
  campaign.wallet=56;
  fresh(&scene,&loop);
  assert(scene.level_id==5 && loop.player.x==105.0f);
  assert(scene.belt_count==3 && scene.belts[0].speed==48.0f);
  assert(scene.checkpoint_total==3 &&
         scene.checkpoints[0]==2200.0f);
  assert(!scene.stage.boss_required);

  /* All horizontal travel comes from 60Hz RIGHT+RUN input, except the
   * actual first conveyor's +48 units/sec while grounded. */
  while(scene.reached_checkpoints==0 && frame<1500){
    const float x_before=loop.player.x;
    const float y_before=loop.player.y;
    unsigned buttons=RIGHT_RUN;
    if(!jump_coin && loop.player.grounded && loop.player.x>109.0f){
      buttons|=RIKITU_INPUT_JUMP;jump_coin=1;
    }else if(!jump_fragment && jump_coin && loop.player.grounded &&
             loop.player.x>1510.0f && loop.player.x<1560.0f){
      buttons|=RIKITU_INPUT_JUMP;jump_fragment=1;
    }else if(!jump_gap && jump_fragment && loop.player.grounded &&
             loop.player.x>1710.0f && loop.player.x<1760.0f){
      buttons|=RIKITU_INPUT_JUMP;jump_gap=1;
    }
    tick(&scene,&loop,buttons);
    /* Restrict belt-speed assertion to flat ground well inside belt,
     * away from jumps/platform edges and any horizontal collisions. */
    if(x_before>1100.0f && x_before<1250.0f &&
       fabsf(y_before-413.0f)<0.01f && loop.player.grounded){
      const float dx=loop.player.x-x_before;
      const float expected=(265.0f+48.0f)*RIKITU_FIXED_DT;
      if(fabsf(dx-expected)>=0.02f){
        fprintf(stderr,"MAP05 conveyor dx %.4f expected %.4f\n",
                (double)dx,(double)expected);
        return 1;
      }
      belt_frames++;
    }
    frame++;
  }
  if(scene.reached_checkpoints!=1 || !jump_coin ||
     !jump_fragment || !jump_gap || belt_frames<10 ||
     !scene.stage.coins[0] || !scene.stage.fragments[0]){
    fprintf(stderr,"MAP05 route: frame=%d x=%.2f cp=%d "
            "jumps=%d%d%d belt_ticks=%d coin=%u frag=%u\n",
            frame,(double)loop.player.x,scene.reached_checkpoints,
            jump_coin,jump_fragment,jump_gap,belt_frames,
            (unsigned)scene.stage.coins[0],
            (unsigned)scene.stage.fragments[0]);
    return 2;
  }
  assert(frame>400 && frame<700);
  assert(scene.active_checkpoint==scene.checkpoints[0]);
  assert(scene.stage.coin_count>0 && scene.stage.fragment_count>=1);
  assert(scene.damage_count>=1 && scene.hearts<scene.max_hearts);
  checkpoint=scene.active_checkpoint;
  old_coins=scene.stage.coin_count;
  old_fragments=scene.stage.fragment_count;
  assert(rikitu_level_save_store_pair(A,B,&scene,&loop,&campaign));
  assert(generation(A)==1);

  /* Advance by genuine controller input before rotating into B. */
  for(frame=0;frame<30;frame++)tick(&scene,&loop,RIGHT_RUN);
  assert(scene.reached_checkpoints==1 &&
         loop.player.x>checkpoint+100.0f);
  coins=scene.stage.coin_count;
  fragments=scene.stage.fragment_count;
  hearts=scene.hearts;
  damage=scene.damage_count;
  old_time=loop.active_seconds;
  assert(rikitu_level_save_store_pair(A,B,&scene,&loop,&campaign));
  assert(generation(B)==2);

  fresh(&loaded,&reboot);
  assert(rikitu_level_save_load_pair(A,B,&loaded,&reboot,&campaign));
  assert(loaded.reached_checkpoints==1 &&
         loaded.active_checkpoint==checkpoint);
  assert(reboot.player.x==checkpoint);
  assert(loaded.stage.coins[0] && loaded.stage.fragments[0]);
  assert(loaded.stage.coin_count==coins &&
         loaded.stage.fragment_count==fragments);
  assert(loaded.hearts==hearts && loaded.damage_count==damage);
  assert(!loaded.stage.boss_required &&
         !loaded.stage.completed);
  assert(reboot.active_seconds>0.0f &&
         reboot.active_seconds<=old_time+0.002f);
  assert(loaded.invulnerability_seconds>0.0f);
  for(frame=0;frame<25;frame++)tick(&loaded,&reboot,RIGHT_RUN);
  assert(reboot.player.x>checkpoint+100.0f);
  assert(loaded.stage.coin_count==coins &&
         loaded.stage.fragment_count==fragments);

  foreign=campaign;foreign.wallet++;
  fresh(&backup,&old_loop);
  assert(!rikitu_level_save_load_pair(A,B,&backup,&old_loop,&foreign));
  assert(backup.stage.coin_count==0 &&
         backup.reached_checkpoints==0);

  /* Break newer B and confirm A is still a decodable checkpoint. */
  f=fopen(B,"r+b");assert(f);
  assert(fseek(f,14,SEEK_SET)==0 && fputc(255,f)!=EOF);
  assert(fclose(f)==0);
  fresh(&backup,&old_loop);
  assert(rikitu_level_save_load_pair(A,B,&backup,&old_loop,&campaign));
  assert(backup.reached_checkpoints==1 &&
         backup.active_checkpoint==checkpoint);
  assert(backup.stage.coin_count==old_coins &&
         backup.stage.fragment_count==old_fragments);
  assert(backup.stage.coins[0] && backup.stage.fragments[0]);
  assert(old_loop.player.x==checkpoint);
  (void)remove(A);(void)remove(B);
  puts("PASS MAP05 authentic controller route, conveyor velocity, "
       "fragment, checkpoint, RKL3 reload and CRC fallback");
  return 0;
}
