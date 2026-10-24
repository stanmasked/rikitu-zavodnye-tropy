#!/usr/bin/env python3
"""Stage174: privately cross-check MAP01 session notes against named browser exports.

Offline only. Each completed attempt is linked by its anonymized filename and
exact SHA-256. Different filenames may have identical bytes and hashes. The
tool prints aggregate counts/hashes only and never authenticates human play.
"""
import argparse
import csv
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import sys

HERE = Path(__file__).resolve().parent
SHA256_RE = re.compile(r'[0-9a-f]{64}\Z', re.ASCII)


def _load_sibling(filename, module_name):
    spec = importlib.util.spec_from_file_location(module_name, HERE / filename)
    if spec is None or spec.loader is None:
        raise RuntimeError('local analyzer unavailable')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


SESSIONS = _load_sibling('analyze_map01_playtest_sessions.py', 'rikitu_sessions_stage174')


def analyse_files(session_file, exports_dir, output_file, expected_sessions_sha256=None):
    session_path = SESSIONS._safe_path(session_file)
    export_root = SESSIONS._safe_path(exports_dir)
    dest = SESSIONS._safe_path(output_file)
    if not export_root.is_dir():
        raise ValueError('browser export directory unavailable')
    if session_path.suffix.lower() != '.csv' or dest.suffix.lower() != '.json':
        raise ValueError('wrong session/output file types')
    if session_path == dest:
        raise ValueError('session input and output must differ')
    if session_path == export_root or export_root in session_path.parents:
        raise ValueError('session log must be outside browser export directory')
    if dest == export_root or export_root in dest.parents:
        raise ValueError('report must be outside browser export directory')
    if not dest.parent.is_dir() or dest.exists():
        raise ValueError('destination unavailable or would overwrite')
    if expected_sessions_sha256 is not None and not SHA256_RE.fullmatch(expected_sessions_sha256):
        raise ValueError('invalid pinned session-log hash')

    session_bytes = SESSIONS._read_regular_file(session_path, SESSIONS.MAX_BYTES)
    session_sha = hashlib.sha256(session_bytes).hexdigest()
    if expected_sessions_sha256 is not None and session_sha != expected_sessions_sha256:
        raise ValueError('session-log SHA-256 mismatch')

    # Export identity is the pair (anonymized filename, SHA-256), not hash alone:
    # byte-identical exports from distinct attempts remain representable.
    exports = SESSIONS._load_exports(export_root)
    summary = SESSIONS.analyse(session_bytes, exports)
    crosscheck = {
        'completed_sessions_expected': summary['counts']['completed'],
        'completed_sessions_matched': summary['linked_browser_exports_verified'],
        'unique_browser_export_hashes': summary['unique_browser_export_hashes'],
        'unmatched_exports': 0,
        'fields_compared': ['MAP01 active seconds', 'hits', 'coins'],
        'mismatches': 0,
        'browser_export_sha256s': summary['linked_browser_export_sha256s'],
    }
    report = {
        'schema': 'rikitu-map01-browser-crosscheck-v2',
        'provenance_verified': False,
        'interpretation': 'Named browser exports and manual MAP01 seconds/hits/coins were cross-checked; this does not authenticate human control or runtime.',
        'source_session_log_size_bytes': len(session_bytes),
        'source_session_log_sha256': session_sha,
        'session_summary': summary['counts'],
        'completion_time_summary_seconds': summary['completion_times_seconds'],
        'stop_reasons': summary['stop_reasons'],
        'browser_export_crosscheck': crosscheck,
        'notes': [
            'Completed sessions are linked by anonymized source_csv_filename plus exact source_csv_sha256.',
            'Different filenames may legitimately have identical SHA-256; filenames keep distinct attempts separate.',
            'Manual fragment/checkpoint/respawn/backtrack/platform observations are not present in the browser CSV and are not independently cross-checked.',
            'All raw inputs remain local; no network access or upload is performed.',
        ],
    }
    encoded = (json.dumps(report, indent=2, sort_keys=True, ensure_ascii=False) + '\n').encode('utf-8')
    out_fd = os.open(dest, os.O_WRONLY | os.O_CREAT | os.O_EXCL |
                     getattr(os, 'O_NOFOLLOW', 0), 0o600)
    try:
        with os.fdopen(out_fd, 'wb', closefd=False) as output:
            output.write(encoded)
            output.flush()
            os.fsync(output.fileno())
    finally:
        os.close(out_fd)
    return {
        'sessions': summary['counts']['sessions'],
        'matched': summary['linked_browser_exports_verified'],
        'unique_hashes': summary['unique_browser_export_hashes'],
        'session_sha256': session_sha,
        'report_sha256': hashlib.sha256(encoded).hexdigest(),
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sessions', type=Path, required=True,
                        help='private manual session CSV, outside the export directory and Git checkouts')
    parser.add_argument('--exports-dir', type=Path, required=True,
                        help='directory containing only P###-A#.csv original browser exports')
    parser.add_argument('--output', type=Path, required=True,
                        help='new aggregate JSON output path outside the export directory')
    parser.add_argument('--expected-sessions-sha256',
                        help='optional previously pinned SHA-256 for the manual session CSV')
    args = parser.parse_args(argv)
    try:
        result = analyse_files(args.sessions, args.exports_dir, args.output,
                               args.expected_sessions_sha256)
    except (ValueError, UnicodeError, csv.Error, OSError):
        print('ERROR: rejected private session/export inputs, cross-check, integrity pin or unsafe destination', file=sys.stderr)
        return 1
    print('PRIVATE LOCAL MAP01 CROSS-CHECK CREATED (not cloud archived)')
    print(f"sessions={result['sessions']}")
    print(f"completed_exports_matched={result['matched']}")
    print(f"unique_export_hashes={result['unique_hashes']}")
    print(f"session_sha256={result['session_sha256']}")
    print(f"report_sha256={result['report_sha256']}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
