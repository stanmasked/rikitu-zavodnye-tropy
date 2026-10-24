# Stage174 — cross-check each human session against its exact browser CSV

Stage173 prepared the anonymized manual playtest log and a local-only
aggregate analyzer. This stage closes the manual matching gap: a correctly
shaped SHA-256 string alone is not enough to prove that the logged seconds,
hits and coins came from the referenced browser export.

## Private input convention

- Each completed attempt has a distinct private export file named exactly
  after its pseudonymous session ID: `P001-A1.csv`, `P001-A2.csv`, etc.
  Renaming the exported file is allowed; editing or re-saving its bytes is
  not. Separate files may legitimately have identical SHA-256 if their
  contents are identical.
- The manual session log adds `source_csv_filename` before
  `source_csv_sha256`. Incomplete attempts leave both fields blank.
- Put the manual log and browser exports in dedicated private directories
  outside any Git checkout. The export directory must contain only
  anonymized `P###-A#.csv` files and must contain every completed export
  referenced by the session log, with no unrelated CSVs.

## Validation behavior

The local analyzer imports the existing Stage163 browser CSV validator.
For every completed session it verifies:

1. The named export exists and the filename matches the anonymized session ID.
2. The SHA-256 of the unchanged export matches the manual log.
3. The export contains exactly one stage-1 record.
4. Active seconds, hits and coins match the browser export exactly.
5. The export directory has no unreferenced files and no completed session
   points to a missing export.

The tool then calculates aggregate first-attempt/repeat statistics. It
does not print session IDs or filenames, never writes individual rows to
the report, refuses symlinks/Git checkouts/overwrites, and creates a
mode-0600 JSON summary. It has no network or Google Drive access. The
report explicitly states that integrity does **not** authenticate human
control.

## Run command

Use absolute paths in the restricted private working directory:

```sh
python /path/to/repository/tools/analyze_map01_playtest_sessions.py \
  --input /private/archive/map01_sessions.csv \
  --exports-dir /private/archive/browser_exports \
  --output /private/archive/map01_summary.json \
  --expected-sha256 PINNED_MANUAL_LOG_SHA256
```

The manual CSV, original browser CSVs, report and manifest must be archived
in the restricted Project Evidence Archive with sizes and SHA-256. Independently
download and verify byte-identical copies. Do not place real player data in
the repository or GitHub Actions artifacts.

All tests in this stage use synthetic browser exports and synthetic session
rows only. The 360–480 second MAP01 target remains unverified until genuine
human-controlled controller sessions are collected and reviewed.
