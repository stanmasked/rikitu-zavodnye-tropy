# Stage165 — full MAP01 runtime-telemetry cross-check

This is an asset-free, scripted physics-input **integration regression**, not a
human speedrun or 6–8 minute target validation.

Stage159 introduced the production in-memory observer used by the Switch
graphical loop. Stage162 drove the actual native MAP01 movement, pickups,
checkpoints and exit without teleporting. This stage connects **both
production modules** in a single deterministic C99 simulation.

Starting from the normal Flower Trail spawn, the harness feeds RIGHT, RUN
and JUMP through `rikitu_demo_advance()` with matched 60 Hz physics-input
boundaries, sampled at 30, 60 and 120 display Hz. After 1180 physics ticks,
it pauses the game loop for **two wall seconds** and resumes without editing
the character, item flags or progress. Every normal and paused frame is
passed to `rikitu_map01_telemetry_observe()`.

## Acceptance

- Precisely 3 newly acquired fragments, 3 new checkpoints, 27 newly
  collected coins, 0 damage events and 1 completion in telemetry.
- The observer's simulation tick count agrees with the game loop and its
  observed frame count agrees with the executed harness frames.
- Pause runs zero physics ticks, keeps character/progress/active-time frozen,
  and increments only paused-render-frame diagnostics.
- Checkpoint active-time samples agree with directly observed game loop
  splits. Identical gameplay counters and times at 30/60/120 Hz, within
  the already approved float/render tolerance.
- GCC, Clang, and Clang ASan+UBSan all pass, with no gameplay, save format,
  art or platform runtime changes.

## Interpretation and custody

The observed active route is about 28.7 seconds plus two seconds of
excluded pause wall time. This is a **scripted optimized full-level path**:
it does not substantiate a first-time player's 6–8 minute experience.
Issues #5 and #19 remain open for realistic navigation, optional goals,
mistakes, checkpoint retries and human playtest samples.

The workflow uses synthetic game state created at the canonical spawn
inside ephemeral CI memory. Logs may show aggregate test facts; no private
user report, original art, raw NeXium capture, SDMC bytes or video is
published or uploaded. If real session evidence is later selected for
permanent retention, it must be stored in the restricted Google Drive
Project Evidence Archive with SHA-256/size manifest and independent
byte-identical readback. This workflow does not have Drive access.
