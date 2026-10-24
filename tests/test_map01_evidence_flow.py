"""Stage175: synthetic end-to-end MAP01 cross-check and evidence handoff test."""
import csv
import hashlib
import importlib.util
import io
from pathlib import Path
import shutil
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from private_evidence_readback import EvidenceError, seal, verify
from private_evidence_bundle import pack, verify_readback

TOOL = ROOT / 'tools/crosscheck_map01_playtest_exports.py'
spec = importlib.util.spec_from_file_location('map01_crosscheck_stage175', TOOL)
crosscheck = importlib.util.module_from_spec(spec)
spec.loader.exec_module(crosscheck)

BROWSER_HEADER = ('Уровень', 'Название', 'Тип', 'Секунды', 'Попадания',
                  'Монеты из 90', 'Цель от, с', 'Цель до, с', 'Сравнение')


def make_browser_export():
    out = io.StringIO(newline='')
    writer = csv.writer(out, delimiter=';', lineterminator='\\r\\n', quoting=csv.QUOTE_ALL)
    writer.writerow(BROWSER_HEADER)
    writer.writerow(('1', 'Цветочная тропа', 'Обычный', '400.0', '2', '18',
                     '360', '480', 'В пределах цели'))
    return ('\\ufeff' + out.getvalue()).encode('utf-8')


def make_session_log(export_bytes):
    export_sha = hashlib.sha256(export_bytes).hexdigest()
    out = io.StringIO(newline='')
    writer = csv.writer(out, delimiter=';', lineterminator='\\r\\n', quoting=csv.QUOTE_ALL)
    writer.writerow(crosscheck.SESSIONS.HEADER)
    writer.writerow(('P001-A1','first','controller','completed','400.0','2','18',
                     '3','3','0','1','yes','yes','none',export_sha))
    writer.writerow(('P002-A1','first','controller','stopped','220.0','4','6',
                     '1','1','2','1','no','no','missed_fragment',''))
    return ('\\ufeff' + out.getvalue()).encode('utf-8')


class Map01EvidenceFlowTests(unittest.TestCase):
    def test_crosscheck_manifest_bundle_and_independent_readback(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root/'private-source'
            receipts = root/'receipts'
            bundles = root/'bundles'
            payload_readback = root/'independent-payload-readback'
            zip_readback = root/'independent-zip-readback'
            for folder in (source, receipts, bundles, payload_readback, zip_readback):
                folder.mkdir()

            export_bytes = make_browser_export()
            session_bytes = make_session_log(export_bytes)
            sessions = source/'manual-sessions.csv'
            browser_csv = source/'P001-A1-browser.csv'
            report = source/'map01-crosscheck.json'
            sessions.write_bytes(session_bytes)
            browser_csv.write_bytes(export_bytes)

            checked = crosscheck.analyse_files(
                sessions, [browser_csv], report, hashlib.sha256(session_bytes).hexdigest())
            self.assertEqual(checked['sessions'], 2)
            self.assertEqual(checked['matched'], 1)
            self.assertEqual(report.stat().st_mode & 0o777, 0o600)
            report_text = report.read_text(encoding='utf-8')
            self.assertNotIn('P001-A1', report_text)
            self.assertNotIn(str(sessions), report_text)

            manifest = receipts/'map01-evidence-v1.json'
            sealed = seal(source, manifest)
            self.assertEqual(sealed['files'], 3)
            self.assertEqual(manifest.stat().st_mode & 0o777, 0o600)

            bundle = bundles/'RIKITU_MAP01_Synthetic_v1.zip'
            packed = pack(source, manifest, sealed['manifest_sha256'], bundle)
            self.assertEqual(packed['files'], 3)
            self.assertEqual(bundle.stat().st_mode & 0o777, 0o600)

            shutil.copytree(source, payload_readback, dirs_exist_ok=True)
            payload_verified = verify(manifest, payload_readback, sealed['manifest_sha256'])
            self.assertEqual(payload_verified['files'], 3)
            self.assertEqual(payload_verified['result'],
                             'BYTE_MATCH_ONLY_REQUIRES_AUTHENTICATED_DRIVE_FETCH')

            downloaded_zip = zip_readback/'RIKITU_MAP01_Synthetic_v1.zip'
            shutil.copy2(bundle, downloaded_zip)
            zip_verified = verify_readback(downloaded_zip, packed['bundle_sha256'],
                                           sealed['manifest_sha256'])
            self.assertEqual(zip_verified['files'], 3)
            self.assertEqual(zip_verified['bundle_sha256'], packed['bundle_sha256'])
            self.assertEqual(zip_verified['result'],
                             'BYTE_MATCH_ONLY_REQUIRES_AUTHENTICATED_DRIVE_FETCH')

            # Tamper with the independent payload copy: the sealed manifest must fail.
            (payload_readback/'P001-A1-browser.csv').write_bytes(b'altered synthetic CSV')
            with self.assertRaisesRegex(EvidenceError, 'private-readback-mismatch'):
                verify(manifest, payload_readback, sealed['manifest_sha256'])

    def test_source_change_after_seal_cannot_be_bundled(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            source=root/'source'; source.mkdir()
            receipts=root/'receipts'; receipts.mkdir()
            bundles=root/'bundles'; bundles.mkdir()
            export=make_browser_export()
            (source/'synthetic.csv').write_bytes(export)
            manifest=receipts/'manifest.json'
            sealed=seal(source,manifest)
            (source/'synthetic.csv').write_bytes(export+b'changed')
            with self.assertRaisesRegex(EvidenceError,'source-changed-since-seal'):
                pack(source,manifest,sealed['manifest_sha256'],bundles/'RIKITU_tampered.zip')
            self.assertFalse((bundles/'RIKITU_tampered.zip').exists())


if __name__ == '__main__':
    unittest.main(verbosity=2)
