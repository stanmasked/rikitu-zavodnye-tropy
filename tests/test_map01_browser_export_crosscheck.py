"""Stage174: synthetic-only cross-check tests for private MAP01 browser exports."""
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


def make_case(changed_first_seconds=None):
    first = browser_csv(400.0 if changed_first_seconds is None else changed_first_seconds, 2, 18)
    repeat = browser_csv(335.0, 1, 42)
    hashes = [hashlib.sha256(first).hexdigest(), hashlib.sha256(repeat).hexdigest()]
    out = io.StringIO(newline='')
    writer = csv.writer(out, delimiter=';', lineterminator='\\r\\n', quoting=csv.QUOTE_ALL)
    writer.writerow(m.SESSIONS.HEADER)
    writer.writerow(('P001-A1','first','controller','completed','400.0','2','18','3','3','0','1','yes','yes','none',hashes[0]))
    writer.writerow(('P001-A2','repeat','controller','completed','335.0','1','42','3','3','0','2','yes','yes','none',hashes[1]))
    writer.writerow(('P002-A1','first','keyboard','stopped','220.0','4','6','1','1','2','1','no','no','missed_fragment',''))
    sessions = ('\\ufeff' + out.getvalue()).encode('utf-8')
    return sessions, first, repeat


class BrowserExportCrosscheckTests(unittest.TestCase):
    def test_crosschecks_values_and_outputs_aggregates_only(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            sessions, first, repeat = make_case()
            session_file, first_file, repeat_file, report_file = (
                root/'sessions.csv', root/'first.csv', root/'repeat.csv', root/'summary.json')
            session_file.write_bytes(sessions)
            first_file.write_bytes(first)
            repeat_file.write_bytes(repeat)
            result = m.analyse_files(session_file, [first_file, repeat_file], report_file,
                                     hashlib.sha256(sessions).hexdigest())
            report = json.loads(report_file.read_text(encoding='utf-8'))
            self.assertEqual(result['sessions'], 3)
            self.assertEqual(result['matched'], 2)
            self.assertEqual(report['session_summary']['completed'], 2)
            self.assertEqual(report['session_summary']['incomplete'], 1)
            self.assertEqual(report['browser_export_crosscheck']['completed_sessions_matched'], 2)
            self.assertEqual(report['browser_export_crosscheck']['mismatches'], 0)
            self.assertEqual(report['browser_export_crosscheck']['fields_compared'],
                             ['MAP01 active seconds', 'hits', 'coins'])
            self.assertFalse(report['provenance_verified'])
            self.assertEqual(report_file.stat().st_mode & 0o777, 0o600)
            self.assertEqual(hashlib.sha256(report_file.read_bytes()).hexdigest(), result['report_sha256'])
            encoded = json.dumps(report)
            for participant_id in ('P001-A1','P001-A2','P002-A1'):
                self.assertNotIn(participant_id, encoded)
            self.assertNotIn(str(session_file), encoded)

    def test_linked_but_mismatched_stage1_value_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            sessions, first, repeat = make_case(changed_first_seconds=401.0)
            sf=root/'sessions.csv'; a=root/'first.csv'; b=root/'repeat.csv'; out=root/'report.json'
            sf.write_bytes(sessions); a.write_bytes(first); b.write_bytes(repeat)
            with self.assertRaisesRegex(ValueError, 'disagree'):
                m.analyse_files(sf,[a,b],out)
            self.assertFalse(out.exists())

    def test_missing_unlinked_and_duplicate_exports_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            sessions, first, repeat=make_case()
            sf=root/'sessions.csv'; a=root/'first.csv'; b=root/'repeat.csv'; extra=root/'extra.csv'
            sf.write_bytes(sessions); a.write_bytes(first); b.write_bytes(repeat)
            with self.assertRaisesRegex(ValueError,'missing'):
                m.analyse_files(sf,[a],root/'missing.json')
            with self.assertRaisesRegex(ValueError,'duplicate'):
                m.analyse_files(sf,[a,a,b],root/'duplicate.json')
            extra.write_bytes(browser_csv(420,0,3))
            with self.assertRaisesRegex(ValueError,'unlinked'):
                m.analyse_files(sf,[a,b,extra],root/'extra.json')

    def test_bad_export_schema_and_pinned_session_hash_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            sessions, first, repeat=make_case()
            sf=root/'sessions.csv'; a=root/'first.csv'; b=root/'repeat.csv'
            sf.write_bytes(sessions); a.write_bytes(first); b.write_bytes(b'not a valid export')
            with self.assertRaises((ValueError, UnicodeError, csv.Error)):
                m.analyse_files(sf,[a,b],root/'bad.json')
            b.write_bytes(repeat)
            with self.assertRaisesRegex(ValueError,'SHA-256'):
                m.analyse_files(sf,[a,b],root/'pinned.json','0'*64)

    def test_output_no_overwrite_and_symlink_git_refusal(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp); sessions, first, repeat=make_case()
            sf=root/'sessions.csv'; a=root/'first.csv'; b=root/'repeat.csv'; out=root/'summary.json'
            sf.write_bytes(sessions); a.write_bytes(first); b.write_bytes(repeat)
            m.analyse_files(sf,[a,b],out)
            with self.assertRaisesRegex(ValueError,'overwrite'):
                m.analyse_files(sf,[a,b],out)
            alias=root/'alias.csv'; alias.symlink_to(a)
            with self.assertRaisesRegex(ValueError,'unsafe'):
                m.analyse_files(sf,[alias,b],root/'symlink.json')
            (root/'.git').mkdir()
            with self.assertRaisesRegex(ValueError,'Git checkout'):
                m.analyse_files(sf,[a,b],root/'git.json')

    def test_cli_prints_only_aggregates_and_hashes(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp); sessions, first, repeat=make_case()
            sf=root/'sessions.csv'; a=root/'first.csv'; b=root/'repeat.csv'; out=root/'summary.json'
            sf.write_bytes(sessions); a.write_bytes(first); b.write_bytes(repeat)
            run=subprocess.run([sys.executable,str(TOOL),'--sessions',str(sf),
                                '--browser-csv',str(a),'--browser-csv',str(b),
                                '--output',str(out)],capture_output=True,text=True)
            self.assertEqual(run.returncode,0,run.stderr)
            self.assertIn('sessions=3',run.stdout)
            self.assertIn('completed_exports_matched=2',run.stdout)
            self.assertNotIn('P001-A1',run.stdout)
            self.assertNotIn(str(sf),run.stdout)


if __name__ == '__main__':
    unittest.main(verbosity=2)
