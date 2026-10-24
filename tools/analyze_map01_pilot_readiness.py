#!/usr/bin/env python3
"""Stage176: privacy-preserving readiness summary for the MAP01 controller pilot.

Offline only. Requires exact browser-export cross-checks and reports only
aggregate descriptive statistics. It does not authenticate human play or
declare the level balanced.
"""
import argparse
import csv
from decimal import Decimal
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import sys
from statistics import median

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location(
    'rikitu_map01_sessions_stage176', HERE / 'analyze_map01_playtest_sessions.py')
if spec is None or spec.loader is None:
    raise RuntimeError('Stage173 session analyzer unavailable')
SESSIONS = importlib.util.module_from_spec(spec)
spec.loader.exec_module(SESSIONS)

MIN_COMPLETED_PER_GROUP = 5
TARGET_LOW = Decimal('360')
TARGET_HIGH = Decimal('480')
MAX_BYTES = SESSIONS.MAX_BYTES


def _safe_absolute(value):
    path = Path(value)
    if not path.is_absolute():
        raise ValueError('all private paths must be absolute')
    return SESSIONS._safe_path(path)


def _median(values):
    if not values:
        return None
    return str(Decimal(str(median(values))).quantize(Decimal('0.1')))


def _range(values):
    if not values:
        return {'min': None, 'max': None}
    return {'min': str(min(values).quantize(Decimal('0.1'))),
            'max': str(max(values).quantize(Decimal('0.1')))}


def build_summary(session_bytes, exports):
    """Validate the log and exports, then summarize only controller timing."""
    validated = SESSIONS.analyse(session_bytes, exports)
    text = session_bytes.decode('utf-8-sig', errors='strict')
    rows = list(csv.reader(io.StringIO(text, newline=''), delimiter=';', strict=True))
    sessions = []
    for row in rows[1:]:
        session_id = row[0]
        participant_id, attempt_suffix = session_id.rsplit('-A', 1)
        sessions.append({
            'participant_id': participant_id,
            'attempt_number': int(attempt_suffix),
            'attempt_type': row[1],
            'input_method': row[2],
            'outcome': row[3],
            'elapsed_seconds': SESSIONS._seconds(row[4]),
            'stop_reason': row[13],
        })

    input_method_counts = {}
    for item in sessions:
        method = item['input_method']
        input_method_counts[method] = input_method_counts.get(method, 0) + 1
    controller = [item for item in sessions if item['input_method'] == 'controller']
    stop_reasons = {}
    for item in controller:
        if item['stop_reason'] != 'none':
            reason = item['stop_reason']
            stop_reasons[reason] = stop_reasons.get(reason, 0) + 1

    groups = {}
    for attempt_type in ('first', 'repeat'):
        attempts = [item for item in controller if item['attempt_type'] == attempt_type]
        completed = [item for item in attempts if item['outcome'] == 'completed']
        times = [item['elapsed_seconds'] for item in completed]
        in_target = sum(TARGET_LOW <= value <= TARGET_HIGH for value in times)
        groups[attempt_type] = {
            'attempts': len(attempts),
            'completed': len(completed),
            'incomplete': len(attempts) - len(completed),
            'completed_median_seconds': _median(times),
            'completed_range_seconds': _range(times),
            'completed_within_360_480_seconds': in_target,
            'completed_outside_360_480_seconds': len(times) - in_target,
        }

    first = groups['first']
    repeat = groups['repeat']
    enough = (first['completed'] >= MIN_COMPLETED_PER_GROUP and
              repeat['completed'] >= MIN_COMPLETED_PER_GROUP)
    if not enough:
        assessment = 'INSUFFICIENT_COMPLETED_CONTROLLER_RUNS'
    else:
        first_median = Decimal(first['completed_median_seconds'])
        repeat_median = Decimal(repeat['completed_median_seconds'])
        if (TARGET_LOW <= first_median <= TARGET_HIGH and
                TARGET_LOW <= repeat_median <= TARGET_HIGH):
            assessment = 'PILOT_MEDIANS_WITHIN_TARGET_REVIEW_REQUIRED'
        else:
            assessment = 'PILOT_MEDIANS_OUTSIDE_TARGET_REVIEW_REQUIRED'

    # Supplemental paired audit: one pair per participant, using completed
    # controller A1 and the earliest completed repeat before any non-controller
    # attempt. This is descriptive only and does not change the frozen rule.
    attempts_by_participant = {}
    for item in sessions:
        attempts_by_participant.setdefault(item['participant_id'], []).append(item)

    paired_first_times = []
    paired_repeat_times = []
    for participant_attempts in attempts_by_participant.values():
        participant_attempts.sort(key=lambda item: item['attempt_number'])
        first_attempt = participant_attempts[0]
        if (first_attempt['attempt_number'] != 1 or
                first_attempt['input_method'] != 'controller' or
                first_attempt['outcome'] != 'completed'):
            continue

        controller_prefix = True
        for repeat_attempt in participant_attempts[1:]:
            if repeat_attempt['input_method'] != 'controller':
                controller_prefix = False
            if (controller_prefix and repeat_attempt['attempt_type'] == 'repeat' and
                    repeat_attempt['outcome'] == 'completed'):
                paired_first_times.append(first_attempt['elapsed_seconds'])
                paired_repeat_times.append(repeat_attempt['elapsed_seconds'])
                break

    paired_deltas = [
        repeat_time - first_time
        for first_time, repeat_time in zip(paired_first_times, paired_repeat_times)
    ]

    return {
        'schema': 'rikitu-map01-controller-pilot-readiness-v2',
        'provenance_verified': False,
        'interpretation': 'Descriptive pilot summary only; this does not authenticate human control or establish that the level is balanced.',
        'browser_exports_cross_checked': True,
        'browser_exports_matched': validated['linked_browser_exports_verified'],
        'minimum_completed_controller_runs_per_group': MIN_COMPLETED_PER_GROUP,
        'target_seconds_inclusive': {'low': 360, 'high': 480},
        'session_counts': {
            'all_sessions': len(sessions),
            'controller_sessions': len(controller),
            'non_controller_sessions_excluded_from_timing': len(sessions) - len(controller),
            'input_method_counts': dict(sorted(input_method_counts.items())),
        },
        'controller_groups': groups,
        'paired_controller_cohort': {
            'matched_participants': len(paired_first_times),
            'first_median_seconds': _median(paired_first_times),
            'repeat_median_seconds': _median(paired_repeat_times),
            'median_repeat_minus_first_seconds': _median(paired_deltas),
            'rule': 'One pair per participant: completed controller A1 and the earliest completed repeat before any non-controller attempt.',
            'affects_frozen_assessment': False,
        },
        'controller_stop_reasons': dict(sorted(stop_reasons.items())),
        'pacing_assessment': assessment,
        'source_session_log_size_bytes': len(session_bytes),
        'notes': [
            'Only controller sessions contribute to the first/repeat pacing summaries.',
            'A group needs at least five completed controller runs before its median is compared with the 360–480 second target.',
            'A within-target median is a pilot signal for review, not proof of balanced pacing or a release criterion.',
            'Incomplete attempts are counted and their stop reasons retained, but their stop times are not treated as completed-run times.',
            'Matched controller pairs are supplemental diagnostics: one completed controller A1 is paired with the earliest completed controller repeat before any non-controller attempt.',
            'The paired repeat-minus-first median is repeat time minus first time; a negative value means faster repeat attempts. Pair metrics do not alter the frozen pacing assessment.',
            'Browser export consistency and SHA-256 checks do not authenticate human control or runtime provenance.',
            'No session IDs, raw rows, source paths or browser CSV contents are emitted.',
        ],
    }


