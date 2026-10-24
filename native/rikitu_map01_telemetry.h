#ifndef RIKITU_MAP01_TELEMETRY_H
#define RIKITU_MAP01_TELEMETRY_H
/* Stage159: volatile, observational MAP01 progress counters.
 * Never writes files, sends data, adjusts gameplay, or changes save schemas.
 * Synthetic/teleported tests must not be presented as real playtime evidence.
 */
#include "rikitu_demo.h"
typedef struct RikituMap01Telemetry {
  int initialized;
  unsigned observed_frames, observed_ticks, paused_frames;
  unsigned new_fragments, new_coins, new_checkpoints, new_damage, completions;
  float checkpoint_active_seconds[RIKITU_MAX_CHECKPOINTS];
  float last_active_seconds;
  int last_fragments, last_coins, last_checkpoints, last_damage, last_completed;
} RikituMap01Telemetry;
/* Starts from the already restored scene: preexisting progress is baseline,
 * not re-counted as a new event. Returns 1 only for a valid MAP01 scene. */
int rikitu_map01_telemetry_start(RikituMap01Telemetry *m,
                                 const RikituScene *scene,
                                 const RikituLoop *loop);
/* Call once after each native demo_advance on MAP01. A rejected observation
 * leaves the counters untouched; the caller may disable diagnostics without
 * affecting gameplay. Multiple events in one frame share a sample timestamp.
 * 0 <= ticks <= 6; "paused" requires 0 ticks. */
int rikitu_map01_telemetry_observe(RikituMap01Telemetry *m,
                                   const RikituScene *scene,
                                   const RikituLoop *loop, int ticks);
#endif
