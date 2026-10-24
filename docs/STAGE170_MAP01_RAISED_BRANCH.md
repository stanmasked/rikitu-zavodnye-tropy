# Stage170 — optional raised branch and safe return in MAP01

This is an **isolated traversal experiment on existing, source-parity-
verified MAP01 geometry**. It does not add or change production platforms,
coin placement, art, collision rules, save formats or runtime behavior.

The existing raised platform at x=490..675 (top y=390) has a cluster of
three optional coins at x=536, 583 and 629 (y=344). The ground line is
y=456. The test compares two controllers starting at the canonical x=105
spawn:

- **Direct baseline:** hold RIGHT+RUN to x>=730 on the ground.
- **Optional elevated route:** approach x≈420, jump using ordinary input,
  land on the existing raised platform, reverse across it to x≈500 to
  inspect/collect the elevated cluster, move right and step off the
  platform, then land on the ground around x≈740. The route uses actual
  collision and gravity; no coordinate, collectible or state writes.

## Acceptance gates

- GCC, Clang and Clang ASan+UBSan pass at 30/60/120 display Hz.
- The branch reaches the platform top, collects all three elevated coins
  plus the existing coin at x=740 on descent, and rejoins the ground alive.
- The volatile observer records ~80–120 px of real leftward movement and
  zero large discontinuities; no teleports or synthetic pickups.
- The branch collects at least three more coins than the direct baseline,
  remains optional (does not require 90 coins), and takes modestly longer
  than the direct path. The results agree across display rates.

## Interpretation

This tests **reachability, reward and return**, not whether the route is
visually discoverable, fun, or enough to satisfy the 6–8 minute level goal.
The platform is already part of the canonical map, so Stage170 does not
claim a newly designed or art-complete branch. If the playtest is promising,
a later proposal can design a true optional branch in a disposable layout
prototype and get approval before changing production geometry.

CI emits only synthetic aggregate counters and uploads no artifacts. Any
genuine play session that is selected for retention must remain in the
restricted Google Drive Project Evidence Archive, with a size/SHA-256
manifest and independent byte-identical readback. GitHub Actions does not
receive Drive credentials in this workflow.
