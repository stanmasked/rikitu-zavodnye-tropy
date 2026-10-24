# Stage159 — native MAP01 ordinary-input timing baseline

The existing 16-level platformer has a **target** of 6–8 minutes per
ordinary level, and 8–10 minutes with a miniboss. That target is **not**
proven by mathematical world-width / player-speed division.

Stage159 provides a small reproducible measurement of the **actual
native input loop** on the real MAP01 «Цветочная тропа» geometry,
starting at its normal x=105 spawn. It does not move the character
by assigning coordinates after initialization and does not use the
test-teleport API.

The synthetic-only C99 test drives real 60-Hz physics with ordinary
`RIGHT`, `RUN`, `JUMP`, `PAUSE` control state for:

- 30 / 60 / 120 displayed frames per second, verifying they integrate
  to the same simulation ticks, one-second elapsed time, position,
  collected coins and health;
- one-second normal walking vs running, checking approximately 225 vs
  265 native pixels moved, not counting interactive detours;
- a paused three-second interval, verifying no accumulated gameplay
  time or position drift, followed by normal resumption;
- an actual airborne jump command from solid ground with no teleport,
  verifying its effect in native physics.

The CI prints only synthetic movement distances and pass/fail.
It uses no real player telemetry, raw pixel capture, character source,
screenshots, extracted game data or private storage credentials.

**Interpretation:** This is a baseline input correctness/physics
sanity test, **not** a successful full MAP01 route or an assertion that
6–8 minute level duration has been achieved. Gaps, enemies, mandatory
fragment collection, secrets, backtracking, health losses and Moki
shop transitions must still be measured through complete playthroughs
with real controller inputs. Issues #5 and #19 remain open until those
full-route timing and playability gates are verified.
