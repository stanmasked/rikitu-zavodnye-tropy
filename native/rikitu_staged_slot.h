#ifndef RIKITU_STAGED_SLOT_H
#define RIKITU_STAGED_SLOT_H
#include <stddef.h>
#include <stdint.h>
/* Experimental same-directory staging, NOT a hardware durability promise.
 * Never unlink/truncate the target before a complete verified sibling
 * staging file is ready. Return 1 only when rename() reports success.
 * This primitive is not yet used by the shipping game save writers.
 */
int rikitu_staged_slot_commit(const char *target,const uint8_t *record,size_t n);
#if defined(__linux__)
/* Internal Linux-host guard: require an absolute path whose parent is
 * already resolved (no . / .. / duplicate slash / symlink-parent alias). */
int rikitu_staged_slot_path_is_canonical(const char *path);
#endif
#endif
