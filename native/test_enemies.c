#include "rikitu_enemies.h"
#include "rikitu_flower.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int near(float a,float b){return fabsf(a-b)<0.03f;}
static void reset(RikituScene *s,RikituLoop *loop) {
  rikitu_flower_init(s);
  memset(loop,0,sizeof(*loop));
  loop->player=s->player;
}
int main(void) {
  RikituScene scene;
  RikituLoop loop;
  RikituEnemy e;
  RikituPlayer p;
  int i;
  reset(&scene,&loop);
  assert(scene.enemy_count==6);
  assert(scene.hearts==3&&scene.max_hearts==3);
  assert(scene.enemies[0].x==820&&scene.enemies[0].a==745);
  assert(scene.enemies[0].b==1030&&scene.enemies[0].dir==-1);
  assert(scene.enemies[1].x==2450&&scene.enemies[1].dir==1);
  assert(scene.enemies[5].x==7130&&scene.enemies[5].dir==1);
  /* Browser patrol velocity is 43 units/second. The first enemy moves left. */
  e=scene.enemies[0];
  rikitu_player_init(&p,105,413);
  assert(rikitu_enemy_tick(&e,&p,450,RIKITU_FIXED_DT)==RIKITU_ENEMY_NONE);
  assert(near(e.x,820.0f-43.0f/60.0f));
  e.x=e.a;e.dir=-1;
  assert(rikitu_enemy_tick(&e,&p,450,RIKITU_FIXED_DT)==RIKITU_ENEMY_NONE);
  assert(near(e.x,e.a)&&e.dir==1);
  e.x=e.b;e.dir=1;
  assert(rikitu_enemy_tick(&e,&p,450,RIKITU_FIXED_DT)==RIKITU_ENEMY_NONE);
  assert(near(e.x,e.b)&&e.dir==-1);
  /* A descending player crosses the top; target dies and player bounces. */
  e=scene.enemies[0];
  rikitu_player_init(&p,825,390);
  p.vy=120;
  assert(rikitu_enemy_tick(&e,&p,422.0f,RIKITU_FIXED_DT)==RIKITU_ENEMY_STOMP);
  assert(e.dead && near(p.vy,-360.0f));
  assert(rikitu_enemy_tick(&e,&p,422.0f,RIKITU_FIXED_DT)==RIKITU_ENEMY_NONE);
  /* Side contact is never treated as a top-down stomp. */
  e=scene.enemies[0];
  rikitu_player_init(&p,825,428);
  p.vy=0;
  assert(rikitu_enemy_tick(&e,&p,471.0f,RIKITU_FIXED_DT)==RIKITU_ENEMY_CONTACT);
  assert(!e.dead);
  assert(rikitu_enemy_tick(NULL,&p,0,RIKITU_FIXED_DT)==RIKITU_ENEMY_INVALID);
  assert(rikitu_enemy_tick(&e,&p,0,-1)==RIKITU_ENEMY_INVALID);
  /* Full integration: collision subtracts one heart and starts invincibility.
   * Repeated overlap during the next 30 game ticks does not remove more. */
  reset(&scene,&loop);
  loop.player.x=825;loop.player.y=413;loop.player.grounded=1;
  assert(rikitu_demo_advance(&scene,&loop,RIKITU_FIXED_DT,0)==1);
  assert(scene.hearts==2&&scene.damage_count==1);
  assert(scene.invulnerability_seconds>1.55f);
  for(i=0;i<30;i++)assert(rikitu_demo_advance(&scene,&loop,RIKITU_FIXED_DT,0)==1);
  assert(scene.hearts==2&&scene.damage_count==1);
  /* A full bounce kills an enemy and preserves all hearts. */
  reset(&scene,&loop);
  loop.player.x=825;loop.player.y=381;loop.player.vy=300;
  loop.player.grounded=0;
  assert(rikitu_demo_advance(&scene,&loop,RIKITU_FIXED_DT,0)==1);
  assert(scene.enemies[0].dead&&near(loop.player.vy,-360));
  assert(scene.hearts==3&&scene.damage_count==0);
  /* Falling after reaching the first checkpoint loses one heart and respawns.
   * Collected fragments/coins remain and defeated enemies stay dead. */
  reset(&scene,&loop);
  scene.active_checkpoint=2050;
  scene.reached_checkpoints=1;
  scene.stage.fragments[0]=1;scene.stage.fragment_count=1;
  scene.stage.coins[0]=1;scene.stage.coin_count=1;
  scene.enemies[0].dead=1;
  loop.player.x=1690;loop.player.y=692;loop.player.vy=50;
  assert(rikitu_demo_advance(&scene,&loop,RIKITU_FIXED_DT,0)==1);
  assert(near(loop.player.x,2050)&&near(loop.player.y,413));
  assert(scene.hearts==2&&scene.damage_count==1);
  assert(near(scene.invulnerability_seconds,1.4f));
  assert(scene.stage.fragment_count==1&&scene.stage.coin_count==1);
  assert(scene.enemies[0].dead&&scene.reached_checkpoints==1);
  /* Pit respawns even while immune, without consuming another heart. */
  loop.player.y=700;
  assert(rikitu_demo_advance(&scene,&loop,RIKITU_FIXED_DT,0)==1);
  assert(near(loop.player.x,2050)&&scene.hearts==2);
  assert(scene.damage_count==1);
  /* Last heart is replenished at checkpoint; no total game-over. */
  scene.invulnerability_seconds=0;
  scene.hearts=1;
  loop.player.y=700;
  assert(rikitu_demo_advance(&scene,&loop,RIKITU_FIXED_DT,0)==1);
  assert(scene.hearts==3 && scene.damage_count==2);
  assert(near(loop.player.x,2050));
  /* Paused gameplay never advances patrol position or invincibility timer. */
  reset(&scene,&loop);
  loop.paused=1;
  scene.invulnerability_seconds=1.0f;
  assert(rikitu_demo_advance(&scene,&loop,1,0)==0);
  assert(near(scene.enemies[0].x,820));
  assert(near(scene.invulnerability_seconds,1));
  scene.enemy_count=RIKITU_MAX_ENEMIES+1;
  loop.paused=0;
  assert(rikitu_demo_advance(&scene,&loop,RIKITU_FIXED_DT,0)==-1);
  puts("PASS native MAP01: six patrols, stomp, hearts, immunity, pit respawn, checkpoint persistence");
  return 0;
}
