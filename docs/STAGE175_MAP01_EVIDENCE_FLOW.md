# Stage175 — end-to-end MAP01 evidence flow (synthetic-only)

## Goal

Stage173/174 validate the manual session log and browser exports. The
existing Stage157/158 tools seal a size/SHA-256 manifest, create an
uncompressed deterministic private evidence ZIP and verify an independent
copy. Stage175 joins these components into one regression test so the first
real human playtest can follow the established evidence-storage rule without
inventing a new pipeline.

## Synthetic test flow

1. Generate a synthetic MAP01 browser CSV and anonymized manual session log.
2. Run `crosscheck_map01_playtest_exports.py`; verify the completed session's
   anonymized filename plus exact browser CSV SHA-256 and Stage 1 seconds/hits/coins.
   Distinct filenames may share identical hashes if their bytes match; the manifest
   preserves both paths. Reusing one filename/file for two attempts is rejected.
3. Place only the synthetic source CSVs and aggregate cross-check report in
   a temporary evidence directory.
4. Seal the directory with `private_evidence_readback.py seal`, pinning
   every relative filename, byte size and SHA-256 in a manifest outside the
   evidence tree.
5. Pack the unchanged source into a deterministic `RIKITU_*.zip` using
   `private_evidence_bundle.py`; verify ZIP member list, receipt digest,
   payload sizes and hashes.
6. Copy both payload files and the ZIP into independent temporary
   readback directories. Verify exact byte identity against the pinned
   manifest and bundle digest; tampering after sealing must fail.

The test uses only generated rows and marker bytes. No real participant
records, browser exports, images, videos, saves or GitHub artifacts are
included or uploaded. The readback result explicitly remains
`BYTE_MATCH_ONLY_REQUIRES_AUTHENTICATED_DRIVE_FETCH`; a local copy is not
claimed to be an authenticated Google Drive download.

## Production boundary

No gameplay, canonical MAP01 geometry, art, save format or browser export
schema changes. This test proves the private analysis-to-manifest/bundle
pipeline is internally consistent, not that a human played the game or that
the level meets its 360–480-second target.

## Real evidence handoff

For actual playtests, store original browser CSVs and the anonymized manual
log in the restricted RIKITU Project Evidence Archive under
`01_Private_Originals`; store the sealed manifest and pinned hashes under
`00_Manifest_and_SHA256`; store aggregate reports under
`03_Aggregate_Reports`; and store CI summaries/handoff notes under
`04_Logs_and_CI` and `05_Handoff_and_Stage_Notes`. Independently fetch
the archived bytes and verify readback before interpreting results. Never
commit private rows or uploads to public GitHub. Issue #5 remains open until
real controller sessions are archived and reviewed.
