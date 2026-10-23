/* Stage150: five real MAP01 camera positions with synthetic-only background.
 * No private SVG, textures, screenshots, files or approved character pixels.
 */
#include "rikitu_flower.h"
#include "rikitu_draw.h"
#include "rikitu_background_blit.h"
#include "rikitu_parallax.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

enum { W=1280, H=720, SW=5120, SRC_STRIDE=5128, DST_STRIDE=1288 };
static uint32_t source[H*SRC_STRIDE];
static uint32_t frame[H*DST_STRIDE];
static const uint32_t PAD=0x12345678u;
static const int camera_position[5]={0,1655,3310,4965,6620};
static const int expected_crop[5]={0,960,1920,2880,3840};

static uint32_t make_pixel(int x,int y){
  return 0xff000000u | ((uint32_t)(y&255)<<16) | (uint32_t)(x&65535);
}
static void reset_frame(void){
  size_t i;
  for(i=0;i<(size_t)H*DST_STRIDE;i++) frame[i]=PAD;
}
static uint32_t screen_pixel(int x,int y){
  return frame[(size_t)y*DST_STRIDE+(size_t)x];
}
static void verify_guard_bytes(void){
  int y,x;
  for(y=0;y<H;y++){
    for(x=W;x<DST_STRIDE;x++)
      assert(screen_pixel(x,y)==PAD);
    for(x=SW;x<SRC_STRIDE;x++)
      assert(source[(size_t)y*SRC_STRIDE+(size_t)x]==PAD);
  }
}
int main(void){
  RikituScene scene;
  RikituBackgroundImage art={source,(size_t)H*SRC_STRIDE,SW,H,SRC_STRIDE};
  int i,y,x,source_x=-1;
  assert(sizeof(camera_position)/sizeof(camera_position[0])==5u);
  for(y=0;y<H;y++){
    for(x=0;x<SW;x++)
      source[(size_t)y*SRC_STRIDE+(size_t)x]=make_pixel(x,y);
    for(x=SW;x<SRC_STRIDE;x++)
      source[(size_t)y*SRC_STRIDE+(size_t)x]=PAD;
  }
  for(i=0;i<5;i++){
    float cam;
    int actor_screen_x,actor_screen_y;
    rikitu_flower_init(&scene);
    assert(scene.level_id==1 && scene.world_width==7900.0f);
    assert(scene.solid_count==19 && scene.coin_total==90);
    scene.player.x=(float)camera_position[i]+W*.37f;
    scene.player.y=413.0f;
    cam=scene.player.x-W*.37f;
    if(cam<0.0f)cam=0.0f;
    if(cam>scene.world_width-W)cam=scene.world_width-W;
    if(cam<0.0f)cam=0.0f;
    reset_frame();
    assert(rikitu_background_blit_rgba8888(
      frame,(size_t)H*DST_STRIDE,W,H,DST_STRIDE,
      &art,(double)cam,scene.world_width,&source_x));
    assert(abs(source_x-expected_crop[i])<=1);
    /* Pixel under clear sky must still match the real selected source crop. */
    assert(screen_pixel(0,120)==make_pixel(source_x,120));
    assert(rikitu_draw_scene_over_background(
      frame,W,H,DST_STRIDE,&scene));
    assert(screen_pixel(0,120)==make_pixel(source_x,120));
    assert(screen_pixel(W-1,120)==make_pixel(source_x+W-1,120));
    /* A solid floor and fixed HUD are drawn in front of the source. */
    assert(screen_pixel(0,0)==0xff311e10u);
    actor_screen_x=(int)floorf(scene.player.x-cam+0.5f)+15;
    actor_screen_y=(int)scene.player.y+20;
    assert(actor_screen_x>=0 && actor_screen_x<W);
    assert(actor_screen_y>84 && actor_screen_y<H);
    assert(screen_pixel(actor_screen_x,actor_screen_y)==0xffe9dc4eu);
    verify_guard_bytes();
  }
  /* Unmodified entry point must repaint deterministic debug sky. */
  rikitu_flower_init(&scene);
  reset_frame();
  assert(rikitu_draw_scene(frame,W,H,DST_STRIDE,&scene));
  assert(screen_pixel(0,120)!=make_pixel(0,120));
  verify_guard_bytes();
  puts("PASS Stage150: MAP01 5 camera crops, real 19 solids/90 coins, HUD/player overlay and private-art isolation");
  return 0;
}
