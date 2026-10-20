#ifndef RIKITU_GROVE_GUEST_QA_H
#define RIKITU_GROVE_GUEST_QA_H
/* Real C99 MAP03 checks compiled into an isolated NeXium guest NRO.
 * Bit 0 map/launchers; bit 1 first spring; bit 2 other two springs;
 * bit 3 airborne safety and exit after 3 fragments.
 */
enum { RIKITU_GROVE_QA_DATA=1, RIKITU_GROVE_QA_FIRST=2,
       RIKITU_GROVE_QA_OTHERS=4, RIKITU_GROVE_QA_GATE=8,
       RIKITU_GROVE_QA_ALL=15 };
unsigned rikitu_guest_grove_checks(void);
#endif
