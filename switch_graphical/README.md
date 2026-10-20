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

## Real NeXium guest-side MAP01 combat audit

A **separate** executable named `rikitu_combat_qa_probe.nro` is built
with `make -C switch_graphical QA_MODE=1`. The default, regular
`rikitu_scene_probe.nro` still uses `main.c` and does not run QA.

The diagnostic executable calls `rikitu_guest_combat_checks()` using the
same portable C99 game functions the normal Switch scene runs:
patrol advancement at 43 units/sec, side-hit damage followed by 30
invulnerable frames, falling stomp bounce and enemy death, and a pit
respawn at checkpoint 2050 with collected fragments, coins and defeated
patrols preserved. These checks also run natively under GCC and Clang.

Four large rectangles at positions x=280, 480, 680, 880 on the guest
frame are green for PASS and red for FAIL, one per scenario in that order.
The actual NeXium screenshot files are examined by
`tests/check_combat_guest.py`; the guest run fails if a rectangle is not
green. This is a real guest-side execution check, **not** an invented
screenshot or an assertion that a human completed the level.

This QA executable contains no user ROMs, new enemy art, approved mascot
sprite copies or permanent save data. Normal assets and gameplay are
unchanged. The long-term work is still to validate ordinary gameplay
using input-only automation, manual playthroughs, persistence and audio.

## MAP01 native checkpoint persistence (experimental)

The Switch NRO now attempts to mount SDMC through libnx. If available,
progress is stored under `sdmc:/switch/RIKITU/` in two alternating
files, `map01-a.rks` and `map01-b.rks`. When SDMC cannot be mounted,
the game continues without persistent storage instead of crashing.

The C99 save format is `RKS1` with a version byte, strict bounds,
generation number and a CRC32 checksum. It includes the latest of the
three checkpoints, health, completed fragments, up to 90 coins, six
defeated patrols, hit count, stage completion and elapsed time. On
resume the player starts at the saved checkpoint with brief immunity.
The other slot retains the last valid generation if a write is partial
or corrupted. This is recovery-oriented file alternation, **not** a
guarantee against every possible SD corruption or sudden power failure.

The state snapshot writes after game progress changes (at most one
attempt per two seconds of on-screen time), and a final write is
attempted before exiting with Plus. X deliberately resets the level
and removes both save slots. Only Flower Trail is supported; the native
format is deliberately separate from browser localStorage and cannot
import or export the browser JSON save yet.

Host verification:
`native/test_save.c` (GCC and Clang) exercises encoding, checksum,
bounds, progress restoration, invalid-data rejection, two-slot
alternation, a corrupted newest slot and partially written slot.

Filesystem mounting and persistence *across two independent NeXium
sessions or on real hardware* still require runtime validation.

## Native MAP02 — Mill by the Creek (experimental)

The second browser route is available in the **normal graphical Switch NRO**
with the Y button. Press Y again to return to MAP01. X resets the currently
selected map. Only MAP01 is persisted to SDMC; switching or resetting MAP02
does not delete MAP01 save files.

MAP02 matches game.js: width 8550; 5 ground spans and 13 elevated platforms;
90 coins; 3 fragments; checkpoints at 2200, 4930 and 6980;
6 patrols and gate x=8370.

The level contains the game's first three-hit mini-boss (Clockwork Beetle):
native C99 boss movement at 48 units/sec between x=7830..7990,
three downward stomps, 0.85-second hit protection, -490 jump rebound.
The gate remains blocked until all three parts are collected and the
boss is defeated. Placeholder violet geometry is NOT the approved design.
No artwork was changed or replaced.

Tests/test-mill-source.cjs compares every MAP02 map coordinate and patrol
against the live game.js source. Native/test_mill.c verifies boss HP,
stomp/recovery, movement and the exit. GCC, Clang, Switch NRO build and
NeXium guest frame checks run through the existing graphical workflows.
The Y-button map switch and complete boss playthrough are not yet
independently confirmed inside NeXium; this is still a technical port.
