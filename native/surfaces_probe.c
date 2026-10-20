/* Text protocol to compare C99 surface reactions with the actual browser code.
 * S playerX playerY grounded playerVy dt levelWidth
 * B conveyorX width speed
 * L type x y
 * E -> evaluate and output playerX playerVy grounded launcherKind
 */
#include "rikitu_surfaces.h"
#include <stdio.h>
int main(void) {
  char line[256];
  RikituPlayer p;
  RikituConveyor belts[12];
  RikituLauncher launchers[24];
  float dt=RIKITU_FIXED_DT,world_width=1000.0f;
  unsigned nb=0,nl=0;
  int ready=0,launched;
  while(fgets(line,sizeof(line),stdin)) {
    if(line[0]=='S'&&line[1]==' ') {
      float x,y,vy;
      int grounded;
      if(sscanf(line+2,"%f %f %d %f %f %f",
                &x,&y,&grounded,&vy,&dt,&world_width)!=6) return 2;
      rikitu_player_init(&p,x,y);
      p.grounded=grounded; p.vy=vy;
      nb=0;nl=0;ready=1;
    } else if(line[0]=='B'&&line[1]==' ') {
      if(!ready||nb==12||
         sscanf(line+2,"%f %f %f",
                &belts[nb].x,&belts[nb].width,&belts[nb].speed)!=3) return 3;
      nb++;
    } else if(line[0]=='L'&&line[1]==' ') {
      int kind;
      if(!ready||nl==24||
         sscanf(line+2,"%d %f %f",&kind,&launchers[nl].x,
                &launchers[nl].y)!=3) return 4;
      launchers[nl].kind=(RikituLauncherKind)kind;
      nl++;
    } else if(line[0]=='E'&&line[1]=='\n') {
      if(!ready) return 5;
      launched=rikitu_apply_surfaces(&p,dt,world_width,
                   belts,nb,launchers,nl);
      if(launched<0) return 6;
      printf("%.9g %.9g %d %d\n",p.x,p.vy,p.grounded,launched);
      ready=0;
    } else return 7;
  }
  return ready||ferror(stdin)?8:0;
}
