# Stage174 — cross-check manual MAP01 sessions against browser exports

## Why

Stage173 validates the anonymized manual session log and each browser CSV
independently, but its procedure left one human step: compare stage seconds,
hits and coins manually. This stage adds a private local-only cross-check so
those values cannot silently drift apart.

## Use only in the restricted private working directory

Keep the original manual log and each original browser export outside any Git
checkout. Archive original bytes in the restricted Google Drive Project
Evidence Archive first, record size/SHA-256, and independently download the
files to verify byte-identical readback. Do not upload populated CSVs, private
reports, screenshots or playtest captures to GitHub.

Example command, with absolute paths and one `--browser-csv` per completed
attempt:

```sh
python /path/to/repo/tools/crosscheck_map01_playtest_exports.py \
  --sessions /private/evidence/map01-sessions.csv \
  --browser-csv /private/evidence/P001-A1.csv \
  --browser-csv /private/evidence/P001-A2.csv \
  --expected-sessions-sha256 PINNED_SESSION_LOG_SHA256 \
  --output /private/evidence/map01-crosscheck-summary.json
```

Replace the example paths and hash with real values; do not put private data
in the command line beyond local paths. Each completed session has a `source_csv_filename` such as `P001-A1.csv`;
the supplied file's basename must match that field and its exact bytes must
hash to `source_csv_sha256`. Every named export must match exactly one
completed attempt; missing, duplicate filenames or unlinked exports are
rejected. Two different named files are allowed to have identical content
and therefore identical SHA-256 values.

## Cross-checked fields

For each completed attempt, the tool parses the linked browser export with
the existing Stage163 CSV validator and compares its Stage 1 row against the
manual log:

- active elapsed seconds;
- hit count;
- coin count.

The tool also runs the Stage173 session schema validator and preserves the
aggregate summary (completion/incomplete counts, first/repeat medians, stop
reasons, platform discovery/use, hits, coins, respawns and backtracking).
Manual fragment/checkpoint/respawn/backtrack/platform observations do not
exist in the browser CSV and therefore are **not** independently confirmed.

The output is aggregate-only JSON with the session-log size/SHA-256,
unique browser-export hashes, number of matched files, compared field list
and mismatch count. The report omits session IDs and filenames. It is written
with mode 0600, refuses symlinks, Git checkouts and overwrites, and the CLI
prints only counts and hashes. A successful cross-check proves data
consistency and byte identity only; it does not authenticate human control,
browser runtime or an unmodified client.

## CI and data handling

The GitHub workflow uses synthetic rows only and does not upload artifacts.
It tests success, mismatched timing, missing/duplicate/unlinked exports,
malformed CSV, SHA-256 pin failures, symlink/Git-checkout refusal, output
permissions and privacy. GitHub Actions has no Drive credentials.

This tool is not playtest evidence. Issue #5 remains open until real,
anonymized controller runs and their evidence package have been privately
archived and independently read back.
