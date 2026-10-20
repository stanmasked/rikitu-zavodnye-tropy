#ifndef RIKITU_SURFACES_H
#define RIKITU_SURFACES_H
/* Portable environmental movement after collision resolution; no platform SDK. */
#include <stddef.h>
#include "rikitu_loop.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct RikituConveyor {
  float x, width, speed;
} RikituConveyor;
typedef enum RikituLauncherKind {
  RIKITU_LAUNCH_NONE = 0,
  RIKITU_LAUNCH_STEAM = 1,
  RIKITU_LAUNCH_CLOCK = 2,
  RIKITU_LAUNCH_GUST = 3,
  RIKITU_LAUNCH_CRYSTAL = 4,
  RIKITU_LAUNCH_MUSHROOM = 5
} RikituLauncherKind;
typedef struct RikituLauncher {
  RikituLauncherKind kind;
  float x, y;
} RikituLauncher;
/* Apply all conveyor displacement first, then one launcher by JavaScript priority:
 * steam, clock, gust, crystal, mushroom. First overlapping item of a kind wins.
 * Return 0 for no launch; 1..5 for the triggered type; -1 for bad arguments.
 * The player must have already completed the floor collision step. */
int rikitu_apply_surfaces(RikituPlayer *player, float dt, float world_width,
                          const RikituConveyor *conveyors, size_t conveyor_count,
                          const RikituLauncher *launchers, size_t launcher_count);
#ifdef __cplusplus
}
#endif
#endif
