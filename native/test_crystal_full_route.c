/* Stage83: controller-only full MAP11 Crystal Caves completion.
 * Replays the authentic spawn with 60Hz RIGHT/RUN/JUMP input only.
 * Covers all four floor gaps, all three fragments/checkpoints, all three
 * crystal launchers and a neutral frame after each launch. No coordinate
 * edits, teleporting, synthetic pickups or modified enemy state.
 * Host input-mask replay is not physical Joy-Con or Switch SD durability QA.
 */
#include "rikitu_crystal.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define RUN_RIGHT (RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN)

enum {
  J_FRAGMENT1, J_GAP1, J_GAP2, J_FRAGMENT2,
  J_GAP3, J_GAP4, J_FRAGMENT3, JUMP_COUNT
};

static const float spring_x[3]={1120.0f,3980.0f,7650.0f};
static const float gap_end[4]={1930.0f,3710.0f,5550.0f,7330.0f};
static const float jump_lo[JUMP_COUNT]={
  1580.0f,1760.0f,3540.0f,4780.0f,5380.0f,7160.0f,8230.0f
};
static const float jump_hi[JUMP_COUNT]={
  1650.0f,1810.0f,3610.0f,4850.0f,5450.0f,7220.0f,8280.0f
};

static void fresh(RikituScene *scene,RikituLoop *loop){
  rikitu_crystal_init(scene);
  memset(loop,0,sizeof(*loop));
  loop->player=scene->player;
}

static void tick(RikituScene *scene,RikituLoop *loop,unsigned input){
  assert(rikitu_demo_advance(scene,loop,RIKITU_FIXED_DT,input)==1);
}

static int spring_at(float x){
  int i;
  for(i=0;i<3;i++)
    if(x+33.0f>spring_x[i]-20.0f && x<spring_x[i]+60.0f)return i;
  return -1;
}

static unsigned gap_bit(int jump){
  switch(jump){
    case J_GAP1:return 1u;
    case J_GAP2:return 2u;
    case J_GAP3:return 4u;
    case J_GAP4:return 8u;
    default:return 0u;
  }
}

static int objective_done(int jump,const RikituScene *scene,
                          unsigned gap_mask){
  switch(jump){
    case J_FRAGMENT1:return scene->stage.fragments[0]!=0;
    case J_GAP1:return (gap_mask&1u)!=0u;
    case J_GAP2:return (gap_mask&2u)!=0u;
    case J_FRAGMENT2:return scene->stage.fragments[1]!=0;
    case J_GAP3:return (gap_mask&4u)!=0u;
    case J_GAP4:return (gap_mask&8u)!=0u;
    case J_FRAGMENT3:return scene->stage.fragments[2]!=0;
    default:return 1;
  }
}

/* A genuine respawn may revisit unfinished route objectives. Only re-arm
 * objectives ahead of the checkpoint; collected fragments stay collected. */
static void rearm_after_respawn(int armed[JUMP_COUNT],float x,
                                const RikituScene *scene,
                                unsigned gap_mask){
  int i;
  for(i=0;i<JUMP_COUNT;i++)
    if(jump_lo[i]>x+40.0f && !objective_done(i,scene,gap_mask))
      armed[i]=0;
}

