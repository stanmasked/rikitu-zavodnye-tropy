#ifndef RIKITU_ENEMIES_H
#define RIKITU_ENEMIES_H
/* Asset-free browser-matching patrol and collision rules. C99, no libnx. */
#include "rikitu_loop.h"
typedef struct RikituEnemy {
  float x, y, w, h, a, b;
  int dir, dead;
} RikituEnemy;
typedef enum RikituEnemyEvent {
  RIKITU_ENEMY_INVALID=-1, RIKITU_ENEMY_NONE=0,
  RIKITU_ENEMY_STOMP=1, RIKITU_ENEMY_CONTACT=2
} RikituEnemyEvent;
/* Move the patrol by exactly 43*dt and resolve overlap with the player.
 * previous_bottom must be measured BEFORE the player's physics step.
 * On a valid stomp: enemy dies and player bounces at vy=-360.
 * On contact: caller handles hearts and invulnerability.
 */
RikituEnemyEvent rikitu_enemy_tick(RikituEnemy *enemy,RikituPlayer *player,
                                   float previous_bottom,float dt);
#endif
