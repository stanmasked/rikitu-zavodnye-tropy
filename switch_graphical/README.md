# RIKITU — experimental 2D framebuffer scene

## Current status after Stage92

The graphical NRO now compiles all sixteen canonical map initializers
(MAP01–MAP16) and includes fixed-step gameplay, controller navigation,
Moki shop/between-level/victory overlays, and separate RKS1/RKL3/RKC2
progress saving. Earlier sections below chronicle individual milestones
and can describe older limitations; they are not a claim that the current
binary is restricted to MAP01. Art remains neutral placeholder rendering,
not the approved Rikitu character sprites, animations or final soundtrack.
NeXium provides emulator evidence, not physical Nintendo Switch QA.

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
Stage82 adds `native/test_cloud_full_route.c`, which replays MAP10
from the authentic spawn using controller masks only. It validates
four airborne gap crossings, all four gusts without horizontal drift,
both conveyor directions, all fragments and checkpoints, three actual
stomps on the Wind Automaton, and the real gated exit. Strict GCC and
Clang run this route in both the browser/game-rule and graphical
Switch host-render workflows.

These are neutral technical graphics; approved character art is not
redrawn. The route certifies host simulation and CI, not physical
Joy-Con input, real Switch performance, finished sprite/audio
integration or SD-card power-loss durability.

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


## Stage72 — all-map RKL3 gameplay checkpoint regression

`native/test_level_resume_all_maps.c` exercises MAP02–MAP16 with real
60 Hz C99 engine ticks. Each map collects its first coin and fragment,
crosses a checkpoint, writes two RKL3 generations and restarts the
scene. It verifies restored checkpoint, hearts, boss health and pickup
flags, then re-enters the original pickup positions to ensure no double
awards. Wrong-wallet and wrong-chapter records must not mutate a fresh
level; damaging the newer slot must restore the older valid state.

The test intentionally positions the player near pickups and checkpoint
triggers. It does not prove complete controller-driven routes, actual
Nintendo Switch SD-card durability or power-loss resistance.


## Stage73 — no-teleport button route, pit respawn and RKL3

`native/test_controller_checkpoint_reboot.c` runs the actual MAP02 C99
game loop at 60 Hz, starting at the unmodified scene spawn. A fixed
sequence of RIGHT, RUN and JUMP **input bitmasks** moves the character
to the first checkpoint, collecting a coin and the first fragment and
crossing an existing gap without direct position assignment. The
character then runs to a second gap and falls to trigger the engine's
checkpoint respawn. The test writes and inspects RKL3 generations
before and after respawn, reloads a fresh scene, and checks the
restored inventory, checkpoint, hearts and damage count. It also
checks pause/resume and movement after the restore.

This is a deterministic host-side button-input regression rather than
a verified physical Joy-Con, Switch HID, or SD durability test. It
does not replace controller-only end-to-end runs for all 16 maps.


## Stage74 — MAP03 input-only checkpoint replay

`native/test_grove_controller_resume.c` starts from the real MAP03
scene spawn and uses 60 Hz RIGHT/RUN/JUMP button masks only.
It records the actual mushroom launch, first coin and fragment,
crosses the real first checkpoint, writes two RKL3 generations,
cold-loads the latest generation and resumes moving. It also rejects
a foreign wallet and corrupts the newer slot to prove recovery from
the previous checkpoint. No direct player coordinate assignments
are performed in the route.

This verifies host engine input semantics, not full-map completion,
real Nintendo Switch HID, Joy-Con hardware or physical SD durability.


## Stage75 — MAP04 real input route, enemy and switch persistence

`native/test_bridge_controller_resume.c` follows MAP04 from its real
spawn using RIGHT/RUN/JUMP input at 60 Hz. The script runs through
the first patrol's actual contact damage, collects the first coin
and fragment, crosses the first floor gap, and reaches checkpoint 1.
RKL3 generation A is saved with lever 1 off. The script then triggers
lever 1 through the actual player-overlap rule and saves generation B.

Reopening MAP04 from scratch restores lever state, player health,
checkpoint, pickups, enemy status and timer. The test revisits the
already-on lever to assert it cannot be counted twice. A corrupted B
record must fall back to A with lever 1 still off; controller input
can then toggle that lever once. The wrong-wallet campaign must not
restore foreign data.

This is host-side C99 button-mask replay rather than hardware HID
testing, and it does not simulate full MAP04 boss completion or
power-cut SD recovery.


## Stage76 — MAP05 controller-driven conveyor, checkpoint and RKL3

