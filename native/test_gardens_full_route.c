/* Stage85: authentic MAP13 Castle Gardens controller-only full route.
 * Uses the native 60Hz engine and RIGHT/RUN/JUMP masks. No teleportation,
 * synthetic item collection, direct enemy mutation, or balance changes.
 */
#include "rikitu_gardens.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define RUN_RIGHT (RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN)

enum {
  J_AVOID1, J_FRAGMENT1, J_GAP1, J_AVOID2, J_GAP2,
  J_AVOID3, J_FRAGMENT2, J_GAP3, J_AVOID4,
  J_AVOID5, J_GAP4, J_FRAGMENT3, J_AVOID6, JUMP_COUNT
};
static const float jump_lo[JUMP_COUNT]={
  760.0f,1690.0f,1840.0f,2630.0f,3830.0f,
  4200.0f,5110.0f,5790.0f,6420.0f,
  7350.0f,7670.0f,8600.0f,8850.0f
};
static const float jump_hi[JUMP_COUNT]={
  805.0f,1750.0f,1900.0f,2690.0f,3865.0f,
  4250.0f,5160.0f,5840.0f,6470.0f,
  7410.0f,7720.0f,8650.0f,8900.0f
};
/* Gap four begins at x=7750; take off near that edge, not 170 units
 * earlier. Otherwise the 60Hz jump can cross x=7830 while already too
 * low to land on the far platform, provoking repeated checkpoint reloads. */
static const float gap_end[4]={2030.0f,3970.0f,5940.0f,7830.0f};
static const float mushroom_x[3]={1100.0f,4780.0f,8190.0f};

