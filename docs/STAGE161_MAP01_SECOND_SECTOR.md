# Stage161 — MAP01 first two sectors by ordinary input

The native C99 diagnostic starts the actual Flower Trail map from the
canonical x=105 position. It uses the production 60 Hz gameplay step and
RIGHT, RUN and JUMP input masks at 30, 60 and 120 display Hz.

The route controller schedules jumps near the first two fragments and
ground gaps while avoiding some enemies. The game engine remains
responsible for movement, damage, collection, respawn and checkpoint state.
No player coordinates are assigned after scene initialization.

The **acceptance goal** is fragment #1 (x=1530), checkpoint #1 (x=2050),
fragment #2 (x=3650) and checkpoint #2 (x=4490). The test prints active
simulation time, damage count and checkpoint samples for each display
rate. These are deterministic *scripted* observations, not natural human
play, not level-duration measurements and not proof of the approved
6–8 minute pacing target.

No private art, raw emulator captures, permanent saves, graphics or
gameplay rules are changed. Any selected real evidence belongs in private
Google Drive with exact byte hash readback; GitHub Actions must not upload
raw evidence. This workflow prints only synthetic counts to the log.