int main(void){
  RikituScene scene;
  RikituLoop loop;
  int armed[JUMP_COUNT]={0},ever[JUMP_COUNT]={0};
  unsigned spring_mask=0,neutral_mask=0,gap_mask=0,gap_pending=0;
  unsigned gap_jump_pending=0,gap_jump_ever=0,gap_air_mask=0;
  int pending_spring=-1,neutral_frames=0,frame=0,i;

  fresh(&scene,&loop);
  assert(scene.level_id==11 && loop.player.x==105.0f);
  assert(scene.world_width==9820.0f && scene.gate_x==9620.0f);
  assert(scene.checkpoint_total==3 && scene.launcher_count==3);
  assert(scene.belt_count==0 && scene.enemy_count==6);
  assert(scene.stage.fragment_count==0 && !scene.stage.completed);
  assert(!scene.stage.boss_required && !scene.boss.alive);
  assert(scene.launchers[0].kind==RIKITU_LAUNCH_CRYSTAL);
  assert(scene.launchers[1].kind==RIKITU_LAUNCH_CRYSTAL);
  assert(scene.launchers[2].kind==RIKITU_LAUNCH_CRYSTAL);

  for(frame=0;frame<4800 && !scene.stage.completed;frame++){
    const float before_x=loop.player.x;
    const int before_grounded=loop.player.grounded;
    unsigned input=RUN_RIGHT;
    int neutral_now=0,neutral_index=-1,jump=-1,idx;

    /* Pause horizontal input for one real simulation tick after each
     * crystal launch; the spring must not inject any sideways movement. */
    if(pending_spring>=0 && !loop.player.grounded &&
       loop.player.vy < -500.0f){
      neutral_now=1;
      neutral_index=pending_spring;
      input=0u;
    }else{
      for(i=0;i<JUMP_COUNT;i++){
        if(armed[i] || objective_done(i,&scene,gap_mask))continue;
        if(loop.player.x>jump_lo[i] && loop.player.x<jump_hi[i]){
          jump=i;
          if(loop.player.grounded){
            const unsigned bit=gap_bit(jump);
            input|=RIKITU_INPUT_JUMP;
            armed[jump]=1;
            ever[jump]=1;
            gap_jump_pending|=bit;
            gap_jump_ever|=bit;
          }else{
            /* Wait on this safe floor/platform until the jump can be
             * performed; this remains ordinary button-mask gameplay. */
            input=0u;
          }
          break;
        }
      }
    }

    tick(&scene,&loop,input);

    if(neutral_now){
      if(fabsf(loop.player.x-before_x)>0.005f){
        fprintf(stderr,"MAP11 crystal spring %d forced horizontal drift %.5f\n",
                neutral_index,(double)(loop.player.x-before_x));
        return 1;
      }
      neutral_mask|=1u<<(unsigned)neutral_index;
      neutral_frames++;
      pending_spring=-1;
    }

    if(!loop.player.grounded && fabsf(loop.player.vy+690.0f)<0.01f){
      idx=spring_at(loop.player.x);
      if(idx>=0){
        spring_mask|=1u<<(unsigned)idx;
        pending_spring=idx;
      }
    }

    /* Count each gap only after a real jump crosses its far edge and the
     * player lands beyond it. A jump attempt by itself is not a pass. */
    for(i=0;i<4;i++){
      const unsigned bit=1u<<(unsigned)i;
      if(!(gap_mask&bit) && (gap_jump_pending&bit) &&
         !before_grounded && before_x<gap_end[i] &&
         loop.player.x>=gap_end[i]){
        gap_pending|=bit;
        gap_air_mask|=bit;
      }
      if((gap_pending&bit) && loop.player.grounded &&
         loop.player.x+loop.player.w>gap_end[i]+5.0f){
        gap_mask|=bit;
        gap_pending&=~bit;
        gap_jump_pending&=~bit;
      }
    }

    /* The engine's respawn moves the player back to a real checkpoint.
     * Clear only unfinished crossing attempts and re-arm objectives ahead. */
    if(before_x>loop.player.x+100.0f){
      gap_pending=0u;
      gap_jump_pending=0u;
      pending_spring=-1;
      rearm_after_respawn(armed,loop.player.x,&scene,gap_mask);
    }

    assert(scene.hearts>=1 && scene.hearts<=scene.max_hearts);
    assert(scene.reached_checkpoints>=0 &&
           scene.reached_checkpoints<=(int)scene.checkpoint_total);
  }

  if(!scene.stage.completed){
    fprintf(stderr,"MAP11 route timed out frame=%d x=%.2f fragments=%d "
            "checkpoints=%d springs=%u neutral=%u gapJump=%u gapAir=%u "
            "gapLand=%u pending=%u\n",
            frame,(double)loop.player.x,scene.stage.fragment_count,
            scene.reached_checkpoints,spring_mask,neutral_mask,
            gap_jump_ever,gap_air_mask,gap_mask,gap_pending);
    return 2;
  }

  assert(scene.stage.fragment_count==3);
  assert(scene.stage.fragments[0] && scene.stage.fragments[1] &&
         scene.stage.fragments[2]);
  assert(rikitu_stage_can_finish(&scene.stage));
  assert(scene.reached_checkpoints==3 && scene.active_checkpoint==7470.0f);
  assert(spring_mask==7u && neutral_mask==7u && neutral_frames>=3);
  assert(gap_jump_ever==15u && gap_air_mask==15u);
  assert(gap_mask==15u && gap_pending==0u && gap_jump_pending==0u);
  assert(ever[J_GAP1] && ever[J_GAP2] && ever[J_GAP3] && ever[J_GAP4]);
  assert(loop.player.x+loop.player.w>scene.gate_x-30.0f);
  assert(scene.stage.completed && !scene.stage.boss_required);
  assert(scene.stage.coin_count>0 && scene.damage_count>0);
  printf("PASS MAP11 full input route: %d frames, 3 fragments, "
         "4 gaps, 3 crystal springs/no drift, 3 checkpoints, "
         "%d damage events\n",frame,scene.damage_count);
  return 0;
}