static void fresh(RikituScene *scene,RikituLoop *loop){
  rikitu_gardens_init(scene);
  memset(loop,0,sizeof(*loop));
  loop->player=scene->player;
}
static void tick(RikituScene *scene,RikituLoop *loop,unsigned input){
  assert(rikitu_demo_advance(scene,loop,RIKITU_FIXED_DT,input)==1);
}
static unsigned gap_bit(int j){
  switch(j){
    case J_GAP1:return 1u;
    case J_GAP2:return 2u;
    case J_GAP3:return 4u;
    case J_GAP4:return 8u;
    default:return 0u;
  }
}
static int mushroom_at(float x){
  int i;
  for(i=0;i<3;i++)
    if(x+33.0f>mushroom_x[i]-20.0f &&
       x<mushroom_x[i]+64.0f)return i;
  return -1;
}
static int objective_done(int j,const RikituScene *s,
                          unsigned gap_mask,float x){
  switch(j){
    case J_FRAGMENT1:return s->stage.fragments[0]!=0;
    case J_FRAGMENT2:return s->stage.fragments[1]!=0;
    case J_FRAGMENT3:return s->stage.fragments[2]!=0;
    case J_GAP1:return (gap_mask&1u)!=0u && x>gap_end[0]+5.0f;
    case J_GAP2:return (gap_mask&2u)!=0u && x>gap_end[1]+5.0f;
    case J_GAP3:return (gap_mask&4u)!=0u && x>gap_end[2]+5.0f;
    case J_GAP4:return (gap_mask&8u)!=0u && x>gap_end[3]+5.0f;
    default:return 0;
  }
}
static void rearm(int armed[JUMP_COUNT],float x,
                  const RikituScene *s,unsigned gap_mask){
  int j;
  for(j=0;j<JUMP_COUNT;j++)
    if(jump_lo[j]>x+40.0f && !objective_done(j,s,gap_mask,x))
      armed[j]=0;
}
int main(void){
  RikituScene scene;
  RikituLoop loop;
  int armed[JUMP_COUNT]={0},ever[JUMP_COUNT]={0};
  unsigned gap_mask=0,gap_pending=0,gap_jump_pending=0;
  unsigned gap_jump_ever=0,gap_air_mask=0;
  unsigned mushroom_mask=0,neutral_mask=0;
  int pending_mushroom=-1,neutral_frames=0,frame=0,i;

  fresh(&scene,&loop);
  assert(scene.level_id==13 && scene.world_width==9950.0f);
  assert(loop.player.x==105.0f && scene.gate_x==9780.0f);
  assert(scene.checkpoint_total==3 && scene.launcher_count==3);
  assert(scene.lever_count==3 && scene.enemy_count==6);
  assert(!scene.stage.boss_required && !scene.boss.alive);

  for(frame=0;frame<5500 && !scene.stage.completed;frame++){
    const float before_x=loop.player.x;
    const int before_grounded=loop.player.grounded;
    unsigned input=RUN_RIGHT;
    int neutral_now=0,neutral_index=-1;

    if(pending_mushroom>=0 && !loop.player.grounded &&
       loop.player.vy < -500.0f){
      neutral_now=1;
      neutral_index=pending_mushroom;
      input=0u;
    }else{
      for(i=0;i<JUMP_COUNT;i++){
        if(armed[i] || objective_done(i,&scene,gap_mask,loop.player.x))
          continue;
        if(loop.player.x>jump_lo[i] && loop.player.x<jump_hi[i]){
          if(loop.player.grounded){
            const unsigned bit=gap_bit(i);
            input|=RIKITU_INPUT_JUMP;
            armed[i]=1;
            ever[i]=1;
            if(!(gap_mask&bit))gap_jump_pending|=bit;
            gap_jump_ever|=bit;
          }else{
            input=0u;
          }
          break;
        }
      }
    }
    tick(&scene,&loop,input);

    if(frame%650==649){
      fprintf(stderr,"MAP13 progress frame=%d x=%.2f hearts=%d damage=%d "
              "fragments=%d checkpoints=%d levers=%d mushroom=%u "
              "jump=%u air=%u land=%u pending=%u\n",
              frame,(double)loop.player.x,scene.hearts,scene.damage_count,
              scene.stage.fragment_count,scene.reached_checkpoints,
              scene.levers_on,mushroom_mask,gap_jump_ever,
              gap_air_mask,gap_mask,gap_jump_pending);
    }

    if(neutral_now){
      if(fabsf(loop.player.x-before_x)>0.005f){
        fprintf(stderr,"MAP13 mushroom %d horizontal drift %.5f\n",
                neutral_index,(double)(loop.player.x-before_x));
        return 1;
      }
      neutral_mask|=1u<<(unsigned)neutral_index;
      neutral_frames++;
      pending_mushroom=-1;
    }
    if(!loop.player.grounded && fabsf(loop.player.vy+730.0f)<0.01f){
      const int m=mushroom_at(loop.player.x);
      if(m>=0){
        mushroom_mask|=1u<<(unsigned)m;
        pending_mushroom=m;
      }
    }

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
    if(before_x>loop.player.x+100.0f){
      gap_pending=0u;
      gap_jump_pending=0u;
      pending_mushroom=-1;
      rearm(armed,loop.player.x,&scene,gap_mask);
    }
    assert(scene.hearts>=1 && scene.hearts<=scene.max_hearts);
    assert(scene.reached_checkpoints>=0 &&
           scene.reached_checkpoints<=(int)scene.checkpoint_total);
  }

  if(!scene.stage.completed){
    fprintf(stderr,"MAP13 route timeout frame=%d x=%.2f fragments=%d "
            "checkpoints=%d levers=%d mush=%u neutral=%u "
            "gapJump=%u gapAir=%u gapLand=%u pending=%u awaiting=%u "
            "damage=%d\n",frame,(double)loop.player.x,
            scene.stage.fragment_count,scene.reached_checkpoints,
            scene.levers_on,mushroom_mask,neutral_mask,gap_jump_ever,
            gap_air_mask,gap_mask,gap_pending,gap_jump_pending,
            scene.damage_count);
    return 2;
  }
  assert(scene.stage.fragment_count==3);
  assert(scene.stage.fragments[0] && scene.stage.fragments[1] &&
         scene.stage.fragments[2]);
  assert(scene.reached_checkpoints==3 &&
         scene.active_checkpoint==7950.0f);
  assert(scene.levers_on==3 && scene.levers[0].on &&
         scene.levers[1].on && scene.levers[2].on);
  assert(mushroom_mask==7u && neutral_mask==7u && neutral_frames>=3);
  assert(gap_jump_ever==15u && gap_air_mask==15u);
  if(gap_mask!=15u || gap_pending || gap_jump_pending){
    fprintf(stderr,"MAP13 crossing check jump=%u air=%u land=%u "
            "pending=%u awaiting=%u\n",
            gap_jump_ever,gap_air_mask,gap_mask,
            gap_pending,gap_jump_pending);
    return 3;
  }
  assert(ever[J_GAP1] && ever[J_GAP2] &&
         ever[J_GAP3] && ever[J_GAP4]);
  assert(rikitu_stage_can_finish(&scene.stage));
  assert(loop.player.x+loop.player.w>scene.gate_x-30.0f);
  assert(!scene.stage.boss_required && scene.stage.completed);
  assert(scene.stage.coin_count>0 && scene.damage_count>0);
  printf("PASS MAP13 full controller route: frames=%d "
         "3 fragments, 4 gaps, 3 mushrooms/no drift, "
         "3 levers, 3 checkpoints, damage=%d\n",
         frame,scene.damage_count);
  return 0;
}
