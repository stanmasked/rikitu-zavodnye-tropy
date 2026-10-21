/* Stage84: controller-only full MAP12 Silent Mine completion.
 * Uses authentic spawn and 60Hz input masks only. Covers four floor gaps,
 * all fragments/checkpoints, three steam vents, three conveyors, three
 * automatic levers, enemy interactions and the real three-hit boss gate.
 * No player-coordinate edits, teleporting or synthetic boss HP changes.
 */
#include "rikitu_mine.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define RUN_RIGHT (RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN)
#define RUN_LEFT  (RIKITU_INPUT_LEFT|RIKITU_INPUT_RUN)

enum { J_FRAGMENT1, J_GAP1, J_GAP2, J_FRAGMENT2,
       J_GAP3, J_GAP4, J_FRAGMENT3, JUMP_COUNT };

static const float vent_x[3]={1480.0f,5140.0f,7580.0f};
static const float gap_end[4]={1990.0f,3940.0f,5980.0f,8050.0f};
static const float jump_lo[JUMP_COUNT]={
  1650.0f,1830.0f,3780.0f,5540.0f,5820.0f,7890.0f,8950.0f
};
static const float jump_hi[JUMP_COUNT]={
  1720.0f,1880.0f,3830.0f,5600.0f,5870.0f,7940.0f,9000.0f
};
static const float belt_x[3]={800.0f,4670.0f,8350.0f};
static const float belt_speed[3]={43.0f,-44.0f,46.0f};
static const float belt_width[3]={290.0f,300.0f,330.0f};

static void fresh(RikituScene *scene,RikituLoop *loop){
  rikitu_mine_init(scene);
  memset(loop,0,sizeof(*loop));
  loop->player=scene->player;
}
static void tick(RikituScene *scene,RikituLoop *loop,unsigned input){
  assert(rikitu_demo_advance(scene,loop,RIKITU_FIXED_DT,input)==1);
}
static int vent_at(float x){
  int i;
  for(i=0;i<3;i++)
    if(x+33.0f>vent_x[i]-18.0f && x<vent_x[i]+52.0f)return i;
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
                          unsigned gap_mask,float player_x){
  /* A historical crossing is enough only while the player is already
   * beyond that gap. A respawn checkpoint before it must jump again. */
  switch(jump){
    case J_FRAGMENT1:return scene->stage.fragments[0]!=0;
    case J_GAP1:return (gap_mask&1u)!=0u && player_x>gap_end[0]+5.0f;
    case J_GAP2:return (gap_mask&2u)!=0u && player_x>gap_end[1]+5.0f;
    case J_FRAGMENT2:return scene->stage.fragments[1]!=0;
    case J_GAP3:return (gap_mask&4u)!=0u && player_x>gap_end[2]+5.0f;
    case J_GAP4:return (gap_mask&8u)!=0u && player_x>gap_end[3]+5.0f;
    case J_FRAGMENT3:return scene->stage.fragments[2]!=0;
    default:return 1;
  }
}
/* Re-arm only unfinished objectives that are genuinely ahead of the last
 * checkpoint. Pickups, lever states, killed patrols and completed gaps stay
 * persistent exactly as in the game loop. */
static void rearm_after_respawn(int armed[JUMP_COUNT],float x,
                                const RikituScene *scene,
                                unsigned gap_mask){
  int i;
  for(i=0;i<JUMP_COUNT;i++)
    if(jump_lo[i]>x+40.0f && !objective_done(i,scene,gap_mask,x))
      armed[i]=0;
}

