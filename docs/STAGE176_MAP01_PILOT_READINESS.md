# Stage176 — MAP01 controller pilot readiness

## Purpose

Stage175 made the private evidence chain reproducible. Stage176 adds a
controller-only descriptive summary so mixed-input data cannot accidentally
determine the MAP01 pacing result. This is a preregistered interpretation
rule for the small pilot, not a claim that the level is balanced.

## Frozen pilot decision rule

- Target: 360–480 seconds inclusive for a completed ordinary MAP01 run.
- Minimum sample: at least five completed controller first attempts and five
  completed controller repeat attempts. A smaller group yields
  `INSUFFICIENT_COMPLETED_CONTROLLER_RUNS`.
- Only rows whose input method is `controller` contribute to the first/repeat
  pacing medians. Keyboard/touch rows remain counted in aggregate but are
  excluded from the controller timing groups.
- The report also counts participants who changed input method and controller repeat attempts whose A1 used keyboard/touch. The frozen median rule is unchanged, so review these diagnostics before interpreting first-versus-repeat results.
- Incomplete attempts count toward attempt totals and controller stop reasons,
  but their stop-time observations are never treated as completed-run times.
- Repeat attempts must be linked to the same pseudonymous participant as a complete prior attempt sequence (A2 requires A1; A3 requires A1 and A2). Missing attempt rows invalidate the log rather than silently changing the repeat group.
- If both controller medians are inside 360–480 seconds, report
  `PILOT_MEDIANS_WITHIN_TARGET_REVIEW_REQUIRED`.
- If either median is outside the target, report
  `PILOT_MEDIANS_OUTSIDE_TARGET_REVIEW_REQUIRED`.
- Neither result is a release gate or proof of balance. Review the full
  aggregate evidence and the observer notes about fragments, checkpoints,
  falls, backtracking and raised-platform discovery. Do not use inferential
  significance claims for this small descriptive pilot.

## Private local command

Keep the manual session CSV and browser export directory outside all Git
checkouts. The directory must contain only the unchanged named
`P###-A#.csv` exports. Use a new output path and pin the session log hash
from the previously created private manifest:

```sh
python /path/to/repo/tools/analyze_map01_pilot_readiness.py \
  --sessions /private/evidence/map01-sessions.csv \
  --exports-dir /private/evidence/browser_exports \
  --expected-sessions-sha256 PINNED_SESSION_LOG_SHA256 \
  --output /private/evidence/map01-pilot-readiness.json
```

The tool validates the Stage173 session schema, checks every completed
browser export by anonymized filename and exact SHA-256, and compares MAP01
active seconds, hits and coins. It writes aggregate-only JSON with mode 0600,
refuses symlinks, Git checkouts and overwrites, and prints counts/assessment/
hashes only. It does not authenticate human control or browser runtime.

## Data handling

All CI cases are synthetic. No real rows, CSV exports, screenshots, videos,
save files or artifacts are uploaded. For real tests, preserve original bytes
in restricted Drive `01_Private_Originals`, seal sizes/SHA-256 in
`00_Manifest_and_SHA256`, save aggregates in `03_Aggregate_Reports`, CI
summaries in `04_Logs_and_CI`, and stage notes in
`05_Handoff_and_Stage_Notes`. Independently download the archived files and
verify byte-identical readback. A local copy is not an authenticated Drive
readback.

Issue #5 remains open until real controller attempts are archived and
reviewed. Do not mix synthetic fixtures, native scripted timings, keyboard
runs or touch runs into the controller pacing median.
