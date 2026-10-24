# Stage163 — private, offline pacing CSV analysis

The browser game's **↓ Отчёт (CSV)** button exports nine columns in a
UTF-8 BOM, semicolon-delimited file: level, title, stage type, active
seconds, hits, coins, lower/upper target and comparison. See
`game.js:exportStageResults` and `tests/test-export-results.py`.

The validator `tools/analyze_private_pacing_csv.py` consumes an original
export entirely **outside any Git checkout**. It refuses symbolic-link
components and overwrites, has a 64-KiB input bound, requires exactly the
expected nine-column format and up to 16 unique ascending levels, and checks:

- level 1–16 and odd/even ordinary/miniboss classification;
- nonnegative finite base-ten seconds, nonnegative hits, and 0–90 coins;
- 360–480 seconds for an ordinary stage, 480–600 with a miniboss;
- independently recomputed target verdict and distance outside its range;
- exact input SHA-256 and size, optionally against a pre-pinned digest.

On success, an **explicitly selected local private JSON** is created with
mode 0600 and exclusive create (no overwrite). Its contents include
stage-by-stage timing, damage and coin counts; it must never be placed in
the public GitHub repository or attached to public Actions artifacts.
Console output contains *only* aggregate stage count and SHA-256 of the
original and report, never private filenames or stage records.

## Example: run locally in an already restricted private folder

```bash
python tools/analyze_private_pacing_csv.py \
  --input /private/RIKITU/RIKITU_stage_results.csv \
  --output /private/RIKITU/pacing-analysis-stage163.json \
  --provenance self-reported-manual \
  --expected-sha256 <PREVIOUSLY_PINNED_64_HEX_SHA256>
```

All paths are illustrative and should be replaced with actual **absolute
private paths**. The destination directory must exist, but the output
file must **not** already exist. To regenerate, select another versioned
filename rather than overwriting. An input digest is only an integrity pin;
it does **not** prove who played the game. The `--expected-sha256` flag
is optional but recommended when an independent source digest exists.

Supported provenance labels are `unverified` (default),
`self-reported-manual`, and `scripted`. **All outputs are explicitly marked
`provenance_verified: false`, including self-reported manual results.**
CSV can be edited or produced through developer teleport; it cannot itself
prove human-controlled completion, playability, a first-time player's
behavior or achievement of 6–8 minutes. Full-run evidence needs an
independently recorded human playthrough, controller settings and review.

## Mandatory archival separation

The original CSV and derived analysis are private evidence. They belong in
restricted Google Drive Project Evidence Archive (appropriate `00`–`07`
areas), accompanied by byte counts, SHA-256, source revision and transfer
notes. An authenticated private upload **followed by independent byte-exact
Drive download/readback** is required before calling them archived.
Stage157/158's manifest and ZIP verifier may be used after transfer.
GitHub Actions is **not** automatically authenticated to personal Google
Drive; this workflow intentionally has no uploads. CI only tests invented
fixtures in temporary directories and never logs actual player records.

## Review sequence

Compare repeated attempts on MAP01 using normal input without developer
teleports. Inspect first-time route choice, missed fragments, checkpoint
restarts, health loss, optional coins and time spent locating the exit.
Only then consider changes to optional detours, signposting, collectibles
and mechanics. Do not change approved canonical RIKITU character art;
Stage162's 28.70-second automated route does **not** prove the 6–8-minute
player pacing target.