int main(void){
  RikituScene scene;
  RikituLoop loop;
  int armed[JUMP_COUNT]={0},ever[JUMP_COUNT]={0};
  unsigned vent_mask=0,neutral_mask=0,belt_mask=0;
  unsigned gap_mask=0,gap_pending=0,gap_jump_pending=0;
  unsigned gap_jump_ever=0,gap_air_mask=0;
  int pending_vent=-1,neutral_frames=0,frame=0,i;
  int boss_hits=0,boss_attempts=0,last_boss_hp=3,retreat=0;
  int boss_jump_active=0;
  size_t enemy_index;

  fresh(&scene,&loop);
  assert(scene.level_id==12 && loop.player.x==105.0f);
  assert(scene.world_width==10700.0f && scene.gate_x==10530.0f);
  assert(scene.checkpoint_total==3 && scene.launcher_count==3);
  assert(scene.belt_count==3 && scene.lever_count==3);
  assert(scene.enemy_count==6 && scene.stage.fragment_count==0);
  assert(scene.stage.boss_required && !scene.stage.boss_defeated);
  assert(scene.boss.alive && scene.boss.hp==3);

  for(frame=0;frame<6000 && !scene.stage.completed;frame++){
    const float before_x=loop.player.x;
    const int before_grounded=loop.player.grounded;
    unsigned input=RUN_RIGHT;
    int neutral_now=0,neutral_index=-1,jump=-1,idx;

    /* One true neutral-input tick after every steam launch proves that the
     * -705 vertical impulse cannot inject horizontal movement. */
    if(pending_vent>=0 && !loop.player.grounded &&
       loop.player.vy < -500.0f){
      neutral_now=1;
      neutral_index=pending_vent;
      input=0u;
    }else if(retreat && scene.boss.alive){
      if(loop.player.grounded && scene.boss.hit_seconds<=0.0f){
        retreat=0;
        input=RUN_RIGHT;
      }else{
        input=RUN_LEFT;
      }
    }else if(scene.boss.alive &&
             loop.player.x>scene.boss.center-450.0f){
      const float target=scene.boss.x-180.0f;
      if(boss_jump_active){
        if(loop.player.grounded)boss_jump_active=0;
        input=RUN_RIGHT;
      }else if(loop.player.grounded &&
               scene.boss.hit_seconds<=0.0f &&
               loop.player.x>target-8.0f &&
               loop.player.x<target+8.0f){
        input=RUN_RIGHT|RIKITU_INPUT_JUMP;
        boss_jump_active=1;
        boss_attempts++;
      }else if(loop.player.x>target+8.0f){
        input=RUN_LEFT;
      }else{
        input=RUN_RIGHT;
      }
    }else{
      for(i=0;i<JUMP_COUNT;i++){
        if(armed[i] || objective_done(i,&scene,gap_mask,loop.player.x))continue;
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
            /* Hold horizontal position on safe ground until a real jump is
             * possible; only ordinary input masks control the player. */
            input=0u;
          }
          break;
        }
      }
    }

    tick(&scene,&loop,input);

    if(frame%600==599){
      unsigned dead_mask=0u;
      for(enemy_index=0;enemy_index<scene.enemy_count;enemy_index++)
        if(scene.enemies[enemy_index].dead)
          dead_mask|=1u<<(unsigned)enemy_index;
      fprintf(stderr,"MAP12 progress frame=%d x=%.2f y=%.2f grounded=%d "
              "vx=%.2f vy=%.2f hearts=%d damage=%d checkpoint=%.0f "
              "invuln=%.2f enemies=%u fragments=%d levers=%d "
              "bossHP=%d\n",
              frame,(double)loop.player.x,(double)loop.player.y,
              loop.player.grounded,(double)loop.player.vx,(double)loop.player.vy,
              scene.hearts,scene.damage_count,(double)scene.active_checkpoint,
              (double)scene.invulnerability_seconds,dead_mask,
              scene.stage.fragment_count,scene.levers_on,scene.boss.hp);
    }

    if(neutral_now){
      if(fabsf(loop.player.x-before_x)>0.005f){
        fprintf(stderr,"MAP12 steam vent %d forced horizontal drift %.5f\n",
                neutral_index,(double)(loop.player.x-before_x));
        return 1;
      }
      neutral_mask|=1u<<(unsigned)neutral_index;
      neutral_frames++;
      pending_vent=-1;
    }
    if(!loop.player.grounded && fabsf(loop.player.vy+705.0f)<0.01f){
      idx=vent_at(loop.player.x);
      if(idx>=0){
        vent_mask|=1u<<(unsigned)idx;
        pending_vent=idx;
      }
    }

    /* Measure actual grounded displacement over each conveyor. */
    for(i=0;i<3;i++){
      if(input==RUN_RIGHT && before_grounded && loop.player.grounded &&
         before_x>=belt_x[i] && before_x<belt_x[i]+belt_width[i]){
        const float expected=265.0f*RIKITU_FIXED_DT+
                             belt_speed[i]*RIKITU_FIXED_DT;
        if(fabsf((loop.player.x-before_x)-expected)<0.02f)
          belt_mask|=1u<<(unsigned)i;
      }
    }

    /* Levers are activated by authentic player overlap in the game loop. */
    assert(scene.levers_on>=0 && scene.levers_on<=3);
    for(i=0;i<3;i++)assert(scene.levers[i].on==0 || scene.levers[i].on==1);

    /* Count a gap only after a jump crosses the far edge while airborne and
     * the player then lands on the far platform. */
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

    if(scene.boss.hp<last_boss_hp){
      boss_hits++;
      last_boss_hp=scene.boss.hp;
      retreat=scene.boss.alive;
      boss_jump_active=0;
    }

    /* The engine's real fatal-damage respawn moves X back to a checkpoint.
     * Clear pending crossings and re-arm only unfinished forward objectives. */
    if(before_x>loop.player.x+100.0f){
      gap_pending=0u;
      gap_jump_pending=0u;
      pending_vent=-1;
      retreat=0;
      boss_jump_active=0;
      rearm_after_respawn(armed,loop.player.x,&scene,gap_mask);
    }

    assert(scene.hearts>=1 && scene.hearts<=scene.max_hearts);
    assert(scene.reached_checkpoints>=0 &&
           scene.reached_checkpoints<=(int)scene.checkpoint_total);
    assert(scene.boss.hp>=0 && scene.boss.hp<=3);
  }

  if(!scene.stage.completed){
    fprintf(stderr,"MAP12 route timed out frame=%d x=%.2f fragments=%d "
            "checkpoints=%d levers=%d vents=%u neutral=%u belts=%u "
            "gapJump=%u gapAir=%u gapLand=%u pending=%u "
            "bossHits=%d bossHP=%d attempts=%d\n",
            frame,(double)loop.player.x,scene.stage.fragment_count,
            scene.reached_checkpoints,scene.levers_on,vent_mask,neutral_mask,
            belt_mask,gap_jump_ever,gap_air_mask,gap_mask,gap_pending,
            boss_hits,scene.boss.hp,boss_attempts);
    return 2;
  }

  assert(scene.stage.fragment_count==3);
  assert(scene.stage.fragments[0] && scene.stage.fragments[1] &&
         scene.stage.fragments[2]);
  assert(rikitu_stage_can_finish(&scene.stage));
  assert(scene.reached_checkpoints==3 && scene.active_checkpoint==8200.0f);
  assert(scene.levers_on==3 && scene.levers[0].on &&
         scene.levers[1].on && scene.levers[2].on);
  assert(vent_mask==7u && neutral_mask==7u && neutral_frames>=3);
  assert(belt_mask==7u);
  assert(gap_jump_ever==15u && gap_air_mask==15u);
  assert(gap_mask==15u && gap_pending==0u && gap_jump_pending==0u);
  for(i=0;i<JUMP_COUNT;i++)assert(ever[i]);
  assert(boss_hits==3 && boss_attempts>=3);
  assert(scene.boss.hp==0 && !scene.boss.alive);
  assert(scene.stage.boss_required && scene.stage.boss_defeated);
  assert(loop.player.x+loop.player.w>scene.gate_x-30.0f);
  assert(scene.stage.completed && scene.stage.coin_count>0);
  assert(scene.damage_count>0);
  printf("PASS MAP12 full input route: %d frames, 3 fragments, 4 gaps, "
         "3 steam vents/no drift, 3 belts, 3 levers, 3 checkpoints, "
         "3 boss hits, %d damage events\n",frame,scene.damage_count);
  return 0;
}
