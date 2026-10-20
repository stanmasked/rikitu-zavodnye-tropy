#ifndef RIKITU_SAVE_H
#define RIKITU_SAVE_H
/* RIKITU MAP01 checkpoint file v1. Portable C99, no console SDK required.
 * Each of two alternating 44-byte slots is independently CRC-checked.
 * Browser localStorage JSON is NOT compatible with this native binary format.
 */
#include <stddef.h>
#include <stdint.h>
#include "rikitu_flower.h"
enum { RIKITU_SAVE_BYTES = 44 };
/* Encode a complete and self-checking single-level checkpoint snapshot.
 * generation starts at 1 and increases for each successful save.
 */
int rikitu_save_encode(uint8_t out[RIKITU_SAVE_BYTES],
                       const RikituScene *scene,const RikituLoop *loop,
                       uint32_t generation);
/* Read the snapshot without applying changes on corrupt/invalid input.
 * Restores at the checkpoint (not mid-jump), with one second of protection.
 */
int rikitu_save_decode(RikituScene *scene,RikituLoop *loop,
                       const uint8_t *bytes,size_t len,
                       uint32_t *generation);
/* file pair is intentionally provided by caller; host tests use temp files
 * and Switch uses two files on sdmc:. Success=1, absent/invalid/error=0.
 * Store updates ONLY the older slot, keeping the newer slot as fallback. */
int rikitu_save_load_pair(const char *a,const char *b,
                          RikituScene *scene,RikituLoop *loop);
int rikitu_save_store_pair(const char *a,const char *b,
                           const RikituScene *scene,const RikituLoop *loop);
#endif
