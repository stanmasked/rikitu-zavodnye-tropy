#ifndef RIKITU_COMBAT_GUEST_QA_H
#define RIKITU_COMBAT_GUEST_QA_H
/* Executes the real portable 60-Hz MAP01 gameplay with controlled positions.
 * Bit 0: patrol; bit 1: contact+invulnerability;
 * bit 2: stomp bounce; bit 3: checkpoint respawn+persisted loot.
 * The same function is linked into host tests and the NeXium QA-only NRO.
 */
enum { RIKITU_QA_PATROL=1, RIKITU_QA_CONTACT=2,
       RIKITU_QA_STOMP=4, RIKITU_QA_RESPAWN=8,
       RIKITU_QA_ALL=15 };
unsigned rikitu_guest_combat_checks(void);
#endif
