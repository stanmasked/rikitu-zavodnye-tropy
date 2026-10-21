/* Stage91: full MAP16 Heart Chamber fixed-step controller route.
 * Only LEFT/RIGHT/RUN/JUMP masks at 60 Hz. Authentic spawn, geometry,
 * fragment pickup, checkpoint, physics, enemy and 3-hit boss states.
 * Do not move/teleport the player or write item/boss flags.
 */
#include "rikitu_heart.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define RUN_RIGHT (RIKITU_INPUT_RIGHT|RIKITU_INPUT_RUN)
#define RUN_LEFT (RIKITU_INPUT_LEFT|RIKITU_INPUT_RUN)

enum { J_GAP1,J_GAP2,J_GAP3,J_GAP4,JUMP_COUNT };
static const float gap_end[4]={2240.0f,4570.0f,6910.0f,9190.0f};
static const float jump_lo[JUMP_COUNT]={
  2085.0f,4410.0f,6750.0f,9030.0f
};
static const float jump_hi[JUMP_COUNT]={
  2140.0f,4460.0f,6800.0f,9080.0f
};
static const float belt_x[3]={880.0f,5070.0f,9200.0f};
static const float belt_speed[3]={44.0f,-44.0f,45.0f};
static const float belt_width[3]={330.0f,300.0f,310.0f};

