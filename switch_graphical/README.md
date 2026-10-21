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

## MAP05 — fifth NeXium guest-side QA row (Stage 40)

The QA-only NRO now executes four MAP05 state checks within the guest
Nintendo Switch runtime, in addition to the existing sixteen MAP01–MAP04
checks. Five rows of four green/red indicators are sampled in TWO real
NeXium screenshots, including row 5 at framebuffer y=598..653.

Checks use the exact portable engine: (1) source-accurate map shape and
three belt definitions, (2) both right-moving belts and running with a
belt, (3) the left-moving belt with neutral, left and right input, and
(4) jumping/airborne belt safety plus the fragment-only exit condition.

GCC and Clang execute precisely the same QA cases on host. This
guest-side test is stronger than an NRO build-only result but remains
a controlled state audit, not an ordinary input-only stage playthrough.
No approved mascot, antagonist, environment sprites or audio change.

## MAP06 — Underground Delivery (Stage 41)

Sixth native C99 technical level, selectable with Y after MAP05 (MAP01 →
MAP02 → MAP03 → MAP04 → MAP05 → MAP06 → MAP01). X resets MAP06. Existing
MAP01 alternating save slots remain untouched by MAP06.

All level data are reproduced from `game.js`: 9910-unit world,
five ground spans, 13 platforms, 90 coins, three fragments, checkpoints at
2300/5060/7360, six patrols and exit x=9730. Environment hazards contain
three conveyors with speeds +42/-48/+50 and three upward steam vents
at x=1350/4740/7910 (browser impulse -705). The three-hit mini-boss
at x=9210 must be defeated in addition to collecting all fragments.

Browser-source parity, GCC and Clang use the *actual* level data and
portable game rules. This is a C99 geometry/logic port; not a complete
normal-controller playthrough and not verified MAP06 persistent storage.
The graphical NRO continues to render neutral debug rectangles and
does not redraw or reinterpret approved mascot and enemy designs.

## MAP06 — NeXium guest audit of steam and three-hit Guardian (Stage 43)

The separate guest-only Switch NRO has 24 independent green/red checks
(six rows, four per map). The final MAP06 row sits at framebuffer
y=656..711, within 1280x720. The Python auditor verifies all 24
signals in two actual NeXium screenshots, and earlier rows stay
mandatory. MAP06 checks include full map/boss structure, all three belt
speeds and input composition, steam launch at three x coordinates with
-705 impulses and airborne isolation, plus the boss-gated completion
condition (all fragments AND the three-hit mini-boss defeated).

Host GCC/Clang execute the same C99 QA function. These are controlled
real engine checks *inside* the guest, not a complete ordinary-controller
playthrough. Final character art and persistent MAP06 saves remain
unverified and unchanged.

## MAP07 — Clock Square native C99 scene (Stage 45)

The seventh original browser stage is now selectable with Y after MAP06.
Its original 9470-unit world has 18 solids, 90 optional coins, six
patrols, three fragments, three checkpoints (2240/4930/7240), and gate
x=9290; there is no mini-boss on this level.

The 60-Hz native portable scene reproduces two grounded conveyors
(+40 at x=2600, -42 at x=6030) and three clock-dial spring platforms
at x=1350, 4140 and 7770. Their -675 upward impulses match the browser
implementation and are not re-triggered while airborne.
C99 GCC/Clang tests exercise the two conveyor directions, three clock
pads, in-air protection, checkpoints, and fragment-only exit. A Node.js
source audit checks all actual browser geometry, coins and patrol data.

The graphics are neutral technical indicators, not redesigns of the
approved Rikitu/other character sprites. End-to-end controller
completion, physical Switch saves for MAP07, final sound and assets
remain unverified. MAP01 save-slot isolation is preserved.

## MAP08 — Orders Depot native C99 (Stage 47)

The eighth browser stage is ported to the portable graphical scene,
selectable by Y after MAP07, restarting with X. Original geometry:
10180 units, five ground spans, thirteen platforms, 90 coins,
three fragments, checkpoints 2380/5230/7710 and six patrols.

Three grounded conveyor belts (+45/-48/+47) and three one-touch
warehouse levers (2120/5390/7560) use the existing 60-Hz engine.
The levers animate the route; the original game.js does NOT require
lever activation to unlock the exit. The three-hit patrol automaton
at x9470, together with the three fragments, gates x9990.

