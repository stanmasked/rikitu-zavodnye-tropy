# RIKITU — experimental 2D framebuffer scene

This is the first graphical Nintendo Switch NRO experiment. It is not the
finished game and does not contain approved RIKITU mascot/character art.

## Screen
- Direct 1280x720 RGBA framebuffer rendered through libnx Framebuffer.
- Neutral cyan player hitbox placeholder; no re-creation of approved fox art.
- Deterministic 3000-pixel test route: sky, platforms, two conveyors,
  two launchers, three fragments, eighteen collectible coins and an exit.
- Scrolling camera, status HUD, pause overlay and finish overlay.
- Uses existing portable C99 movement, surface interactions and stage rules.

## Controls
D-pad / left stick: movement. A/B: jump. R/ZR: run.
Minus: pause. X: restart the synthetic route. Plus: exit.

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
