# Stage171 — disposable two-tier optional branch layout

Stage170 proved that the existing raised MAP01 platform is accessible and
recoverable, but it only adds a small coin detour. This stage evaluates a
more legible *candidate* without editing canonical production geometry.

## Fixture-only proposal

The test copies the real Flower Trail scene, then adds two temporary
platforms **only in the in-memory test fixture**:

- Tier A: x=2300..2440, top y=390.
- Tier B: x=2460..2600, top y=320.
- Existing platform x=2080..2280 serves as the approach reference.
- Existing platform x=2630..2815 serves as the lower return step.

Three optional coin coordinates are remapped **only in the fixture** to
x=2500, 2540 and 2580 at y=270. The test disables enemy AI in both direct
baseline and branch fixture to isolate platform traversal. Production
level files, actual collectible positions and enemy spawns remain untouched.

## Controller scenario

From the canonical spawn, both baseline and prototype use ordinary
RIGHT/RUN/JUMP input to collect mandatory fragment #1 and activate
checkpoint x=2050. The prototype then jumps to tier A, reverses a short
distance to inspect the route, jumps to tier B, traverses the optional
coin group, steps onto the existing lower platform, and returns to the
ground beyond it. No coordinates, pickup flags, health or checkpoint state
are directly written during the run.

## Acceptance and limits

GCC, Clang and ASan/UBSan must pass at 30/60/120Hz. The prototype must
collect all three optional fixture coins, preserve the required fragment
and checkpoint, return alive to the main ground, and show genuine
leftward movement with no large discontinuities. It must collect at least
three more coins and take modestly longer than the direct baseline.

This is **layout feasibility only**. It does not prove visual discoverability,
fun, art readability, player comprehension or the 6–8 minute pacing target.
No production geometry should be changed based on this test alone; a later
proposal requires art/layout review and real controller playtests.

CI outputs synthetic aggregate counters only and uploads no artifacts.
Any real playtest selected for retention must remain in the restricted
Google Drive Project Evidence Archive with a file-size/SHA-256 manifest
and independent byte-identical readback. GitHub Actions has no Drive
credentials in this workflow.
