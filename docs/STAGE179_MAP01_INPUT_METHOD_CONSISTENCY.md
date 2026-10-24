# Stage179 — MAP01 mixed-input diagnostics

## Why

Stage176 freezes the controller timing rule: only controller rows feed the
first/repeat medians, while keyboard and touch rows remain in the overall
aggregate but are excluded from controller timing. Stage178 now guarantees
that repeat attempt IDs have a complete prior attempt sequence.

One remaining interpretation risk is a participant who used keyboard or touch
for A1 and a controller for A2. Their controller repeat is still a valid
controller session, but it is not a same-input first/repeat comparison. This
stage adds aggregate-only diagnostics without silently changing the frozen
Stage176 median rule.

## New aggregate fields

- `session_counts.participants_with_multiple_input_methods`: number of
  pseudonymous participants who used more than one input method across their
  recorded attempts.
- `controller_repeat_input_consistency.controller_repeat_attempts_after_controller_first`:
  controller repeat rows whose A1 was also controller.
- `controller_repeat_input_consistency.controller_repeat_attempts_after_noncontroller_first`:
  controller repeat rows whose A1 used keyboard or touch.

No participant IDs, session IDs or per-row data are emitted. The counts are
diagnostic only. All controller rows continue to contribute to the existing
Stage176 median groups, so the output includes an explicit interpretation note
to review mixed-input sequences before comparing medians.

## Tests and evidence boundary

Synthetic-only tests verify that a mixed-input participant is counted, that
the two repeat-consistency counters add up correctly, and that the frozen
first/repeat median assessment is unchanged. The summary remains aggregate-only
and makes no claim that a person controlled the game. Real controller runs and
independent private Drive readback remain required before issue #5 can close.