`native/test_workshop_controller_resume.c` drives MAP05 from its
original spawn with 60 Hz RIGHT/RUN/JUMP input only. The test verifies
that conveyor #1 increases horizontal travel by its actual +48 units
per second while grounded (versus the normal +265 units/sec RUN speed),
collects the first coin and fragment, crosses the initial floor gap
and activates the first checkpoint. It then writes two RKL3
generations, restores checkpoint, coins, fragments, hearts, damage
counter and timer into a newly initialized scene, and resumes motion.
A foreign-wallet restore is rejected. Corruption of the newer slot
must recover the older checkpoint record.

The executable is a portable C99 host input regression, not proof of
physical Switch Joy-Con input, a full MAP05 playthrough, or SD-card
crash/power-loss durability.


## Stage77 — MAP06 input-driven conveyor and steam checkpoint resume

`native/test_delivery_controller_resume.c` replays real 60Hz input
masks from the MAP06 spawn. It observes a grounded +42-unit/sec
conveyor boost, the automatic -705-unit/sec steam launch, first coin
and fragment, a gap-crossing jump, patrol damage and checkpoint #1.
The test writes two RKL3 save generations, loads a freshly initialized
scene, checks inventory/health/damage/timer/boss state, and resumes
controller input. A mismatched wallet cannot load. Damage to the latest
save causes fallback to the earlier checkpoint.

This test checks the native game-loop's host-side input semantics,
not a complete MAP06 boss battle or actual Switch HID/SD durability.


## Stage78 — MAP07 input-driven clock launch, conveyor and RKL3

`native/test_clock_controller_resume.c` uses the original MAP07 scene
spawn and authentic 60 Hz RIGHT/RUN/JUMP input rather than changing
the player's world position. It validates the first clock pad's
-675 units/sec vertical impulse, opening coin, fragment, gap and
checkpoint 1. Past that checkpoint it measures the first conveyor's
+40 units/sec velocity contribution during grounded running.

The route saves RKL3 generation 1 before the conveyor and generation 2
after it, cold-reloads a fresh scene, verifies health, coins,
fragments, damage and elapsed time, and resumes input. Foreign-wallet
loading is rejected; corrupting the latest slot falls back to the
first generation and its checkpoint.

The C99 test is host button-mask replay, not physical Joy-Con testing,
full MAP07 completion or Switch SD power-off recovery.


## Stage79 — MAP08 controller, conveyor, lever and RKL3 rollback

`native/test_orders_controller_resume.c` starts from MAP08's authentic
spawn and uses only 60Hz RIGHT/RUN/JUMP input. It checks the +45
units/sec conveyor boost during grounded running, the first patrol's
real contact damage, a collected coin/fragment, the first floor gap,
the touch-activated lever and the first checkpoint. It saves RKL3
generation A *before* the lever (checkpoint 0, lever OFF), and
generation B *after* the lever and checkpoint (checkpoint 1, lever ON).
On a fresh scene, loading B must restore the lever, checkpoint, enemy,
boss, pickups and hearts; revisiting the lever by LEFT/RIGHT input
must not count it twice. A foreign wallet is rejected, and corrupting
B must roll back to the older pre-lever A state with pickups intact.

These are native C99 host button-mask tests, not full MAP08 boss
completion or tests of physical Joy-Con hardware and Switch SD
power-loss resilience.


## Stage80 — MAP09 gust, neutral input and pre/post-checkpoint recovery

`native/test_mist_controller_resume.c` replays the actual MAP09
Misty Pass spawn using 60Hz RIGHT/RUN/JUMP input rather than changing
player coordinates. It checks the first -690 units/sec wind gust and
then sends a full neutral input frame while airborne to assert
zero involuntary horizontal drift. The path receives genuine patrol
contact damage, collects the opening coin and fragment, crosses the
first gap and reaches the first checkpoint.

RKL3 generation A is written before checkpoint 1 at spawn checkpoint
105; generation B is written after reaching checkpoint 1 at X=2280.
The regression restores the latest generation into a freshly
initialized scene, validates pickups, hearts, damage, timer and
controller motion, rejects a foreign campaign wallet, then corrupts
B to prove rollback to the *older pre-checkpoint* A state with
collected items intact.

The test runs under host GCC and Clang. It does not establish full
MAP09 completion, actual Joy-Con HID compatibility or physical
Switch SD crash/power-cut durability.


## Stage81 — full MAP09 route from spawn to the real exit

`native/test_mist_full_route.c` replays the complete Misty Pass using
only 60Hz RIGHT/RUN/JUMP input from the authentic spawn. It crosses all
four floor gaps, collects all three fragments, activates the three
checkpoints and reaches the real fragment-gated exit. It also checks
each gust's vertical impulse and injects a neutral input frame after
every launch to prove that the surface never forces horizontal drift.

