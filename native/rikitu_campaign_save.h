#ifndef RIKITU_CAMPAIGN_SAVE_H
#define RIKITU_CAMPAIGN_SAVE_H
/* Separate RKC2 campaign progress. Never read or overwrite legacy MAP01 RKS1.
 * Both slots are 32-byte CRC-checked records with wrap-safe generations.
 * In-progress scene pickups are NOT snapshotted. On reload start at the
 * beginning of the current chapter with earned wallet and upgrades intact.
 */
#include <stddef.h>
#include <stdint.h>
#include "rikitu_campaign.h"
enum { RIKITU_CAMPAIGN_SAVE_BYTES=32 };
int rikitu_campaign_save_encode(
    uint8_t out[RIKITU_CAMPAIGN_SAVE_BYTES],
    const RikituCampaign *campaign,uint32_t generation);
int rikitu_campaign_save_decode(
    RikituCampaign *campaign,const uint8_t *bytes,size_t size,
    uint32_t *generation);
/* The two paths must be distinct and must NOT point to map01-*.rks files.
 * Returns 1 on success, 0 for absent/invalid/file I/O failures.
 * Load never alters the caller's campaign on failure. */
int rikitu_campaign_save_load_pair(
    const char *a,const char *b,RikituCampaign *campaign);
int rikitu_campaign_save_store_pair(
    const char *a,const char *b,const RikituCampaign *campaign);
#endif
