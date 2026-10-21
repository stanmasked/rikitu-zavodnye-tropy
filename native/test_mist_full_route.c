/* Stage81: input-only full MAP09 Misty Pass completion.
 * The hero starts at the authentic spawn and uses 60Hz controller masks
 * only. No player coordinate edits or checkpoint teleports are allowed.
 * Covers all four floor gaps, all three fragments, all three gust pads,
 * each checkpoint, neutral-input drift after every gust, enemy contacts,
 * and the real fragment-gated exit.
 */
#include "rikitu_mist.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define RUN_RIGHT (RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN)
enum { J_START, J_FRAGMENT1, J_GAP1, J_GAP2,
       J_FRAGMENT2, J_GAP3, J_GAP4, J_FRAGMENT3, JUMP_COUNT };

static const float gust_x[3]={1240.0f,3880.0f,7520.0f};
static const float gap_end[4]={1910.0f,3590.0f,5340.0f,7110.0f};

static void fresh(RikituScene *scene,RikituLoop *loop){
  rikitu_mist_init(scene);
  memset(loop,0,sizeof(*loop));
  loop->player=scene->player;
}
static void tick(RikituScene *scene,RikituLoop *loop,unsigned input){
  assert(rikitu_demo_advance(scene,loop,RIKITU_FIXED_DT,input)==1);
}
/* A damage respawn can return to any already-reached checkpoint. Re-arm
 * only jumps that lie ahead of that checkpoint; pickups stay collected. */