The route re-arms only jump triggers that are ahead of a genuine damage
respawn checkpoint; it never edits player coordinates to skip gameplay.
The test permits authentic enemy damage and verifies that progression
survives respawns while collected fragments remain collected.

The full route runs with strict GCC and Clang in both the normal host
CI and graphical Switch host-render CI. It is a host simulation, not
physical Joy-Con, Switch hardware, SD durability or 60fps performance
certification.


## Stage83 — MAP11 Crystal Caves full input route

`native/test_crystal_full_route.c` replays MAP11 from its real spawn with
60Hz RIGHT/RUN/JUMP input masks. It verifies all four floor-gap crossings,
all three required fragments, the three checkpoints, and the three crystal
springs. After each `-690 units/sec` launch, a neutral input tick must leave
horizontal position unchanged. The route also requires actual enemy contact damage, uses deliberate
jumps to pass the later patrols without repeated respawn loops, and reaches
the real fragment-gated exit; it does not teleport the player or edit
enemy/pickup state.

Strict GCC and Clang checks run in both the browser/game-rule workflow and
graphical Switch host-render workflow. These are deterministic host-side
input replays, not physical Joy-Con, real-console frame-rate or SD power-loss
tests.

## Stage84 — MAP12 Silent Mine full input route

`native/test_mine_full_route.c` replays the twelfth level from its
authentic spawn using only 60Hz RIGHT/RUN/JUMP/LEFT input masks. It
checks all four floor-gap crossings, all three fragments and checkpoints,
the three steam vents, all three conveyor directions, all three automatic
levers, real enemy interactions, three genuine boss stomps and the
boss-gated exit. A neutral-input tick after each `-705 units/sec` vent
launch must leave horizontal position unchanged.

The test is compiled with strict GCC and Clang in both browser/game-rule
CI and graphical Switch host-render CI. It is deterministic host simulation,
not physical Joy-Con, real-console performance, finished art/audio or
SD-card power-loss certification. No player coordinates, pickup flags,
lever state or boss HP are injected by the test.

## Stage85 — MAP13 Castle Gardens controller route

`native/test_gardens_full_route.c` starts at the authentic spawn and attempts
full level completion via the normal 60 Hz game loop with only RIGHT/RUN/JUMP
input masks. It records three fragments, four physically crossed gaps, three
mushroom launches at -730 units/sec (with a neutral-input no-horizontal-drift
frame for each), three contact-activated levers, three checkpoints, and the
fragment-gated exit. Real damage/respawns are allowed; historical crossing
bits are not reopened when a previously completed gap is jumped again after
respawning. GCC and Clang compile and run the route in both host CI jobs.

A passing input replay is not a physical Joy-Con or Nintendo Switch hardware
playtest. It does not change canonical RIKITU art or gameplay geometry.

## Stage86 — MAP14 Heart Foundry authentic opening route

The `native/test_foundry_input_opening.c` regression replays real 60 Hz
RIGHT/RUN/JUMP input from the original MAP14 spawn to checkpoint one. It
checks the first +45 conveyor-belt displacement, steam launcher vertical
impulse (-705) with a neutral-input/no-horizontal-drift frame, first
fragment, airborne passage and grounded landing beyond gap one, automatic
lever activation, and the original checkpoint at x=2600.

The mandatory three-hit boss remains alive and the gate remains locked in
this opening-stage test. Unlike isolated surface contract tests, this
regression never assigns player coordinates or modifies pickups/enemies.
Both GCC and Clang run it in standard and graphical-host CI. This is not
yet the full MAP14 route or physical Nintendo Switch controller QA.

## Stage87 — MAP14 Heart Foundry full controller route

`native/test_foundry_full_route.c` extends the verified Stage86 opening into
the whole level using 60Hz RIGHT/RUN/JUMP/LEFT native-engine input masks.
The first elevated fragment and first pit are deliberately one jump, not
two impossible consecutive jumps. The test requires four real airborne
pit crossings followed by grounded far-platform landings; three fragments;
three steam vents with zero-input no-horizontal-drift frames; three
conveyors with measured grounded velocity; three touch-triggered levers;
three checkpoints; real damage; and the three-hit mandatory boss before
the level exit. Respawns only reset the test's pending crossing state.
Neither player positions nor pickup/boss state are directly edited.

Strict GCC/Clang input replay runs in ordinary and graphical host CI. This
is not an assertion of real Switch hardware/Joy-Con or SD durability QA.

## Stage88 — MAP15 Clock Gallery authentic input-driven opening

