#include "rikitu_draw.h"
#include <math.h>
#include <stdio.h>

typedef struct Canvas { uint32_t *pixels; int w,h,stride; } Canvas;
static uint32_t rgb(unsigned r,unsigned g,unsigned b) {
  return 0xff000000u | (b<<16) | (g<<8) | r;
}
static int clip(int v,int lo,int hi) {
  return v<lo?lo:(v>hi?hi:v);
}
static void box(Canvas *c,int x,int y,int w,int h,uint32_t color){
  int left,top,right,bottom,row,col;
  if(w<=0||h<=0)return;
  left=clip(x,0,c->w);top=clip(y,0,c->h);
  right=clip(x+w,0,c->w);bottom=clip(y+h,0,c->h);
  for(row=top;row<bottom;row++) {
    uint32_t *p=c->pixels+(size_t)row*c->stride;
    for(col=left;col<right;col++)p[col]=color;
  }
}
static void diamond(Canvas *c,int cx,int cy,int radius,uint32_t color){
  int d;
  for(d=-radius;d<=radius;d++){
    const int half=radius-(d<0?-d:d);
    box(c,cx-half,cy+d,half*2+1,1,color);
  }
}
static void disk(Canvas *c,int cx,int cy,int r,uint32_t color){
  int y,x;
  for(y=-r;y<=r;y++)
    for(x=-r;x<=r;x++)
      if(x*x+y*y<=r*r)box(c,cx+x,cy+y,1,1,color);
}
/* Minimal block font for the technical scene only; no game-brand artwork. */
static const char *const font_keys=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/-!.";
static const unsigned char font[][7]={
{0,0,0,0,0,0,0}, /* space */
{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},
{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},
{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
{14,17,16,23,17,17,14},{17,17,17,31,17,17,17},
{14,4,4,4,4,4,14},{7,2,2,2,18,18,12},
{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},
{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},
{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},
{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},
{2,6,10,18,31,2,2},{31,16,30,1,1,17,14},
{6,8,16,30,17,17,14},{31,1,2,4,8,8,8},
{14,17,17,14,17,17,14},{14,17,17,15,1,2,12},
{0,4,4,0,4,4,0},{1,2,4,8,16,0,0},
{0,0,31,0,31,0,0},{0,0,0,31,0,0,0},
{4,4,4,4,0,4,0},{0,0,0,0,0,12,12}
};
static void label(Canvas *c,int x,int y,const char *s,int scale,uint32_t color) {
  int n;
  if(!s||scale<1)return;
  for(n=0;s[n];n++) {
    const char ch=(s[n]>='a'&&s[n]<='z')?s[n]-32:s[n];
    int index=0,k,r,b;
    while(font_keys[index]&&font_keys[index]!=ch)index++;
    if(!font_keys[index]) index=0;
    for(r=0;r<7;r++)
      for(b=0;b<5;b++)
        if(font[index][r]&(1u<<(4-b)))
          box(c,x+n*6*scale+b*scale,y+r*scale,scale,scale,color);
    k=0;(void)k;
  }
}
static int worldx(float x,float camera){
  return (int)floorf(x-camera+0.5f);
}
int rikitu_draw_scene(uint32_t *pixels,int width,int height,
                      int stride_pixels,const RikituScene *s) {
  Canvas c;
  uint32_t sky,ink=rgb(232,240,250);
  float camera;
  int x,y,i;
  char status[64];
  if(!pixels||!s||width<320||height<240||stride_pixels<width||
     width>3840||height>2160||!isfinite(s->world_width)||
     s->world_width<=0.0f||!isfinite(s->player.x)||
     !isfinite(s->player.y)||!isfinite(s->gate_x)||
     s->solid_count>RIKITU_MAX_SOLIDS || s->coin_total>RIKITU_MAX_COINS ||
     s->belt_count>RIKITU_MAX_BELTS ||
     s->launcher_count>RIKITU_MAX_LAUNCHERS ||
     s->checkpoint_total>RIKITU_MAX_CHECKPOINTS ||
     s->enemy_count>RIKITU_MAX_ENEMIES)return 0;
  c.pixels=pixels;c.w=width;c.h=height;c.stride=stride_pixels;
  camera=s->player.x-width*.37f;
  if(camera<0.0f)camera=0.0f;
  if(camera>s->world_width-width)camera=s->world_width-width;
  if(camera<0.0f)camera=0.0f;
  /* Deterministic sky gradient and parallax silhouettes. */
  for(y=0;y<height;y++){
    const unsigned t=(unsigned)((size_t)y*80u/(unsigned)height);
    sky=rgb(33+t/3,74+t/2,118+t);
    box(&c,0,y,width,1,sky);
  }
  for(i=0;i<9;i++){
    x=(i*301-(int)(camera*.23f))%(width+400);
    if(x<0)x+=width+400;
    box(&c,x-100,330+i%3*24,170,190,rgb(54,115,132));
    box(&c,x-55,310+i%3*24,90,26,rgb(54,115,132));
  }
  /* A few geometry-only scenery elements; no altered approved art. */
  for(i=0;i<(int)s->solid_count;i++){
    const RikituRect *b=&s->solids[i];
    if(!isfinite(b->x)||!isfinite(b->y)||!isfinite(b->w)||!isfinite(b->h))
      return 0;
    x=worldx(b->x,camera);y=(int)b->y;
    box(&c,x,y,(int)b->w,(int)b->h,rgb(74,82,101));
    box(&c,x,y,(int)b->w,13,rgb(104,185,119));
    for(int tile=0;tile<(int)b->w;tile+=56)
      box(&c,x+tile,y+22,22,5,rgb(93,103,117));
  }
  for(i=0;i<(int)s->belt_count;i++){
    const RikituConveyor *belt=&s->belts[i];
    x=worldx(belt->x,camera);
    box(&c,x,529,(int)belt->width,11,rgb(181,133,61));
    for(int col=6;col<(int)belt->width;col+=28)
      diamond(&c,x+col,534,3,rgb(239,218,142));
  }
  for(i=0;i<(int)s->launcher_count;i++){
    const RikituLauncher *l=&s->launchers[i];
    x=worldx(l->x,camera);
    box(&c,x-14,522,40,18,rgb(89,177,197));
    diamond(&c,x+6,517,10,rgb(187,246,242));
  }
  for(i=0;i<(int)s->coin_total;i++){
    if(!s->stage.coins[i]){
      x=worldx(s->coins[i].x,camera);y=(int)s->coins[i].y;
      disk(&c,x,y,10,rgb(255,219,75));
      box(&c,x-1,y-5,3,11,rgb(159,101,39));
    }
  }
  for(i=0;i<RIKITU_STAGE_FRAGMENTS;i++){
    if(!s->stage.fragments[i]){
      x=worldx(s->fragments[i].x,camera);y=(int)s->fragments[i].y;
      diamond(&c,x,y,20,rgb(247,186,72));
      diamond(&c,x,y,10,rgb(255,240,155));
    }
  }
  /* Neutral orange patrol rectangles, not approved antagonist sprites. */
  for(i=0;i<(int)s->enemy_count;i++) {
    const RikituEnemy *e=&s->enemies[i];
    if(e->dead)continue;
    x=worldx(e->x,camera);
    y=(int)e->y;
    box(&c,x-2,y-2,(int)e->w+4,(int)e->h+4,rgb(39,43,63));
    box(&c,x,y,(int)e->w,(int)e->h,rgb(236,141,87));
    box(&c,x+6,y+8,5,5,rgb(49,56,71));
    box(&c,x+22,y+8,5,5,rgb(49,56,71));
  }
  x=worldx(s->gate_x,camera);
  box(&c,x,436,26,104,rgb(23,40,53));
  box(&c,x-15,420,56,22,
      rikitu_stage_can_finish(&s->stage)?rgb(87,228,148):rgb(227,104,109));
  box(&c,x+12,466,6,40,rgb(238,245,222));
  /* Neutral rectangular player hitbox: not a character rendition. */
  x=worldx(s->player.x,camera);y=(int)s->player.y;
  box(&c,x-3,y-3,(int)s->player.w+6,(int)s->player.h+6,rgb(13,40,60));
  box(&c,x,y,(int)s->player.w,(int)s->player.h,rgb(78,220,233));
  box(&c,x+5,y+10,7,7,rgb(18,57,81));
  box(&c,x+20,y+10,7,7,rgb(18,57,81));
  box(&c,x+8,y+29,17,4,rgb(31,119,141));
  /* Fixed-screen HUD: title, counters and controller instructions. */
  box(&c,0,0,width,84,rgb(16,30,49));
  box(&c,0,80,width,4,rgb(76,176,187));
  label(&c,25,15,s->level_id==1?"RIKITU 01 FLOWER TRAIL":"RIKITU TECH DEMO",3,ink);
  (void)snprintf(status,sizeof(status),"FRAGMENTS %d/3  COINS %d",
       s->stage.fragment_count,s->stage.coin_count);
  label(&c,26,56,status,2,rgb(250,220,124));
  (void)snprintf(status,sizeof(status),"HEARTS %d/%d HITS %d",
                 s->hearts,s->max_hearts,s->damage_count);
  label(&c,495,15,status,2,rgb(247,166,161));
  if(s->level_id==1){
    (void)snprintf(status,sizeof(status),"CHECKPOINTS %d/3",
                   s->reached_checkpoints);
    label(&c,590,56,status,2,rgb(149,221,203));
  }
  label(&c,width-370,25,"A JUMP / R RUN",2,ink);
  label(&c,width-370,51,"PLUS EXIT",2,ink);
  if(s->paused) {
    box(&c,width/2-185,height/2-65,370,130,rgb(16,30,49));
    label(&c,width/2-114,height/2-18,"PAUSED",4,ink);
  } else if(s->stage.completed) {
    box(&c,width/2-260,height/2-65,520,130,rgb(16,50,49));
    label(&c,width/2-232,height/2-18,"STAGE COMPLETE",4,ink);
  }
  return 1;
}
