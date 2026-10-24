#!/usr/bin/env python3
"""Stage173: offline summary of anonymized MAP01 human-playtest notes.

Reads a local semicolon-delimited session log, never accesses the network,
never certifies human provenance, and prints aggregates/hashes only.
"""
import argparse
import csv
from decimal import Decimal, InvalidOperation
import hashlib
import io
import json
import os
from pathlib import Path
import re
import stat
import sys
from statistics import median

HEADER = (
    'session_id', 'attempt_type', 'input_method', 'outcome',
    'elapsed_seconds', 'hits', 'coins', 'fragments', 'checkpoints',
    'respawns', 'backtrack_count', 'raised_platform_seen',
    'raised_platform_used', 'stop_reason', 'source_csv_sha256',
)
MAX_BYTES = 262144
MAX_ROWS = 1000
ID_RE = re.compile(r'P[0-9]{3}-A[1-9][0-9]?\Z', re.ASCII)
INT_RE = re.compile(r'[0-9]{1,7}\Z', re.ASCII)
SECONDS_RE = re.compile(r'[0-9]{1,5}(?:\.[0-9]{1,3})?\Z', re.ASCII)
SHA_RE = re.compile(r'[0-9a-f]{64}\Z', re.ASCII)
ATTEMPTS = {'first', 'repeat'}
INPUTS = {'controller', 'keyboard', 'touch'}
OUTCOMES = {'completed', 'stopped', 'abandoned', 'technical_issue'}
YES_NO_UNCERTAIN = {'yes', 'no', 'uncertain'}
STOP_REASONS = {
    'none', 'missed_fragment', 'gap_fall', 'lost_route',
    'repeated_failure', 'quit', 'technical_issue', 'other',
}


def _int(value, maximum):
    if not INT_RE.fullmatch(value):
        raise ValueError('invalid integer')
    number = int(value)
    if number > maximum:
        raise ValueError('integer out of range')
    return number


def _seconds(value):
    if not SECONDS_RE.fullmatch(value):
        raise ValueError('invalid elapsed seconds')
    try:
        number = Decimal(value)
    except InvalidOperation as exc:
        raise ValueError('invalid elapsed seconds') from exc
    if number <= 0 or number > Decimal(86400):
        raise ValueError('elapsed seconds out of range')
    return number


def analyse(data):
    """Validate untrusted UTF-8-SIG session-log bytes and return aggregates only."""
    if not isinstance(data, bytes) or not 0 < len(data) <= MAX_BYTES or b'\0' in data:
        raise ValueError('invalid session-log byte envelope')
    text = data.decode('utf-8-sig', errors='strict')
    rows = list(csv.reader(io.StringIO(text, newline=''), delimiter=';', strict=True))
    if not 2 <= len(rows) <= MAX_ROWS + 1 or tuple(rows[0]) != HEADER:
        raise ValueError('unexpected session-log schema or row count')

    sessions = []
    seen_ids = set()
    linked_exports = set()
    for row in rows[1:]:
        if len(row) != len(HEADER):
            raise ValueError('incorrect session-log column count')
        (session_id, attempt_type, input_method, outcome, elapsed_text, hits_text,
         coins_text, fragments_text, checkpoints_text, respawns_text, backtrack_text,
         platform_seen, platform_used, stop_reason, source_hash) = row
        if not ID_RE.fullmatch(session_id) or session_id in seen_ids:
            raise ValueError('invalid or duplicate anonymized session ID')
        seen_ids.add(session_id)
        attempt_number = int(session_id.rsplit('-A', 1)[1])
        if attempt_type not in ATTEMPTS or (
                (attempt_number == 1) != (attempt_type == 'first')):
            raise ValueError('attempt type does not match anonymized attempt number')
        if input_method not in INPUTS or outcome not in OUTCOMES:
            raise ValueError('invalid input method or outcome')
        elapsed = _seconds(elapsed_text)
        hits = _int(hits_text, 1000000)
        coins = _int(coins_text, 90)
        fragments = _int(fragments_text, 3)
        checkpoints = _int(checkpoints_text, 3)
        respawns = _int(respawns_text, 10000)
        backtracks = _int(backtrack_text, 10000)
        if platform_seen not in YES_NO_UNCERTAIN or platform_used not in YES_NO_UNCERTAIN:
            raise ValueError('invalid raised-platform observation')
        if platform_used == 'yes' and platform_seen != 'yes':
            raise ValueError('platform cannot be used if not observed')
        if stop_reason not in STOP_REASONS:
            raise ValueError('invalid stop reason')
        if outcome == 'completed':
            if fragments != 3 or stop_reason != 'none' or not SHA_RE.fullmatch(source_hash):
                raise ValueError('completed attempt needs all fragments, no stop reason, and linked CSV hash')
            if source_hash in linked_exports:
                raise ValueError('browser export hash reused by multiple completed sessions')
            linked_exports.add(source_hash)
        else:
            if stop_reason == 'none' or source_hash:
                raise ValueError('incomplete attempt needs a stop reason and no completion CSV hash')
            if outcome == 'technical_issue' and stop_reason != 'technical_issue':
                raise ValueError('technical issue outcome needs matching stop reason')
        sessions.append({
            'attempt_type': attempt_type,
            'input_method': input_method,
            'outcome': outcome,
            'elapsed_seconds': elapsed,
            'hits': hits,
            'coins': coins,
            'fragments': fragments,
            'checkpoints': checkpoints,
            'respawns': respawns,
            'backtrack_count': backtracks,
            'raised_platform_seen': platform_seen,
            'raised_platform_used': platform_used,
            'stop_reason': stop_reason,
            'source_csv_sha256': source_hash,
        })

    first = [s for s in sessions if s['attempt_type'] == 'first']
    repeat = [s for s in sessions if s['attempt_type'] == 'repeat']
    first_done = [s for s in first if s['outcome'] == 'completed']
    repeat_done = [s for s in repeat if s['outcome'] == 'completed']

    def _median(values):
        if not values:
            return None
        return str(Decimal(str(median(values))).quantize(Decimal('0.1')))

    def _within(values):
        return sum(1 for s in values if Decimal(360) <= s['elapsed_seconds'] <= Decimal(480))

    stop_counts = {}
    for s in sessions:
        if s['stop_reason'] != 'none':
            stop_counts[s['stop_reason']] = stop_counts.get(s['stop_reason'], 0) + 1

    report = {
        'schema': 'rikitu-map01-human-playtest-summary-v1',
        'provenance_verified': False,
        'interpretation': 'Self-reported/manual session notes; integrity checks do not authenticate human play.',
        'counts': {
            'sessions': len(sessions),
            'first_attempts': len(first),
            'repeat_attempts': len(repeat),
            'completed': sum(1 for s in sessions if s['outcome'] == 'completed'),
            'incomplete': sum(1 for s in sessions if s['outcome'] != 'completed'),
            'first_completed': len(first_done),
            'repeat_completed': len(repeat_done),
            'first_completion_within_360_480s': _within(first_done),
            'repeat_completion_within_360_480s': _within(repeat_done),
            'raised_platform_seen_yes': sum(1 for s in sessions if s['raised_platform_seen'] == 'yes'),
            'raised_platform_used_yes': sum(1 for s in sessions if s['raised_platform_used'] == 'yes'),
            'total_hits': sum(s['hits'] for s in sessions),
            'total_coins': sum(s['coins'] for s in sessions),
            'total_respawns': sum(s['respawns'] for s in sessions),
            'total_backtrack_events': sum(s['backtrack_count'] for s in sessions),
        },
        'completion_times_seconds': {
            'first_attempt_median': _median([s['elapsed_seconds'] for s in first_done]),
            'repeat_attempt_median': _median([s['elapsed_seconds'] for s in repeat_done]),
        },
        'stop_reasons': dict(sorted(stop_counts.items())),
        'linked_browser_exports': len(linked_exports),
        'linked_browser_export_sha256s': sorted(linked_exports),
        'notes': [
            'Ordinary MAP01 target is 360-480 active seconds.',
            'Incomplete elapsed times are stop-time observations, not completed-run durations.',
            'The browser export supplies completed-stage seconds/hits/coins; the manual log supplies route observations.',
            'Do not combine scripted/native fixture timings with human-controlled results.',
        ],
    }
    return report


