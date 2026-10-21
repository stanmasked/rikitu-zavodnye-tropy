#ifndef RIKITU_LEVEL_SAVE_H
#define RIKITU_LEVEL_SAVE_H
/* RKL3 fixed 64-byte in-level checkpoint, separate from RKC2 and RKS1.
 * Caller initializes the correct stage BEFORE decode; this module then
 * restores pickups/checkpoint/combat state at a safe respawn point.
 * No filesystem or Switch platform dependencies in this module.
 */
#include <stddef.h>
#include <stdint.h>
#include "rikitu_draw.h"
enum { RIKITU_LEVEL_SAVE_BYTES=64 };
int rikitu_level_save_encode(uint8_t out[RIKITU_LEVEL_SAVE_BYTES],
                             const RikituScene *scene,
                             const RikituLoop *loop,
                             const RikituCampaign *campaign,
                             uint32_t generation);
/* Validate a record's self-contained CRC/generation for alternating slots.
 * Full scene/campaign validation is still required before actual restore. */
int rikitu_level_save_inspect(const uint8_t *bytes,size_t size,
                              uint32_t *generation);
int rikitu_level_save_decode(RikituScene *scene,RikituLoop *loop,
                             const RikituCampaign *campaign,
                             const uint8_t *bytes,size_t size,
                             uint32_t *generation);
/* Alternating SD/host file pair, .rkl only. Never opens legacy .rks/.rkc. */
int rikitu_level_save_load_pair(const char *a,const char *b,
                                 RikituScene *scene,RikituLoop *loop,
                                 const RikituCampaign *campaign);
int rikitu_level_save_store_pair(const char *a,const char *b,
                                  const RikituScene *scene,
                                  const RikituLoop *loop,
                                  const RikituCampaign *campaign);
#endif
