#include "rikitu_grove_guest_qa.h"
#include <assert.h>
#include <stdio.h>
int main(void){
  const unsigned bits=rikitu_guest_grove_checks();
  printf("MAP03 mushroom guest suite 0x%02x/0x%02x\n",bits,RIKITU_GROVE_QA_ALL);
  assert(bits==RIKITU_GROVE_QA_ALL);
  assert(rikitu_guest_grove_checks()==RIKITU_GROVE_QA_ALL);
  puts("PASS MAP03 all four guest check scenarios on host");
  return 0;
}
