/* Stage88: real 60Hz MAP15 Clock Gallery opening route.
 * No teleporting, artificial pickups, enemy edits or physics modifications.
 * From the original spawn, collect fragment one and cross pit one with
 * actual player input, then touch lever/checkpoint one.
 */
#include "rikitu_gallery.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define RUN_RIGHT (RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN)
enum { FIRST_GAP_END=2050 };

static void fresh(RikituScene *s,RikituLoop *l){
  rikitu_gallery_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
static void tick(RikituScene *s,RikituLoop *l,unsigned input){
  assert(rikitu_demo_advance(s,l,RIKITU_FIXED_DT,input)==1);
}

int main(void){
  RikituScene scene;
  RikituLoop loop;
  int frame,clock_launched=0,clock_neutral=0;
  int fragment_jump=0,gap_jump=0,gap_air=0,gap_land=0;
  int respawns=0,neutral_next=0,gap_pending=0;
  fresh(&scene,&loop);
  assert(scene.level_id==15 && scene.world_width==10300.0f);
  assert(scene.gate_x==10120.0f && loop.player.x==105.0f);
  assert(scene.launcher_count==3 && scene.lever_count==3);
  assert(scene.belt_count==0 && scene.checkpoint_total==3);
  assert(scene.enemy_count==6 && !scene.stage.boss_required);
  assert(!scene.boss.alive && !scene.stage.completed);

  for(frame=0;frame<2000 && scene.reached_checkpoints<1;frame++){
    const float before_x=loop.player.x;
    const int before_grounded=loop.player.grounded;
    unsigned input=RUN_RIGHT;
    int neutral_now=0;
    if(neutral_next && !loop.player.grounded &&
       loop.player.vy<-500.0f){
      input=0u;
      neutral_now=1;
    }else if(!fragment_jump && !scene.stage.fragments[0] &&
             loop.player.x>=1650.0f && loop.player.x<1710.0f){
      if(loop.player.grounded){
        input|=RIKITU_INPUT_JUMP;
        fragment_jump=1;
      }else{
        input=0u;
      }
    }else if(!gap_jump && loop.player.x>=1890.0f &&
             loop.player.x<1940.0f){
      if(loop.player.grounded){
        input|=RIKITU_INPUT_JUMP;
        gap_jump=1;
      }else{
        input=0u;
      }
    }
    tick(&scene,&loop,input);

    if(neutral_now){
      assert(fabsf(loop.player.x-before_x)<0.005f);
      clock_neutral++;
      neutral_next=0;
    }
    if(!loop.player.grounded &&
       fabsf(loop.player.vy+675.0f)<0.01f &&
       loop.player.x<1400.0f){
      clock_launched=1;
      neutral_next=1;
    }
    if(gap_jump && !gap_air &&
       !before_grounded && before_x<FIRST_GAP_END &&
       loop.player.x>=FIRST_GAP_END){
      gap_air=1;
      gap_pending=1;
    }
    if(gap_pending && loop.player.grounded &&
       loop.player.x+loop.player.w>FIRST_GAP_END+5.0f){
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
    assert(!scene.stage.boss_required && !scene.boss.alive);
  }

  if(scene.reached_checkpoints!=1 ||
     !scene.stage.fragments[0] || !fragment_jump ||
     !clock_launched || !clock_neutral ||
     !gap_jump || !gap_air || !gap_land ||
     !scene.levers[0].on){
    fprintf(stderr,"MAP15 opening incomplete frame=%d x=%.2f "
            "fragments=%d checkpoint=%d lever=%d clock=%d neutral=%d "
            "fragmentJump=%d gapJump=%d gapAir=%d gapLand=%d "
            "respawns=%d\n",
            frame,(double)loop.player.x,scene.stage.fragment_count,
            scene.reached_checkpoints,scene.levers_on,
            clock_launched,clock_neutral,fragment_jump,
            gap_jump,gap_air,gap_land,respawns);
    return 2;
  }
  assert(scene.active_checkpoint==2500.0f);
  assert(scene.levers_on==1 && scene.levers[0].on);
  assert(scene.stage.fragment_count>=1 && scene.stage.coin_count>0);
  assert(!scene.stage.completed && !rikitu_stage_can_finish(&scene.stage));
  printf("PASS MAP15 real-input opening: %d frames, clock/no drift, "
         "fragment, first pit/landing, lever, checkpoint, respawns=%d\n",
         frame,respawns);
  return 0;
}
