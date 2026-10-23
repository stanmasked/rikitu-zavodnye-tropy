# RIKITU CI evidence retention — Stage145

**Scope:** Public GitHub workflow artifacts. This document does not grant approval to publish game artwork, original files, SDMC saves, screenshots, traces, or other private materials.

## Default public CI rules

- Preserve the existing game and emulator tests, their exit codes, and synthetic assertions.
- Raw guest screenshots, SDMC bytes, frame images, and pixel-accurate renders must **not** be uploaded to public Actions artifacts. Any runner-local output is temporary diagnostic material, **not** confirmed archival evidence.
- A build artifact is uploaded only if a downstream job in the **same workflow** needs it. Those eight NRO transfers use `retention-days: 1` and a single explicit `.nro` path.
- Two standalone build jobs keep validating their NROs, but no longer upload binaries to public artifacts. The pacing-audit workflow likewise keeps validating its synthetic geometry and no longer uploads its intermediate JSON report.
- `tests/test_ci_artifact_policy.py` scans **all** workflow YAML files. Only seven audited workflows may declare uploads, limited to the eight exact required NRO handoffs; every other workflow must declare no uploads. The storage-policy workflow runs on changes to any workflow file.

## Required private archival gate

The permanent project archive is a private Google Drive **Project Evidence Archive** with eight areas `00`–`07`. Immutable originals, approved source art, raw captures, generated builds, and reports must remain in their prescribed private areas. A public workflow run is **not** an archival substitute.

For each item selected for permanent retention: record source identity, exact original byte count, SHA-256 and generation tool/revision; transfer to the appropriate private Drive location; read the copy back and independently compare size and SHA-256; then mark archived. Never claim an item archived when only runner-local or Google Docs text-normalized content exists.

No automatic private transfer is currently configured in these seven public workflows. If a run creates raw evidence that must be retained, run that evidence collection under an **authenticated private archive workflow** instead. Do not silently treat a public CI run's expired runner files as a preserved research record.

Deleting historical Actions artifacts is **separate**, requires an allowlist of exact artifact IDs, and may occur only after each item has a verified private byte-exact copy. Do not generalize the one-time Stage142 cleanup.

## Out of scope

This change does not alter the game binary, save formats, game mechanics, approved character designs, or NeXium test logic. The additional `ci.yml` real-control PNG upload is handled separately in Draft PRs #118/#119; it must also be removed before public-retention compliance is complete.
