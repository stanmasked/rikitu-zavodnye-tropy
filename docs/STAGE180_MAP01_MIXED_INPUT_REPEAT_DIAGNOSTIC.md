# Stage180 — MAP01 mixed-input repeat diagnostics

## Purpose

Stage179 added the matched controller cohort and aggregate input-method checks.
Review of the current report identified one remaining interpretation gap:
the report counted controller repeats after a non-controller A1, but did not
separately count controller repeats preceded by a non-controller attempt later
in the sequence.

## Change

The readiness report schema advances to
`rikitu-map01-controller-pilot-readiness-v3`. The
`controller_repeat_input_consistency` object now includes:

- `controller_repeat_attempts_after_controller_first`: controller repeat rows
  for participants whose A1 used controller input.
- `controller_repeat_attempts_after_noncontroller_first`: controller repeat
  rows for participants whose A1 used keyboard or touch.
- `controller_repeat_attempts_after_any_noncontroller_attempt`: controller
  repeat rows with at least one earlier keyboard/touch attempt for the same
  participant, regardless of whether that earlier attempt was complete.

The new count is attempt-order-aware. A keyboard A2 followed by controller A3
counts as one repeat after a prior non-controller attempt. A keyboard A3 after
a completed controller A2 does not retroactively change the classification of
A2. A repeat after non-controller A1 contributes to both relevant diagnostics.

## Frozen pacing rule unchanged

The Stage176 pacing assessment still includes all completed controller A1
rows in the first-attempt group and all completed controller repeat rows in the
repeat group, with at least five completed rows in each group. The new field
is descriptive only. It does not remove rows from the frozen medians and does
not change the matched-cohort rule introduced in Stage179.

The paired controller cohort still requires a completed controller A1 and the
earliest completed controller repeat before any non-controller attempt in
that participant's sequence. The mixed-input diagnostics help interpret why
the paired cohort can differ from the frozen independent groups.

## Regression coverage

Synthetic-only tests verify:
- clean controller data reports zero controller repeats after any earlier
  non-controller attempt;
- controller A1, an incomplete/stopped keyboard A2, and controller A3 reports one such controller repeat;
- a keyboard A3 after a completed controller A2 does not retroactively count
  the earlier controller repeat;
- the frozen repeat group and paired cohort are unchanged;
- the aggregate report remains free of session IDs and private paths.

## Privacy and evidence boundary

No real participant rows, browser exports, captures, or gameplay timing
measurements are used. CI uses synthetic fixtures only. The MAP01 pilot
archive remains empty until real human-controlled attempts are recorded,
saved in restricted Google Drive, and independently downloaded for exact
byte/SHA-256 verification. Issue #5 remains open until the real pilot evidence
is reviewed. A green CI run does not authenticate human control or establish
that MAP01 is balanced.
