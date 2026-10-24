# Stage179 — MAP01 matched-controller cohort audit

## Purpose

Add a privacy-preserving diagnostic to the MAP01 pilot readiness report so the team can distinguish the frozen controller first/repeat group summaries from the subset of participants who have a comparable completed controller pair.

## Pair definition

The report selects at most one pair per pseudonymous participant:

1. A1 must be completed with `input_method=controller`.
2. The repeat must be a completed controller attempt.
3. Every recorded attempt from A1 through the selected repeat must use controller input.
4. The selected repeat is the earliest attempt satisfying these rules. Stage178 independently rejects skipped or orphan attempt numbers.

Incomplete and technical attempts remain in the session log and continue to establish the attempt sequence, but an incomplete A1 cannot form a completed pair. A keyboard/touch attempt before a repeat breaks the controller-only sequence for this paired diagnostic.

## Added aggregate fields

- `paired_controller_cohort.matched_participants`
- `paired_controller_cohort.first_median_seconds`
- `paired_controller_cohort.repeat_median_seconds`
- `paired_controller_cohort.median_repeat_minus_first_seconds`
- `paired_controller_cohort.affects_frozen_assessment` is always `false`

The paired difference is repeat time minus first time; a negative value means the repeat was faster. Pseudonymous participant IDs are used only in memory and are never included in the report.

## Frozen decision rule remains unchanged

The existing `pacing_assessment` still uses all completed controller first attempts and all completed controller repeat attempts, requiring at least five completed runs in each group. The new paired metrics are supplemental and do not change that threshold, target range, or assessment. They are descriptive only and do not authenticate human control or prove that MAP01 is balanced.

## Validation

The synthetic-only regression suite checks:
- five clean controller pairs and their aggregate medians;
- exclusion from the paired cohort when A1 is incomplete;
- exclusion when a participant's A1 or intervening attempt uses keyboard/touch;
- continued use of the frozen independent-group readiness assessment;
- privacy of the aggregate report.

No real participant rows, browser exports, screenshots, captures, or gameplay measurements are used in CI.

## Next step

Run the real controller pilot only when human attempts are available. Preserve original files unchanged in the private Drive archive, record size and SHA-256 values in a new evidence manifest, independently download and verify the stored copies, then run the offline analyzers on private local copies outside all Git checkouts. Synthetic results must never be mixed with human timing data.
