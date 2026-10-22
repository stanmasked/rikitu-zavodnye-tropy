#ifndef RIKITU_STAGED_PAIR_H
#define RIKITU_STAGED_PAIR_H
#include <stddef.h>
#include <stdint.h>
/* Experimental Linux-host API. Serializes generation selection AND commit
 * for both slots, using one stable advisory pair lock. Does not replace
 * production RKS1/RKC2/RKL3 writers or run in shipping Switch NRO.
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
int rikitu_staged_pair_commit(
  const char *slot_a,const char *slot_b,size_t size,
  RikituStagedValidate validate,RikituStagedEncode encode,
  void *context,uint32_t *committed_generation);
#endif
