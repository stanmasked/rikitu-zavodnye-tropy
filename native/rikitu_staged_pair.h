#ifndef RIKITU_STAGED_PAIR_H
#define RIKITU_STAGED_PAIR_H
#include <stddef.h>
#include <stdint.h>
/* Pair transaction API used by the production RKS1/RKC2/RKL3 writers.
 * Linux uses a stable advisory pair lock plus canonical-path/inode guards.
 * Switch uses a libnx mutex to serialize callers within this process and
 * accepts only save paths under sdmc:/switch/RIKITU/. That mutex is not a
 * cross-process lock. Neither backend claims physical SDMC power-loss
 * durability; unverified targets fail closed.
 *
 * Callbacks must be pure validations/encodings of exactly 'size' bytes.
 * User can reject foreign RKL3 campaign identity in validate().
 * The next generation is passed to encode() with the pair lock held.
 * Returns success only when a staged record was fully committed.
 */
typedef int (*RikituStagedValidate)(
  const uint8_t *record,size_t size,void *context,uint32_t *generation);
typedef int (*RikituStagedEncode)(
  uint8_t *record,size_t size,uint32_t generation,void *context);
/* Legacy permissive API. It serializes writes but retains the historical
 * tie-break when valid slots have an ambiguous generation relationship.
 * Production writers should use _checked, or _if_current when they track the
 * generation they loaded.
 */
int rikitu_staged_pair_commit(
  const char *slot_a,const char *slot_b,size_t size,
  RikituStagedValidate validate,RikituStagedEncode encode,
  void *context,uint32_t *committed_generation);

/* Production-safe variant: refuse to choose between two valid slots when
 * they have the same generation but different bytes, or generations exactly
 * 2^31 apart. This does not detect stale caller snapshots; use _if_current
 * when the caller knows the generation from which its state was loaded.
 */
int rikitu_staged_pair_commit_checked(
  const char *slot_a,const char *slot_b,size_t size,
  RikituStagedValidate validate,RikituStagedEncode encode,
  void *context,uint32_t *committed_generation);

/* Optimistic stale-snapshot guard. expected_generation must match the
 * newest codec-VALID generation
 * found INSIDE the exclusive pair lock. Use 0 when both slots are
 * missing/invalid. If a newer save exists, FAIL WITHOUT WRITING any
 * slot, even if the supplied snapshot would otherwise encode cleanly.
 * Ambiguous pairs (same generation with different valid bytes, or
 * generations separated by exactly 2^31) fail closed without writes.
 * Identical duplicate records at the same generation are permitted.
 * Caller must reload/merge before retrying; do not blindly advance
 * expected_generation. Returns committed generation only on success.
 */
int rikitu_staged_pair_commit_if_current(
  const char *slot_a,const char *slot_b,size_t size,
  RikituStagedValidate validate,RikituStagedEncode encode,
  void *context,uint32_t expected_generation,
  uint32_t *committed_generation);
#endif
