# Stage169 — ordinary-input alternative coin route on MAP01

The base Stage162 route already proves that RIKITU can collect all
**three mandatory fragments**, activate **three checkpoints** and touch
the Flower Trail exit in 28.70 active simulation seconds without a
developer teleport. It collects only **27 of 90** optional coins.

This Stage169 regression experiments with **four extra jump commands**
inside the unchanged 7900px canonical MAP01. They are issued on even
native 60-Hz physics-tick boundaries so the same input timeline can be
sampled at 30/60/120 display Hz. The test uses ordinary production
RIGHT/RUN/JUMP input and **does not change** any world geometry,
coin placement, collision tolerance, required fragments, stage goal or
game physics. A route must prove both:

- All mandatory fragments, checkpoints and the exit are still reachable.
- This optional route collects **more than 27 coins** without requiring
  all 90, and results agree across frame rates and GCC/Clang/ASan+UBSan.

If this new route cannot meet these conditions, Stage169 remains Draft
and the controller policy should be diagnosed, not manipulated through
test teleports. We must distinguish **additional optional rewards**
from genuinely interesting exploration: increasing a coin counter
alone does not establish satisfying choices or 6–8 minutes of play.

The test is a deterministic input script, **not** human gameplay
evidence or a balanced 6–8 minute normal-level duration. Player tests
remain necessary for discovery, safe revisiting, readability, challenge
and the four sections' actual time demands. Existing approved
characters and artwork remain unchanged.

Only code, test documentation and aggregate log lines enter public
GitHub Actions. No raw video, game saves, graphics, original assets
or private player data are uploaded. Any retained genuine evidence
belongs in restricted private Google Drive with independent byte-exact
size/SHA-256 readback before declaring it archived.
