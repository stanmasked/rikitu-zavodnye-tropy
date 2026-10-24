# Stage164 — browser-export-to-private-analyzer contract

This is a **synthetic-only cross-module integration test**, not a player
playthrough and not a real-run timing measurement.

The existing browser export test (`tests/test-export-results.py`)
reconstructs a deliberately invented local-storage state containing two
completed levels (402.5 and 531.5 seconds). Playwright clicks the real
`↓ Отчёт (CSV)` button and reads the browser's downloaded CSV bytes.

Stage164 uses those **actual downloaded bytes**, not a second mock CSV,
as the input to Stage163's `analyse()` function. The test asserts:

- the original CSV SHA-256 and byte count match the parser's receipt;
- both rows are accepted with correct ordinary/boss target windows;
- two entries fall within the scripted synthetic target ranges;
- total hits and coins are 3 and 150, respectively;
- the input is marked `scripted`, **never provenance-verified**;
- a modified verdict is rejected without changing other row fields;
- the existing new-campaign reset still clears the saved CSV journal.

The Playwright download is read on the CI runner and not uploaded to
public GitHub Actions artifacts. No real user CSV, original artwork,
screenshots, emulator captures or permanent save files are used.

Passing this test establishes only compatibility between the real
browser serializer and the offline CSV validator. It does not prove an
authentic user run, the target **6–8 minutes** for MAP01, or authenticated
upload into private Google Drive. Actual evidence still requires a
restricted private transfer with an independent SHA-256 readback.

No game logic, save format, canonical characters, display graphics,
real-world player data or Switch NRO packaging is changed in Stage164.
