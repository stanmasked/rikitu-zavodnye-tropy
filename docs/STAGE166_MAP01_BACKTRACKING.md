# Stage166 — recover a missed MAP01 fragment by normal backtracking

This addresses a critical real-playability question in issue #5: what if a
player passes beneath a required fragment and then decides to go back?

A test controller must begin at MAP01's real spawn and use only
RIGHT/RUN, LEFT/RUN and JUMP through the production C99 physics loop.
Its phases are **not** character teleports:

1. Move to x1590 on foot, **deliberately miss** fragment #1 at x1530.
2. Reverse left over existing terrain to x1390, still with zero fragments.
3. Return right and jump to recover that original first fragment.
4. Land near x1590, then cross the first ground gap x1660–1735.
5. Reach the first checkpoint x2050 with exactly one mandatory fragment.

The native telemetry observer must report exactly one new fragment and one
new checkpoint, without duplicate events. Render frame rates of 30/60/120
must all pass with approximate agreement on active times. If the scenario
fails, investigate physics, trigger ordering, collision and route-controller
choices without lowering assertions or enabling developer teleports.

This is a **scripted test of reachability/reversibility**. It does not
measure a first-time player's reaction, exploration, optional routes, or
the 6–8 minute target. The results must never be reported as human
performance. No gameplay rules, save formats, approved character
appearance, MAP01 art or raw private material are modified.

The workflow builds GCC, Clang and Clang ASan+UBSan, logs only synthetic
aggregates, and does not upload artifacts. Selected real gameplay
captures remain restricted to Google Drive Project Evidence Archive with
independent SHA-256/byte-count readback; GitHub-to-Drive auth is not
available in this workflow.