static void rearm_after_respawn(int armed[JUMP_COUNT],float x){
  if(x<200.0f){
    memset(armed,0,sizeof(int)*JUMP_COUNT);
  }else if(x<3400.0f){
    armed[J_GAP2]=armed[J_FRAGMENT2]=armed[J_GAP3]=
      armed[J_GAP4]=armed[J_FRAGMENT3]=0;
  }else if(x<4550.0f){
    armed[J_FRAGMENT2]=armed[J_GAP3]=armed[J_GAP4]=
      armed[J_FRAGMENT3]=0;
  }else if(x<5190.0f){
    armed[J_GAP3]=armed[J_GAP4]=armed[J_FRAGMENT3]=0;
  }else if(x<6960.0f){
    armed[J_GAP4]=armed[J_FRAGMENT3]=0;
  }else if(x<8000.0f){
    armed[J_FRAGMENT3]=0;
  }
}
static int launcher_at(float x){
  int i;
  for(i=0;i<3;i++)
    if(x+33.0f>gust_x[i]-24.0f && x<gust_x[i]+66.0f)return i;
  return -1;
}
int main(void){
  RikituScene scene;
  RikituLoop loop;
  int armed[JUMP_COUNT]={0},ever[JUMP_COUNT]={0};
  unsigned gust_mask=0,neutral_mask=0,gap_mask=0,gap_pending=0;
  int pending_gust=-1,neutral_frames=0,frame=0,i;
  fresh(&scene,&loop);
  assert(scene.level_id==9 && loop.player.x==105.0f);
  assert(scene.world_width==9520.0f && scene.gate_x==9330.0f);
  assert(scene.checkpoint_total==3 && scene.launcher_count==3);
  assert(scene.stage.fragment_count==0 && !scene.stage.completed);
  assert(!scene.stage.boss_required && scene.stage.boss_defeated);

  for(frame=0;frame<3600 && !scene.stage.completed;frame++){
    const float before_x=loop.player.x;
    unsigned input=RUN_RIGHT;
    int neutral_now=0,neutral_index=-1;
    int jump=-1,idx;

    /* A neutral frame immediately after each gust proves that the launch
     * impulse changes vertical velocity only; it cannot move X by itself. */
    if(pending_gust>=0 && !loop.player.grounded &&
       loop.player.vy < -500.0f){
      neutral_now=1;
      neutral_index=pending_gust;
      input=0u;
    }else if(!armed[J_START] && loop.player.grounded &&
             loop.player.x>109.0f){
      jump=J_START;
    }else if(!armed[J_FRAGMENT1] && loop.player.grounded &&
             loop.player.x>1570.0f && loop.player.x<1620.0f){
      jump=J_FRAGMENT1;
    }else if(!armed[J_GAP1] && loop.player.grounded &&
             loop.player.x>1780.0f && loop.player.x<1880.0f){
      jump=J_GAP1;
    }else if(!armed[J_GAP2] && loop.player.grounded &&
             loop.player.x>3440.0f && loop.player.x<3530.0f){
      jump=J_GAP2;
    }else if(!armed[J_FRAGMENT2] && loop.player.grounded &&
             loop.player.x>4550.0f && loop.player.x<4620.0f){
      jump=J_FRAGMENT2;
    }else if(!armed[J_GAP3] && loop.player.grounded &&
             loop.player.x>5190.0f && loop.player.x<5270.0f){
      jump=J_GAP3;
    }else if(!armed[J_GAP4] && loop.player.grounded &&
             loop.player.x>6960.0f && loop.player.x<7020.0f){
      jump=J_GAP4;
    }else if(!armed[J_FRAGMENT3] && loop.player.grounded &&
             loop.player.x>8000.0f && loop.player.x<8080.0f){
      jump=J_FRAGMENT3;
    }
    if(jump>=0){
      input|=RIKITU_INPUT_JUMP;
      armed[jump]=1;
      ever[jump]=1;
    }

    tick(&scene,&loop,input);
    if(neutral_now){
      if(fabsf(loop.player.x-before_x)>0.005f){
        fprintf(stderr,"MAP09 gust %d forced horizontal drift %.5f\n",
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

    /* A jump attempt alone is not enough: record a gap only after the
     * player crosses its far edge and lands beyond it. Pending crossings
     * are discarded if a genuine damage respawn moves X backwards. */
    for(i=0;i<4;i++){
      const unsigned bit=1u<<(unsigned)i;
      if(!(gap_mask&bit) && before_x<gap_end[i] &&
         loop.player.x>=gap_end[i])
        gap_pending|=bit;
      if((gap_pending&bit) && loop.player.grounded &&
         loop.player.x>gap_end[i]+5.0f){
        gap_mask|=bit;
        gap_pending&=~bit;
      }
    }
    /* Falling or taking fatal damage can respawn at the last checkpoint.
     * Re-arm jumps ahead of that checkpoint, without editing player state. */
    if(before_x>loop.player.x+100.0f){
      gap_pending=0u;
      rearm_after_respawn(armed,loop.player.x);
    }
    assert(scene.hearts>=1 && scene.hearts<=scene.max_hearts);
    assert(scene.reached_checkpoints>=0 &&
           scene.reached_checkpoints<=(int)scene.checkpoint_total);
  }

  if(!scene.stage.completed){
    fprintf(stderr,"MAP09 route timed out frame=%d x=%.2f "
            "fragments=%d checkpoints=%d gusts=%u neutral=%u\n",
            frame,(double)loop.player.x,scene.stage.fragment_count,
            scene.reached_checkpoints,gust_mask,neutral_mask);
    return 2;
  }
  for(i=0;i<JUMP_COUNT;i++)assert(ever[i]);
  assert(scene.stage.fragment_count==3);
  assert(scene.stage.fragments[0] && scene.stage.fragments[1] &&
         scene.stage.fragments[2]);
  assert(rikitu_stage_can_finish(&scene.stage));
  assert(scene.reached_checkpoints==3 &&
         scene.active_checkpoint==7290.0f);
  assert(gust_mask==7u && neutral_mask==7u && neutral_frames>=3);
  assert(gap_mask==15u && gap_pending==0u);
  assert(loop.player.x+loop.player.w>scene.gate_x-30.0f);
  assert(scene.stage.completed && !scene.stage.boss_required);
  assert(scene.stage.coin_count>0 && scene.damage_count>0);
  printf("PASS MAP09 full input route: %d frames, 3 fragments, "
         "4 gaps, 3 gusts/no drift, 3 checkpoints, %d damage events\n",
         frame,scene.damage_count);
  return 0;
}
