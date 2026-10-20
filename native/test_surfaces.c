#include "rikitu_surfaces.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static int near(float x,float y) {return fabsf(x-y)<0.02f;}
static RikituPlayer make_player(float x,float bottom,int grounded) {
  RikituPlayer p;rikitu_player_init(&p,x,bottom-43.0f);p.grounded=grounded;return p;
}
int main(void) {
  const float dt=RIKITU_FIXED_DT;
  const float width=1000.0f;
  RikituPlayer p;
  RikituConveyor belts[]={{60.0f,55.0f,600.0f},{20.0f,110.0f,-800.0f}};
  RikituLauncher launchers[]={
    {RIKITU_LAUNCH_MUSHROOM,85.0f,100.0f},
    {RIKITU_LAUNCH_CRYSTAL,85.0f,100.0f},
    {RIKITU_LAUNCH_GUST,85.0f,100.0f},
    {RIKITU_LAUNCH_CLOCK,85.0f,100.0f},
    {RIKITU_LAUNCH_STEAM,85.0f,100.0f}
  };
  RikituLauncher only;
  int i;float expected[] = {0.0f,-705.0f,-675.0f,-690.0f,-690.0f,-730.0f};
  p=make_player(50,100,1);
  assert(rikitu_apply_surfaces(&p,dt,width,belts,1,NULL,0)==0);
  assert(near(p.x,60.0f) && p.grounded);
  p=make_player(50,100,1);
  assert(rikitu_apply_surfaces(&p,dt,width,belts,1,launchers,5)==RIKITU_LAUNCH_STEAM);
  assert(near(p.x,60.0f) && near(p.vy,-705.0f) && !p.grounded);
  for(i=1;i<=5;i++) {
    p=make_player(85,100,1);
    only.kind=(RikituLauncherKind)i;only.x=85.0f;only.y=100.0f;
    assert(rikitu_apply_surfaces(&p,dt,width,NULL,0,&only,1)==i);
    assert(near(p.vy,expected[i]) && !p.grounded);
  }
  /* Clock wins over a mushroom even when mushroom appears first. */
  p=make_player(85,100,1);
  assert(rikitu_apply_surfaces(&p,dt,width,NULL,0,launchers,5)==
         RIKITU_LAUNCH_STEAM);
  /* Mushroom alignment requires strict less-than 7 pixels. */
  only.kind=RIKITU_LAUNCH_MUSHROOM;only.x=85.0f;only.y=107.0f;
  p=make_player(85,100,1);
  assert(rikitu_apply_surfaces(&p,dt,width,NULL,0,&only,1)==0);
  only.y=106.9f;
  p=make_player(85,100,1);
  assert(rikitu_apply_surfaces(&p,dt,width,NULL,0,&only,1)==RIKITU_LAUNCH_MUSHROOM);
  /* Grounded only, and no mutation while airborne. */
  p=make_player(70,100,0);
  assert(rikitu_apply_surfaces(&p,dt,width,belts,2,launchers,5)==0);
  assert(near(p.x,70.0f) && near(p.vy,0.0f));
  /* First overlapping belt takes precedence over subsequent belts. */
  p=make_player(80,100,1);
  assert(rikitu_apply_surfaces(&p,dt,width,belts,2,NULL,0)==0);
  assert(near(p.x,90.0f));
  /* Clamp the horizontal range after belt displacement. */
  belts[0].x=910;belts[0].width=110;belts[0].speed=1000;
  p=make_player(966,100,1);
  assert(rikitu_apply_surfaces(&p,dt,width,belts,1,NULL,0)==0);
  assert(near(p.x,967.0f));
  /* Exactly touching the left or right window is not an overlap. */
  only.kind=RIKITU_LAUNCH_STEAM;only.x=90;only.y=100;
  p=make_player(39,100,1); /* right edge = 72 */
  assert(rikitu_apply_surfaces(&p,dt,width,NULL,0,&only,1)==0);
  p=make_player(142,100,1);
  assert(rikitu_apply_surfaces(&p,dt,width,NULL,0,&only,1)==0);
  p=make_player(50,100,1);
  assert(rikitu_apply_surfaces(&p,dt,width,NULL,1,NULL,0)==-1);
  assert(rikitu_apply_surfaces(&p,dt,width,NULL,0,NULL,1)==-1);
  assert(rikitu_apply_surfaces(NULL,dt,width,NULL,0,NULL,0)==-1);
  assert(rikitu_apply_surfaces(&p,-1.0f,width,NULL,0,NULL,0)==-1);
  assert(rikitu_apply_surfaces(&p,dt,10.0f,NULL,0,NULL,0)==-1);
  puts("PASS portable surfaces: conveyor displacement, all five launchers, order, bounds and validation");
  return 0;
}