`native/test_gallery_input_opening.c` uses 60 Hz RIGHT/RUN/JUMP from the
unchanged MAP15 spawn to checkpoint #1 (x=2500). It observes the actual
clock-pad -675 launch and neutral/no-horizontal-drift frame, the first
elevated fragment, a genuine jump over the first 80-unit floor gap with
airborne far-edge crossing and grounded landing, automatic activation of
lever #1, and a real checkpoint. All actions are simulated by the game
engine, not by manually setting player coordinates or pickup flags.

Strict GCC/Clang checks are wired into both CI paths. The test does not
claim full MAP15 completion, Switch/Joy-Con hardware certification or
independent SD-card cold-restart durability.

## Stage89 — MAP15 Clock Gallery complete controller route

The initial `native/test_gallery_full_route.c` route drives the actual
native 60 Hz game loop from the level's original spawn using controller
masks. It verifies four far-edge airborne crossings and subsequent
grounded landings, three elevated fragments, three real clock-pad -675
launches with zero-input horizontal stability, three checkpoints and all
three touch-triggered levers. At the third clock launcher, controller input carries Rikitu beyond
the raised x8490..8695 platform, allows gravity to return him to ground,
and walks left below the ledge to touch lever #3. The return mode is
latched until the lever actually activates. No coordinates, pickup flags,
enemy states or boss states are edited by the test.

Host strict GCC/Clang checks run in both ordinary and graphical workflows.
Full success still requires those checks; the test does not certify actual
Switch controller hardware, SD durability or final approved character art.
 
## Stage90 — MAP16 Heart Chamber first checkpoint by real controller input

`native/test_heart_input_opening.c` replays the authentic MAP16 starting
position through the normal 60Hz RIGHT/RUN/JUMP engine commands to
checkpoint one (x=2930), collecting the first elevated fragment and
crossing the opening 80-unit pit via an airborne far-edge crossing and
grounded far-platform landing. It also measures the first +44 conveyor
belt and verifies the first -705 steam launch leaves horizontal motion
unchanged after a neutral input frame. The three-hit final boss must
remain alive and the exit unavailable.

This input regression does not teleport the player, alter enemy or
pickup state, rebalance physics, validate physical Joy-Cons, or certify
durability of a real Switch SD card.

## Stage91 — complete MAP16 Heart Chamber by real 60Hz controller input

`native/test_heart_full_route.c` is the first candidate for a complete
finale replay. It begins at x=105 and uses only LEFT/RIGHT/RUN/JUMP
masks, requiring two deliberate jumps to elevated fragments (plus first
platform walking pickup), all three fragment pickups, four airborne
far-edge crossings followed by landings, five mixed steam/clock launchers
with zero-input no-horizontal-drift frames, three measured conveyor
velocities, all three checkpoints, and three authentic stomp hits on the
mandatory final boss before entering the fragment-and-boss-gated exit.

No test code edits player coordinates, flags pickups, unlocks the gate or
changes boss HP. Both GCC and Clang run this deterministic host route in
ordinary and graphical CI. This is *not* verification on actual Nintendo
Switch hardware, a human playthrough, or an SD power-cut durability test.
 
## Stage92 — real finale gameplay to campaign victory and CRC-checked save

Compile `native/test_heart_full_route.c` with
`-DRIKITU_STAGE92_REAL_VICTORY`, including `rikitu_campaign.c` and
`rikitu_campaign_save.c`. Stage91's regular full-route executable remains
unchanged. The Stage92 variant starts from the real MAP16 spawn, obtains
all three actual fragments, stomps the final boss three times and enters
the gate before invoking `rikitu_campaign_finish` on the game's earned
`RikituStage`. It then checks mandatory final Moki shop mode, transitions
to campaign victory, roundtrips the RKC2 serialized final-state record
with generation and CRC, and confirms a corrupt copy is rejected without
mutating the decoded state. The campaign cannot advance before the boss
and fragments are complete.

Both GCC and Clang replay the full ending plus serialization assertions
in normal and graphical host CI. A valid in-memory RKC2 roundtrip is
not evidence of actual Switch SD filesystem durability or real-console
end-to-end UI testing.

## Stage93 — NeXium all-sixteen-level guest selector smoke

