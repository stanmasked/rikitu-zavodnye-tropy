#include "rikitu_demo.h"
#include <string.h>
void rikitu_demo_init(RikituScene *s) {
  unsigned i;
  if(!s)return;
  memset(s,0,sizeof(*s));
  s->world_width=3000.0f;s->gate_x=2890.0f;
  rikitu_player_init(&s->player,120.0f,497.0f);
  s->player.grounded=1;
  rikitu_stage_init(&s->stage,0);
  s->solids[0]=(RikituRect){0.0f,540.0f,3000.0f,180.0f};
  s->solids[1]=(RikituRect){520.0f,468.0f,240.0f,18.0f};
  s->solids[2]=(RikituRect){1140.0f,452.0f,260.0f,18.0f};
  s->solids[3]=(RikituRect){1980.0f,466.0f,240.0f,18.0f};
  s->fragments[0]=(RikituPoint){610.0f,506.0f};
  s->fragments[1]=(RikituPoint){1460.0f,506.0f};
  s->fragments[2]=(RikituPoint){2540.0f,506.0f};
  for(i=0;i<RIKITU_DEMO_COINS;i++){
    s->coins[i].x=210.0f+130.0f*(float)i;
    s->coins[i].y=510.0f;
  }
  s->belts[0]=(RikituConveyor){815.0f,180.0f,85.0f};
  s->belts[1]=(RikituConveyor){1610.0f,160.0f,-65.0f};
  s->launchers[0]=(RikituLauncher){RIKITU_LAUNCH_STEAM,1050.0f,540.0f};
  s->launchers[1]=(RikituLauncher){RIKITU_LAUNCH_CLOCK,1850.0f,540.0f};
}
