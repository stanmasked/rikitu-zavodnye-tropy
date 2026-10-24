# Stage174 — cross-check each MAP01 session against its named browser CSV

## Why

Stage173 validates the anonymized manual session log. Stage174 binds each
completed attempt to a distinct, anonymized browser-export filename and
checks its exact SHA-256 plus MAP01 seconds, hits and coins. A hash alone is
not enough to identify an attempt because two distinct exports can have
identical bytes.

## Private input convention

- Each completed attempt has its own file named exactly after its
  pseudonymous session ID: `P001-A1.csv`, `P001-A2.csv`, etc. Renaming the
  downloaded file is allowed; editing or re-saving its bytes is not.
- The manual log records both `source_csv_filename` and
  `source_csv_sha256`. Incomplete attempts leave both blank.
- Keep the manual log and the export directory outside any Git checkout.
  The export directory must contain only the referenced `P###-A#.csv`
  files, with no missing or unreferenced exports.
- Distinct filenames may legitimately have the same SHA-256 when their
  bytes are identical. The tool checks filename plus hash, so it does not
  mistake identical data for reuse of one filename.

## Run command

Use absolute paths in the restricted private working directory:

```sh
python /path/to/repo/tools/crosscheck_map01_playtest_exports.py \
  --sessions /private/evidence/map01-sessions.csv \
  --exports-dir /private/evidence/browser_exports \
  --expected-sessions-sha256 PINNED_SESSION_LOG_SHA256 \
  --output /private/evidence/map01-crosscheck-summary.json
```

The directory should contain files such as `P001-A1.csv` and `P001-A2.csv`,
whose names match the completed session rows. Do not put the manual log or
the output report inside the browser-export directory. The tool rejects
missing, duplicate-name, unlinked, malformed or inconsistent exports.

## Cross-checked fields

For every completed attempt, the tool validates the named export using the
existing Stage163 CSV analyzer and compares its Stage 1 row against the
manual log:

- active elapsed seconds;
- hit count;
- coin count.

The Stage173 session validator also checks the anonymized schema, attempt
type, stop reason, platform observations and numeric bounds. Manual
fragment/checkpoint/respawn/backtrack/platform observations do not exist in
the browser CSV and therefore are **not** independently confirmed.

The aggregate-only JSON contains the session-log size/SHA-256, summary
counts, unique browser-export hashes, compared fields and mismatch count.
It is written with mode 0600, refuses symlinks, Git checkouts and overwrites,
and prints counts/hashes only. A successful cross-check proves consistency
and byte identity only; it does not authenticate human control, browser
runtime or an unmodified client.

## CI and data handling

CI uses synthetic rows only and uploads no artifacts. Tests cover valid
matches, identical-byte exports with distinct filenames, timing/value
mismatches, missing and unlinked files, malformed CSV, hash-pin failures,
symlink/Git-checkout refusal, output permissions and privacy. GitHub Actions
has no Drive credentials.

This tool is not playtest evidence. Issue #5 remains open until real,
anonymized controller runs and their evidence package have been privately
archived and independently read back.
