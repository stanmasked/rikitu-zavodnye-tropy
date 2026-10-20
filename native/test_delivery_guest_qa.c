#include "rikitu_delivery_guest_qa.h"
#include <assert.h>
#include <stdio.h>
int main(void){
  const unsigned bits=rikitu_guest_delivery_checks();
  printf("MAP06 NeXium guest QA masks: %02x of %02x\n",
         bits,RIKITU_DELIVERY_QA_ALL);
  assert(bits==RIKITU_DELIVERY_QA_ALL);
  assert(rikitu_guest_delivery_checks()==RIKITU_DELIVERY_QA_ALL);
  puts("PASS MAP06 independent guest checks under host GCC/Clang");
  return 0;
}
