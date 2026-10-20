#ifndef RIKITU_BRIDGE_GUEST_QA_H
#define RIKITU_BRIDGE_GUEST_QA_H
/* Four genuine 60-Hz MAP04 checks inside Switch guest and host. */
enum { RIKITU_BRIDGE_QA_MAP=1, RIKITU_BRIDGE_QA_LEVERS=2,
       RIKITU_BRIDGE_QA_GUARDIAN=4, RIKITU_BRIDGE_QA_GATE=8,
       RIKITU_BRIDGE_QA_ALL=15 };
unsigned rikitu_guest_bridge_checks(void);
#endif
