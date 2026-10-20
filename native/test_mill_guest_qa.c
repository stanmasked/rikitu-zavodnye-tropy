#include "rikitu_mill_guest_qa.h"
#include <assert.h>
#include <stdio.h>
int main(void){
  const unsigned mask=rikitu_guest_mill_checks();
  printf("MAP02 guest suite 0x%02x of 0x%02x\n",mask,RIKITU_MILL_QA_ALL);
  assert(mask==RIKITU_MILL_QA_ALL);
  assert(rikitu_guest_mill_checks()==RIKITU_MILL_QA_ALL);
  puts("PASS MAP02 same C99 guest suite on host GCC/Clang");
  return 0;
}
