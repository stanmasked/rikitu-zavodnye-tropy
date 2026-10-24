"""Stage174: synthetic-only session-to-browser-export cross-check tests."""
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

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / 'tools/crosscheck_map01_playtest_exports.py'
spec = importlib.util.spec_from_file_location('crosscheck_stage174', TOOL)
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)

BROWSER_HEADER = ('Уровень', 'Название', 'Тип', 'Секунды', 'Попадания',
                  'Монеты из 90', 'Цель от, с', 'Цель до, с', 'Сравнение')


def browser_csv(seconds, hits, coins):
    verdict = 'Ниже цели' if seconds < 360 else ('Выше цели' if seconds > 480 else 'В пределах цели')
    out = io.StringIO(newline='')
    writer = csv.writer(out, delimiter=';', lineterminator='\\r\\n', quoting=csv.QUOTE_ALL)
    writer.writerow(BROWSER_HEADER)
    writer.writerow(('1', 'Цветочная тропа', 'Обычный', str(seconds), str(hits),
                     str(coins), '360', '480', verdict))
    return ('\\ufeff' + out.getvalue()).encode('utf-8')


def make_case(changed_first_seconds=None, identical_exports=False):
    first = browser_csv(400.0 if changed_first_seconds is None else changed_first_seconds, 2, 18)
    repeat = first if identical_exports else browser_csv(335.0, 1, 42)
    hashes = [hashlib.sha256(first).hexdigest(), hashlib.sha256(repeat).hexdigest()]
    out = io.StringIO(newline='')
    writer = csv.writer(out, delimiter=';', lineterminator='\\r\\n', quoting=csv.QUOTE_ALL)
    writer.writerow(m.SESSIONS.HEADER)
    writer.writerow(('P001-A1','first','controller','completed','400.0','2','18','3','3','0','1','yes','yes','none','P001-A1.csv',hashes[0]))
    writer.writerow(('P001-A2','repeat','controller','completed',
                     '400.0' if identical_exports else '335.0',
                     '2' if identical_exports else '1','18' if identical_exports else '42',
                     '3','3','0','2','yes','yes','none','P001-A2.csv',hashes[1]))
    writer.writerow(('P002-A1','first','keyboard','stopped','220.0','4','6','1','1','2','1','no','no','missed_fragment','',''))
    sessions = ('\\ufeff' + out.getvalue()).encode('utf-8')
    return sessions, {'P001-A1.csv': first, 'P001-A2.csv': repeat}


def write_case(root, sessions, exports):
    session_file = root/'sessions.csv'
    export_dir = root/'browser_exports'
    export_dir.mkdir()
    session_file.write_bytes(sessions)
    for name, data in exports.items():
        (export_dir/name).write_bytes(data)
    return session_file, export_dir


