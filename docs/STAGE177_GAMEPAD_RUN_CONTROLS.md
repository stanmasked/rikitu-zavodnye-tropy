# Stage177 — gamepad run control for MAP01 human pilot

## Problem

The browser prototype already supported gamepad movement, jump and Start/pause,
but running speed was bound only to the keyboard's left Shift key. A controller
pilot could move through the level but could not use the same walk/run choice
as a keyboard player. The help text also did not explain the actual mapping.

## Changes

- Left analog stick and standard D-pad left/right control movement.
- Standard gamepad A/B buttons (indices 0/1) trigger jump.
- Hold RB/R1 (standard button index 5) to run at the existing run speed.
- Start (button index 9) starts the title screen, advances between levels/menus, and toggles pause during play.
- Keyboard movement and Shift-running remain unchanged.
- Regenerated `RIKITU_play_now.html` from `index.html`, `style.css`,
  `game-core.js` and `game.js`; source/standalone parity is enforced by
  `scripts/build_singlefile.py --check`.
- Updated the movement parity harness for the new gamepad-run state and added
  `tests/test-gamepad-controls.py`, which injects a synthetic standard Gamepad
  API object and verifies D-pad motion, A-button jump, analog walk versus RB
  run speed, Start-only launch and Start/pause timer behavior.

## Evidence boundary

The synthetic Gamepad API test is a software regression only. It is not a
human-controlled playtest, does not prove physical-controller compatibility
on every browser/OS, and is not pacing evidence. Issue #5 remains open until
real controller sessions are recorded and privately archived under the
Stage173–176 protocol.

No level geometry, fragment placement, coin placement, save format, or native
C99 rules were changed. The only runtime behavior addition is the existing
run speed becoming reachable through a standard gamepad shoulder button.
