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

## MAP02 real guest mini-boss audit

The diagnostic combat NRO (`make -C switch_graphical QA_MODE=1`)
now executes the portable MAP01 and MAP02 game rules as **eight**
separate checks inside NeXium's emulated Nintendo Switch environment.
Two rows of four green/red squares appear in its framebuffer.

MAP02 guest checks: source-parity map and boss spawn; first 60-Hz stomp
and 0.85s cooldown; sideways collision and 1.6s player immunity;
the third successful stomp unlocking the gate after three fragments.
The same C99 function is tested natively under GCC and Clang.

This is a guest-side gameplay-state test. It does not yet certify an
unassisted start-to-finish playthrough of the second level or rendering
of the approved original mascot/boss sprites.

## MAP03 native route — Mushroom Grove

The graphics NRO now cycles its technical routes with **Y**:
Flower Trail → Mill by the Creek → Mushroom Grove → Flower Trail.
X restarts the selected route. Only MAP01 has SDMC persistence; MAP02
and MAP03 never overwrite or delete the MAP01 saves.

MAP03 geometry is copied exactly from the browser game.js:
8270 world units, five ground sections, 13 extra platforms, 90 coins,
three fragments, three checkpoints, six patrol enemies and gate x=8140.
Three optional spring mushrooms are at x=940, 2670 and 5660.
They use the existing portable C99 launcher engine with the browser's
-730 upward impulse when grounded and touching the mushroom top.
The temporary mushroom indicators are simple geometric placeholders
and are **not** approved or newly designed artwork.

The source-parity test checks every coordinate, enemy patrol and mushroom
launcher against game.js. GCC/Clang also test each mushroom on a full
fixed-timestep update and verify a boss-free exit after the third fragment.
Switch NRO compilation, existing MAP01/MAP02 guest audits, and browser
regression must all pass before merging the map.

This is not an end-to-end playthrough. MAP03 sound, story screens, original
character sprites and persistent MAP03 saves are not implemented.

## MAP03 NeXium guest-side mushroom spring audit

The standalone QA-only NRO now carries **three rows of four** independent
indicators (twelve total), for MAP01, MAP02 and MAP03. It uses the real
C99 game simulation in each scenario, and checks the results by observing
the NeXium guest framebuffer in two independent captured PNG frames.

The new MAP03 row validates map/source shape and all three launcher
coordinates; a real 60-Hz physics tick on the first mushroom at x=940;
independent launch ticks for the mushrooms at x=2670 and x=5660; and the
fact that airborne players cannot activate mushrooms, while the exit
becomes available after all three fragments with no boss to defeat.
All three spring impulses are -730 and retain the player's health.

The same native guest suite also compiles and runs under strict GCC/Clang.
Its passing status is evidence of execution **inside NeXium** but not a
player-controlled start-to-finish completion of MAP03. Original approved
sprites, sounds and persistent saves for MAP03 remain future work.

## MAP04 native route — Bridge of Gears (experimental)

The fourth original browser map is now an asset-free Switch scene
selected by pressing Y after MAP03: Flower Trail → Mill Creek →
Mushroom Grove → Bridge of Gears → Flower Trail. Button X resets the
currently selected route; only MAP01 stores SDMC saves.

The MAP04 C99 copy uses 8880 game units, five separated ground spans,
13 platforms, 90 coins, three collectible fragments, three checkpoints,
six patrolling enemies, three lever switches at x=2260,4930,6990,
and guardian boss x=8290 with three stomp-hit HP. The exit is x=8740.

Levers follow the *actual browser implementation*: automatic activation
when horizontally overlapping [x,x+34] while the player's bottom is
within 75 pixels of ground level 456. They activate only once and use
simple green/grey geometric indicators. They do NOT gate completion:
three collected fragments AND defeating the guardian are mandatory.
The existing portable three-hit boss mechanic is reused.

The source-parity test verifies every piece of geometry against the
current game.js. Native C99 tests check all three levers, repeat-touch
idempotence, bonus visual interaction vs gate logic and guardian hits.
The route is not a finished asset-integrated release or a confirmed
controller-only playthrough; neither MAP04 saves nor sound/approved
sprites are implemented yet.

## MAP04 bridge levers and Guardian: NeXium guest audit

An extra fourth row in the separate QA NRO checks actual C99 gameplay
inside NeXium. Sixteen squares now cover the four currently ported
maps (four checks per map); the new four cover Bridge of Gears map
parameters, the touch/once-only operation of three levers, the first
Guardian stomp and temporary hit immunity, and victory after collecting
three fragments and defeating the boss even when zero levers are on.

Two real NeXium framebuffer captures must show all 16 green squares;
the same MAP04 checks run with GCC and Clang. This is guest-side state
evidence, not a full normal-controller level playthrough and not a
substitute for final approved character/boss sprites.

## MAP05 — Workshop Street (Stage 39, source-first port)

The fifth real browser level is now selectable in the *normal* graphical
Switch NRO via Y: MAP01 → MAP02 → MAP03 → MAP04 → MAP05 → MAP01. X
restarts MAP05 without altering MAP01's dual-slot save files. MAP05 has
9140 world units, five ground spans and 13 platforms, three fragments,
90 coins, six patrols, checkpoints at 2200/4760/6920, and an exit at 8950.
There is no boss on this stage; three fragments unlock the exit.

Conveyors use the exact browser positions, widths, order and speeds:
x=1070 (width 300, +48 units/s), x=3890 (width 300, -42), and x=6960
(width 300, +52). The existing 60-Hz grounded-only surface code moves
players on these belts after the collision step and before pickups.
Airborne players get no belt movement. Simple debug arrows at y=430
indicate conveyor direction; they are not approved environment art.

A new JS source-parity test checks all source-generated coordinates and
coins; strict GCC and Clang tests cover both movement directions, three
speeds, player input composition, jumping immunity to belts, checkpoint
activation, bounds checks and fragment-gated completion. Full ordinary
controller playthrough, authentic sprites/audio and MAP05 hardware-save
validation remain *unverified* and are outside this Stage 39 change.
