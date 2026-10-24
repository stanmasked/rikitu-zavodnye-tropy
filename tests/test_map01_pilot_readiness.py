"""Stage176: synthetic-only controller pilot readiness tests."""
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
TOOL = ROOT / 'tools/analyze_map01_pilot_readiness.py'
spec = importlib.util.spec_from_file_location('map01_pilot_stage176', TOOL)
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)

BROWSER_HEADER = ('Уровень', 'Название', 'Тип', 'Секунды', 'Попадания',
                  'Монеты из 90', 'Цель от, с', 'Цель до, с', 'Сравнение')


def browser_csv(seconds, hits=2, coins=18):
    seconds = str(seconds)
    value = float(seconds)
    verdict = 'Ниже цели' if value < 360 else ('Выше цели' if value > 480 else 'В пределах цели')
    out = io.StringIO(newline='')
    writer = csv.writer(out, delimiter=';', lineterminator='\r\n', quoting=csv.QUOTE_ALL)
    writer.writerow(BROWSER_HEADER)
    writer.writerow(('1', 'Цветочная тропа', 'Обычный', seconds, str(hits), str(coins),
                     '360', '480', verdict))
    return ('\ufeff' + out.getvalue()).encode('utf-8')


def build_dataset(first_times, repeat_times, add_keyboard=True, add_incomplete=True):
    exports = {}
    rows = []
    for i, seconds in enumerate(first_times, start=1):
        sid = f'P{i:03d}-A1'
        data = browser_csv(seconds, 2, 18)
        filename = sid + '.csv'
        exports[filename] = data
        rows.append((sid, 'first', 'controller', 'completed', str(seconds), '2', '18',
                     '3', '3', '0', '1', 'yes', 'yes', 'none', filename,
                     hashlib.sha256(data).hexdigest()))
    for i, seconds in enumerate(repeat_times, start=1):
        sid = f'P{i:03d}-A2'
        data = browser_csv(seconds, 1, 42)
        filename = sid + '.csv'
        exports[filename] = data
        rows.append((sid, 'repeat', 'controller', 'completed', str(seconds), '1', '42',
                     '3', '3', '0', '2', 'yes', 'yes', 'none', filename,
                     hashlib.sha256(data).hexdigest()))
    if add_incomplete:
        rows.append(('P998-A1', 'first', 'controller', 'stopped', '220', '4', '6',
                     '1', '1', '2', '1', 'no', 'no', 'missed_fragment', '', ''))
    if add_keyboard:
        sid = 'P999-A1'
        data = browser_csv(450, 0, 10)
        filename = sid + '.csv'
        exports[filename] = data
        rows.append((sid, 'first', 'keyboard', 'completed', '450', '0', '10',
                     '3', '3', '0', '0', 'yes', 'no', 'none', filename,
                     hashlib.sha256(data).hexdigest()))
    out = io.StringIO(newline='')
    writer = csv.writer(out, delimiter=';', lineterminator='\r\n', quoting=csv.QUOTE_ALL)
    writer.writerow(m.SESSIONS.HEADER)
    writer.writerows(rows)
    return ('\ufeff' + out.getvalue()).encode('utf-8'), exports


def write_dataset(root, session_bytes, exports):
    session_file = root/'manual-sessions.csv'
    exports_dir = root/'browser_exports'
    exports_dir.mkdir()
    session_file.write_bytes(session_bytes)
    for name, data in exports.items():
        (exports_dir/name).write_bytes(data)
    return session_file, exports_dir