`tests/run_nexium_graphical.sh` has a scoped
`RIKITU_MAP_CYCLE_QA=1` mode for the **actual compiled graphical NRO**.
After the existing cyan-marker movement test, it issues fifteen Y key
presses, capturing each of the sixteen level HUDs and one additional Y
press for the MAP16-to-MAP01 wrap. The companion
`tests/check_emulator_map_cycle.py` checks screenshot dimensions, 16
distinct fixed HUD title strips and a matching returned MAP01 strip.
It intentionally does not claim OCR, finished art, real Joy-Con
compatibility, live player-cleared chapter transitions or console SD
power-cut durability. NeXium's default keyboard mapping assigns Nintendo Y to X11 key `z`
(see nexium-gui/src/controller_config.rs in nexium-emu/nexium).
The HUD comparison excludes NeXium's changing FPS performance panel
at x=14..238; earlier attempts using X11 key `y` were not valid
level-switch evidence. Evidence PNGs are uploaded by the existing NeXium
artifact step. Stage93 adds no new character art or physics changes.
 
## Stage94 — connected 16-stage campaign/shop/restart integration

`native/test_campaign_transition_save_chain.c` starts with all 16 real
native scene initializers in their campaign order. Using the production
pickup and boss collision APIs with test-supplied overlap coordinates, it
checks each gate's initial block, all three fragments, 90 coins, three
real boss-hit events when a boss is required, and the earned completion.
The real campaign state machine must then enter BETWEEN or SHOP, save an
RKC2 campaign snapshot, successfully reload after the equivalent of an
app restart, display its overlay, and advance to the real next scene.
Four Moki heart upgrades (MAP02/06/10/14) are independently saved and
reloaded while still at the shop; all 16 stage transitions end in a
persisted VICTORY with seven hearts.

This is a full-campaign **state transition and serialization fixture**,
not 16 human/gamepad playthroughs: test coordinates isolate contact
events to avoid spending hours crossing all stages in CI. Existing
full-route input replays check individual levels. No external game assets,
approved Rikitu sprites, game geometry or balance are changed. RKC2
host-file round trips do not establish physical Switch SD power-loss
durability, nor do these checks replace GUI input in NeXium.
 
## Stage95 — real host process RKS1 persistence (NeXium limitation isolated)

`native/test_save_cold_process.c` uses the production RKS1
`rikitu_save_store_pair` and `rikitu_save_load_pair` APIs but executes
three **separate Linux OS processes** sharing on-disk checkpoint slots:
`seed`, `resume`, `verify`. The first process creates generation 1 at
checkpoint 2050, the second loads it and commits generation 2 at
checkpoint 4490, and the third restores it without a further write.
Every process verifies slot generations, CRC and saved pickup/enemy data.
GCC and Clang run separate three-process chains in both CI workflows.

This is stronger than a single-process C99 serialization round trip,
but it is not an SDMC mount test. The earlier NeXium guest QA still
fails at guest SDMC mount; its separate Draft PR #35 is retained as a
strict, non-green test until actual guest filesystem support succeeds.
Neither host files nor the NeXium guest test establish physical SD-card
power-loss durability.
 
## Stage95 — default libnx SDMC mount reuse in the actual Switch NRO

The default libnx `__appInit()` invokes `fsdevMountSdmc()` before
`main()`. Calling it a second time with the same name can fail with
libnx result 0x559: `fsdevMountDevice` returned -1 because `sdmc`
was already registered. Stage95 first calls
`fsdevGetDeviceFileSystem("sdmc")` to reuse this existing mount; only
if missing will it explicitly mount SDMC, and only its own registration
is unmounted on exit. RKS1, RKC2 and RKL3 save/reload initialization
continues after the same `save_ready` directory check. This change
does not modify save-file formats or save-game progress.

The original NeXium guest probe exposed result 0x559, but
emulator-side SD file I/O must still pass the separate three-launch
test in PR #35. GCC/Clang host persistence is separately covered by
Stage95 PR #89. A successful NRO build or source test cannot claim
physical console power-loss safety.

## Stage95 — SDMC independent cold guest starts (Draft PR35)

The `QA_MODE=2` NRO is a separate diagnostic executable, not the shipping
16-level app. Three separate NeXium processes share `NEXIUM_DATA_ROOT`.
Session 1 seeds a CRC-checked RKS1 checkpoint, session 2 loads it and
writes the next checkpoint, and session 3 validates persistence without
writing. Each captured guest screenshot must contain four green indicators,
and the host checks that alternating files have the right CRC, generations
and checkpoint numbers. A red first indicator means `fsdevMountSdmc()`
failed *before* game save decoding and must not be described as a
successful guest persistence test. The old October 20 CI evidence had no
green SDMC mount indicator; this refresh retains that failure signal.
Even success here would verify emulator file persistence, not real
Nintendo Switch SD power-loss durability or filesystem crash safety.
 
## Stage96 — NeXium guest campaign RKC2 + level RKL3 cold restarts

