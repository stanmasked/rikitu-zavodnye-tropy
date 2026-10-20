# RIKITU: first Nintendo Switch native probe

This folder contains a minimal text-console NRO smoke application, NOT
the Nintendo Switch port of the 16-level game. It exercises the portable
60-Hz player-loop code in ../native/ using real libnx HID input for
standard Joy-Con / Pro Controller styles.

Controls: D-pad / left stick moves, A or B jumps, R/ZR runs, Minus pauses,
X resets, and Plus exits. The probe uses a neutral console with numeric
coordinates. It contains no approved character art, ROM content,
level assets, music, screenshots, save files or network services.

Build with devkitPro devkitA64 and libnx:

    make -C switch

The Makefile is adapted from the official switchbrew/switch-examples
application template and compiles ONLY main.c, rikitu_rules.c and
rikitu_loop.c. This avoids accidentally linking C99 unit-test entrypoints.
GitHub Actions uses devkitpro/devkita64:20260219 and publishes a build
artifact when successful. A passing compile does not prove that controllers,
timing, or graphics work in a Switch emulator or physical console.

For this preliminary smoke test, a nominal 60-Hz timestep is advanced once
per loop with a sleep request; scheduling and input latency require future
real-device or emulator verification before calling it the final game loop.

Future work: renderable levels, proper frame pacing, all gameplay systems,
audio, storage/save migration, visual character assets (only when authorized),
full game validation, and a separately verified distributable NRO.
