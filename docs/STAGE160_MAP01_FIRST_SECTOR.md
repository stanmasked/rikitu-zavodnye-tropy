# Stage160 — MAP01 opening route, real native input

Goal: check whether an ordinary native input controller can reach the **first
mandatory fragment**, cross the first 75-pixel gap in the existing Flower
Trail geometry, then reach the first checkpoint at x=2050. Do not use debug
teleports or direct player-coordinate edits after canonical spawn.

The isolated C99 regression drives ordinary RIGHT, RUN and JUMP masks
through the existing production physics/collectible engine at 30/60/120Hz.
All tested render frame rates must collect the same first fragment and
cross the same first checkpoint. Times are approximate **active simulation
seconds** recorded from native gameplay, not stopwatch or human sessions.
A predeclared one-jump input policy is test automation, not a player
judgment about difficulty.

Interpretation: this test covers only the first sector. It cannot prove
that all three fragments, safe return routes, the MAP01 finish gate, or the
6–8 minute pacing target work for a player. No geometry, physics, approved
character art, Switch save records, or on-screen graphics are changed.

Storage policy: CI publishes only pass/fail and synthetic aggregate counts
to job logs. No guest frames, raw saves, recordings, approval assets, or
real user results are uploaded to GitHub. If any actual capture is
retained it must go into private Google Drive with independent SHA-256
readback. Authenticated GitHub-to-Drive upload remains a separate task.
