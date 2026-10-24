# Stage159 — MAP01 in-memory observational telemetry

This is a **local diagnostic observer**, not a networking service or a
gameplay change. The Switch scene probe observes MAP01 after every
`rikitu_demo_advance` call and tracks **only in RAM**: observed frames,
native simulated ticks, paused frames, increments in pickups/checkpoints,
damage events, level completion, and coarse checkpoint sample timestamps
based on the existing active-time clock. It does not write or upload anything.

## Interpretations and limits

- MAP01 is the Flower Trail (`level_id == 1`). Returning to a new stage or
  restarting the debug run begins a fresh observer. A loaded RKS1 record
  establishes the **baseline**; preexisting pickups are never counted anew.
- An event observed after an advance is timestamped at **end of that frame**.
  Multiple checkpoint crossings in one frame share the same timestamp.
  `-1` indicates a checkpoint already passed before observation began, or
  not yet reached. There is no claim of sub-frame precision.
- Pausing does not add simulation ticks or advance the active clock. Invalid
  or regressed progress is rejected without mutating the observer. Diagnostics
  can be disabled; they never block movement or damage saves.
- Unit tests exercise genuine physics ticks for initial progress and then
  inject synthetic state changes to test counters. Synthetic/teleported
  progress is **not** a 6–8 minute human playthrough, and this stage does not
  validate pacing. No session is captured or certified by NeXium yet.
- No new format is written to RKS1/RKC2/RKL3. No original/approved art,
  capture, PNG, ROM, personal identifier or cloud credential is present.
  All CI artifacts remain disabled. Only approved byte-exact private
  evidence may be manually handed to Google Drive under the shared
  retention policy; nothing in this stage claims authenticated transfer.

## Test gates

The dedicated workflow builds and runs synthetic regressions with GCC,
Clang, ASan/UBSan; the existing full browser/game CI, storage-policy audit,
and Switch NRO build are independent release gates. Do not merge merely
because the dedicated test passed.
