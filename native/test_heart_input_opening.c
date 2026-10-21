/* Stage90: real controller-mask MAP16 Heart Chamber opening.
 * Uses the genuine native 60Hz simulation from spawn to checkpoint #1.
 * No coordinate edits, synthetic pickups, modified geometry or boss HP.
 */
#include "rikitu_heart.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define RUN_RIGHT (RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN)
#define GAP1_END 2240.0f

static void fresh(RikituScene *s,RikituLoop *l){
  rikitu_heart_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
static void tick(RikituScene *s,RikituLoop *l,unsigned mask){
  assert(rikitu_demo_advance(s,l,RIKITU_FIXED_DT,mask)==1);
}
int main(void){
  RikituScene scene;
  RikituLoop loop;
  int frame,vent_launched=0,vent_neutral=0,first_belt=0;
  int fragment_jump=0,gap_jump=0,gap_air=0,gap_land=0;
  int respawns=0,neutral_next=0,gap_pending=0;
  fresh(&scene,&loop);
  assert(scene.level_id==16 && scene.world_width==11750.0f);
  assert(scene.gate_x==11560.0f && loop.player.x==105.0f);
  assert(scene.belt_count==3 && scene.launcher_count==5);
  assert(scene.enemy_count==6 && scene.checkpoint_total==3);
  assert(scene.stage.boss_required && scene.boss.hp==3);
  assert(scene.boss.alive && !scene.stage.boss_defeated);

  for(frame=0;frame<2250 && scene.reached_checkpoints<1;frame++){
    const float before_x=loop.player.x;
    const int before_grounded=loop.player.grounded;
    unsigned input=RUN_RIGHT;
    int neutral_now=0;
    if(neutral_next && !loop.player.grounded &&
       loop.player.vy<-500.0f){
      input=0u;
      neutral_now=1;
    }else if(!fragment_jump && !scene.stage.fragments[0] &&
             loop.player.x>=1920.0f && loop.player.x<1990.0f){
      if(loop.player.grounded){
        input=RUN_RIGHT|RIKITU_INPUT_JUMP;
        fragment_jump=1;
      }else{
        input=0u;
      }
    }else if(!gap_jump && loop.player.x>=2085.0f &&
             loop.player.x<2140.0f){
      if(loop.player.grounded){
        input=RUN_RIGHT|RIKITU_INPUT_JUMP;
        gap_jump=1;
      }else{
        input=0u;
      }
    }
    tick(&scene,&loop,input);

    if(neutral_now){
      assert(fabsf(loop.player.x-before_x)<0.005f);
      vent_neutral++;
      neutral_next=0;
    }
    if(!loop.player.grounded &&
       fabsf(loop.player.vy+705.0f)<0.01f &&
       loop.player.x<1900.0f){
      vent_launched=1;
      neutral_next=1;
    }
    if(input==RUN_RIGHT && before_grounded && loop.player.grounded &&
       before_x>=880.0f && before_x<1180.0f){
      const float expected=(265.0f+44.0f)*RIKITU_FIXED_DT;
      if(fabsf(loop.player.x-before_x-expected)<0.02f)
        first_belt=1;
    }
    if(gap_jump && !gap_air &&
       !before_grounded && before_x<GAP1_END &&
       loop.player.x>=GAP1_END){
      gap_air=1;
      gap_pending=1;
    }
    if(gap_pending && loop.player.grounded &&
       loop.player.x+loop.player.w>GAP1_END+5.0f){
      gap_land=1;
      gap_pending=0;
    }
    if(before_x>loop.player.x+100.0f){
      respawns++;
      neutral_next=0;
      gap_pending=0;
      if(!scene.stage.fragments[0])fragment_jump=0;
      if(!gap_land)gap_jump=0;
    }
    assert(scene.hearts>=1 && scene.hearts<=scene.max_hearts);
    assert(scene.boss.hp==3 && scene.boss.alive &&
           !scene.stage.boss_defeated);
  }
  if(scene.reached_checkpoints!=1 ||
     !scene.stage.fragments[0] || !fragment_jump ||
     !gap_jump || !gap_air || !gap_land ||
     !vent_launched || !vent_neutral || !first_belt){
    fprintf(stderr,"MAP16 opening incomplete frame=%d x=%.2f "
            "fragment=%d checkpoint=%d belt=%d vent=%d neutral=%d "
            "fragmentJump=%d gapJump=%d air=%d land=%d "
            "respawns=%d damage=%d\n",
            frame,(double)loop.player.x,scene.stage.fragment_count,
            scene.reached_checkpoints,first_belt,vent_launched,
            vent_neutral,fragment_jump,gap_jump,gap_air,gap_land,
            respawns,scene.damage_count);
    return 2;
  }
  assert(scene.active_checkpoint==2930.0f);
  assert(scene.stage.fragment_count>=1 && scene.stage.coin_count>0);
  assert(!scene.stage.completed && !rikitu_stage_can_finish(&scene.stage));
  printf("PASS MAP16 real-input opening: %d frames, belt, steam/no drift, "
         "fragment, first pit/landing, checkpoint, respawns=%d\n",
         frame,respawns);
  return 0;
}
