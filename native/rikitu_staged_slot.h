#ifndef RIKITU_STAGED_SLOT_H
#define RIKITU_STAGED_SLOT_H
#include <stddef.h>
#include <stdint.h>
/* Same-directory staging primitive used by the save-pair transaction API.
 * Never unlink/truncate the target before a complete verified sibling
 * staging file is ready. Return 1 only when rename() reports success.
 * Successful rename/readback is not a physical SDMC power-loss guarantee.
 * Linux additionally guards pending/target paths; Switch runs within the
 * pair API's process mutex and relies on the SDMC filesystem contract.
 */
int rikitu_staged_slot_commit(const char *target,const uint8_t *record,size_t n);
#if defined(__linux__)
/* Internal Linux-host guard: require an absolute path whose parent is
 * already resolved (no . / .. / duplicate slash / symlink-parent alias). */
int rikitu_staged_slot_path_is_canonical(const char *path);
#endif
#endif
