#ifndef RIKITU_MILL_GUEST_QA_H
#define RIKITU_MILL_GUEST_QA_H
/* True guest C99 MAP02 checks. Four independent bits correspond to four
 * green squares in NeXium's second diagnostic row.
 */
enum { RIKITU_MILL_QA_DATA=1,RIKITU_MILL_QA_STOMP=2,
       RIKITU_MILL_QA_CONTACT=4,RIKITU_MILL_QA_GATE=8,
       RIKITU_MILL_QA_ALL=15 };
unsigned rikitu_guest_mill_checks(void);
#endif
