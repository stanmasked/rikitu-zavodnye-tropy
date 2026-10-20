/* Deterministic C99 movement snapshots for actual browser-physics replay.
 * Protocol: I x y vx vy w h grounded coyote buffer previousJump facing
 *           S inputMask hasFloor
 */
#include "rikitu_loop.h"
#include <stdio.h>
int main(void) {
  char line[512];
  RikituPlayer player;
  RikituRect floor={0.0f,100.0f,1000.0f,300.0f};
  int ready=0;
  while(fgets(line,sizeof(line),stdin)) {
    if(line[0]=='I' && line[1]==' ') {
      if(sscanf(line+2,"%f %f %f %f %f %f %d %f %f %d %d",
                &player.x,&player.y,&player.vx,&player.vy,
                &player.w,&player.h,&player.grounded,
                &player.coyote_seconds,&player.jump_buffer_seconds,
                &player.previous_jump,&player.facing)!=11) return 2;
      ready=1;
    } else if(line[0]=='S' && line[1]==' ') {
      unsigned mask;
      int has_floor;
      if(!ready || sscanf(line+2,"%u %d",&mask,&has_floor)!=2 ||
         !rikitu_player_step(&player,mask,RIKITU_FIXED_DT,1000.0f,
                            has_floor?&floor:NULL,has_floor?1U:0U)) return 3;
      printf("%.9g %.9g %.9g %.9g %.9g %.9g %d %d %d\n",
             player.x,player.y,player.vx,player.vy,
             player.coyote_seconds,player.jump_buffer_seconds,
             player.grounded,player.previous_jump,player.facing);
    } else return 4;
  }
  return ferror(stdin)?5:0;
}
