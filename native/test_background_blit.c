#include "rikitu_background_blit.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Synthetic 80x8 panorama; no artwork, external resources or private pixels. */
enum {SW=80,SH=8,SS=84,DW=32,DH=8,DS=36};
static uint32_t source[SH*SS];
static uint32_t target[DH*DS];
static const uint32_t SENTINEL=0x13572468u;

static uint32_t synthetic(int x,int y){
  return 0xff000000u | ((uint32_t)y<<16) | (uint32_t)x;
}
static void fill(void){
  int y,x;
  for(y=0;y<SH;y++)
    for(x=0;x<SS;x++)
      source[y*SS+x]=(x<SW)?synthetic(x,y):0xabcdef12u;
  for(y=0;y<DH*DS;y++)target[y]=SENTINEL;
}
static void assert_expected(int crop){
  int y,x;
  for(y=0;y<DH;y++){
    for(x=0;x<DW;x++)assert(target[y*DS+x]==synthetic(x+crop,y));
    for(x=DW;x<DS;x++)assert(target[y*DS+x]==SENTINEL);
  }
  for(y=0;y<SH;y++)
    for(x=SW;x<SS;x++)
      assert(source[y*SS+x]==0xabcdef12u);
}
static void test_cameras(void){
  const RikituBackgroundImage img={source,SH*SS,SW,SH,SS};
  int x=-1;
  fill();
  assert(rikitu_background_blit_rgba8888(target,DH*DS,DW,DH,DS,
                                         &img,0.0,100.0,&x));
  assert(x==0);assert_expected(0);
  fill();x=-1;
  assert(rikitu_background_blit_rgba8888(target,DH*DS,DW,DH,DS,
                                         &img,34.0,100.0,&x));
  assert(x==24);assert_expected(24);
  fill();x=-1;
  assert(rikitu_background_blit_rgba8888(target,DH*DS,DW,DH,DS,
                                         &img,68.0,100.0,&x));
  assert(x==48);assert_expected(48);
  fill();x=-1;
  assert(rikitu_background_blit_rgba8888(target,DH*DS,DW,DH,DS,
                                         &img,1000.0,100.0,&x));
  assert(x==48);assert_expected(48);
}
static void test_failures_leave_frame_intact(void){
  const RikituBackgroundImage good={source,SH*SS,SW,SH,SS};
  const RikituBackgroundImage short_source={source,SH*SS-5,SW,SH,SS};
  const RikituBackgroundImage short_stride={source,SH*SS,SW,SH,79};
  const RikituBackgroundImage wrong_height={source,SH*SS,SW,SH-1,SS};
  const RikituBackgroundImage narrow={source,SH*SS,31,SH,SS};
  const RikituBackgroundImage alias={target,DH*DS,DW,DH,DS};
  int x=777, n;
  fill();
  assert(!rikitu_background_blit_rgba8888(NULL,DH*DS,DW,DH,DS,&good,0,100,&x));
  assert(!rikitu_background_blit_rgba8888(target,DH*DS,0,DH,DS,&good,0,100,&x));
  assert(!rikitu_background_blit_rgba8888(target,DH*DS,DW,DH,DW-1,&good,0,100,&x));
  assert(!rikitu_background_blit_rgba8888(target,DH*DS-5,DW,DH,DS,&good,0,100,&x));
  assert(!rikitu_background_blit_rgba8888(target,DH*DS,DW,DH,DS,NULL,0,100,&x));
  assert(!rikitu_background_blit_rgba8888(target,DH*DS,DW,DH,DS,&short_source,0,100,&x));
  assert(!rikitu_background_blit_rgba8888(target,DH*DS,DW,DH,DS,&short_stride,0,100,&x));
  assert(!rikitu_background_blit_rgba8888(target,DH*DS,DW,DH,DS,&wrong_height,0,100,&x));
  assert(!rikitu_background_blit_rgba8888(target,DH*DS,DW,DH,DS,&narrow,0,100,&x));
  assert(!rikitu_background_blit_rgba8888(target,DH*DS,DW,DH,DS,&alias,0,100,&x));
  assert(!rikitu_background_blit_rgba8888(target,DH*DS,DW,DH,DS,&good,NAN,100,&x));
  assert(!rikitu_background_blit_rgba8888(target,DH*DS,DW,DH,DS,&good,0,INFINITY,&x));
  assert(!rikitu_background_blit_rgba8888(target,DH*DS,DW,DH,DS,&good,0,DW-1,&x));
  assert(x==777);
  for(n=0;n<DH*DS;n++)assert(target[n]==SENTINEL);
}
int main(void){
  test_cameras();
  test_failures_leave_frame_intact();
  puts("PASS Stage148: panorama source crop, 0/50/100% camera, stride sentinels, invalid buffers");
  return 0;
}
