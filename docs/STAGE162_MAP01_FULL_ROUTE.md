# Stage162 — full MAP01 scripted input-only regression

The goal is a native, reproducible full Flower Trail route using **only**
controller button masks (RIGHT, RUN, JUMP). The production C99 engine,
collision/pickups, damage/respawn and fixed 60 Hz physics drive all changes.
No direct assignment of player coordinates or collectible flags is made
after the canonical scene initialization.

Check all three mandatory fragments (x=1530, 3650 and 6870), all three
checkpoints (x=2050, 4490, 6150), the exit gate, and frame-rate parity
at 30 / 60 / 120 display Hz. Record deterministic per-checkpoint active
times, damage and coin counts. All output is synthetic aggregate test
data (CI logs only). No game asset, artwork, recording, private save,
raw emulator screenshot, or personal session is uploaded.

**Interpretation:** A scripted, optimized route is intentionally not a
natural first-time human playthrough. Its duration is a lower-end
benchmark only, not confirmation of the approved **6–8 minute** ordinary
level pacing. If the full-route test passes very quickly, the next phase
should evaluate exploration, risk, detours and subjective playability with
normal player input, then improve layout where necessary. Do not alter
the existing approved canonical character art or claim an approved
rendered MAP01 background has been integrated.

Long-term archives require restricted private Drive storage and
independent byte-identical size/SHA-256 readback. No automatic GitHub
Actions-to-Google Drive credential or uploader is assumed.
