#ifndef RIKITU_DELIVERY_GUEST_QA_H
#define RIKITU_DELIVERY_GUEST_QA_H
/* Independent, real game-engine checks for MAP06 in a guest-only Switch NRO. */
enum {
  RIKITU_DELIVERY_QA_DATA=1,
  RIKITU_DELIVERY_QA_BELTS=2,
  RIKITU_DELIVERY_QA_VENTS=4,
  RIKITU_DELIVERY_QA_BOSS_GATE=8,
  RIKITU_DELIVERY_QA_ALL=15
};
unsigned rikitu_guest_delivery_checks(void);
#endif
