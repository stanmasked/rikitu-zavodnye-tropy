#include "rikitu_workshop_guest_qa.h"
#include <assert.h>
#include <stdio.h>
int main(void){
  const unsigned bits=rikitu_guest_workshop_checks();
  printf("MAP05 guest cases: 0x%02x / 0x%02x\n",bits,RIKITU_WORKSHOP_QA_ALL);
  assert(bits==RIKITU_WORKSHOP_QA_ALL);
  assert(rikitu_guest_workshop_checks()==RIKITU_WORKSHOP_QA_ALL);
  puts("PASS MAP05 guest-side C99 conveyor parity on host");
  return 0;
}