def _safe_path(value):
    path = Path(value)
    if not path.is_absolute() or any(part.is_symlink() for part in (path, *path.parents)):
        raise ValueError('unsafe path')
    resolved = path.resolve(strict=False)
    if any((parent / '.git').exists() for parent in (resolved, *resolved.parents)):
        raise ValueError('Git checkout forbidden for private input/output')
    return resolved


def private_report(input_file, output_file, expected_sha256=None):
    source = _safe_path(input_file)
    dest = _safe_path(output_file)
    if source == dest or source.suffix.lower() != '.csv' or dest.suffix.lower() != '.json':
        raise ValueError('wrong input/output file types')
    if not dest.parent.is_dir() or dest.exists():
        raise ValueError('destination unavailable or would overwrite')
    if expected_sha256 is not None and not SHA_RE.fullmatch(expected_sha256):
        raise ValueError('bad pinned SHA-256')

    fd = os.open(source, os.O_RDONLY | getattr(os, 'O_NOFOLLOW', 0))
    try:
        meta = os.fstat(fd)
        if not stat.S_ISREG(meta.st_mode) or not 0 < meta.st_size <= MAX_BYTES:
            raise ValueError('invalid private input size/type')
        with os.fdopen(fd, 'rb', closefd=False) as inp:
            data = inp.read(MAX_BYTES + 1)
        if len(data) != meta.st_size:
            raise ValueError('private input changed during read')
    finally:
        os.close(fd)

    source_sha = hashlib.sha256(data).hexdigest()
    if expected_sha256 is not None and source_sha != expected_sha256:
        raise ValueError('pinned SHA-256 mismatch')
    report = analyse(data)
    report['source_size_bytes'] = len(data)
    report['source_sha256'] = source_sha
    encoded = (json.dumps(report, indent=2, sort_keys=True, ensure_ascii=False) + '\n').encode('utf-8')
    out_fd = os.open(dest, os.O_WRONLY | os.O_CREAT | os.O_EXCL |
                     getattr(os, 'O_NOFOLLOW', 0), 0o600)
    try:
        with os.fdopen(out_fd, 'wb', closefd=False) as out:
            out.write(encoded)
            out.flush()
            os.fsync(out.fileno())
    finally:
        os.close(out_fd)
    return {
        'sessions': report['counts']['sessions'],
        'source_sha256': source_sha,
        'report_sha256': hashlib.sha256(encoded).hexdigest(),
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--expected-sha256', help='previously pinned original notes CSV SHA-256')
    args = parser.parse_args(argv)
    try:
        result = private_report(args.input, args.output, args.expected_sha256)
    except (ValueError, UnicodeError, csv.Error, OSError):
        print('ERROR: rejected private session log, schema, integrity pin or unsafe destination', file=sys.stderr)
        return 1
    print('PRIVATE LOCAL PLAYTEST SUMMARY CREATED (not cloud archived)')
    print(f"sessions={result['sessions']}")
    print(f"source_sha256={result['source_sha256']}")
    print(f"report_sha256={result['report_sha256']}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
