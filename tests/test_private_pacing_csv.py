"""Stage163: strictly synthetic fixtures, no private user exports or saves."""
import csv
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

TOOL = Path(__file__).resolve().parents[1] / 'tools/analyze_private_pacing_csv.py'
spec = importlib.util.spec_from_file_location('private_pacing', TOOL)
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


def fixture(entries=((1, 'Цветочная тропа', '402.5', 2, 70),
                     (2, 'Мельница у ручья', '531.5', 1, 80))):
    out = io.StringIO(newline='')
    writer = csv.writer(out, delimiter=';', lineterminator='\r\n', quoting=csv.QUOTE_ALL)
    writer.writerow(m.HEADER)
    for stage, name, seconds, hits, coins in entries:
        boss = stage % 2 == 0
        low, high = (480, 600) if boss else (360, 480)
        v = m._seconds(seconds)
        verdict = 'Ниже цели' if v < low else ('Выше цели' if v > high else 'В пределах цели')
        writer.writerow((stage, name, 'С мини-боссом' if boss else 'Обычный',
                         seconds, hits, coins, low, high, verdict))
    return ('\ufeff' + out.getvalue()).encode('utf-8')


class ParserTests(unittest.TestCase):
    def test_reference_csv(self):
        b = fixture()
        r = m.analyse(b)
        self.assertEqual(r['schema'], 'rikitu-private-pacing-analysis-v1')
        self.assertEqual(r['source_sha256'], hashlib.sha256(b).hexdigest())
        self.assertEqual(r['counts'], {'stages': 2, 'within': 2, 'below': 0,
                                       'above': 0, 'total_hits': 3, 'total_coins': 150})
        self.assertFalse(r['provenance_verified'])
        self.assertEqual(r['stages'][0]['active_seconds'], '402.5')
        self.assertEqual(r['stages'][1]['kind'], 'miniboss')

    def test_partial_and_boundaries(self):
        r = m.analyse(fixture(((1, 'Цветочная тропа', '359.99', 0, 0),
                               (2, 'Мельница у ручья', '600.01', 9, 90),
                               (3, 'Грибная роща', '360', 0, 0),
                               (4, 'Мост', '600', 1, 2))), 'scripted')
        self.assertEqual((r['counts']['below'], r['counts']['above'],
                          r['counts']['within']), (1, 1, 2))
        self.assertEqual(r['stages'][0]['seconds_outside_target'], '0.01')
        self.assertEqual(r['stages'][1]['seconds_outside_target'], '0.01')
        self.assertEqual(r['provenance_label'], 'scripted')
        self.assertFalse(r['provenance_verified'])

    def test_rogue_rows_and_duplicate(self):
        for b in (b'', b'not csv', fixture().replace(b'"1";', b'"2";', 1),
                  fixture().replace(b'"402.5";', b'"NaN";', 1),
                  fixture().replace(b'"531.5";', b'"1e3";', 1),
                  fixture().replace(b'\r\n"2";', b'\r\n"17";', 1),
                  fixture().replace(b'"70";', b'"91";', 1),
                  fixture().replace('В пределах цели'.encode('utf-8'), b'invalid', 1),
                  fixture() + b'garbage' * 10000,
                  fixture() + b'\x00',
                  fixture().replace('Цветочная тропа'.encode('utf-8'), b'=A1+1', 1)):
            with self.subTest(length=len(b)):
                with self.assertRaises((ValueError, UnicodeError, csv.Error)):
                    m.analyse(b)

    def test_does_not_automatically_certify_humans(self):
        for provenance in m.PROVENANCE:
            self.assertFalse(m.analyse(fixture(), provenance)['provenance_verified'])
        with self.assertRaises(ValueError):
            m.analyse(fixture(), 'verified-real-human')

    def test_private_write_integrity_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as d:
            d = Path(d)
            src, out = d / 'synthetic.csv', d / 'analysis.json'
            raw = fixture()
            src.write_bytes(raw)
            pinned = hashlib.sha256(raw).hexdigest()
            result = m.private_report(src, out, expected_sha256=pinned)
            self.assertEqual(result['stages'], 2)
            self.assertEqual(json.loads(out.read_text('utf-8'))['source_sha256'], pinned)
            self.assertEqual(out.stat().st_mode & 0o777, 0o600)
            self.assertEqual(hashlib.sha256(out.read_bytes()).hexdigest(), result['report_sha256'])
            with self.assertRaises(ValueError):
                m.private_report(src, out)
            with self.assertRaises(ValueError):
                m.private_report(src, d / 'other.json', expected_sha256='0' * 64)
            self.assertFalse((d / 'other.json').exists())

    def test_reject_symlink_and_git_worktree(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / 'real.csv').write_bytes(fixture())
            (root / 'linked.csv').symlink_to(root / 'real.csv')
            with self.assertRaises(ValueError):
                m.private_report(root / 'linked.csv', root / 'report.json')
            (root / '.git').mkdir()
            with self.assertRaises(ValueError):
                m.private_report(root / 'real.csv', root / 'report.json')

    def test_safe_cli_reports_only_hashes(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            src, dest = root / 'private-name.csv', root / 'private-report.json'
            src.write_bytes(fixture())
            result = subprocess.run([sys.executable, str(TOOL), '--input', str(src),
                                     '--output', str(dest), '--provenance', 'self-reported-manual'],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertNotIn('private-name', result.stdout)
            self.assertNotIn('Цветочная', result.stdout)
            self.assertFalse(json.loads(dest.read_text('utf-8'))['provenance_verified'])
            result2 = subprocess.run([sys.executable, str(TOOL), '--input', str(src),
                                      '--output', str(dest)], capture_output=True, text=True)
            self.assertEqual(result2.returncode, 1)
            self.assertNotIn(str(src), result2.stderr)


if __name__ == '__main__':
    unittest.main(verbosity=2)