Strict GCC/Clang C99 integration tests and Node.js browser-source parity
are provided. This technical prototype does NOT certify controller-only
level completion, approved character art/audio integration, nor
persistent saves for MAP08. MAP01 saves stay isolated.

## MAP09 — Misty Pass native C99 (Stage 48)

The ninth original game.js stage is selectable with Y after MAP08.
Misty Pass covers 9520 world units, 18 solids, 90 coins,
three fragments, three checkpoints (2280/4880/7290),
six patrols and a fragment-only gate at x9330.

Three gust lifts, at x=1240/3880/7520, use the existing real
60Hz engine and browser's -690 vertical impulse. They do
not add horizontal drift and do not retrigger mid-flight.
Browser-to-C99 data parity and host GCC/Clang game tests are included.

This remains a neutral debug NRO without redesigned character sprites.
Full controller-only playthrough, hardware save tests and final audio/art
are not yet verified.

## MAP10 — Cloud Dock source-first C99 scene (Stage 49)

The tenth original browser level is selectable after MAP09 with Y,
reset with X. Original world size 10320, 18 solids, 90 coins,
three fragments, checkpoints 2470/5420/8020, six patrol enemies
and gate x10140. The mandatory 3-hit Wind Automaton at x9650
must be defeated in addition to finding all three fragments.

Four rising gusts at x980/4300/7120/8620 share the original
grounded-only -690 vertical impulse, with NO forced horizontal drift.
Two belts (+38 at x2860 and -38 at x6520) use the existing 60Hz
engine. MAP10 combines both surface families, constrained by
RIKITU_MAX_LAUNCHERS=4 and RIKITU_MAX_BELTS=4.

Automated Node.js source-audit and GCC/Clang fixed-step tests
cover every source coordinate, gust horizontal composition,
both belt directions, checkpoints, boss gating and invalid sizes.
These are neutral technical graphics; existing approved character
art is not redrawn. This is not a complete ordinary-input
playthrough, finished sprite/audio integration or hardware-save test.

## MAP11 — Crystal Caves source-first C99 (Stage 50)

Technical MAP11 is available by cycling Y after MAP10 (X restarts
current map). The original 9820-unit world includes 18 solids, 90 coins,
six patrols, three fragments, checkpoints 2370/5050/7470, and exit
at x9620. There is no boss or conveyor on this level.

Three grounded crystal springs (x1120,3980,7650) give browser-accurate
-690 vertical impulse without forcing horizontal drift, and may not
relaunch a player in midair. An automated Node.js source-data audit
verifies every platform, coin and patrol, and GCC/Clang fixed-step
tests verify all three springs, checkpoint progression and the
fragment-only exit.

The stage uses neutral graphical diagnostics, not remade approved
sprites; ordinary controller-only completion, finished audio and
hardware save testing remain separate unfinished tasks.

## MAP12 — Silent Mine native C99 (Stage 52)

The twelfth browser level is selectable after MAP11 using Y;
X resets it without touching the MAP01 persistent saves.
Original world width 10700, 18 solids, 90 optional coins,
six patrols, three fragments, checkpoints 2520/5490/8200
and exit x10530 gated by defeating the 3-hit Drill Guard at x9930.

Three grounded-only steam jets (x1480/5140/7580) impart
vy=-705; three conveyors (+43/-44/+46) and three touch levers
(x2590/6030/8260) match the original browser data. As in game.js,
the levers are optional and do not themselves unlock the exit.
Node.js source-parity plus GCC/Clang strict C99 integration tests
cover the springs, belts, lever state, airborne protection,
fragment-and-boss exit and malformed scene capacities.

Graphics in this NRO are technical placeholders rather than
approved character art. No final controller-only playthrough,
real hardware save or audio/visual certification is claimed.

## MAP13 — Castle Gardens browser source parity (Stage 53)

The thirteenth technical NRO stage is available by cycling Y
after MAP12, reset with X without affecting MAP01 save slots.
Original world width 9950, 18 solids, 90 optional coins, six
patrols, three fragments, checkpoints 2520/5280/7950,
and fragment-only gate x9780.

