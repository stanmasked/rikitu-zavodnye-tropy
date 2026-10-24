"""Stage174: synthetic-only tests for private session/CSV cross-checks."""
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

TOOL = Path(__file__).resolve().parents[1] / 'tools/analyze_map01_playtest_sessions.py'
spec = importlib.util.spec_from_file_location('map01_sessions', TOOL)
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)

BROWSER_HEADER = ('Уровень','Название','Тип','Секунды','Попадания',
                  'Монеты из 90','Цель от, с','Цель до, с','Сравнение')


def browser_export(seconds, hits, coins):
    value=float(seconds)
    verdict='Ниже цели' if value < 360 else ('Выше цели' if value > 480 else 'В пределах цели')
    out=io.StringIO(newline='')
    writer=csv.writer(out,delimiter=';',lineterminator='\r\n',quoting=csv.QUOTE_ALL)
    writer.writerow(BROWSER_HEADER)
    writer.writerow(('1','Цветочная тропа','Обычный',seconds,hits,coins,'360','480',verdict))
    return ('\ufeff'+out.getvalue()).encode('utf-8')


def rows_and_exports():
    exports={
        'P001-A1.csv':browser_export('400.0',2,18),
        'P001-A2.csv':browser_export('335.0',1,42),
        'P003-A1.csv':browser_export('480.0',0,55),
    }
    digest={name:hashlib.sha256(data).hexdigest() for name,data in exports.items()}
    sessions=[
        ('P001-A1','first','controller','completed','400.0','2','18','3','3','0','1','yes','yes','none','P001-A1.csv',digest['P001-A1.csv']),
        ('P001-A2','repeat','controller','completed','335.0','1','42','3','3','0','2','yes','yes','none','P001-A2.csv',digest['P001-A2.csv']),
        ('P002-A1','first','keyboard','stopped','220.0','4','6','1','1','2','1','no','no','missed_fragment','',''),
        ('P003-A1','first','controller','completed','480.0','0','55','3','3','0','0','uncertain','no','none','P003-A1.csv',digest['P003-A1.csv']),
    ]
    return sessions,exports


def fixture(data_rows=None):
    session_rows,exports=rows_and_exports()
    out=io.StringIO(newline='')
    writer=csv.writer(out,delimiter=';',lineterminator='\r\n',quoting=csv.QUOTE_ALL)
    writer.writerow(m.HEADER)
    for row in session_rows if data_rows is None else data_rows:
        writer.writerow(row)
    return ('\ufeff'+out.getvalue()).encode('utf-8'),exports


def export_metadata(export_bytes):
    result={}
    for name,data in export_bytes.items():
        parsed=m._BROWSER.analyse(data,'unverified')
        stage=parsed['stages'][0]
        result[name]={'sha256':hashlib.sha256(data).hexdigest(),
                      'stage':stage['stage'],'seconds':m.Decimal(stage['active_seconds']),
                      'hits':stage['hits'],'coins':stage['coins']}
    return result


def write_exports(folder, export_bytes):
    folder.mkdir(parents=True,exist_ok=True)
    for name,data in export_bytes.items():
        (folder/name).write_bytes(data)


