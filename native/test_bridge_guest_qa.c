#include "rikitu_bridge_guest_qa.h"
#include <stdio.h>
#include <assert.h>
int main(void){
  const unsigned b=rikitu_guest_bridge_checks();
  printf("MAP04 guest audit 0x%02x / 0x%02x\n",b,RIKITU_BRIDGE_QA_ALL);
  assert(b==RIKITU_BRIDGE_QA_ALL);
  assert(rikitu_guest_bridge_checks()==RIKITU_BRIDGE_QA_ALL);
  puts("PASS MAP04 guest-code parity on host");
  return 0;
}
