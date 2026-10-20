#include "rikitu_draw.h"
#include "rikitu_demo.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { W=1280,H=720,STRIDE=1288 };
static uint32_t pixels[H*STRIDE];
static uint32_t pixel(int x,int y) {return pixels[(size_t)y*STRIDE+x];}
static void save_ppm(const char *file) {
  FILE *f=fopen(file,"wb");
  int y,x;
  assert(f);
  assert(fprintf(f,"P6\n%d %d\n255\n",W,H)>0);
  for(y=0;y<H;y++)
    for(x=0;x<W;x++){
      uint32_t p=pixel(x,y);
      unsigned char channels[3]={(unsigned char)p,
         (unsigned char)(p>>8),(unsigned char)(p>>16)};
      assert(fwrite(channels,1,3,f)==3);
    }
  assert(fclose(f)==0);
}
int main(int argc,char **argv) {
  RikituScene s;
  uint32_t fragment_before,fragment_after;
  int y,x;
  rikitu_demo_init(&s);
  assert(s.stage.fragment_count==0 && s.world_width==3000);
  for(y=0;y<H;y++)for(x=0;x<STRIDE;x++)pixels[(size_t)y*STRIDE+x]=0x12345678u;
  assert(rikitu_draw_scene(pixels,W,H,STRIDE,&s));
  assert(pixel(120,500)==0xffe9dc4eu); /* neutral cyan square */
  assert(pixel(100,560)==0xff65524au); /* solid platform fill */
  fragment_before=pixel(610,506);
  assert(fragment_before==0xff9bf0ffu); /* inner gold fragment */
  assert(pixel(0,100)!=pixel(0,400)); /* sky gradient */
  for(y=0;y<H;y++)for(x=W;x<STRIDE;x++)
    assert(pixels[(size_t)y*STRIDE+x]==0x12345678u);
  s.stage.fragments[0]=1;s.stage.fragment_count=1;
  assert(rikitu_draw_scene(pixels,W,H,STRIDE,&s));
  fragment_after=pixel(610,506);
  assert(fragment_before!=fragment_after);
  s.player.x=2680.0f;
  assert(rikitu_draw_scene(pixels,W,H,STRIDE,&s)); /* camera follows to gate */
  s.paused=1;
  assert(rikitu_draw_scene(pixels,W,H,STRIDE,&s));
  s.paused=0;s.stage.completed=1;
  assert(rikitu_draw_scene(pixels,W,H,STRIDE,&s));
  assert(!rikitu_draw_scene(NULL,W,H,STRIDE,&s));
  assert(!rikitu_draw_scene(pixels,W,H,W-1,&s));
  assert(!rikitu_draw_scene(pixels,200,H,STRIDE,&s));
  assert(!rikitu_draw_scene(pixels,W,H,STRIDE,NULL));
  rikitu_demo_init(&s);
  assert(rikitu_draw_scene(pixels,W,H,STRIDE,&s));
  if(argc>1)save_ppm(argv[1]);
  puts("PASS portable scene renderer 1280x720: player, camera, stage HUD, fragment, clipping, stride guards");
  return 0;
}
