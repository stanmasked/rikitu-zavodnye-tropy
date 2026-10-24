#!/usr/bin/env python3
"""Stage174: privately cross-check MAP01 manual sessions against browser CSVs.

Offline only. Reads local files, uploads nothing, and emits aggregate results
and SHA-256 values only. A successful comparison does not authenticate human
play or prove that the browser export came from an unmodified game.
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
import stat
import sys

MAX_SESSION_BYTES = 262144
MAX_EXPORT_BYTES = 65536
MAX_EXPORTS = 100
SHA256_RE = __import__('re').compile(r'[0-9a-f]{64}\\Z', __import__('re').ASCII)
HERE = Path(__file__).resolve().parent


def _load_sibling(filename, module_name):
    spec = importlib.util.spec_from_file_location(module_name, HERE / filename)
    if spec is None or spec.loader is None:
        raise RuntimeError('local analyzer unavailable')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


SESSIONS = _load_sibling('analyze_map01_playtest_sessions.py', 'rikitu_sessions_stage174')
BROWSER = _load_sibling('analyze_private_pacing_csv.py', 'rikitu_browser_csv_stage174')


def _safe_path(value):
    path = Path(value)
    if not path.is_absolute() or any(part.is_symlink() for part in (path, *path.parents)):
        raise ValueError('unsafe path')
    resolved = path.resolve(strict=False)
    if any((parent / '.git').exists() for parent in (resolved, *resolved.parents)):
        raise ValueError('Git checkout forbidden for private input/output')
    return resolved


def _read_private_csv(value, limit):
    path = _safe_path(value)
    if path.suffix.lower() != '.csv':
        raise ValueError('CSV input required')
    fd = os.open(path, os.O_RDONLY | getattr(os, 'O_NOFOLLOW', 0))
    try:
        meta = os.fstat(fd)
        if not stat.S_ISREG(meta.st_mode) or not 0 < meta.st_size <= limit:
            raise ValueError('invalid private input size/type')
        with os.fdopen(fd, 'rb', closefd=False) as source:
            data = source.read(limit + 1)
        if len(data) != meta.st_size:
            raise ValueError('private input changed during read')
    finally:
        os.close(fd)
    return path, data


def _manual_links(data):
    text = data.decode('utf-8-sig', errors='strict')
    rows = list(csv.reader(io.StringIO(text, newline=''), delimiter=';', strict=True))
    expected = {}
    for row in rows[1:]:
        if row[3] != 'completed':
            continue
        source_hash = row[14]
        # The Stage173 analyzer has already validated the schema, unique IDs,
        # hash format, outcomes and uniqueness of completed export hashes.
        expected[source_hash] = {
            'seconds': Decimal(row[4]),
            'hits': int(row[5]),
            'coins': int(row[6]),
        }
    return expected


def analyse_files(session_file, browser_files, output_file, expected_sessions_sha256=None):
    if len(browser_files) > MAX_EXPORTS:
        raise ValueError('too many browser exports')
    if expected_sessions_sha256 is not None and not SHA256_RE.fullmatch(expected_sessions_sha256):
        raise ValueError('invalid pinned session-log hash')
    session_path, session_bytes = _read_private_csv(session_file, MAX_SESSION_BYTES)
    if expected_sessions_sha256 is not None and hashlib.sha256(session_bytes).hexdigest() != expected_sessions_sha256:
        raise ValueError('session-log SHA-256 mismatch')
    session_report = SESSIONS.analyse(session_bytes)
    expected = _manual_links(session_bytes)

    supplied = {}
    for filename in browser_files:
        path, data = _read_private_csv(filename, MAX_EXPORT_BYTES)
        digest = hashlib.sha256(data).hexdigest()
        if digest in supplied:
            raise ValueError('duplicate browser export supplied')
        if digest not in expected:
            raise ValueError('unlinked browser export supplied')
        report = BROWSER.analyse(data)
        stage1 = next((stage for stage in report['stages'] if stage['stage'] == 1), None)
        if stage1 is None:
            raise ValueError('linked browser export has no MAP01 record')
        target = expected[digest]
        if (Decimal(stage1['active_seconds']) != target['seconds'] or
                stage1['hits'] != target['hits'] or stage1['coins'] != target['coins']):
            raise ValueError('manual MAP01 values disagree with linked browser export')
        supplied[digest] = {'size': len(data), 'path': path}

    if set(supplied) != set(expected):
        raise ValueError('missing completed-session browser export')

    dest = _safe_path(output_file)
    if dest.suffix.lower() != '.json' or dest == session_path:
        raise ValueError('wrong output path/type')
    if not dest.parent.is_dir() or dest.exists():
        raise ValueError('destination unavailable or would overwrite')

    report = {
        'schema': 'rikitu-map01-browser-crosscheck-v1',
        'provenance_verified': False,
        'interpretation': 'Manual notes cross-checked against linked browser CSV values; this does not authenticate human control or game runtime.',
        'source_session_log_size_bytes': len(session_bytes),
        'source_session_log_sha256': hashlib.sha256(session_bytes).hexdigest(),
        'session_summary': session_report['counts'],
        'completion_time_summary_seconds': session_report['completion_times_seconds'],
        'stop_reasons': session_report['stop_reasons'],
        'browser_export_crosscheck': {
            'completed_sessions_expected': len(expected),
            'completed_sessions_matched': len(supplied),
            'unmatched_exports': 0,
            'fields_compared': ['MAP01 active seconds', 'hits', 'coins'],
            'mismatches': 0,
            'browser_export_sha256s': sorted(supplied),
        },
        'notes': [
            'Manual fragment, checkpoint, respawn, backtrack and platform observations are not present in the browser CSV and are not independently cross-checked.',
            'A CSV hash proves byte identity only, not human provenance.',
            'All raw inputs remain local; no network access or upload is performed.',
        ],
    }
    encoded = (json.dumps(report, indent=2, sort_keys=True, ensure_ascii=False) + '\\n').encode('utf-8')
    out_fd = os.open(dest, os.O_WRONLY | os.O_CREAT | os.O_EXCL |
                     getattr(os, 'O_NOFOLLOW', 0), 0o600)
    try:
        with os.fdopen(out_fd, 'wb', closefd=False) as output:
            output.write(encoded)
            output.flush()
            os.fsync(output.fileno())
    finally:
        os.close(out_fd)
    try:
        dir_fd = os.open(dest.parent, os.O_RDONLY)
        try:
            os.fsync(dir_fd)
        finally:
            os.close(dir_fd)
    except OSError:
        # Some filesystems do not permit fsync on directories.
        pass
    return {
        'sessions': session_report['counts']['sessions'],
        'matched': len(supplied),
        'session_sha256': hashlib.sha256(session_bytes).hexdigest(),
        'report_sha256': hashlib.sha256(encoded).hexdigest(),
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sessions', type=Path, required=True,
                        help='private manual session CSV, outside any Git checkout')
    parser.add_argument('--browser-csv', type=Path, action='append', default=[],
                        help='one exact original browser export per completed MAP01 attempt; repeat option')
    parser.add_argument('--output', type=Path, required=True,
                        help='new JSON output path in private storage')
    parser.add_argument('--expected-sessions-sha256',
                        help='optional previously pinned SHA-256 for the manual session CSV')
    args = parser.parse_args(argv)
    try:
        result = analyse_files(args.sessions, args.browser_csv, args.output,
                               args.expected_sessions_sha256)
    except (ValueError, UnicodeError, csv.Error, OSError):
        print('ERROR: rejected private session/export inputs, cross-check, integrity pin or unsafe destination', file=sys.stderr)
        return 1
    print('PRIVATE LOCAL MAP01 CROSS-CHECK CREATED (not cloud archived)')
    print(f"sessions={result['sessions']}")
    print(f"completed_exports_matched={result['matched']}")
    print(f"session_sha256={result['session_sha256']}")
    print(f"report_sha256={result['report_sha256']}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