The independent `QA_MODE=4` NRO exercises both production campaign and
in-level persistence APIs using the same on-SDMC paths as the 16-map
Switch build. In three separate NeXium GUI processes sharing a dedicated
`NEXIUM_DATA_ROOT`, it first seeds a MAP04 fixture with 4 maximum hearts
(1 Moki upgrade), 157 wallet coins, checkpoint #1, fragment/coin/enemy
flags; then it reloads that state and saves checkpoint #2, a second
fragment/coin/enemy, 2 current hearts and an injured 2-HP boss; the
third launch restores this final state without writing. Both 32-byte
RKC2 and 64-byte RKL3 dual-slot families must have generations 1 and 2,
correct CRC, and exact state fields, verified by a Python host-side
auditor plus green indicators within each guest frame.

These are deliberately constructed states in a separate test NRO, NOT a
human controller playthrough or a proof of physical SD-card power-loss
safety. They establish that libnx+NeXium can really mount, read and write
these two save families between independent emulator executions.
 
## Stage97 — recover from a torn/corrupt newest SDMC slot in NeXium

`tests/check_torn_guest_slots.py` adds explicit simulated media faults to
the previously verified RKS1 and RKC2/RKL3 guest cold restart process.
For each family, two *independent* NeXium NRO runs first commit intact A
generation 1 and B generation 2. The host then deliberately damages the
newer B: CRC corruption for RKS1 and RKC2, and an incomplete 13-byte
RKL3 write. The older A and its SHA256 digest must remain unchanged.
A third independent guest must reload the older A, show all four green
status indicators for the older checkpoint, and recreate B. The auditor
checks both complete CRC-valid records, two generations and byte-for-byte
identity with the original saved data; a fourth guest then loads the
repaired latest B without writing.

This proves that the actual libnx/NRO save codecs use the older valid slot
after synthetic corruption and safely rewrite the newer slot in NeXium.
It deliberately does **not** simulate real SD flushes, sudden electrical
power loss, filesystem metadata damage, or actual Switch hardware.
 
## Stage98 — reverse A-slot recovery after simulated interrupted writes

Stage97's guest test damaged the latest B while preserving A. Stage98
exercises the symmetric and equally important opposite slot orientation.
After seed/resume in two independent NeXium NRO executions, the host
moves original A/gen1 to the B path and original B/gen2 to the A path.
Only newest A is then damaged: truncate RKS1 and RKC2 to 13 bytes,
and corrupt RKL3's CRC byte. The older B must remain byte-for-byte
intact. A third independent guest restores the previous version from B,
reports all green status markers and rewrites A/gen2. The host checks
the two original SHA256 digests and correct CRC/generation ordering
A/gen2 and B/gen1. A fourth guest validates the restored latest A.

The `tests/check_reverse_guest_slots.py --selftest` first proves that
injected faults are rejected and correctly repaired fixtures pass.
This is controlled corruption of completed host-backed SDMC files,
not a kill of a guest while a real write is in flight, not fsync
validation, and not physical Switch power-interruption certification.
No shipping game content, accepted art or physics changes.


## Stage99 — uint32 generation wraparound, both slot orientations

RKS1, RKC2 and RKL3 save slots contain independent CRC32 checksums and
uint32 generation counters. Stage99 tests the rarely reached transition
from generation 0xffffffff to 1 (zero is disallowed), in both directions
A(max) -> B(1) and B(max) -> A(1).

The first real guest writes valid A/gen1; the host changes only its
generation to 0xffffffff and recalculates CRC, optionally moving the
whole record into slot B. This synthesizes the high counter without
billions of NRO runs. In a second independent guest, the codec must
load this first checkpoint and write the upgraded state to the opposite
slot at generation 1. The host verifies generation, CRC, checkpoint
and SHA256 stability of the original older save. A third independent
NeXium guest must load the *newer generation 1* rather than the stale
0xffffffff record, displaying four green phase-specific checks.
The post-verify auditor requires byte-exact stability of both slots.

Stage99 adds an opt-in screenshot-only audit path for the dedicated
QA NROs because previous Stage95–98 checkers intentionally assert
generation sequences 1 then 2. Those existing checkers and all regular
NeXium scenarios remain unchanged. The new selftest in standard CI
requires missing/zero-generation saves and mutated backups to fail.

This is a deterministic test of wraparound logic on an emulator-backed
SDMC directory, not a hardware power-loss test and not a reproduction
of years of actual writes. No shipping gameplay, maps, graphics or
approved Rikitu character designs are modified.


## Stage100 — interleaved campaign and level saves at crash boundaries

