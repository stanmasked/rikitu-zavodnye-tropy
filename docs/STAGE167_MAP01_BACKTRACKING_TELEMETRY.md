# Stage167 — observable MAP01 backward movement, no trajectory retention

Issue #5 calls for tracking return trips when players miss fragments.
Stage159's existing MAP01 observer counts time, coins, fragments,
checkpoints and damage. Stage166 adds a scripted ordinary-controller
scenario that physically turns back to retrieve a missed fragment.

This stage adds **RAM-only per-frame displacement aggregates**:

- `leftward_distance_px`: positive sum of ordinary negative differences in
  observed player x (a **motion** measure, not an assertion of player intent).
- `leftward_frames`: display frames with such ordinary leftward motion.
- `large_left_discontinuities`: number of observed left jumps exceeding
  64 pixels in one sample, counted separately as ambiguous discontinuities
  rather than ordinary backtracking. This heuristic **does not prove** that
  a respawn occurred.
- `last_player_x`: previous x position only, not a raw position history.
  It is initialized from the current scene state when an observer begins.

Samples with no 60-Hz simulation ticks but a changed position are
rejected without changing any observer counter. Nonfinite coordinates
and positions outside the stage width are rejected. Genuine leftward
steps are measured in the unchanged production physics loop; a synthetic
large jump verifies exclusion of discontinuities.

The full input-only optimized MAP01 route should report zero reverse
movement. The separate missed-fragment test can later be used to assert
positive leftward distance when it has been merged into main.

## Privacy and interpretation

There is **no new file, network export, gameplay mutation or save-format
change**. These values are volatile instrumentation for local development.
They must not be presented as personal human playthrough evidence or
as an authenticated estimate of the intended 6–8 minute level duration.
Backwards movement could reflect enemy contact or other mechanics; the
observer does not infer why a character moved.

The existing Stage159 GCC/Clang/ASan+UBSan CI and Stage165 full-route
GCC/Clang/ASan+UBSan CI exercise the changed implementation. The full
browser/game regression and storage-policy tests also remain mandatory.
No raw user frames, files, original art, SDMC saves or records are
uploaded to public GitHub. Private selected evidence stays in the
restricted Google Drive Project Evidence Archive and requires independent
size/SHA-256 readback before it is called archived.
