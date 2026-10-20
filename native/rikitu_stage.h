#ifndef RIKITU_STAGE_H
#define RIKITU_STAGE_H
/* Asset-free C99 collectible/exit state for one of the 16 RIKITU stages. */
#include <stddef.h>
#include "rikitu_rules.h"
#ifdef __cplusplus
extern "C" {
#endif
enum { RIKITU_STAGE_FRAGMENTS=3, RIKITU_STAGE_COINS=90 };
typedef struct RikituPoint { float x, y; } RikituPoint;
typedef struct RikituStage {
  unsigned char fragments[RIKITU_STAGE_FRAGMENTS];
  unsigned char coins[RIKITU_STAGE_COINS];
  int fragment_count, coin_count, boss_required, boss_defeated, completed;
} RikituStage;
typedef enum RikituGateResult {
  RIKITU_GATE_INVALID=-1, RIKITU_GATE_NOT_REACHED=0,
  RIKITU_GATE_BLOCKED=1, RIKITU_GATE_COMPLETED=2,
  RIKITU_GATE_ALREADY_COMPLETED=3
} RikituGateResult;
typedef enum RikituNextMode {
  RIKITU_NEXT_BETWEEN=1, RIKITU_NEXT_SHOP=2, RIKITU_NEXT_VICTORY=3
} RikituNextMode;
void rikitu_stage_init(RikituStage *state, int boss_required);
/* Returns number newly collected, or -1 for invalid input.
 * coin_total may be 0..90. Pickup rectangles match game.js geometry.
 * A second overlap cannot collect the same item again. */
int rikitu_stage_pickups(RikituStage *state, RikituRect player,
                         const RikituPoint fragments[RIKITU_STAGE_FRAGMENTS],
                         const RikituPoint *coins, size_t coin_total);
int rikitu_stage_can_finish(const RikituStage *state);
RikituGateResult rikitu_stage_touch_gate(RikituStage *state,
                                        RikituRect player, float gate_x);
/* Upon clearing a stage: shop after each even stage, between otherwise. */
RikituNextMode rikitu_stage_next_mode(int stage_number);
/* Upon leaving Moki's shop: the final stage goes to victory. */
RikituNextMode rikitu_stage_after_shop(int stage_number);
#ifdef __cplusplus
}
#endif
#endif
