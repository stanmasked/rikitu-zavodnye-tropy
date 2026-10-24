# Stage168 — authentic checkpoint recovery after a missed jump

This regression covers the **player error and recovery** gap in pacing
and playability issue #5. It exercises MAP01 Flower Trail's existing
geometry, enemy patrols and production C99 fixed-step physics, not a
mocked state transition.

1. Start at the normal x=105 spawn and use RIGHT/RUN plus one ordinary
   JUMP near x1542 to collect fragment #1 and cross ground gap #1.
2. Reach the first checkpoint at x2050 and verify fragment #1 is held.
3. Run forward with **no second jump**, intentionally fall through the
   second gap between x3020 and x3095, and allow the native damage/
   respawn logic to execute.
4. Verify that the actual physical respawn returns to checkpoint x2050,
   retains fragment #1 and already-earned coins and increments damage.
5. Continue via controller input until x2210 to demonstrate that the
   player can resume forward progression without a reset.
6. At 30, 60 and 120Hz compare checkpoint/fall samples and state.
   The Stage167 observer must record the large left displacement as one
   *ambiguous discontinuity*, **not** ordinary voluntary backtracking.

Only gameplay controls are injected: no direct coordinate, flag, health,
checkpoint or save mutations occur after map init. The actual time in
this scripted fixture is for reproducibility, not evidence that an
unfamiliar human can complete MAP01 within 6–8 minutes. If tests fail,
diagnose environment/physics/controller strategy and keep Draft open,
rather than relaxing game-state assertions.

No changes to game physics, level geometry, approved character designs,
graphics assets, sound, SDMC save formats or archival policies. CI logs
synthetic counters and uploads nothing. Private real-session captures
must be stored in restricted Google Drive Project Evidence Archive,
with source hashes and independent byte-identical readback.
