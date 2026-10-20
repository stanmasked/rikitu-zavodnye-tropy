# RIKITU — experimental 2D framebuffer scene

This is the first graphical Nintendo Switch NRO experiment. It is not the
finished game and does not contain approved RIKITU mascot/character art.

## Screen
- Direct 1280x720 RGBA framebuffer rendered through libnx Framebuffer.
- Neutral cyan player hitbox placeholder; no re-creation of approved fox art.
- First real browser MAP01 geometry: Flower Trail (7900 game units),
  five ground spans, fourteen platforms, three fragments, three checkpoints,
  90 coins and exit at 7770. The 3000-unit synthetic demo is still used
  by isolated renderer and timing regression tests.
- Scrolling camera, status HUD, pause overlay and finish overlay.
- Uses existing portable C99 movement, surface interactions and stage rules.

## Controls
D-pad / left stick: movement. A/B: jump. R/ZR: run.
Minus: pause. X: restart Flower Trail. Plus: exit.

## Verification
- Rasterizer: native/rikitu_draw.c
- Synthetic environment: native/rikitu_demo.c
- Host tests: native/test_draw.c (GCC and Clang, strict C99).
- GitHub Actions creates a PPM host-render preview and a graphical NRO;
  the NRO must pass its NRO0 binary integrity check.

The preview is produced by host-side source code, NOT the Switch emulator.
This smoke demo is not an optimized 60-FPS game engine. It still uses nominal
frame timing, not measured delta, and does not render the real 16 maps,
approved character art, full animations, music, enemies or persistent saves.
Controller behavior and graphics require emulator/device validation.

Build using devkitPro devkitA64/libnx: make -C switch_graphical
The Makefile derives from the official switchbrew application template.
No game ROMs or user-owned originals are published.

## Measured frame-time integration

The libnx graphical loop uses armGetSystemTick() / armTicksToNs() to
measure elapsed time. A portable routine rikitu_demo_advance() accumulates
the delta and advances gameplay at fixed 60 Hz, capped at 100 ms or six
ticks to prevent excessive catch-up after a stall.

Each simulation tick now applies movement, landing, conveyor velocity,
steam/clock launch, item pickups and the exit condition in browser order.
Pause and stage completion do not accumulate backlog. A short 1 ms sleep
only yields CPU time; it does not define gameplay speed.

GCC/Clang regression checks compare movement at simulated 30/60/120
render frames per second, stalled frames, jumps, pause, coin collection,
belt motion and one-shot completion. This is simulation-time stability,
not yet proof of stable 60 FPS in a physical Switch or NeXium.

## Browser MAP01: Flower Trail source parity

The running graphical NRO now loads the first browser campaign map from
native/rikitu_flower_data.h. Every solid, platform, fragment, checkpoint,
coin and the gate/world width are checked directly against game.js by
node tests/test-flower-source.cjs. C99 GCC/Clang integration tests also
exercise checkpoint crossing, real gaps, coin coordinates, and the gate.

The currently ported terrain and collectibles DO NOT yet implement the
original six moving enemies, health/damage, enemy collisions, hazards,
respawn/checkpoint persistence, decorative sprites, music or real character
art. The first native level is therefore not yet a fully playable or
finishable reproduction of its browser counterpart. The original browser
level remains the source of truth and is not modified.

## Flower Trail patrols, combat and checkpoint respawn

The six MAP01 patrol spawn positions and their bounds match game.js.
Every 60 Hz tick advances alive enemies by 43 units/s, reversing at the
original patrol endpoints. Falling onto a patrol kills it and bounces the
player at -360; sideways contact costs one heart with 1.6 seconds of
invulnerability. Falling below the browser's world height+150 threshold
returns to the latest checkpoint, with 1.4 seconds of invulnerability.
Coins, fragments, cleared patrols and checkpoint progress are retained.
After all hearts are lost, hearts refill and the player respawns rather
than reaching a game-over screen, matching the browser's accessible rules.
A neutral orange rectangle represents each enemy until approved original
sprites can be integrated without redesign.

GCC/Clang tests cover patrol bounds, stomps, immunity, checkpoint respawn,
full heart reset and pause. A separate JS source-parity test checks all six
enemy coordinates and the matching damage/bounce constants.

Remaining: approved sprites, audio, full boss mechanics, actual complete
campaign maps, persistent saves/loads and end-to-end manual playtesting.
