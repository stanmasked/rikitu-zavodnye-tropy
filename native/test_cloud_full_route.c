/* Stage82: controller-only full MAP10 Cloud Dock completion.
 * Replays from the authentic spawn through four gaps, four gusts, both
 * conveyor belts, all fragments/checkpoints and the three-hit boss.
 * No player-coordinate edits, teleporting or synthetic boss HP changes.
 */
#include "rikitu_cloud.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define RUN_RIGHT (RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN)
#define RUN_LEFT  (RIKITU_INPUT_LEFT|RIKITU_INPUT_RUN)
enum { J_GAP1, J_GAP2, J_FRAGMENT2, J_GAP3, J_GAP4, JUMP_COUNT };

static const float gust_x[4]={980.0f,4300.0f,7120.0f,8620.0f};
static const float gap_end[4]={2010.0f,3900.0f,5740.0f,7750.0f};
static const float belt_x[2]={2860.0f,6520.0f};
static const float belt_speed[2]={38.0f,-38.0f};
static const float belt_width=300.0f;

static void fresh(RikituScene *scene,RikituLoop *loop){
  rikitu_cloud_init(scene);
  memset(loop,0,sizeof(*loop));
  loop->player=scene->player;
}
static void tick(RikituScene *scene,RikituLoop *loop,unsigned input){
  assert(rikitu_demo_advance(scene,loop,RIKITU_FIXED_DT,input)==1);
}
static unsigned gap_bit(int jump){
  if(jump==J_GAP1)return 1u;
  if(jump==J_GAP2)return 2u;
  if(jump==J_GAP3)return 4u;
  if(jump==J_GAP4)return 8u;
  return 0u;
}
/* Respawn restores only triggers ahead of the actual checkpoint. Collected
 * fragments, defeated patrols, and previously validated crossings persist. */