class PilotReadinessTests(unittest.TestCase):
    def test_controller_only_medians_and_privacy(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            sessions, exports = build_dataset(
                [400, 420, 390, 480, 360],
                [350, 400, 440, 460, 410])
            session_file, exports_dir = write_dataset(root, sessions, exports)
            output = root/'pilot-summary.json'
            result = m.analyse_files(session_file, exports_dir, output,
                                     hashlib.sha256(sessions).hexdigest())
            report = json.loads(output.read_text(encoding='utf-8'))
            self.assertEqual(result['sessions'], 12)
            self.assertEqual(result['controller_sessions'], 11)
            self.assertEqual(result['first_completed'], 5)
            self.assertEqual(result['repeat_completed'], 5)
            self.assertEqual(result['assessment'], 'PILOT_MEDIANS_WITHIN_TARGET_REVIEW_REQUIRED')
            self.assertEqual(report['controller_groups']['first']['completed_median_seconds'], '400.0')
            self.assertEqual(report['controller_groups']['repeat']['completed_median_seconds'], '410.0')
            self.assertEqual(report['controller_groups']['first']['completed_within_360_480_seconds'], 5)
            self.assertEqual(report['controller_groups']['repeat']['completed_within_360_480_seconds'], 4)
            self.assertEqual(report['session_counts']['non_controller_sessions_excluded_from_timing'], 1)
            self.assertEqual(report['session_counts']['input_method_counts'], {'controller': 11, 'keyboard': 1})
            paired = report['paired_controller_cohort']
            self.assertEqual(paired['matched_participants'], 5)
            self.assertEqual(paired['first_median_seconds'], '400.0')
            self.assertEqual(paired['repeat_median_seconds'], '410.0')
            self.assertEqual(paired['median_repeat_minus_first_seconds'], '-20.0')
            self.assertFalse(paired['affects_frozen_assessment'])
            self.assertEqual(report['controller_groups']['first']['incomplete'], 1)
            self.assertFalse(report['provenance_verified'])
            self.assertEqual(output.stat().st_mode & 0o777, 0o600)
            self.assertEqual(hashlib.sha256(output.read_bytes()).hexdigest(), result['report_sha256'])
            text = json.dumps(report)
            for sid in ('P001-A1', 'P001-A2', 'P998-A1', 'P999-A1'):
                self.assertNotIn(sid, text)
            self.assertNotIn(str(session_file), text)

    def test_matched_controller_pairs_exclude_mixed_input_and_incomplete_first(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            sessions, exports = build_dataset(
                [400, 420, 390, 480, 360],
                [350, 400, 440, 460, 410])
            parsed = list(csv.reader(
                io.StringIO(sessions.decode('utf-8-sig'), newline=''),
                delimiter=';', strict=True))
            header, rows = parsed[0], parsed[1:]
            for row in rows:
                if row[0] == 'P004-A1':
                    row[3] = 'stopped'
                    row[4] = '220'
                    row[5] = '4'
                    row[6] = '6'
                    row[7] = '1'
                    row[8] = '1'
                    row[9] = '2'
                    row[10] = '1'
                    row[11] = 'no'
                    row[12] = 'no'
                    row[13] = 'missed_fragment'
                    row[14] = ''
                    row[15] = ''
                elif row[0] == 'P005-A1':
                    row[2] = 'keyboard'
            exports.pop('P004-A1.csv')

            out = io.StringIO(newline='')
            writer = csv.writer(out, delimiter=';', lineterminator='\\r\\n',
                                quoting=csv.QUOTE_ALL)
            writer.writerow(header)
            writer.writerows(rows)
            sessions = ('\\ufeff' + out.getvalue()).encode('utf-8')

            session_file = root/'manual-sessions.csv'
            exports_dir = root/'browser_exports'
            exports_dir.mkdir()
            session_file.write_bytes(sessions)
            for name, data in exports.items():
                (exports_dir/name).write_bytes(data)

            output = root/'summary.json'
            m.analyse_files(session_file, exports_dir, output)
            report = json.loads(output.read_text(encoding='utf-8'))
            paired = report['paired_controller_cohort']
            self.assertEqual(paired['matched_participants'], 3)
            self.assertEqual(paired['first_median_seconds'], '400.0')
            self.assertEqual(paired['repeat_median_seconds'], '400.0')
            self.assertEqual(paired['median_repeat_minus_first_seconds'], '-20.0')
            # The frozen assessment still uses all controller runs and remains
            # insufficient here because only three controller A1 runs completed.
            self.assertEqual(report['pacing_assessment'],
                             'INSUFFICIENT_COMPLETED_CONTROLLER_RUNS')

    def test_fewer_than_five_completed_per_group_is_insufficient(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            sessions, exports = build_dataset([400, 420, 390, 480], [400, 410, 430, 450],
                                               add_keyboard=False, add_incomplete=False)
            session_file, exports_dir = write_dataset(root, sessions, exports)
            result = m.analyse_files(session_file, exports_dir, root/'summary.json')
            self.assertEqual(result['assessment'], 'INSUFFICIENT_COMPLETED_CONTROLLER_RUNS')

    def test_outside_target_median_is_review_signal_not_pass(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            sessions, exports = build_dataset([520, 530, 510, 525, 515], [400, 410, 420, 430, 440],
                                               add_keyboard=False, add_incomplete=False)
            session_file, exports_dir = write_dataset(root, sessions, exports)
            result = m.analyse_files(session_file, exports_dir, root/'summary.json')
            self.assertEqual(result['assessment'], 'PILOT_MEDIANS_OUTSIDE_TARGET_REVIEW_REQUIRED')

    def test_missing_export_and_hash_pin_fail_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            sessions, exports = build_dataset([400, 420, 390, 480, 360], [400, 410, 430, 450, 420],
                                               add_keyboard=False, add_incomplete=False)
            session_file, exports_dir = write_dataset(root, sessions, exports)
            (exports_dir/'P005-A2.csv').unlink()
            with self.assertRaisesRegex(ValueError, 'missing'):
                m.analyse_files(session_file, exports_dir, root/'missing.json')
            (exports_dir/'P005-A2.csv').write_bytes(exports['P005-A2.csv'])
            with self.assertRaisesRegex(ValueError, 'SHA-256'):
                m.analyse_files(session_file, exports_dir, root/'pin.json', '0'*64)

    def test_output_location_overwrite_and_cli_privacy(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            sessions, exports = build_dataset([400, 420, 390, 480, 360], [400, 410, 430, 450, 420])
            session_file, exports_dir = write_dataset(root, sessions, exports)
            with self.assertRaisesRegex(ValueError, 'outside browser export'):
                m.analyse_files(session_file, exports_dir, exports_dir/'report.json')
            output = root/'summary.json'
            run = subprocess.run([sys.executable, str(TOOL), '--sessions', str(session_file),
                                  '--exports-dir', str(exports_dir), '--output', str(output)],
                                 capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)
            self.assertIn('pacing_assessment=PILOT_MEDIANS_WITHIN_TARGET_REVIEW_REQUIRED', run.stdout)
            self.assertNotIn('P001-A1', run.stdout)
            self.assertNotIn(str(session_file), run.stdout)
            with self.assertRaisesRegex(ValueError, 'overwrite'):
                m.analyse_files(session_file, exports_dir, output)


if __name__ == '__main__':
    unittest.main(verbosity=2)