Three mushroom springs at x1100/4780/8190 give -730 upwards
from the grounded state and do not auto-trigger in flight.
Three one-touch levers x2330/6110/8080 are optional
decorative interactions; they do not unlock the exit.
Source-to-C99 audits and strict GCC/Clang fixed-step engine
tests cover spring impulses, player control, all switches,
airborne safety, checkpoint and fragment-only completion.

Sprites in the technical NRO are placeholders, not updated
versions of approved Rikitu LAB characters. No full
ordinary-controller playthrough, approved audio, hardware
saves or asset-based remake completion is claimed.

## MAP14 — Heart Foundry source-parity C99 scene (Stage 54)

The fourteenth original level runs in the technical NRO (Y to
cycle after MAP13, X to reset; MAP01 saves remain isolated).
Geometry: 11200 world units, 18 solids, 90 coins, six patrols,
three fragments, three checkpoints 2600/5800/8690, and gate x11020.
Three grounded steam jets x1570/5420/8380 apply a -705 lift,
three conveyor belts x800/4670/8940 use +45/-42/+47 speeds,
and optional touch levers x2380/6500/8770 do not gate the exit.
The 3-HP Assembly Giant at x10470 must also be defeated.
Exact browser data parity and host GCC/Clang C99 fixed-step tests
are included. This is neutral graphics, not approved sprite art or
an input-only playthrough / hardware save validation.

## MAP15 — Clock Gallery C99 browser parity (Stage 55)

The fifteenth original stage is accessible via Y after MAP14,
X restarts the stage, and MAP01 save-slot isolation is unchanged.
World width 10300, 18 solid geometry items, 90 coins, six patrols,
three fragments, checkpoints x2500/5430/8210, gate x10120,
and no mandatory boss or countdown timer.

Three grounded-only clock dials x1150/4500/8470 give an upward
-675 impulse without horizontal drift; three optional switches
x2320/6360/8550 visually activate once but never gate the exit.
A Node.js coordinate parity auditor and strict GCC/Clang
C99 engine tests check launcher action, airborne exclusions,
normal directional control, levers, checkpoints and fragment exit.

This remains a neutral geometry/debug NRO, not new or modified
approved character art. Complete controller-input playthrough,
real Switch hardware and final audio/assets are not verified.

## MAP16 — Heart Chamber final technical stage (Stage 56)

Sixteenth original browser level, selectable via Y after MAP15;
X restarts it. MAP01 persistence is isolated. Source-authentic 11750
world units, 18 solids, 90 coins, six patrols, three fragments,
checkpoints 2930/6260/9260 and exit x11560, gated by defeating the
three-hit Keeper of Minutes boss at x10950.

This finale combines five grounded-only launchers: three steam
(1700/5640/8580) at -705 upward velocity and two clock pads
(3310/7410) at -675. Three conveyor belts at x880/5070/9200
apply +44/-44/+45. The static scene capacity has increased from
four to five launchers with the same bounds checks; 6 is rejected.
No art/sprite redesign or change to serialized MAP01 save version.
Browser source-data parity and strict GCC/Clang C99 tests are included.

The stage is a neutral geometry-first technical preview, not a
verified normal-controller full campaign playthrough, final audio/
approved-character sprite integration or real Switch hardware save.

## Campaign progression core (Stage 61)

The portable C99 state machine `native/rikitu_campaign.[ch]` implements
all sixteen chapter transitions, eight Moki shop stops, coin-wallet
accounting (once per completed gate), and four heart upgrades at stages
2/6/10/14 with original prices 120/160/200/240. It rejects repeated
exit/purchase transitions and validates stage and wallet bounds.

The `native/test_campaign_flow.c` regression is compiled and executed
with GCC and Clang in CI. This PR changes the portable core, **not**
the Switch controller/UI yet. Existing diagnostic level selection,
MAP01-only saves and approved visual designs are unchanged.

## Campaign controller flow (Stage 62)

Normal stage completion now switches to a chapter intermission (odd
chapters) or the Moki shop (even chapters). A proceeds from intermission,
A buys an available heart upgrade in the shop, and B leaves the shop
for the next chapter; finishing the shop after chapter 16 opens the
victory overlay. New levels restore the earned max-hearts capacity.

