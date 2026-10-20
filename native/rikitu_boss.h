#ifndef RIKITU_BOSS_H
#define RIKITU_BOSS_H
/* Portable 60-Hz boss logic copied from game.js, without any sprite art. */
#include "rikitu_loop.h"
typedef struct RikituBoss {
  float x,y,w,h,center,hit_seconds;
  int hp,dir,alive;
} RikituBoss;
typedef enum RikituBossEvent {
  RIKITU_BOSS_INVALID=-1,RIKITU_BOSS_NONE=0,
  RIKITU_BOSS_HURT=1,RIKITU_BOSS_DEFEATED=2,
  RIKITU_BOSS_CONTACT=3
} RikituBossEvent;
/* Browser rules: moves 48/s between center +/-80, 3 stomp hits,
 * 0.85s recovery, player bounces with vy=-490 and is placed above boss.
 * Caller applies heart damage only for CONTACT.
 */
RikituBossEvent rikitu_boss_tick(RikituBoss *boss,RikituPlayer *player,
                                  float previous_bottom,float dt);
#endif
