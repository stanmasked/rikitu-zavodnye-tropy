/* Initial libnx console/joypad smoke test, not the 16-level game. */
#include <stdio.h>
#include <string.h>
#include <switch.h>
#include "rikitu_loop.h"

static unsigned rikitu_hid_to_input(u64 held) {
  unsigned mask=0;
  if(held & (HidNpadButton_Left | HidNpadButton_StickLLeft))
    mask |= RIKITU_INPUT_LEFT;
  if(held & (HidNpadButton_Right | HidNpadButton_StickLRight))
    mask |= RIKITU_INPUT_RIGHT;
  if(held & (HidNpadButton_A | HidNpadButton_B))
    mask |= RIKITU_INPUT_JUMP;
  if(held & (HidNpadButton_R | HidNpadButton_ZR))
    mask |= RIKITU_INPUT_RUN;
  return mask;
}

int main(void) {
  PadState pad;
  RikituLoop game;
  const RikituRect floor={0.0f,456.0f,1000.0f,140.0f};
  int status=0;
  memset(&game,0,sizeof(game));
  rikitu_player_init(&game.player,105.0f,456.0f-43.0f);
  game.player.grounded=1;
  consoleInit(NULL);
  padConfigureInput(1,HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&pad);
  printf("RIKITU: ZAVODNYE TROPY\n");
  printf("Native Nintendo Switch input/physics probe\n");
  printf("D-pad / left stick = move  A/B = jump  R/ZR = run\n");
  printf("Minus = pause  X = reset  Plus = exit\n");
  printf("NOT a game build: graphics, maps and sound pending.\n");
  while(appletMainLoop()) {
    unsigned input;
    int n;
    u64 held,pressed;
    padUpdate(&pad);
    held=padGetButtons(&pad);
    pressed=padGetButtonsDown(&pad);
    if(pressed & HidNpadButton_Plus) break;
    if(pressed & HidNpadButton_Minus) game.paused=!game.paused;
    if(pressed & HidNpadButton_X) {
      memset(&game,0,sizeof(game));
      rikitu_player_init(&game.player,105.0f,456.0f-43.0f);
      game.player.grounded=1;
    }
    input=rikitu_hid_to_input(held);
    /* Nominal 60-Hz smoke step, not a display-synced renderer. */
    n=rikitu_loop_advance(&game,RIKITU_FIXED_DT,input,1000.0f,&floor,1);
    if(n<0) {status=1;break;}
    printf("\x1b[8;1H");
    printf("Input: 0x%02x   Ticks: %d      \n",input,n);
    printf("X=%8.2f Y=%8.2f          \n",game.player.x,game.player.y);
    printf("VX=%7.2f VY=%7.2f        \n",game.player.vx,game.player.vy);
    printf("Grounded: %d   Paused: %d      \n",
           game.player.grounded,game.paused);
    printf("Game seconds: %8.2f     \n",game.active_seconds);
    consoleUpdate(NULL);
    svcSleepThread(16666667LL);
  }
  consoleExit(NULL);
  return status;
}
