# Stage172 — prove the two-tier branch is optional in the same fixture

Stage171 proved the two-tier route is physically traversable, but compared
its reward against a direct run on canonical geometry. This follow-up
closes the experimental gap: the test now runs **three cases**:

1. Direct route on canonical MAP01 geometry.
2. Direct route that **skips** the branch while the exact same test-only
   platforms and relocated optional rewards are present.
3. The branch route that visits both tiers and collects the three
   fixture-only optional coins.

The baseline and skip controller both start at the normal x=105 spawn,
use ordinary RIGHT/RUN/JUMP input for mandatory fragment #1 and checkpoint
x=2050, then continue to x≈2890. The fixture's two extra platforms and
coin relocation exist only in memory in the test; both baseline and skip
runs disable enemy AI equally to isolate geometry. Production files remain
unchanged.

## Acceptance

- Skip route and canonical baseline match for active time, final position,
  mandatory fragment/checkpoint state, and baseline coin count within
  explicit render-frame tolerances.
- Skip route leaves all three branch reward flags untouched and still
  reaches the same main ground segment.
- Branch route collects all three optional fixture coins, keeps the same
  mandatory fragment/checkpoint, returns alive, and records real reverse
  movement with zero large discontinuities.
- Branch earns at least three more coins than the skip route and adds a
  modest traversal time. GCC, Clang and ASan/UBSan pass at 30/60/120Hz.

The incidental total coin count may differ by at most one at 30Hz because
the controller cannot react between two fixed physics ticks in one render
frame. The three branch rewards and mandatory progression must match.

This remains a **test-only feasibility prototype**. It is not an approved
production layout, visual discoverability result, or human 6–8-minute
pacing measurement. Do not edit canonical level data or art without a
separate approved design stage. CI emits aggregate synthetic counters only
and uploads no artifacts. Real playtest evidence, if retained, must use
restricted Google Drive, a size/SHA-256 manifest, and independent
byte-identical readback.
