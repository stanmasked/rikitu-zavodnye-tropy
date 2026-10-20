#ifndef RIKITU_LOOP_H
#define RIKITU_LOOP_H
/* Fixed-step, platform-independent subset of the browser player loop (C99).
 * Rendering, audio, save files, enemies, and Switch HID are deliberately absent.
 */
#include <stddef.h>
#include "rikitu_rules.h"
#ifdef __cplusplus
extern "C" {
#endif
enum { RIKITU_INPUT_LEFT = 1, RIKITU_INPUT_RIGHT = 2,
       RIKITU_INPUT_JUMP = 4, RIKITU_INPUT_RUN = 8 };
#define RIKITU_FIXED_DT (1.0f / 60.0f)
typedef struct RikituPlayer {
  float x, y, vx, vy, w, h;
  float coyote_seconds, jump_buffer_seconds;
  int grounded, previous_jump, facing;
} RikituPlayer;
typedef struct RikituLoop {
  RikituPlayer player;
  float accumulator_seconds, active_seconds;
  int paused;
} RikituLoop;
void rikitu_player_init(RikituPlayer *player, float x, float y);
/* One 60Hz step of horizontal motion, jump/coyote buffer, gravity and floors.
 * World rectangles use the same top-left positive-down convention as JS.
 * Returns 0 for invalid arguments, 1 on success. */
int rikitu_player_step(RikituPlayer *player, unsigned input, float dt,
                       float world_width, const RikituRect *solids,
                       size_t solid_count);
/* Clamp incoming wall delta to 100ms, simulate at most six steps per call.
 * Returns ticks executed, 0 while paused, or -1 for invalid arguments. */
int rikitu_loop_advance(RikituLoop *loop, float frame_seconds,
                        unsigned input, float world_width,
                        const RikituRect *solids, size_t solid_count);
#ifdef __cplusplus
}
#endif
#endif