RKC2 holds campaign identity (chapter, mode, wallet, purchased/max hearts).
RKL3 contains one chapter's in-level checkpoint. These are independent
dual-slot files, not an atomic four-file transaction. The Stage100 native
GCC/Clang integration test exercises the production encoders, decoders
and pair writers at two possible interrupt boundaries:

* Chapter transition: RKC2 already reports MAP04 with the same wallet,
  but both RKL3 slots still hold MAP03. A fresh MAP04 scene must refuse
  to load those foreign checkpoints; its next save preserves the previous
  MAP03 B backup.
* Campaign rollback: the latest MAP04 RKC2 is CRC-corrupted, so older
  MAP03 RKC2 is authoritative; the new MAP04 RKL3 slot must not mask
  the intact MAP03 B checkpoint, even if generations differ.
* Same-chapter wallet change: RKC2 has a new wallet but RKL3 retains
  the previous wallet. The checkpoint must be rejected; after writing
  a matching new RKL3, a rollback of the campaign RKC2 must recover
  the older checkpoint with the earlier wallet, byte-for-byte intact.

The test asserts a rejected decode cannot partially alter the live scene,
uses SHA-equivalent byte snapshots of the preserved backup, and runs under
both native GCC and Clang with strict warnings. This validates current
identity-based fallback. It does not make separate file writes atomic,
measure SD flush ordering, or prove sudden power-loss durability. No
shipping save format, game rules, maps, or approved artwork changed.


## Stage101 — NeXium rejects CRC-valid foreign RKL3 checkpoints

Unlike Stage97/98's truncated or checksum-invalid slot, Stage101 modifies
a complete RKL3 record and recalculates a *correct* CRC32. Only the newest
checkpoint is changed: either MAP04 becomes MAP03 or the encoded wallet
becomes 158 rather than campaign RKC2's 157. The independent RKC2 slots
remain unchanged. Both A-newest and B-newest slot layouts are exercised.

For each identity fault/slot orientation, two independent real guest NRO
starts seed legitimate MAP04 RKC2/RKL3 generation 1 and 2. With the guest
stopped, host fault injection creates a semantically foreign, checksum-valid
newest RKL3 while preserving older RKL3 byte for byte. A third guest must
read the older matching checkpoint, show four green resume indicators and
repair the foreign slot. The auditor requires the original RKL3 data to
be recreated byte exactly, surviving backup SHA256 unchanged, campaign
RKC2 B/generation2 untouched and A/generation3 matching its expected
checksum. A fourth independent guest must load the upgraded checkpoint
with green verify indicators without altering any file.

The regular CI also self-tests rejection of the uncorrected identity
mismatch and of a modified older backup. Existing normal and faulted
Stage95–100 guest scenarios continue unchanged. This is not a real SD
power cut and does not make RKC2/RKL3 updates an atomic transaction.
No gameplay, map logic, character art or shipping-save writer modified.


## Stage102 — failed target file open and retry without losing a good slot

The native C99 fixture `native/test_save_write_retry.c` tests all three
production pair writers (RKS1, RKC2 and RKL3). For each format it creates
one verified slot, tests A-survives/B-blocked and B-survives/A-blocked,
and makes the target path an empty directory so `fopen(...,"wb")` must
fail deterministically, regardless of runner permissions.

Every failed write must report failure while preserving the last valid
save byte-for-byte. Restoring from the remaining valid slot must still
recover the previous state. Removing the directory and repeating the
save must write the new state to the opposite slot without rewriting
the backup. The fixture asserts checkpoint, health, coin progress or
campaign wallet changes survive the successful retry and runs under both
GCC and Clang with strict C99 warnings.

This tests a **failed file open**, not a torn write after an open succeeds,
an SD flush failure, a crash between writes, or physical Switch power
interruption. No shipping codec, gameplay, accepted map, asset or character
design is altered.


## Stage103 — injected partial write and flush failures

The native `test_save_write_faults.c` fixture links the **actual** RKS1,
RKC2 and RKL3 C99 pair writers with Linux-host-only linker overrides for
`fwrite` and `fflush`. No shipping writer implementation is edited.

For every codec, the fixture tests both backup orientations (A and B)
with two independent fault modes: (1) `fwrite` writes only half of its
fixed-size record and reports a short count; (2) a test wrapper performs
the underlying flush, truncates the target record to half its size, then
reports a simulated `fflush` failure. Each case requires the production
writer to return failure, a truncated unusable target, and a byte-identical
untouched original backup. The production loader must select that backup.
With injection disabled, an immediate retry must create a complete valid
generation-2 target and restore the updated level checkpoint, health,
coin progress or campaign wallet without altering the backup.