static void rearm_after_respawn(int armed[JUMP_COUNT],float x){
  if(x<200.0f){
    memset(armed,0,sizeof(int)*JUMP_COUNT);
  }else if(x<3000.0f){
    armed[J_GAP2]=armed[J_FRAGMENT2]=armed[J_GAP3]=armed[J_GAP4]=0;
  }else if(x<5000.0f){
    armed[J_FRAGMENT2]=armed[J_GAP3]=armed[J_GAP4]=0;
  }else if(x<5600.0f){
    armed[J_GAP3]=armed[J_GAP4]=0;
  }else if(x<7600.0f){
    armed[J_GAP4]=0;
  }
}
static int launcher_at(float x){
  int i;
  for(i=0;i<4;i++)
    if(x+33.0f>gust_x[i]-24.0f && x<gust_x[i]+66.0f)return i;
  return -1;
}
int main(void){
  RikituScene scene;
  RikituLoop loop;
  int armed[JUMP_COUNT]={0},ever[JUMP_COUNT]={0};
  unsigned gap_jump_pending=0,gap_jump_ever=0,gap_air_mask=0;
  unsigned gap_mask=0,gap_pending=0,gust_mask=0,neutral_mask=0;
  unsigned belt_mask=0;
  int pending_gust=-1,neutral_frames=0,frame=0,i;
  int boss_hits=0,boss_attempts=0,last_boss_hp=3,retreat=0;
  fresh(&scene,&loop);
  assert(scene.level_id==10 && loop.player.x==105.0f);
  assert(scene.world_width==10320.0f && scene.gate_x==10140.0f);
  assert(scene.checkpoint_total==3 && scene.launcher_count==4);
  assert(scene.belt_count==2 && scene.enemy_count==6);
  assert(scene.stage.fragment_count==0 && !scene.stage.completed);
  assert(scene.stage.boss_required && !scene.stage.boss_defeated);
  assert(scene.boss.alive && scene.boss.hp==3);

  for(frame=0;frame<4800 && !scene.stage.completed;frame++){
    const float before_x=loop.player.x;
    const int before_grounded=loop.player.grounded;
    unsigned input=RUN_RIGHT;
    int neutral_now=0,neutral_index=-1;
    int jump=-1,boss_attack=0,idx;

    /* A neutral frame immediately after each launch proves that gusts
     * change vertical velocity only and never force horizontal movement. */
    if(pending_gust>=0 && !loop.player.grounded &&
       loop.player.vy < -500.0f){
      neutral_now=1;
      neutral_index=pending_gust;
      input=0u;
    }else if(retreat && scene.boss.alive){
      if(loop.player.grounded && scene.boss.hit_seconds<=0.0f){
        retreat=0;
        input=RUN_RIGHT;
      }else{
        input=RUN_LEFT;
      }
    }else if(scene.boss.alive && loop.player.grounded &&
             scene.boss.hit_seconds<=0.0f &&
             loop.player.x>9470.0f && loop.player.x<9510.0f){
      input=RUN_RIGHT|RIKITU_INPUT_JUMP;
      boss_attack=1;
      boss_attempts++;
    }else{
      if(!armed[J_GAP1] && loop.player.grounded &&
         loop.player.x>1800.0f && loop.player.x<1900.0f){
        jump=J_GAP1;
      }else if(!armed[J_GAP2] && loop.player.grounded &&
               loop.player.x>3700.0f && loop.player.x<3800.0f){
        jump=J_GAP2;
      }else if(!armed[J_FRAGMENT2] && loop.player.grounded &&
               loop.player.x>5080.0f && loop.player.x<5150.0f){
        jump=J_FRAGMENT2;
      }else if(!armed[J_GAP3] && loop.player.grounded &&
               loop.player.x>5550.0f && loop.player.x<5650.0f){
        jump=J_GAP3;
      }else if(!armed[J_GAP4] && loop.player.grounded &&
               loop.player.x>7560.0f && loop.player.x<7660.0f){
        jump=J_GAP4;
      }
      if(jump>=0){
        input|=RIKITU_INPUT_JUMP;
        armed[jump]=1;
        ever[jump]=1;
        gap_jump_pending|=gap_bit(jump);
        gap_jump_ever|=gap_bit(jump);
      }
    }

    tick(&scene,&loop,input);

    if(neutral_now){
      if(fabsf(loop.player.x-before_x)>0.005f){
        fprintf(stderr,"MAP10 gust %d forced horizontal drift %.5f\n",
                neutral_index,(double)(loop.player.x-before_x));
        return 1;
      }
      neutral_mask|=1u<<(unsigned)neutral_index;
      neutral_frames++;
      pending_gust=-1;
    }
    if(!loop.player.grounded && fabsf(loop.player.vy+690.0f)<0.01f){
      idx=launcher_at(loop.player.x);
      if(idx>=0){
        gust_mask|=1u<<(unsigned)idx;
        pending_gust=idx;
      }
    }

    /* Measure the actual 60Hz displacement while grounded on each belt. */
    for(i=0;i<2;i++){
      if(input==RUN_RIGHT && before_grounded && loop.player.grounded &&
         before_x>=belt_x[i] && before_x<belt_x[i]+belt_width){
        const float expected=265.0f*RIKITU_FIXED_DT+
                             belt_speed[i]*RIKITU_FIXED_DT;
        if(fabsf((loop.player.x-before_x)-expected)<0.02f)
          belt_mask|=1u<<(unsigned)i;
      }
    }

    /* A gap crossing is counted only when an airborne player body crosses
     * into the far platform; the same tick may include the landing. */
    for(i=0;i<4;i++){
      const unsigned bit=1u<<(unsigned)i;
      const float far_edge=gap_end[i]+3.0f;
      if(!(gap_mask&bit) && (gap_jump_pending&bit) &&
         !before_grounded && before_x+loop.player.w<=far_edge &&
         loop.player.x+loop.player.w>far_edge){
        gap_pending|=bit;
        gap_air_mask|=bit;
      }
      if((gap_pending&bit) && loop.player.grounded &&
         loop.player.x+loop.player.w>far_edge){
        gap_mask|=bit;
        gap_pending&=~bit;
        gap_jump_pending&=~bit;
      }
    }

    if(scene.boss.hp<last_boss_hp){
      boss_hits++;
      last_boss_hp=scene.boss.hp;
      retreat=scene.boss.alive;
    }
    if(before_x>loop.player.x+100.0f){
      gap_pending=0u;
      gap_jump_pending=0u;
      rearm_after_respawn(armed,loop.player.x);
    }
    assert(scene.hearts>=1 && scene.hearts<=scene.max_hearts);
    assert(scene.reached_checkpoints>=0 &&
           scene.reached_checkpoints<=(int)scene.checkpoint_total);
    assert(scene.boss.hp>=0 && scene.boss.hp<=3);
    (void)boss_attack;
  }

  if(!scene.stage.completed){
    fprintf(stderr,"MAP10 route timed out frame=%d x=%.2f fragments=%d "
            "checkpoints=%d gust=%u neutral=%u gapJump=%u gapAir=%u "
            "gapLand=%u belt=%u bossHits=%d bossHP=%d attempts=%d\n",
            frame,(double)loop.player.x,scene.stage.fragment_count,
            scene.reached_checkpoints,gust_mask,neutral_mask,gap_jump_ever,
            gap_air_mask,gap_mask,belt_mask,boss_hits,scene.boss.hp,
            boss_attempts);
    return 2;
  }
  for(i=0;i<JUMP_COUNT;i++)assert(ever[i]);
  assert(scene.stage.fragment_count==3);
  assert(scene.stage.fragments[0] && scene.stage.fragments[1] &&
         scene.stage.fragments[2]);
  assert(rikitu_stage_can_finish(&scene.stage));
  assert(scene.reached_checkpoints==3 && scene.active_checkpoint==8020.0f);
  assert(gap_jump_ever==15u && gap_air_mask==15u);
  assert(gap_mask==15u && gap_pending==0u && gap_jump_pending==0u);
  assert(gust_mask==15u && neutral_mask==15u && neutral_frames>=4);
  assert(belt_mask==3u);
  assert(boss_hits==3 && boss_attempts>=3);
  assert(scene.boss.hp==0 && !scene.boss.alive);
  assert(scene.stage.boss_defeated && scene.stage.boss_required);
  assert(scene.stage.completed && scene.damage_count>0);
  assert(scene.stage.coin_count>0);
  assert(loop.player.x+loop.player.w>scene.gate_x-30.0f);
  printf("PASS MAP10 full input route: %d frames, 3 fragments, 4 gaps, "
         "4 gusts/no drift, 2 belts, 3 checkpoints, 3 boss hits, "
         "%d damage events\n",frame,scene.damage_count);
  return 0;
}
