#ifndef RIKITU_WORKSHOP_GUEST_QA_H
#define RIKITU_WORKSHOP_GUEST_QA_H
/* Four independent MAP05 cases run with the real engine on host and Switch. */
enum {
  RIKITU_WORKSHOP_QA_DATA=1,
  RIKITU_WORKSHOP_QA_FORWARD=2,
  RIKITU_WORKSHOP_QA_REVERSE=4,
  RIKITU_WORKSHOP_QA_AIRBORNE_GATE=8,
  RIKITU_WORKSHOP_QA_ALL=15
};
unsigned rikitu_guest_workshop_checks(void);
#endif