static void fresh(RikituScene *s,RikituLoop *l){
  rikitu_heart_init(s);
  memset(l,0,sizeof(*l));l->player=s->player;
}
static void tick(RikituScene *s,RikituLoop *l,unsigned input){
  assert(rikitu_demo_advance(s,l,RIKITU_FIXED_DT,input)==1);
}
static int launch_at(const RikituScene *s,float x,float vy){
  size_t i;
  for(i=0;i<s->launcher_count;i++){
    const RikituLauncher *l=&s->launchers[i];
    const float expected=(l->kind==RIKITU_LAUNCH_STEAM)?-705.0f:-675.0f;
    if(fabsf(vy-expected)<0.01f &&
       x+33.0f>l->x-18.0f && x<l->x+52.0f)
      return (int)i;
  }
  return -1;
}
static int objective_done(int jump,unsigned gap_mask,float x){
  const unsigned bit=1u<<(unsigned)jump;
  return (gap_mask&bit)!=0u && x>gap_end[jump]+5.0f;
}
static void rearm_after_respawn(int armed[JUMP_COUNT],float x,
                                unsigned gap_mask){
  int i;
  for(i=0;i<JUMP_COUNT;i++)
    if(jump_lo[i]>x+40.0f && !objective_done(i,gap_mask,x))
      armed[i]=0;
}
int main(void){
  RikituScene s;RikituLoop l;
  int armed[JUMP_COUNT]={0},ever[JUMP_COUNT]={0};
  unsigned launch_mask=0,neutral_mask=0,belt_mask=0;
  unsigned gap_mask=0,gap_pending=0,gap_jump_pending=0;
  unsigned gap_jump_ever=0,gap_air_mask=0;
  int pending_launch=-1,neutral_frames=0,frame=0,i;
  int boss_hits=0,boss_attempts=0,last_boss_hp=3,retreat=0;
  int boss_jump_active=0;
  int fragment2_jump=0,fragment3_jump=0;

  fresh(&s,&l);
  assert(s.level_id==16 && l.player.x==105.0f);
  assert(s.world_width==11750.0f && s.gate_x==11560.0f);
  assert(s.checkpoint_total==3 && s.launcher_count==5);
  assert(s.belt_count==3 && s.enemy_count==6);
  assert(s.stage.boss_required && !s.stage.boss_defeated);
  assert(s.boss.alive && s.boss.hp==3);
  assert(s.stage.fragment_count==0);

  for(frame=0;frame<10500 && !s.stage.completed;frame++){
    const float before_x=l.player.x;
    const int before_grounded=l.player.grounded;
    unsigned input=RUN_RIGHT;
    int neutral_now=0,neutral_index=-1;

    if(pending_launch>=0 && !l.player.grounded &&
       l.player.vy<-500.0f){
      input=0u;neutral_now=1;neutral_index=pending_launch;
    }else if(retreat && s.boss.alive){
      if(l.player.grounded && s.boss.hit_seconds<=0.0f){
        retreat=0;input=RUN_RIGHT;
      }else input=RUN_LEFT;
    }else if(!s.stage.fragments[1] && !fragment2_jump &&
             l.player.x>6030.0f && l.player.x<6080.0f){
      /* Fragment two floats above the x6150..6345 ledge. A real
       * approach jump, distinct from gap #3, intersects its pickup. */
      if(l.player.grounded){
        input=RUN_RIGHT|RIKITU_INPUT_JUMP;
        fragment2_jump=1;
      }else input=0u;
    }else if(!s.stage.fragments[2] && !fragment3_jump &&
             l.player.x>9790.0f && l.player.x<9840.0f){
      /* Fragment three similarly sits above the x9930..10125 ledge. */
      if(l.player.grounded){
        input=RUN_RIGHT|RIKITU_INPUT_JUMP;
        fragment3_jump=1;
      }else input=0u;
    }else if(s.boss.alive && l.player.x>s.boss.center-450.0f){
      const float target=s.boss.x-180.0f;
      if(boss_jump_active){
        if(l.player.grounded)boss_jump_active=0;
        input=RUN_RIGHT;
      }else if(l.player.grounded &&
               s.boss.hit_seconds<=0.0f &&
               l.player.x>target-8.0f &&
               l.player.x<target+8.0f){
        input=RUN_RIGHT|RIKITU_INPUT_JUMP;
        boss_jump_active=1;
        boss_attempts++;
      }else if(l.player.x>target+8.0f)input=RUN_LEFT;
      else input=RUN_RIGHT;
    }else{
      for(i=0;i<JUMP_COUNT;i++){
        const unsigned bit=1u<<(unsigned)i;
        if(armed[i]||objective_done(i,gap_mask,l.player.x))continue;
        if(l.player.x>jump_lo[i] && l.player.x<jump_hi[i]){
          if(l.player.grounded){
            input|=RIKITU_INPUT_JUMP;
            armed[i]=1;ever[i]=1;
            if(!(gap_mask&bit))gap_jump_pending|=bit;
            gap_jump_ever|=bit;
          }else input=0u;
          break;
        }
      }
    }
    tick(&s,&l,input);

    if(frame%750==749){
      fprintf(stderr,"MAP16 progress frame=%d x=%.2f y=%.2f "
              "hearts=%d damage=%d frags=%d cp=%d "
              "launch=%u belt=%u gaps=%u bossHP=%d attempts=%d\n",
              frame,(double)l.player.x,(double)l.player.y,s.hearts,
              s.damage_count,s.stage.fragment_count,s.reached_checkpoints,
              launch_mask,belt_mask,gap_mask,s.boss.hp,boss_attempts);
    }

    if(neutral_now){
      if(fabsf(l.player.x-before_x)>0.005f){
        fprintf(stderr,"MAP16 launcher %d horizontal drift %.5f\n",
                neutral_index,(double)(l.player.x-before_x));
        return 1;
      }
      neutral_mask|=1u<<(unsigned)neutral_index;
      neutral_frames++;
      pending_launch=-1;
    }
    if(!l.player.grounded && l.player.vy<-500.0f){
      const int j=launch_at(&s,l.player.x,l.player.vy);
      if(j>=0){launch_mask|=1u<<(unsigned)j;pending_launch=j;}
    }

    for(i=0;i<3;i++){
      if(input==RUN_RIGHT && before_grounded && l.player.grounded &&
         before_x>=belt_x[i] &&
         before_x<belt_x[i]+belt_width[i]){
        const float expected=(265.0f+belt_speed[i])*RIKITU_FIXED_DT;
        if(fabsf((l.player.x-before_x)-expected)<0.02f)
          belt_mask|=1u<<(unsigned)i;
      }
    }

    for(i=0;i<4;i++){
      const unsigned bit=1u<<(unsigned)i;
      if(!(gap_mask&bit) && (gap_jump_pending&bit) &&
         !before_grounded && before_x<gap_end[i] &&
         l.player.x>=gap_end[i]){
        gap_pending|=bit;gap_air_mask|=bit;
      }
      if((gap_pending&bit) && l.player.grounded &&
         l.player.x+l.player.w>gap_end[i]+5.0f){
        gap_mask|=bit;gap_pending&=~bit;gap_jump_pending&=~bit;
      }
    }
    if(s.boss.hp<last_boss_hp){
      boss_hits++;last_boss_hp=s.boss.hp;
      retreat=s.boss.alive;boss_jump_active=0;
    }
    if(before_x>l.player.x+100.0f){
      gap_pending=0u;gap_jump_pending=0u;
      pending_launch=-1;retreat=0;boss_jump_active=0;
      if(!s.stage.fragments[1] && l.player.x<6030.0f)fragment2_jump=0;
      if(!s.stage.fragments[2] && l.player.x<9790.0f)fragment3_jump=0;
      rearm_after_respawn(armed,l.player.x,gap_mask);
    }
    assert(s.hearts>=1 && s.hearts<=s.max_hearts);
    assert(s.reached_checkpoints>=0 && s.reached_checkpoints<=3);
    assert(s.boss.hp>=0 && s.boss.hp<=3);
  }
  if(!s.stage.completed){
    fprintf(stderr,"MAP16 timeout frame=%d x=%.2f fragments=%d "
            "checkpoint=%d launches=%u neutral=%u belts=%u "
            "gapJump=%u gapAir=%u gapLand=%u pending=%u waiting=%u "
            "bossHP=%d bossHits=%d bossAttempts=%d\n",
            frame,(double)l.player.x,s.stage.fragment_count,
            s.reached_checkpoints,launch_mask,neutral_mask,belt_mask,
            gap_jump_ever,gap_air_mask,gap_mask,gap_pending,
            gap_jump_pending,s.boss.hp,boss_hits,boss_attempts);
    return 2;
  }
  assert(s.stage.fragment_count==3);
  assert(s.stage.fragments[0]&&s.stage.fragments[1]&&s.stage.fragments[2]);
  assert(fragment2_jump && fragment3_jump);
  assert(s.reached_checkpoints==3 && s.active_checkpoint==9260.0f);
  assert(launch_mask==31u && neutral_mask==31u && neutral_frames>=5);
  assert(belt_mask==7u);
  assert(gap_jump_ever==15u && gap_air_mask==15u);
  if(gap_mask!=15u||gap_pending!=0u||gap_jump_pending!=0u){
    fprintf(stderr,"MAP16 crossing mismatch air=%u land=%u "
            "pending=%u waiting=%u\n",
            gap_air_mask,gap_mask,gap_pending,gap_jump_pending);
    return 3;
  }
  for(i=0;i<JUMP_COUNT;i++)assert(ever[i]);
  assert(boss_hits==3 && boss_attempts>=3);
  assert(s.boss.hp==0 && !s.boss.alive);
  assert(s.stage.boss_defeated && rikitu_stage_can_finish(&s.stage));
  assert(l.player.x+l.player.w>s.gate_x-30.0f);
  assert(s.stage.completed && s.stage.coin_count>0);
  printf("PASS MAP16 final full input route: frames=%d "
         "3 fragments, 4 gaps, 5 steam/clock launchers/no drift, "
         "3 belts, 3 checkpoints, 3 boss hits, damage=%d\n",
         frame,s.damage_count);
  return 0;
}
