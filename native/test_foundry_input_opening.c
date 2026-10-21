/* Stage86: first authentic MAP14 Heart Foundry section (no teleporting).
 * Exercises native 60Hz RIGHT/RUN/JUMP input from the real spawn through
 * the first conveyor, steam vent, fragment, gap, lever and checkpoint.
 * Boss/exit remain untouched; this is not a full MAP14 completion.
 */
#include "rikitu_foundry.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define RUN_RIGHT (RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN)
enum { FIRST_GAP_END=2120 };
static void fresh(RikituScene *s,RikituLoop *l){
  rikitu_foundry_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
static void tick(RikituScene *s,RikituLoop *l,unsigned input){
  assert(rikitu_demo_advance(s,l,RIKITU_FIXED_DT,input)==1);
}
int main(void){
  RikituScene scene;
  RikituLoop loop;
  int frame,steam_launched=0,steam_neutral=0;
  int first_gap_jumped=0,first_gap_air=0,first_gap_landed=0;
  int first_belt_measured=0,respawn_count=0;
  int neutral_next=0,gap_pending=0;
  fresh(&scene,&loop);
  assert(scene.level_id==14 && scene.world_width==11200.0f);
  assert(scene.gate_x==11020.0f && loop.player.x==105.0f);
  assert(scene.belt_count==3 && scene.launcher_count==3);
  assert(scene.lever_count==3 && scene.checkpoint_total==3);
  assert(scene.stage.boss_required && scene.boss.hp==3);
  assert(scene.boss.alive && !scene.stage.boss_defeated);

  for(frame=0;frame<1800 && scene.reached_checkpoints<1;frame++){
    const float before_x=loop.player.x;
    const int before_grounded=loop.player.grounded;
    unsigned input=RUN_RIGHT;
    int neutral_now=0;

    if(neutral_next && !loop.player.grounded &&
       loop.player.vy<-500.0f){
      input=0u;
      neutral_now=1;
    }else if(!first_gap_jumped &&
             loop.player.x>=1900.0f && loop.player.x<1940.0f){
      if(loop.player.grounded){
        input=RUN_RIGHT|RIKITU_INPUT_JUMP;
        first_gap_jumped=1;
      }else{
        /* Stop on the safe first platform until an actual jump can start. */
        input=0u;
      }
    }

    tick(&scene,&loop,input);

    if(neutral_now){
      assert(fabsf(loop.player.x-before_x)<0.005f);
      steam_neutral++;
      neutral_next=0;
    }
    if(!loop.player.grounded && fabsf(loop.player.vy+705.0f)<0.01f &&
       loop.player.x<1800.0f){
      steam_launched=1;
      neutral_next=1;
    }
    if(input==RUN_RIGHT && before_grounded && loop.player.grounded &&
       before_x>=820.0f && before_x<1070.0f){
      const float expected=(265.0f+45.0f)*RIKITU_FIXED_DT;
      if(fabsf(loop.player.x-before_x-expected)<0.02f)
        first_belt_measured=1;
    }

    if(first_gap_jumped && !first_gap_air &&
       !before_grounded && before_x<FIRST_GAP_END &&
       loop.player.x>=FIRST_GAP_END){
      first_gap_air=1;
      gap_pending=1;
    }
    if(gap_pending && loop.player.grounded &&
       loop.player.x+loop.player.w>FIRST_GAP_END+5.0f){
      first_gap_landed=1;
      gap_pending=0;
    }

    if(before_x>loop.player.x+100.0f){
      respawn_count++;
      neutral_next=0;
      gap_pending=0;
      if(!first_gap_landed)first_gap_jumped=0;
    }
    assert(scene.hearts>=1 && scene.hearts<=scene.max_hearts);
    assert(scene.boss.hp==3 && scene.boss.alive &&
           !scene.stage.boss_defeated);
  }

  if(scene.reached_checkpoints!=1){
    fprintf(stderr,"MAP14 first checkpoint timeout: frame=%d x=%.2f "
            "fragments=%d lever=%d belt=%d vent=%d neutral=%d "
            "jump=%d airborne=%d land=%d respawn=%d\n",
            frame,(double)loop.player.x,scene.stage.fragment_count,
            scene.levers_on,first_belt_measured,steam_launched,
            steam_neutral,first_gap_jumped,first_gap_air,
            first_gap_landed,respawn_count);
    return 2;
  }
  if(!scene.stage.fragments[0] || !first_gap_jumped ||
     !first_gap_air || !first_gap_landed ||
     !steam_launched || !steam_neutral || !first_belt_measured ||
     !scene.levers[0].on){
    fprintf(stderr,"MAP14 first route incomplete: x=%.2f "
            "fragment=%d lever=%d vent=%d neutral=%d belt=%d "
            "jump=%d air=%d land=%d\n",
            (double)loop.player.x,scene.stage.fragments[0],
            scene.levers[0].on,steam_launched,steam_neutral,
            first_belt_measured,first_gap_jumped,first_gap_air,
            first_gap_landed);
    return 3;
  }
  assert(scene.active_checkpoint==2600.0f);
  assert(scene.reached_checkpoints==1);
  assert(scene.levers_on==1);
  assert(scene.stage.fragment_count>=1);
  assert(scene.stage.coin_count>0);
  assert(!scene.stage.completed);
  assert(!rikitu_stage_can_finish(&scene.stage));
  printf("PASS MAP14 real-input opening: %d frames, first steam/no drift, "
         "conveyor, fragment, gap, lever, checkpoint, respawns=%d\n",
         frame,respawn_count);
  return 0;
}