The matrix has twelve cases (3 formats x 2 backup orientations x 2 fault
modes), compiled and executed separately with GCC and Clang using strict
C99 warnings. These tests exercise error-path control flow and recovery,
but do **not** simulate real physical power loss, true OS writeback timing,
storage-device cache durability, atomic multi-file transactions, or
`fsync`. The linker fault hooks are test-only and never part of Switch NRO.


## Stage104 — ambiguous fclose: complete or torn candidate

Stage104 links unmodified RKS1, RKC2 and RKL3 pair writers into a
native GCC/Clang test with a host-only `--wrap=fclose` hook. The hook
ignores closes of files opened for reading, but makes the one writer's
close return `EOF` with `errno=EIO`, after actually closing the stream.

Two deterministic outcomes are modeled:

* **Torn target:** Immediately before the close, the test truncates the
  fully flushed target to half its expected length. The writer returns
  failure; the previous backup stays byte-identical, the loader selects
  that backup, and a retry writes generation 2 into the damaged target.
* **Complete target despite error:** The actual close succeeds and a valid
  generation-2 save remains on disk, but the wrapper reports failure.
  The loader selects gen2. The following save writes gen3 over the
  original gen1 slot **while preserving the valid gen2 backup**. This
  matches current two-slot selection semantics.

Each outcome runs for RKS1/RKC2/RKL3 with A or B as the original backup:
12 cases per compiler, checking correct generations, state restoration,
byte-level preservation and retry behavior. Nothing is added to shipping
libnx code, save formats, maps or approved assets.

**Critical durability limitation:** an actual OS `fclose`/device-cache
error can leave apparently complete bytes that have *not* reached stable
storage. CRC-valid readback alone cannot certify flush ordering or safe
power-loss recovery. In the complete-target case, the next write may
overwrite the original gen1 slot. Therefore this test characterizes the
existing behavior; it does not prove that the two-slot algorithm is
power-fail atomic or that old gen1 always survives subsequent writes.
A future durable-storage design should be validated with real fsync/
directory-sync guarantees on the target filesystem/hardware.


## Stage105 — isolated staged-slot commit prototype (not shipping)

The optional native `rikitu_staged_slot.c` helper uses a sibling
`.pending` path. It writes the requested bytes there, checks `fwrite`,
`fflush` and `fclose`, reopens the staging file for an exact-byte
readback, then calls `rename` to replace the destination. **It never
removes the existing destination first.** On any pre-rename error it
returns failure, removes only the staging path, and leaves *both*
already-existing slot paths byte-for-byte unchanged.

This helper is **NOT connected to the shipping game**. The Stage105
test invokes it directly on Linux host; five host-only failures
(short write, failed flush, ambiguous close, falsified readback, failed
rename) are injected for both existing backup orientations and three
record sizes. It verifies that both slots survive failed staging and
an unmodified retry can commit new bytes. This makes 30 native cases
for each of GCC and Clang.

**Critical limitations and next gate:** this prototype does not call
`fsync`, does not prove concurrent-writer safety, and makes NO promise
that libnx's SDMC/FAT rename supports overwriting an existing slot or
is atomic/durable during power loss. No shipping save writer is changed.
Before adopting it, compile and exercise actual replacement semantics
on a Switch-target NRO and verify behaviour in NeXium; physical SD
power-cut durability will still require separate hardware evidence.


## Stage106 — NeXium-only same-directory SDMC replace semantics

QA_MODE=5 builds `rikitu_staged_rename_qa_probe.nro`. It runs
`rikitu_staged_slot_commit` inside a real Switch guest, against separate
`sdmc:/switch/RIKITU/stage106-a.bin` and `stage106-b.bin` files,
**never** against RKS1/RKC2/RKL3 game saves.

Three independent NeXium cold launches reuse the same private SD root.
The first seeds A=OLD and B=STALE, then attempts a sibling-pending
commit of B=NEW **over the already-existing B**. The second reads the
persisted OLD/NEW pair and attempts A=LATEST over already-existing A.
The third checks LATEST/NEW byte-for-byte without writing. Every guest
renders four green/red status checks. The host audits exact file bytes,
the lack of leftover .pending files and both guest screenshots.

The new workflow `.github/workflows/stage106-staged-rename.yml` builds
this dedicated NRO, obtains NeXium, and checks all three cold starts. A
failed replace on emulated SDMC must remain a **failure**: neither the
test nor the staging helper removes a live target as a workaround.

Even if the test passes, NeXium is not hardware evidence for atomic
FAT rename or power-loss durability. Shipping saves remain unchanged;
a physical SD hardware test and a safe durability protocol are still
required before production adoption.
