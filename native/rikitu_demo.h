#ifndef RIKITU_DEMO_H
#define RIKITU_DEMO_H
#include "rikitu_draw.h"
/* Deterministic, synthetic level for graphics/input testing only.
 * NOT a copy of any approved RIKITU world or mascot artwork.
 */
void rikitu_demo_init(RikituScene *scene);
/* Advance measured wall-clock delta in <=6 fixed 60-Hz ticks.
 * Each tick handles physics, conveyor/launcher interactions, pickups and exit
 * in browser order. Paused/completed states do not accumulate simulation time.
 * Returns tick count, or -1 on invalid input. */
int rikitu_demo_advance(RikituScene *scene, RikituLoop *loop,
                        float wall_seconds, unsigned input);
#endif
