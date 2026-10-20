#include "rikitu_combat_guest_qa.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  unsigned mask=rikitu_guest_combat_checks();
  printf("Guest combat checks: 0x%02X / 0x%02X\n",mask,RIKITU_QA_ALL);
  assert(mask==RIKITU_QA_ALL);
  assert(rikitu_guest_combat_checks()==RIKITU_QA_ALL);
  puts("PASS native NeXium guest QA code also executes on GCC/Clang");
  return 0;
}
