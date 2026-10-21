/* Stage78: MAP07 Clock Square authentic controller-input route.
 * Real 60Hz player physics, clock launch, fragment, checkpoint,
 * conveyor, RKL3 two-slot recovery. No direct X/Y assignments.
 * Host input-mask replay, not physical Joy-Con / SD validation.
 */
#include "rikitu_clock.h"
#include "rikitu_demo.h"
#include "rikitu_level_save.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define A "/tmp/rikitu-stage78-clock-a.rkl"
#define B "/tmp/rikitu-stage78-clock-b.rkl"
#define RIGHT_RUN (RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN)

static void fresh(RikituScene *scene,RikituLoop *loop){
  rikitu_clock_init(scene);
  memset(loop,0,sizeof(*loop));
  loop->player=scene->player;
}
static void tick(RikituScene *scene,RikituLoop *loop,unsigned buttons){
  assert(rikitu_demo_advance(scene,loop,RIKITU_FIXED_DT,buttons)==1);
}
static uint32_t generation(const char *path){
  uint8_t bytes[RIKITU_LEVEL_SAVE_BYTES];
  uint32_t gen=0;
  FILE *file=fopen(path,"rb");
  assert(file);
  assert(fread(bytes,1,sizeof(bytes),file)==sizeof(bytes));
  assert(fclose(file)==0);
  assert(rikitu_level_save_inspect(bytes,sizeof(bytes),&gen));
  return gen;
}
int main(void){
  RikituScene scene,loaded,backup;
  RikituLoop loop,reboot,old_loop;
  RikituCampaign campaign,foreign;
  float checkpoint,elapsed;
  int frame=0,jump_coin=0,jump_fragment=0,jump_gap=0;
  int pad_count=0,belt_ticks=0,old_coins,old_fragments;
  int coins,fragments,hearts,damage;
  FILE *file;
  (void)remove(A);(void)remove(B);
  assert(rikitu_campaign_init(&campaign,7));
  campaign.wallet=77;
  fresh(&scene,&loop);
  assert(scene.level_id==7 && loop.player.x==105.0f);
  assert(scene.belt_count==2 && scene.belts[0].speed==40.0f);
  assert(scene.launcher_count==3 &&
         scene.launchers[0].kind==RIKITU_LAUNCH_CLOCK);
  assert(!scene.stage.boss_required && !scene.boss.alive);

  /* At x ~1302, the clock launch automatically sets vy=-675.
   * Three JUMP button edges are used for coin, fragment and gap. */
  while(scene.reached_checkpoints==0 && frame<1500){
    const float x_before=loop.player.x;
    unsigned buttons=RIGHT_RUN;
    if(!jump_coin && loop.player.grounded && loop.player.x>109.0f){
      buttons|=RIKITU_INPUT_JUMP;jump_coin=1;
    }else if(!jump_fragment && loop.player.grounded &&
             loop.player.x>1570.0f && loop.player.x<1620.0f){
      buttons|=RIKITU_INPUT_JUMP;jump_fragment=1;
    }else if(!jump_gap && jump_fragment && loop.player.grounded &&
             loop.player.x>1760.0f && loop.player.x<1810.0f){
      buttons|=RIKITU_INPUT_JUMP;jump_gap=1;
    }
    tick(&scene,&loop,buttons);
    if(x_before>1250.0f && x_before<1400.0f &&
       fabsf(loop.player.vy+675.0f)<0.01f &&
       !loop.player.grounded)pad_count++;
    frame++;
  }
  if(scene.reached_checkpoints!=1 || !jump_coin ||
     !jump_fragment || !jump_gap || pad_count!=1 ||
     !scene.stage.coins[0] || !scene.stage.fragments[0]){
    fprintf(stderr,"MAP07 route frame=%d x=%.2f cp=%d "
            "jumps=%d%d%d pad=%d coin=%u fragment=%u\n",
            frame,(double)loop.player.x,scene.reached_checkpoints,
            jump_coin,jump_fragment,jump_gap,pad_count,
            (unsigned)scene.stage.coins[0],
            (unsigned)scene.stage.fragments[0]);
    return 1;
  }
  assert(frame>400 && frame<700);
  assert(scene.active_checkpoint==2240.0f);
  assert(scene.damage_count>=1 && scene.hearts<scene.max_hearts);
  checkpoint=scene.active_checkpoint;
  old_coins=scene.stage.coin_count;
  old_fragments=scene.stage.fragment_count;
  assert(rikitu_level_save_store_pair(A,B,&scene,&loop,&campaign));
  assert(generation(A)==1);

  /* Visit the first conveyor, which is past checkpoint #1, using
   * RIGHT+RUN only. Its speed adds +40 to the base 265 units/sec. */
  for(frame=0;loop.player.x<2650.0f && frame<300;frame++){
    const float x_before=loop.player.x;
    const float y_before=loop.player.y;
    tick(&scene,&loop,RIGHT_RUN);
    if(x_before>2605.0f && x_before<2642.0f &&
       fabsf(y_before-413.0f)<0.01f && loop.player.grounded){
      const float delta=loop.player.x-x_before;
      const float expected=(265.0f+40.0f)*RIKITU_FIXED_DT;
      if(fabsf(delta-expected)>0.03f){
        fprintf(stderr,"MAP07 conveyor dx=%.4f expected=%.4f\n",
                (double)delta,(double)expected);
        return 2;
      }
      belt_ticks++;
    }
  }
  if(loop.player.x<2650.0f || belt_ticks<5 ||
     scene.reached_checkpoints!=1){
    fprintf(stderr,"MAP07 conveyor frame=%d x=%.2f belt=%d cp=%d\n",
            frame,(double)loop.player.x,belt_ticks,
            scene.reached_checkpoints);
    return 3;
  }
  coins=scene.stage.coin_count;
  fragments=scene.stage.fragment_count;
  hearts=scene.hearts;
  damage=scene.damage_count;
  elapsed=loop.active_seconds;
  assert(scene.stage.coins[0] && scene.stage.fragments[0]);
  assert(rikitu_level_save_store_pair(A,B,&scene,&loop,&campaign));
  assert(generation(B)==2);

  /* Fresh MAP07 initialization, latest generation wins. */
  fresh(&loaded,&reboot);
  assert(rikitu_level_save_load_pair(A,B,&loaded,&reboot,&campaign));
  assert(loaded.reached_checkpoints==1 &&
         loaded.active_checkpoint==checkpoint);
  assert(reboot.player.x==checkpoint &&
         loaded.player.x==checkpoint);
  assert(loaded.stage.coin_count==coins &&
         loaded.stage.fragment_count==fragments);
  assert(loaded.stage.coins[0] && loaded.stage.fragments[0]);
  assert(loaded.hearts==hearts && loaded.damage_count==damage);
  assert(loaded.invulnerability_seconds>0.0f);
  assert(!loaded.stage.boss_required && !loaded.stage.completed);
  assert(reboot.active_seconds>0.0f &&
         reboot.active_seconds<=elapsed+0.002f);
  for(frame=0;frame<25;frame++)tick(&loaded,&reboot,RIGHT_RUN);
  assert(reboot.player.x>checkpoint+100.0f);
  assert(loaded.stage.coins[0] && loaded.stage.fragments[0]);

  foreign=campaign;foreign.wallet++;
  fresh(&backup,&old_loop);
  assert(!rikitu_level_save_load_pair(A,B,&backup,&old_loop,&foreign));
  assert(backup.stage.coin_count==0 && backup.reached_checkpoints==0);

  /* Damage B's CRC-protected data; A must resume an earlier state. */
  file=fopen(B,"r+b");assert(file);
  assert(fseek(file,14,SEEK_SET)==0 && fputc(255,file)!=EOF);
  assert(fclose(file)==0);
  fresh(&backup,&old_loop);
  assert(rikitu_level_save_load_pair(A,B,&backup,&old_loop,&campaign));
  assert(backup.reached_checkpoints==1 &&
         backup.active_checkpoint==checkpoint);
  assert(backup.stage.coin_count==old_coins &&
         backup.stage.fragment_count==old_fragments);
  assert(backup.stage.coins[0] && backup.stage.fragments[0]);
  assert(old_loop.player.x==checkpoint);
  (void)remove(A);(void)remove(B);
  puts("PASS MAP07 clock-pad button route, conveyor, checkpoint, "
       "RKL3 reload and two-slot fallback");
  return 0;
}
