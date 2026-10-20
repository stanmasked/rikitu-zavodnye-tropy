/* Campaign-wide host integration test: all 16 original maps in one binary.
 * Neutral graphics only. No ROM, Nintendo SDK, or approved character art.
 */
#include "rikitu_flower.h"
#include "rikitu_mill.h"
#include "rikitu_grove.h"
#include "rikitu_bridge.h"
#include "rikitu_workshop.h"
#include "rikitu_delivery.h"
#include "rikitu_clock.h"
#include "rikitu_orders.h"
#include "rikitu_mist.h"
#include "rikitu_cloud.h"
#include "rikitu_crystal.h"
#include "rikitu_mine.h"
#include "rikitu_gardens.h"
#include "rikitu_foundry.h"
#include "rikitu_gallery.h"
#include "rikitu_heart.h"
#include "rikitu_draw.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*SceneInit)(RikituScene *);
static const SceneInit scene_init[16]={
  rikitu_flower_init,rikitu_mill_init,rikitu_grove_init,rikitu_bridge_init,
  rikitu_workshop_init,rikitu_delivery_init,rikitu_clock_init,rikitu_orders_init,
  rikitu_mist_init,rikitu_cloud_init,rikitu_crystal_init,rikitu_mine_init,
  rikitu_gardens_init,rikitu_foundry_init,rikitu_gallery_init,rikitu_heart_init
};
enum { W=RIKITU_SCENE_WIDTH,H=RIKITU_SCENE_HEIGHT,PAD=4 };
int main(void){
  RikituScene scene;
  RikituLoop loop;
  uint32_t *pixels;
  int level,seen_five=0,boss_count=0;
  const size_t count=(size_t)W*H;
  assert(sizeof(scene_init)/sizeof(scene_init[0])==16u);
  pixels=(uint32_t*)malloc((count+2u*PAD)*sizeof(uint32_t));
  assert(pixels);
  for(level=1;level<=16;level++){
    size_t i;
    int nonzero=0;
    /* The preceding map may leave a dirty scene; init must clear it. */
    memset(&scene,0xa5,sizeof(scene));
    scene_init[level-1](&scene);
    assert(scene.level_id==level);
    assert(scene.world_width>scene.gate_x && scene.gate_x>0.0f);
    assert(scene.solid_count>=18 && scene.solid_count<=RIKITU_MAX_SOLIDS);
    assert(scene.coin_total==90 && scene.coin_total<=RIKITU_MAX_COINS);
    assert(scene.checkpoint_total==3 &&
           scene.checkpoint_total<=RIKITU_MAX_CHECKPOINTS);
    assert(scene.belt_count<=RIKITU_MAX_BELTS);
    assert(scene.launcher_count<=RIKITU_MAX_LAUNCHERS);
    assert(scene.enemy_count==6 && scene.enemy_count<=RIKITU_MAX_ENEMIES);
    assert(scene.lever_count<=RIKITU_MAX_LEVERS);
    assert(scene.hearts==RIKITU_STARTING_HEARTS &&
           scene.max_hearts==RIKITU_STARTING_HEARTS);
    assert(scene.stage.fragment_count==0 && scene.stage.coin_count==0);
    assert(!scene.stage.completed && !rikitu_stage_can_finish(&scene.stage));
    assert(scene.player.w==33.0f && scene.player.h==43.0f);
    assert(scene.active_checkpoint==105.0f);
    if(scene.stage.boss_required){
      assert(scene.boss.hp==3 && scene.boss.alive==1);
      boss_count++;
    }else{
      assert(!scene.boss.alive && scene.stage.boss_defeated);
    }
    for(i=0;i<scene.launcher_count;i++)
      assert(scene.launchers[i].kind>=RIKITU_LAUNCH_STEAM &&
             scene.launchers[i].kind<=RIKITU_LAUNCH_MUSHROOM);
    if(scene.launcher_count==5){assert(level==16);seen_five++;}
    memset(&loop,0,sizeof(loop));
    loop.player=scene.player;
    assert(rikitu_demo_advance(&scene,&loop,RIKITU_FIXED_DT,0)==1);
    assert(scene.hearts==RIKITU_STARTING_HEARTS && scene.damage_count==0);
    assert(!scene.stage.completed);
    for(i=0;i<count+2u*PAD;i++)pixels[i]=0xa55a5aa5u;
    assert(rikitu_draw_scene(pixels+PAD,W,H,W,&scene)==1);
    /* A 640x360 canvas only showed the sky: the actual player is
     * below y=400. At the real 1280x720 resolution, assert one
     * unoccluded body pixel, not merely nonempty background.
     * Initial x=105 also ensures that the camera is at the origin. */
    assert(scene.player.x>=0.0f && scene.player.x<200.0f);
    assert(scene.player.y>=0.0f && scene.player.y<520.0f);
    {
      const size_t body_pixel=PAD+
          (size_t)((int)scene.player.y+20)*W+
          (size_t)((int)scene.player.x+15);
      assert(body_pixel<PAD+count);
      assert(pixels[body_pixel]==0xffe9dc4eu);
    }
    for(i=0;i<PAD;i++){
      assert(pixels[i]==0xa55a5aa5u);
      assert(pixels[count+PAD+i]==0xa55a5aa5u);
    }
    for(i=0;i<count;i++)
      if(pixels[PAD+i]!=0u && pixels[PAD+i]!=0xa55a5aa5u){
        nonzero=1;break;
      }
    assert(nonzero);
    /* Reinitialization resets pickups, hearts and counters. */
    scene.stage.coins[0]=1;
    scene.stage.coin_count=1;
    scene.hearts=1;
    scene_init[level-1](&scene);
    assert(scene.level_id==level && scene.hearts==RIKITU_STARTING_HEARTS &&
           !scene.stage.coins[0] && scene.stage.coin_count==0);
    printf("MAP%02d: one tick, player pixel, framebuffer guard and reset PASS\n",level);
  }
  assert(seen_five==1 && boss_count>=1);
  free(pixels);
  puts("PASS 16/16 campaign scenes: tick, visible player pixels, buffer bounds and reset");
  return 0;
}