class SessionAnalyzerTests(unittest.TestCase):
    def test_aggregates_and_exact_browser_cross_checks(self):
        raw,exports=fixture()
        report=m.analyse(raw,export_metadata(exports))
        self.assertEqual(report['schema'],'rikitu-map01-human-playtest-summary-v2')
        self.assertFalse(report['provenance_verified'])
        self.assertTrue(report['browser_exports_cross_checked'])
        self.assertEqual(report['counts']['sessions'],4)
        self.assertEqual(report['counts']['first_attempts'],3)
        self.assertEqual(report['counts']['repeat_attempts'],1)
        self.assertEqual(report['counts']['completed'],3)
        self.assertEqual(report['counts']['incomplete'],1)
        self.assertEqual(report['counts']['first_completion_within_360_480s'],2)
        self.assertEqual(report['counts']['repeat_completion_within_360_480s'],0)
        self.assertEqual(report['completion_times_seconds']['first_attempt_median'],'440.0')
        self.assertEqual(report['completion_times_seconds']['repeat_attempt_median'],'335.0')
        self.assertEqual(report['counts']['total_hits'],7)
        self.assertEqual(report['counts']['total_coins'],121)
        self.assertEqual(report['counts']['total_respawns'],2)
        self.assertEqual(report['counts']['total_backtrack_events'],4)
        self.assertEqual(report['stop_reasons'],{'missed_fragment':1})
        self.assertEqual(report['linked_browser_exports_verified'],3)
        self.assertEqual(report['unique_browser_export_hashes'],3)
        encoded=json.dumps(report)
        for private_id in ('P001-A1','P001-A2','P002-A1','P003-A1'):
            self.assertNotIn(private_id,encoded)

    def test_identical_export_bytes_allowed_for_distinct_named_sessions(self):
        identical = browser_export('400.0', 2, 18)
        digest = hashlib.sha256(identical).hexdigest()
        session_rows = [
            ('P001-A1','first','controller','completed','400.0','2','18','3','3','0','1','yes','yes','none','P001-A1.csv',digest),
            ('P001-A2','repeat','controller','completed','400.0','2','18','3','3','0','1','yes','yes','none','P001-A2.csv',digest),
        ]
        out = io.StringIO(newline='')
        writer = csv.writer(out, delimiter=';', lineterminator='\\r\\n', quoting=csv.QUOTE_ALL)
        writer.writerow(m.HEADER)
        writer.writerows(session_rows)
        raw = ('\\ufeff' + out.getvalue()).encode('utf-8')
        exports = {'P001-A1.csv': identical, 'P001-A2.csv': identical}
        report = m.analyse(raw, export_metadata(exports))
        self.assertEqual(report['counts']['completed'], 2)
        self.assertEqual(report['linked_browser_exports_verified'], 2)
        self.assertEqual(report['unique_browser_export_hashes'], 1)
        self.assertEqual(report['linked_browser_export_sha256s'], [digest])

        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root/'sessions.csv'
            destination = root/'summary.json'
            exports_dir = root/'exports'
            source.write_bytes(raw)
            write_exports(exports_dir, exports)
            result = m.private_report(source, destination, exports_dir,
                                      hashlib.sha256(raw).hexdigest())
            self.assertEqual(result['linked_exports'], 2)
            self.assertEqual(json.loads(destination.read_text('utf-8'))['unique_browser_export_hashes'], 1)

    def test_strict_rejections(self):
        good,exports=fixture()
        metadata=export_metadata(exports)
        original,_=rows_and_exports()
        invalid=[]
        bad=list(original);bad[0]=bad[0][:4]+('NaN',)+bad[0][5:];invalid.append(bad)
        bad=list(original);bad[0]=bad[0][:2]+('real-person-name',)+bad[0][3:];invalid.append(bad)
        bad=list(original);bad[0]=('P001-A2',)+bad[0][1:];invalid.append(bad)
        bad=list(original);bad[0]=bad[0][:1]+('repeat',)+bad[0][2:];invalid.append(bad)
        bad=list(original);bad[0]=bad[0][:7]+('2',)+bad[0][8:];invalid.append(bad)
        bad=list(original);bad[0]=bad[0][:11]+('no',)+bad[0][12:];invalid.append(bad)
        bad=list(original);bad[0]=bad[0][:14]+('P001-A2.csv',)+bad[0][15:];invalid.append(bad)
        bad=list(original);bad[0]=bad[0][:15]+('0'*64,);invalid.append(bad)
        bad=list(original);bad[2]=bad[2][:14]+('P002-A1.csv',)+bad[2][15:];invalid.append(bad)
        bad=list(original);bad[2]=bad[2][:13]+('none',)+bad[2][14:];invalid.append(bad)
        bad=list(original);bad[2]=bad[2][:6]+('91',)+bad[2][7:];invalid.append(bad)
        bad=list(original);bad[2]=bad[2][:13]+('unknown',)+bad[2][14:];invalid.append(bad)
        for data_rows in invalid:
            with self.subTest(first_id=data_rows[0][0]):
                with self.assertRaises(ValueError):
                    m.analyse(fixture(data_rows)[0],metadata)
        for raw in (b'',b'not csv',good+b'\x00',b'\xff'*20,good+b'x'*m.MAX_BYTES):
            with self.subTest(length=len(raw)):
                with self.assertRaises((ValueError,UnicodeError,csv.Error)):
                    m.analyse(raw,metadata)

    def test_source_export_missing_hash_or_values_mismatch(self):
        raw,exports=fixture()
        metadata=export_metadata(exports)
        with self.assertRaises(ValueError):
            m.analyse(raw,{})
        bad=dict(metadata)
        bad['P001-A1.csv']=dict(metadata['P001-A1.csv'],hits=999)
        with self.assertRaises(ValueError):
            m.analyse(raw,bad)
        bad=dict(metadata)
        bad['P001-A1.csv']=dict(metadata['P001-A1.csv'],sha256='0'*64)
        with self.assertRaises(ValueError):
            m.analyse(raw,bad)
        bad=dict(metadata)
        bad.pop('P003-A1.csv')
        with self.assertRaises(ValueError):
            m.analyse(raw,bad)
        bad=dict(metadata)
        bad['P004-A1.csv']=dict(metadata['P003-A1.csv'])
        with self.assertRaises(ValueError):
            m.analyse(raw,bad)

    def test_incomplete_attempt_requires_reason_and_blank_export_link(self):
        original,exports=rows_and_exports()
        bad=list(original)
        bad[2]=bad[2][:13]+('none',)+bad[2][14:]
        with self.assertRaises(ValueError):
            m.analyse(fixture(bad)[0],export_metadata(exports))
        bad=list(original)
        bad[2]=bad[2][:14]+('P002-A1.csv',)+bad[2][15:]
        with self.assertRaises(ValueError):
            m.analyse(fixture(bad)[0],export_metadata(exports))

    def test_private_write_integrity_permissions_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);source=root/'sessions.csv';dest=root/'summary.json';exports_dir=root/'exports'
            raw,exports=fixture();source.write_bytes(raw);write_exports(exports_dir,exports)
            pin=hashlib.sha256(raw).hexdigest()
            result=m.private_report(source,dest,exports_dir,pin)
            self.assertEqual(result['sessions'],4)
            self.assertEqual(result['linked_exports'],3)
            self.assertEqual(dest.stat().st_mode&0o777,0o600)
            report=json.loads(dest.read_text(encoding='utf-8'))
            self.assertEqual(report['source_sha256'],pin)
            self.assertTrue(report['browser_exports_cross_checked'])
            self.assertEqual(hashlib.sha256(dest.read_bytes()).hexdigest(),result['report_sha256'])
            with self.assertRaises(ValueError):
                m.private_report(source,dest,exports_dir)
            with self.assertRaises(ValueError):
                m.private_report(source,root/'wrong.json',exports_dir,'0'*64)
            self.assertFalse((root/'wrong.json').exists())

    def test_export_directory_rejects_unreferenced_files_and_symlinks(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);source=root/'sessions.csv';dest=root/'summary.json';exports_dir=root/'exports'
            raw,exports=fixture();source.write_bytes(raw);write_exports(exports_dir,exports)
            (exports_dir/'P004-A1.csv').write_bytes(browser_export('410.0',1,10))
            with self.assertRaises(ValueError):
                m.private_report(source,dest,exports_dir)
            (exports_dir/'P004-A1.csv').unlink()
            (exports_dir/'alias.csv').symlink_to(exports_dir/'P001-A1.csv')
            with self.assertRaises(ValueError):
                m.private_report(source,dest,exports_dir)

    def test_reject_git_checkout_and_symlink_input(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);source=root/'sessions.csv';dest=root/'summary.json';exports_dir=root/'exports'
            raw,exports=fixture();source.write_bytes(raw);write_exports(exports_dir,exports)
            (root/'alias.csv').symlink_to(source)
            with self.assertRaises(ValueError):
                m.private_report(root/'alias.csv',dest,exports_dir)
            (root/'.git').mkdir()
            with self.assertRaises(ValueError):
                m.private_report(source,dest,exports_dir)

    def test_cli_prints_only_aggregates_and_hashes(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);source=root/'notes.csv';dest=root/'summary.json';exports_dir=root/'exports'
            raw,exports=fixture();source.write_bytes(raw);write_exports(exports_dir,exports)
            run=subprocess.run([sys.executable,str(TOOL),'--input',str(source),
                                '--exports-dir',str(exports_dir),'--output',str(dest)],
                               capture_output=True,text=True)
            self.assertEqual(run.returncode,0,run.stderr)
            self.assertIn('sessions=4',run.stdout)
            self.assertIn('linked_exports=3',run.stdout)
            for private_id in ('P001-A1','P001-A2','P002-A1','P003-A1'):
                self.assertNotIn(private_id,run.stdout)
            self.assertNotIn(str(source),run.stdout)
            self.assertFalse(json.loads(dest.read_text('utf-8'))['provenance_verified'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
