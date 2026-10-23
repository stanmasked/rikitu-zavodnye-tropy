#include "rikitu_draw.h"
#include "rikitu_demo.h"
#include "rikitu_background_blit.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
enum { W=1280,H=720,SW=1600,DS=1288,SS=1608 };
static uint32_t src[H*SS],dst[H*DS];
static uint32_t px(int x,int y){return 0xff000000u | ((uint32_t)y<<16) | (uint32_t)x;}
static uint32_t at(int x,int y){return dst[(size_t)y*DS+x];}
int main(void){
  RikituScene scene;
  RikituBackgroundImage image={src,H*SS,SW,H,SS};
  int x,y,crop=-1;
  for(y=0;y<H;y++)for(x=0;x<SS;x++)src[y*SS+x]=x<SW?px(x,y):0xabcdef12u;
  rikitu_demo_init(&scene);
  for(y=0;y<H*DS;y++)dst[y]=0x12345678u;
  assert(rikitu_background_blit_rgba8888(dst,H*DS,W,H,DS,&image,0.0,scene.world_width,&crop));
  assert(crop==0);
  assert(rikitu_draw_scene_over_background(dst,W,H,DS,&scene));
  assert(at(0,100)==px(0,100));
  assert(at(120,500)==0xffe9dc4eu);
  assert(at(100,560)==0xff65524au);
  assert(at(0,0)!=px(0,0));
  for(y=0;y<H;y++)for(x=W;x<DS;x++)assert(dst[y*DS+x]==0x12345678u);
  for(y=0;y<H*DS;y++)dst[y]=0x12345678u;
  assert(rikitu_draw_scene(dst,W,H,DS,&scene));
  assert(at(0,100)!=px(0,100));
  assert(at(120,500)==0xffe9dc4eu);
  assert(!rikitu_draw_scene_over_background(NULL,W,H,DS,&scene));
  assert(!rikitu_draw_scene_over_background(dst,200,H,DS,&scene));
  puts("PASS Stage149 optional synthetic background and unchanged default scene");
  return 0;
}