Y still cycles the 16 debug stages and X restarts the selected stage;
both begin a fresh debug campaign for that level. Completing a level
awards stage coins only once, and overlay screens are not simulated
during paused gameplay. MAP01 keeps its existing on-disk save format;
later chapter wallet/upgrades remain session-only until a separately
versioned full-campaign persistence feature is implemented. Approved
sprites and final audio are unchanged. CI builds the real native NRO
plus independent GCC/Clang overlay tests.

## Switch campaign save wiring (Stage 64)

When SD card is mounted, the full campaign metadata is restored from
two independently CRC-checked RKC2 records in `campaign-a.rkc` and
`campaign-b.rkc`, entirely separate from legacy `map01-a.rks` and
`map01-b.rks`. Autosave occurs after a chapter gate, a successful
Moki purchase, next-chapter/shop exit and on quitting with Plus.

Only **committed** chapter, Moki wallet/upgrades and chapter mode are
stored: reloading a `playing` record restarts the chapter. The old
MAP01 checkpoint continues to work when resuming MAP01. **Debug Y/X
stage selection is isolated** and cannot rewrite real campaign
metadata; diagnostic progress is not carried into next app start.

No filesystem writes are attempted without SD mount. This is a
versioned portable RKC2 metadata scheme, not a save of full gameplay
state, mid-level progress, approved artwork or a hardware-certified
save workflow. On-write I/O errors are currently best-effort.

## RKL3 in-level portable checkpoint codec (Stage 67)

`native/rikitu_level_save.[ch]` defines a 64-byte CRC32 checkpoint
for each of the 16 chapters. It includes the checkpoint index, health,
fragments, per-coin pickup bitset, defeated patrol bitset, lever states,
boss HP/defeat, elapsed time and a wallet/upgrades identity guard
matching RKC2. Saved records restore at a checkpoint rather than in
mid-air, reset boss immunity and apply temporary player protection.
Only an uncompleted playing scene is accepted; completed stages are
recorded by RKC2 to avoid paying out rewards twice.

This isolated PR adds codec plus strict GCC/Clang host tests. It does
not yet wire RKL3 into device SD files; MAP01 RKS1 and RKC2 are
preserved unchanged. That follow-up requires independent save-pair
recovery and checkpoint lifecycle testing. It is not a gamepad route.

## RKL3 two-slot I/O (Stage 68)

`rikitu_level_save_io.c` adds independent alternating 64-byte
`.rkl` files, generation-based selection with wraparound, checksum
validation and fallback on corrupt or truncated writes. Both loading
and writing require paths ending `.rkl`, so MAP01 `.rks` and campaign
`.rkc` slots are never accessed by this component. Older checkpoints
also must match the chapter and RKC2 wallet/upgrade identity before
being loaded. Host GCC/Clang regression tests exercise recovery and
legacy-file preservation. This PR still does **not** enable automatic
Switch SD writes: that is the next integration stage.

## Nintendo Switch in-level checkpoints (Stage 69)

At startup, a playing RKC2 chapter 2–16 may restore its most recent
valid RKL3 checkpoint from `level-a.rkl`/`level-b.rkl` if its chapter,
wallet and heart upgrades still match. Recovered pickups, defeated
patrols and boss HP resume at the last checkpoint with one second of
protection. Chapter 1 continues using only its original RKS1 files.

During ordinary gameplay, the NRO writes RKL3 on meaningful progress
changes (coins, fragments, enemies, boss, checkpoint, health or levers)
with at least two seconds between writes, and once on Plus quit.
No RKL3 writes occur in debug Y/X runs or after completing a chapter.
Once-only stage rewards remain the separate RKC2 responsibility.

Writes are best-effort: SD failures are not surfaced in the UI and
actual Nintendo Switch hardware save durability remains unverified.
Approved art, ROM contents and existing file schemas remain unchanged.

## Stage70 — gameplay-level checkpoint replay

`native/test_level_resume_gameplay.c` uses the real fixed-step C99
engine to collect a coin and a fragment in MAP02, then crosses a real
checkpoint and persists two RKL3 generations. It restarts MAP02,
restores the latest checkpoint, revisits the previously collected items
and checks that no duplicate pickups are awarded. Corrupting the
newest record falls back to the earlier valid state. Wrong-chapter
and wrong-wallet restores are rejected. GCC and Clang execute this
portable integration test in the graphical host CI.

The test controls starting positions to focus on save semantics;
it is not a full controller-only level completion or a hardware SD
durability test. No approved character assets are changed.
