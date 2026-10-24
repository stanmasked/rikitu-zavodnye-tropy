#!/usr/bin/env python3
"""Stage163: offline, private-only audit of RIKITU browser pacing CSV exports.

No network, no CSV evaluation, and no automatic claim of human provenance.
"""
import argparse
import csv
from decimal import Decimal
import hashlib
import io
import json
import os
from pathlib import Path
import re
import stat
import sys

HEADER = ('Уровень', 'Название', 'Тип', 'Секунды', 'Попадания',
          'Монеты из 90', 'Цель от, с', 'Цель до, с', 'Сравнение')
SECONDS_RE = re.compile(r'[0-9]+(?:\.[0-9]{1,9})?\Z', re.ASCII)
INTEGER_RE = re.compile(r'[0-9]{1,12}\Z', re.ASCII)
SHA256_RE = re.compile(r'[0-9a-f]{64}\Z', re.ASCII)
MAX_BYTES = 65536
PROVENANCE = ('unverified', 'self-reported-manual', 'scripted')


def _integer(s, ceiling):
    if not INTEGER_RE.fullmatch(s):
        raise ValueError('invalid unsigned integer')
    result = int(s)
    if result > ceiling:
        raise ValueError('unsigned integer out of bounds')
    return result


def _seconds(s):
    if not SECONDS_RE.fullmatch(s) or len(s) > 24:
        raise ValueError('invalid time number')
    result = Decimal(s)
    if result > Decimal(86400):
        raise ValueError('time exceeds 24 hours')
    return result


def _text(s):
    if not 1 <= len(s) <= 100 or s.strip() != s or s[0] in '=+-@' or any(
            ord(ch) < 32 or ord(ch) == 127 for ch in s):
        raise ValueError('invalid chapter name')


def analyse(data, provenance='unverified'):
    """Validate untrusted UTF-8-SIG bytes using game.js' exact CSV columns."""
    if provenance not in PROVENANCE:
        raise ValueError('invalid provenance label')
    if not isinstance(data, bytes) or not 0 < len(data) <= MAX_BYTES or b'\0' in data:
        raise ValueError('invalid CSV byte envelope')
    text = data.decode('utf-8-sig', errors='strict')
    rows = list(csv.reader(io.StringIO(text, newline=''), delimiter=';', strict=True))
    if not 2 <= len(rows) <= 17 or tuple(rows[0]) != HEADER:
        raise ValueError('unexpected CSV schema or row count')

    per_stage = []
    last_stage = 0
    within = below = above = total_hits = total_coins = 0
    for row in rows[1:]:
        if len(row) != 9:
            raise ValueError('incorrect CSV column count')
        stage = _integer(row[0], 16)
        if stage <= last_stage or stage < 1:
            raise ValueError('levels must be strictly ascending without duplicates')
        last_stage = stage
        _text(row[1])
        boss = stage % 2 == 0
        low, high = ((480, 600) if boss else (360, 480))
        if row[2] != ('С мини-боссом' if boss else 'Обычный'):
            raise ValueError('inconsistent level type')
        seconds = _seconds(row[3])
        hits = _integer(row[4], 1000000)
        coins = _integer(row[5], 90)
        if _integer(row[6], 86400) != low or _integer(row[7], 86400) != high:
            raise ValueError('inconsistent pacing targets')
        verdict = 'Ниже цели' if seconds < low else ('Выше цели' if seconds > high else 'В пределах цели')
        if row[8] != verdict:
            raise ValueError('inconsistent pacing verdict')
        if seconds < low:
            below += 1
            delta = Decimal(low) - seconds
        elif seconds > high:
            above += 1
            delta = seconds - Decimal(high)
        else:
            within += 1
            delta = Decimal(0)
        total_hits += hits
        total_coins += coins
        per_stage.append({
            'stage': stage, 'kind': 'miniboss' if boss else 'ordinary',
            'active_seconds': str(seconds), 'hits': hits, 'coins': coins,
            'target_low_seconds': low, 'target_high_seconds': high,
            'classification': 'below' if seconds < low else ('above' if seconds > high else 'within'),
            'seconds_outside_target': str(delta),
        })
    return {
        'schema': 'rikitu-private-pacing-analysis-v1',
        'provenance_label': provenance,
        'provenance_verified': False,
        'notes': 'CSV integrity and arithmetic only; does not establish human play or authentic runtime capture',
        'source_size_bytes': len(data),
        'source_sha256': hashlib.sha256(data).hexdigest(),
        'counts': {'stages': len(per_stage), 'within': within, 'below': below,
                   'above': above, 'total_hits': total_hits, 'total_coins': total_coins},
        'stages': per_stage,
    }


def _safe_location(path):
    """Refuse Git checkouts, symlink components, and relative paths."""
    path = Path(path)
    if not path.is_absolute() or any(p.is_symlink() for p in (path, *path.parents)):
        raise ValueError('unsafe path')
    resolved = path.resolve(strict=False)
    if any((p / '.git').exists() for p in (resolved, *resolved.parents)):
        raise ValueError('Git checkout forbidden for private input/output')
    return resolved


def private_report(input_file, output_file, provenance='unverified', expected_sha256=None):
    source = _safe_location(input_file)
    dest = _safe_location(output_file)
    if source == dest or dest.suffix != '.json' or source.suffix.lower() != '.csv':
        raise ValueError('wrong input/output file types')
    if not dest.parent.is_dir() or dest.exists():
        raise ValueError('destination unavailable or would overwrite')
    if expected_sha256 is not None and not SHA256_RE.fullmatch(expected_sha256):
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
    actual_sha256 = hashlib.sha256(data).hexdigest()
    if expected_sha256 is not None and actual_sha256 != expected_sha256:
        raise ValueError('pinned SHA-256 mismatch')
    report = analyse(data, provenance)
    encoded = (json.dumps(report, indent=2, sort_keys=True,
                          ensure_ascii=False) + '\n').encode('utf-8')
    out_fd = os.open(dest, os.O_WRONLY | os.O_CREAT | os.O_EXCL |
                     getattr(os, 'O_NOFOLLOW', 0), 0o600)
    try:
        with os.fdopen(out_fd, 'wb', closefd=False) as out:
            out.write(encoded)
            out.flush()
            os.fsync(out.fileno())
    finally:
        os.close(out_fd)
    return {'stages': report['counts']['stages'],
            'source_sha256': actual_sha256,
            'report_sha256': hashlib.sha256(encoded).hexdigest()}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--provenance', choices=PROVENANCE, default='unverified')
    parser.add_argument('--expected-sha256', help='previously pinned original CSV SHA-256')
    args = parser.parse_args(argv)
    try:
        result = private_report(args.input, args.output, args.provenance,
                                args.expected_sha256)
    except (ValueError, UnicodeError, csv.Error, OSError):
        print('ERROR: rejected private input, schema, integrity pin or unsafe destination', file=sys.stderr)
        return 1
    print('PRIVATE LOCAL ANALYSIS CREATED (not cloud archived)')
    print(f"stages={result['stages']}")
    print(f"source_sha256={result['source_sha256']}")
    print(f"report_sha256={result['report_sha256']}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
