"""Stage173/175: synthetic-only tests for anonymized MAP01 session summaries."""
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


def rows():
    return [
        ('P001-A1','first','controller','completed','400.0','2','18','3','3','0','1','yes','yes','none','P001-A1.csv','a'*64),
        ('P001-A2','repeat','controller','completed','335.0','1','42','3','3','0','2','yes','yes','none','P001-A2.csv','b'*64),
        ('P002-A1','first','keyboard','stopped','220.0','4','6','1','1','2','1','no','no','missed_fragment','',''),
        ('P003-A1','first','controller','completed','480.0','0','55','3','3','0','0','uncertain','no','none','P003-A1.csv','c'*64),
    ]


def fixture(data_rows=None):
    out = io.StringIO(newline='')
    writer = csv.writer(out, delimiter=';', lineterminator='\r\n', quoting=csv.QUOTE_ALL)
    writer.writerow(m.HEADER)
    for row in rows() if data_rows is None else data_rows:
        writer.writerow(row)
    return ('\ufeff' + out.getvalue()).encode('utf-8')


class SessionAnalyzerTests(unittest.TestCase):
    def test_aggregates_only_and_never_authenticates_provenance(self):
        raw=fixture()
        report=m.analyse(raw)
        self.assertEqual(report['schema'],'rikitu-map01-human-playtest-summary-v2')
        self.assertFalse(report['provenance_verified'])
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
        self.assertEqual(report['linked_browser_exports'],3)
        self.assertEqual(report['unique_browser_export_hashes'],3)
        encoded=json.dumps(report)
        for private_id in ('P001-A1','P001-A2','P002-A1','P003-A1'):
            self.assertNotIn(private_id,encoded)

    def test_identical_export_content_hash_is_allowed_for_distinct_files(self):
        data=rows()
        data[1]=data[1][:15]+(data[0][15],)
        report=m.analyse(fixture(data))
        self.assertEqual(report['linked_browser_exports'],3)
        self.assertEqual(report['unique_browser_export_hashes'],2)
        self.assertEqual(len(report['linked_browser_export_sha256s']),2)

    def test_strict_rejections(self):
        good=rows()
        invalid=[]
        bad=list(good);bad[0]=bad[0][:4]+('NaN',)+bad[0][5:];invalid.append(bad)
        bad=list(good);bad[0]=bad[0][:2]+('real-person-name',)+bad[0][3:];invalid.append(bad)
        bad=list(good);bad[0]=('P001-A2',)+bad[0][1:];invalid.append(bad)
        bad=list(good);bad[0]=bad[0][:1]+('repeat',)+bad[0][2:];invalid.append(bad)
        bad=list(good);bad[0]=bad[0][:7]+('2',)+bad[0][8:];invalid.append(bad)
        bad=list(good);bad[0]=bad[0][:11]+('no',)+bad[0][12:];invalid.append(bad)
        bad=list(good);bad[0]=bad[0][:14]+('P001-A2.csv',)+bad[0][15:];invalid.append(bad)
        bad=list(good);bad[0]=bad[0][:15]+('z'*64,);invalid.append(bad)
        bad=list(good);bad[2]=bad[2][:14]+('P002-A1.csv',)+bad[2][15:];invalid.append(bad)
        bad=list(good);bad[2]=bad[2][:13]+('none',)+bad[2][14:];invalid.append(bad)
        bad=list(good);bad[2]=bad[2][:6]+('91',)+bad[2][7:];invalid.append(bad)
        bad=list(good);bad[2]=bad[2][:13]+('unknown',)+bad[2][14:];invalid.append(bad)
        for data_rows in invalid:
            with self.subTest(row=data_rows[0][0]):
                with self.assertRaises(ValueError):
                    m.analyse(fixture(data_rows))
        raw=fixture()
        for data in (b'',b'not csv',raw+b'\x00',b'\xff'*20,raw+b'x'*m.MAX_BYTES):
            with self.subTest(length=len(data)):
                with self.assertRaises((ValueError,UnicodeError,csv.Error)):
                    m.analyse(data)

    def test_incomplete_attempt_needs_stop_reason_and_blank_browser_link(self):
        bad=rows()
        bad[2]=bad[2][:13]+('none',)+bad[2][14:]
        with self.assertRaises(ValueError):
            m.analyse(fixture(bad))
        bad=rows()
        bad[2]=bad[2][:14]+('P002-A1.csv',)+bad[2][15:]
        with self.assertRaises(ValueError):
            m.analyse(fixture(bad))

    def test_private_write_integrity_permissions_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);source=root/'sessions.csv';dest=root/'summary.json'
            raw=fixture();source.write_bytes(raw)
            pin=hashlib.sha256(raw).hexdigest()
            result=m.private_report(source,dest,pin)
            self.assertEqual(result['sessions'],4)
            self.assertEqual(dest.stat().st_mode&0o777,0o600)
            report=json.loads(dest.read_text(encoding='utf-8'))
            self.assertEqual(report['source_sha256'],pin)
            self.assertEqual(hashlib.sha256(dest.read_bytes()).hexdigest(),result['report_sha256'])
            with self.assertRaises(ValueError):
                m.private_report(source,dest)
            with self.assertRaises(ValueError):
                m.private_report(source,root/'wrong.json','0'*64)
            self.assertFalse((root/'wrong.json').exists())

    def test_reject_symlink_and_git_checkout(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            (root/'source.csv').write_bytes(fixture())
            (root/'alias.csv').symlink_to(root/'source.csv')
            with self.assertRaises(ValueError):
                m.private_report(root/'alias.csv',root/'out.json')
            (root/'.git').mkdir()
            with self.assertRaises(ValueError):
                m.private_report(root/'source.csv',root/'out.json')

    def test_cli_prints_only_aggregates_and_hashes(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);source=root/'notes.csv';dest=root/'summary.json'
            source.write_bytes(fixture())
            run=subprocess.run([sys.executable,str(TOOL),'--input',str(source),'--output',str(dest)],
                               capture_output=True,text=True)
            self.assertEqual(run.returncode,0,run.stderr)
            self.assertIn('sessions=4',run.stdout)
            for private_id in ('P001-A1','P001-A2','P002-A1','P003-A1'):
                self.assertNotIn(private_id,run.stdout)
            self.assertNotIn(str(source),run.stdout)
            self.assertFalse(json.loads(dest.read_text('utf-8'))['provenance_verified'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
