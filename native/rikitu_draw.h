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
#include "rikitu_enemies.h"
#include "rikitu_boss.h"
#include "rikitu_campaign.h"

enum { RIKITU_SCENE_WIDTH = 1280, RIKITU_SCENE_HEIGHT = 720,
       RIKITU_DEMO_COINS = 18, RIKITU_DEMO_SOLIDS = 4,
       RIKITU_DEMO_BELTS = 2, RIKITU_DEMO_LAUNCHERS = 2,
       RIKITU_MAX_SOLIDS = 32, RIKITU_MAX_COINS = 90,
       RIKITU_MAX_BELTS = 4, RIKITU_MAX_LAUNCHERS = 5,
       RIKITU_MAX_CHECKPOINTS = 3, RIKITU_MAX_ENEMIES = 12,
       RIKITU_MAX_LEVERS = 3 };

typedef struct RikituLever {
  float x,y;
  int on;
} RikituLever;

typedef struct RikituScene {
  RikituPlayer player;
  RikituStage stage;
  RikituPoint fragments[RIKITU_STAGE_FRAGMENTS];
  RikituPoint coins[RIKITU_MAX_COINS];
  RikituRect solids[RIKITU_MAX_SOLIDS];
  RikituConveyor belts[RIKITU_MAX_BELTS];
  RikituLauncher launchers[RIKITU_MAX_LAUNCHERS];
  float checkpoints[RIKITU_MAX_CHECKPOINTS];
  RikituEnemy enemies[RIKITU_MAX_ENEMIES];
  RikituBoss boss;
  RikituLever levers[RIKITU_MAX_LEVERS];
  float world_width, gate_x, active_checkpoint;
  size_t solid_count, coin_total, belt_count, launcher_count;
  size_t checkpoint_total, enemy_count, lever_count;
  int levers_on;
  int hearts, max_hearts, damage_count;
  float invulnerability_seconds;
  int reached_checkpoints, level_id, paused;
} RikituScene;

/* Draw the scene to a user-owned framebuffer with stride in 32-bit pixels.
 * Returns 1 for success, 0 for invalid arguments. No memory allocations. */
int rikitu_draw_scene(uint32_t *pixels, int width, int height,
                      int stride_pixels, const RikituScene *scene);
/* Geometry-only chapter/shop/victory overlay, using existing debug font. */
int rikitu_draw_campaign_overlay(uint32_t *pixels,int width,int height,
                                 int stride_pixels,const RikituCampaign *c,
                                 const RikituHeartOffer *offer);
#endif
