#ifndef RIKITU_DRAW_H
#define RIKITU_DRAW_H
/* Portable, asset-free RGBA8888 2D debug renderer (not approved character art).
 * Pixel format: little-endian u32 0xAABBGGRR as used by libnx RGBA8_MAXALPHA.
 */
#include <stddef.h>
#include <stdint.h>
#include "rikitu_loop.h"
#include "rikitu_stage.h"
#include "rikitu_surfaces.h"

enum { RIKITU_SCENE_WIDTH = 1280, RIKITU_SCENE_HEIGHT = 720,
       RIKITU_DEMO_COINS = 18, RIKITU_DEMO_SOLIDS = 4,
       RIKITU_DEMO_BELTS = 2, RIKITU_DEMO_LAUNCHERS = 2 };

typedef struct RikituScene {
  RikituPlayer player;
  RikituStage stage;
  RikituPoint fragments[RIKITU_STAGE_FRAGMENTS];
  RikituPoint coins[RIKITU_DEMO_COINS];
  RikituRect solids[RIKITU_DEMO_SOLIDS];
  RikituConveyor belts[RIKITU_DEMO_BELTS];
  RikituLauncher launchers[RIKITU_DEMO_LAUNCHERS];
  float world_width, gate_x;
  int paused;
} RikituScene;

/* Draw the scene to a user-owned framebuffer with stride in 32-bit pixels.
 * Returns 1 for success, 0 for invalid arguments. No memory allocations. */
int rikitu_draw_scene(uint32_t *pixels, int width, int height,
                      int stride_pixels, const RikituScene *scene);
#endif
