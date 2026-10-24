# Stage175 — explicit anonymized filenames for human-session exports

Stage174 already cross-checks browser-export SHA-256, MAP01 stage, active
seconds, hits and coins. Stage173 also records a SHA-256 in each completed
manual session. The remaining ambiguity was that hashes were used as the
only link key.

Two genuinely separate exports can contain identical bytes when different
attempts happen to produce the same exported values. Identical SHA-256 is
then correct, not evidence that one file was reused. This stage uses the
private, anonymized filename `P001-A1.csv` as the unique session-to-file
link and retains SHA-256 as the byte-integrity check.

## Schema and cross-check

The manual session schema now ends with:

```csv
source_csv_filename;source_csv_sha256
```

For a completed `P001-A1` session, the filename must be exactly
`P001-A1.csv`; the SHA-256 must match those exact bytes. Incomplete rows
leave both fields blank. The cross-check command must receive one browser
CSV per completed session, with the matching anonymized basename. It rejects
missing files, duplicate filenames, unlinked files, invalid Stage-1 CSVs,
hash mismatch and differences in seconds/hits/coins.

Different filenames may legitimately have the same SHA-256. The aggregate
report counts matched files separately from unique hashes and does not
include filenames or participant IDs.

## Privacy and storage

The private Drive session template now contains both fields. Rename each
browser export's archived copy to its anonymized ID without editing/re-saving
its bytes. Keep the original CSV, manual session log, aggregate JSON and
size/SHA-256 manifest in the restricted Project Evidence Archive. Independently
download and verify byte-identical copies. No real player data or exports
are included in this PR or uploaded by CI.

This is an integrity/linkage improvement, not proof of human provenance and
not a playtime result. MAP01's 360–480 second target still requires actual
anonymized human-controlled controller runs.