def analyse_files(session_file, exports_dir, output_file, expected_sessions_sha256=None):
    source = _safe_absolute(session_file)
    exports_root = _safe_absolute(exports_dir)
    dest = _safe_absolute(output_file)
    if source.suffix.lower() != '.csv' or dest.suffix.lower() != '.json':
        raise ValueError('wrong input/output file types')
    if source == dest:
        raise ValueError('session input and output must differ')
    if source == exports_root or exports_root in source.parents:
        raise ValueError('session log must be outside browser export directory')
    if dest == exports_root or exports_root in dest.parents:
        raise ValueError('report must be outside browser export directory')
    if not dest.parent.is_dir() or dest.exists():
        raise ValueError('destination unavailable or would overwrite')
    if expected_sessions_sha256 is not None and not SESSIONS.SHA_RE.fullmatch(expected_sessions_sha256):
        raise ValueError('invalid pinned session-log SHA-256')

    session_bytes = SESSIONS._read_regular_file(source, MAX_BYTES)
    session_sha = hashlib.sha256(session_bytes).hexdigest()
    if expected_sessions_sha256 is not None and session_sha != expected_sessions_sha256:
        raise ValueError('session-log SHA-256 mismatch')
    exports = SESSIONS._load_exports(exports_root)
    report = build_summary(session_bytes, exports)
    report['source_session_log_sha256'] = session_sha
    encoded = (json.dumps(report, indent=2, sort_keys=True, ensure_ascii=False) + '\n').encode('utf-8')
    fd = os.open(dest, os.O_WRONLY | os.O_CREAT | os.O_EXCL |
                 getattr(os, 'O_NOFOLLOW', 0), 0o600)
    try:
        with os.fdopen(fd, 'wb', closefd=False) as output:
            output.write(encoded)
            output.flush()
            os.fsync(output.fileno())
    finally:
        os.close(fd)
    return {
        'sessions': report['session_counts']['all_sessions'],
        'controller_sessions': report['session_counts']['controller_sessions'],
        'first_completed': report['controller_groups']['first']['completed'],
        'repeat_completed': report['controller_groups']['repeat']['completed'],
        'assessment': report['pacing_assessment'],
        'session_sha256': session_sha,
        'report_sha256': hashlib.sha256(encoded).hexdigest(),
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sessions', type=Path, required=True)
    parser.add_argument('--exports-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--expected-sessions-sha256')
    args = parser.parse_args(argv)
    try:
        result = analyse_files(args.sessions, args.exports_dir, args.output,
                               args.expected_sessions_sha256)
    except (ValueError, UnicodeError, csv.Error, OSError):
        print('ERROR: rejected private pilot inputs, export cross-check, hash pin or unsafe destination', file=sys.stderr)
        return 1
    print('PRIVATE LOCAL MAP01 PILOT SUMMARY CREATED (not cloud archived)')
    print(f"sessions={result['sessions']}")
    print(f"controller_sessions={result['controller_sessions']}")
    print(f"first_completed={result['first_completed']}")
    print(f"repeat_completed={result['repeat_completed']}")
    print(f"pacing_assessment={result['assessment']}")
    print(f"session_sha256={result['session_sha256']}")
    print(f"report_sha256={result['report_sha256']}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