class BrowserExportCrosscheckTests(unittest.TestCase):
    def test_crosschecks_values_and_outputs_aggregates_only(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            sessions, exports = make_case()
            session_file, export_dir = write_case(root, sessions, exports)
            report_file = root/'summary.json'
            result = m.analyse_files(session_file, export_dir, report_file,
                                     hashlib.sha256(sessions).hexdigest())
            report = json.loads(report_file.read_text(encoding='utf-8'))
            self.assertEqual(result['sessions'], 3)
            self.assertEqual(result['matched'], 2)
            self.assertEqual(result['unique_hashes'], 2)
            self.assertEqual(report['session_summary']['completed'], 2)
            self.assertEqual(report['session_summary']['incomplete'], 1)
            self.assertEqual(report['browser_export_crosscheck']['completed_sessions_matched'], 2)
            self.assertEqual(report['browser_export_crosscheck']['mismatches'], 0)
            self.assertFalse(report['provenance_verified'])
            self.assertEqual(report_file.stat().st_mode & 0o777, 0o600)
            self.assertEqual(hashlib.sha256(report_file.read_bytes()).hexdigest(), result['report_sha256'])
            encoded = json.dumps(report)
            for participant_id in ('P001-A1','P001-A2','P002-A1'):
                self.assertNotIn(participant_id, encoded)
            self.assertNotIn(str(session_file), encoded)

    def test_identical_export_bytes_for_distinct_attempt_filenames_are_allowed(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            sessions,exports=make_case(identical_exports=True)
            session_file,export_dir=write_case(root,sessions,exports)
            result=m.analyse_files(session_file,export_dir,root/'summary.json')
            report=json.loads((root/'summary.json').read_text('utf-8'))
            self.assertEqual(result['matched'],2)
            self.assertEqual(result['unique_hashes'],1)
            self.assertEqual(report['browser_export_crosscheck']['completed_sessions_expected'],2)
            self.assertEqual(report['browser_export_crosscheck']['unique_browser_export_hashes'],1)
            self.assertEqual(len(report['browser_export_crosscheck']['browser_export_sha256s']),1)

    def test_linked_but_mismatched_stage1_value_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            sessions,exports=make_case(changed_first_seconds=401.0)
            session_file,export_dir=write_case(root,sessions,exports)
            with self.assertRaisesRegex(ValueError,'does not match|do not match'):
                m.analyse_files(session_file,export_dir,root/'report.json')
            self.assertFalse((root/'report.json').exists())

    def test_missing_and_unlinked_exports_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            sessions,exports=make_case()
            session_file,export_dir=write_case(root,sessions,exports)
            (export_dir/'P001-A2.csv').unlink()
            with self.assertRaisesRegex(ValueError,'missing'):
                m.analyse_files(session_file,export_dir,root/'missing.json')
            (export_dir/'P001-A2.csv').write_bytes(exports['P001-A2.csv'])
            (export_dir/'P004-A1.csv').write_bytes(browser_csv(420,0,3))
            with self.assertRaisesRegex(ValueError,'unreferenced'):
                m.analyse_files(session_file,export_dir,root/'extra.json')

    def test_malformed_export_and_pinned_session_hash_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            sessions,exports=make_case()
            bad=b'not a valid export'
            rows=list(csv.reader(io.StringIO(sessions.decode('utf-8-sig'),newline=''),delimiter=';',strict=True))
            rows[1][15]=hashlib.sha256(bad).hexdigest()
            out=io.StringIO(newline='')
            writer=csv.writer(out,delimiter=';',lineterminator='\\r\\n',quoting=csv.QUOTE_ALL)
            writer.writerows(rows)
            sessions=('\ufeff'+out.getvalue()).encode('utf-8')
            session_file,export_dir=write_case(root,sessions,exports)
            (export_dir/'P001-A1.csv').write_bytes(bad)
            with self.assertRaises((ValueError,UnicodeError,csv.Error)):
                m.analyse_files(session_file,export_dir,root/'bad.json')
            with self.assertRaisesRegex(ValueError,'SHA-256'):
                m.analyse_files(session_file,export_dir,root/'pinned.json','0'*64)

    def test_output_must_be_outside_exports_and_never_overwrites(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            sessions,exports=make_case()
            session_file,export_dir=write_case(root,sessions,exports)
            with self.assertRaisesRegex(ValueError,'outside browser export'):
                m.analyse_files(session_file,export_dir,export_dir/'summary.json')
            out=root/'summary.json'
            m.analyse_files(session_file,export_dir,out)
            with self.assertRaisesRegex(ValueError,'overwrite'):
                m.analyse_files(session_file,export_dir,out)

    def test_symlink_and_git_checkout_refusal(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            sessions,exports=make_case()
            session_file,export_dir=write_case(root,sessions,exports)
            (export_dir/'alias.csv').symlink_to(export_dir/'P001-A1.csv')
            with self.assertRaises(ValueError):
                m.analyse_files(session_file,export_dir,root/'symlink.json')
            (export_dir/'alias.csv').unlink()
            (root/'.git').mkdir()
            with self.assertRaisesRegex(ValueError,'Git checkout'):
                m.analyse_files(session_file,export_dir,root/'git.json')

    def test_cli_prints_only_aggregates_and_hashes(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            sessions,exports=make_case()
            session_file,export_dir=write_case(root,sessions,exports)
            out=root/'summary.json'
            run=subprocess.run([sys.executable,str(TOOL),'--sessions',str(session_file),
                                '--exports-dir',str(export_dir),'--output',str(out)],
                               capture_output=True,text=True)
            self.assertEqual(run.returncode,0,run.stderr)
            self.assertIn('sessions=3',run.stdout)
            self.assertIn('completed_exports_matched=2',run.stdout)
            self.assertIn('unique_export_hashes=2',run.stdout)
            self.assertNotIn('P001-A1',run.stdout)
            self.assertNotIn(str(session_file),run.stdout)


if __name__ == '__main__':
    unittest.main(verbosity=2)
