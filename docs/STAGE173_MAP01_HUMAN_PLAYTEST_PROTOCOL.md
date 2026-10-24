# Stage173 — MAP01 human-controlled playtest and private evidence protocol

## Purpose

Stage170–172 establish only **scripted physics feasibility**. They do not
prove that a first-time player can discover the current Flower Trail routes
or that MAP01 takes 6–8 minutes. This protocol prepares the first genuine
controller-controlled measurement without changing gameplay.

The production game currently exports one completed-stage row per campaign
for each stage. The browser CSV contains stage, name, type, active seconds,
hits, coins, target range and comparison. It does **not** record individual
respawns, backtracking, route discovery or why an incomplete attempt ended.
The manual session log adds those observations.

The two-tier Stage171/172 layout exists only in native test fixtures. Do not
present it as part of the playable production build. The current live
candidate to observe is the existing raised platform and its optional coins.

## Pilot plan

- Run at least **5 first-attempt** MAP01 sessions with normal controller
  input and no route hints. Prefer different first-time players.
- Run at least **5 repeat attempts** after the first attempts. Use the same
  input method for comparable timings; keep controller sessions separate
  from keyboard/touch experiments.
- Give only the neutral goal: “Find the three fragments and reach the exit;
  coins are optional.” Do not coach the route, tell players where the
  raised platform is, or request they collect all 90 coins.
- Do not use devtools, debug state, scripted input, teleport, save edits or
  test-only geometry. A run is human evidence only if a person controls the
  game throughout.
- Do not stop at 5 if the results are inconclusive. This is an exploratory
  pilot, not a statistically representative study.

## Per-attempt procedure

1. Start a fresh campaign before **each** attempt. The browser stores only
   the first completion record for a stage in that campaign; resetting
   clears the record journal. Do not combine repeat attempts into one CSV.
2. Start the timer when active play begins. Pauses should be excluded from
   active time; note unusually long pauses separately in private notes.
3. Observe without prompting. Record the number of fragments/checkpoints,
   hits, actual respawns, backtrack/return events, and whether the player
   noticed and used the existing raised platform. Distinguish “hit” from
   “respawn”; they are not equivalent.
4. On completion, export `RIKITU_stage_results.csv`. Preserve the original
   bytes; do not edit/re-save it in a spreadsheet. Rename the archived copy
   to the anonymized session ID, e.g. `P001-A1.csv`; renaming must not alter
   its bytes. Record the filename and SHA-256 in the session log. The browser
   CSV is evidence of an export, not proof by itself of human control.
5. For an incomplete attempt, record the HUD elapsed time at stopping and
   choose an outcome/stop reason. Leave both export filename and hash blank.
6. Ask only after the attempt: Was the goal clear? Was any required fragment
   hard to find? Did you notice the raised platform? Did you know how to
   recover after a fall? What felt repetitive or unfair? Store answers only
   as anonymized categorical notes; do not collect names, email, voice or
   video unless separately consented.

## Manual session CSV schema

Use UTF-8 with BOM optional, semicolon delimiters, and the exact header below.
Keep the populated file **outside the Git checkout** and only in restricted
private storage.

```csv
session_id;attempt_type;input_method;outcome;elapsed_seconds;hits;coins;fragments;checkpoints;respawns;backtrack_count;raised_platform_seen;raised_platform_used;stop_reason;source_csv_filename;source_csv_sha256
```

- `session_id`: anonymized `P001-A1`, `P001-A2`, etc.; no real names.
- `attempt_type`: `first` for A1; `repeat` for A2 and later.
- `input_method`: `controller`, `keyboard` or `touch`.
- `outcome`: `completed`, `stopped`, `abandoned` or `technical_issue`.
- `elapsed_seconds`: completed active seconds from browser CSV; for an
  incomplete attempt, elapsed active seconds at stopping (not a completion).
- `hits`, `coins`, `fragments`, `checkpoints`: values from the export
  where available; for incomplete attempts, values observed at stop.
- `respawns`: actual returns to checkpoint, counted manually.
- `backtrack_count`: distinct intentional-looking return episodes, not
  raw distance. Mark observations as uncertain when intent is unclear.
- `raised_platform_seen/used`: `yes`, `no` or `uncertain`.
- `stop_reason`: `none`, `missed_fragment`, `gap_fall`, `lost_route`,
  `repeated_failure`, `quit`, `technical_issue` or `other`.
- `source_csv_filename`: exact anonymized filename `P###-A#.csv` for a
  completed attempt; it must match `session_id`. Blank for incomplete runs.
- `source_csv_sha256`: hash of the exact browser CSV bytes; blank for
  incomplete runs. Each attempt needs a separately named file. Two exports
  may legitimately have identical hashes if their bytes match; distinct
  filenames keep those attempts separate, while a reused file is rejected.

## Private analysis

1. Archive each original browser CSV and the manual session log in the
   restricted Google Drive Project Evidence Archive. Preserve file sizes
   and SHA-256 hashes in the manifest, then download an independent copy
   and confirm byte-identical readback. Do not upload raw exports or
   session rows to public GitHub. GitHub Actions does not have Drive
   credentials.
2. Put the unchanged `P###-A#.csv` exports in a dedicated private export
   directory that contains no unrelated files, outside any Git checkout.
   Run `tools/analyze_private_pacing_csv.py` on each original export,
   pinning its SHA-256.
3. Run `tools/analyze_map01_playtest_sessions.py` with `--input`,
   `--exports-dir` and `--output`, then run
   `tools/crosscheck_map01_playtest_exports.py` with `--sessions`,
   `--exports-dir` and `--output`. The tools link each attempt by both
   anonymized filename and SHA-256, compare MAP01 seconds/hits/coins, and
   allow identical hashes only for distinct named files. They refuse Git
   checkouts/overwrites, write mode-0600 reports, print aggregates/hashes
   only, and never upload anything.
4. Archive both summaries, their SHA-256 values and the input hashes beside
   the originals. Verify readback of each summary as well as the raw files.

## Interpretation and exit gate

Ordinary MAP01 target remains **360–480 active seconds**. Report first
attempts separately from repeat attempts; do not average scripted native
fixtures with human sessions. Incomplete attempts are not completion-time
samples. Review medians, completion rates, hits, respawns, required-fragment
misses, return trips and platform discovery together. If times are below
target, improve meaningful traversal/exploration rather than forcing all
90 coins or adding a hard timer.

Do not close issue #5 or claim the level is balanced until real, anonymized
controller runs are archived and reviewed. This protocol and its synthetic
tests are not themselves playtest evidence.
