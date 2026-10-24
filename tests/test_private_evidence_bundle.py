"""Stage158 synthetic-only ZIP/Drive-handoff tests; no real pixels or saves."""
import hashlib
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from private_evidence_readback import EvidenceError, seal
from private_evidence_bundle import pack, verify_readback


class PrivateEvidenceBundleTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        root = Path(self.tmp.name)
        self.root = root
        self.source = root / "synthetic-source"
        self.source.mkdir()
        (self.source / "frames").mkdir()
        (self.source / "frames" / "image.png").write_bytes(b"SYNTHETIC ONLY PIXELS")
        (self.source / "guest.log").write_bytes(b"SYNTHETIC ONLY TRACE\n")
        self.receipts = root / "receipts"
        self.receipts.mkdir()
        self.manifest = self.receipts / "run-v1.json"
        self.bundle = root / "private-out" / "RIKITU_capture-v1.zip"
        self.bundle.parent.mkdir()
        self.readback = root / "independent-download" / "RIKITU_capture-v1.zip"
        self.readback.parent.mkdir()
        self.receipt_sha = None

    def seal_private(self):
        result = seal(self.source, self.manifest)
        self.receipt_sha = result["manifest_sha256"]
        return result

    def make_bundle(self):
        return pack(self.source, self.manifest, self.receipt_sha, self.bundle)

    def copy_to_readback(self):
        shutil.copy2(self.bundle, self.readback)

    def test_roundtrip_and_exact_zip_members(self):
        self.seal_private()
        result = self.make_bundle()
        self.assertEqual(result["result"], "PRIVATE_BUNDLE_CREATED_NOT_UPLOADED")
        self.assertEqual(result["files"], 2)
        self.assertEqual(self.bundle.stat().st_mode & 0o777, 0o600)
        with zipfile.ZipFile(self.bundle) as archive:
            self.assertEqual(archive.namelist(), [
                "PRIVATE_EVIDENCE_MANIFEST.json",
                "evidence/frames/image.png", "evidence/guest.log",
            ])
            self.assertEqual(archive.read("evidence/frames/image.png"),
                             b"SYNTHETIC ONLY PIXELS")
        self.copy_to_readback()
        after = verify_readback(self.readback, result["bundle_sha256"],
                                self.receipt_sha)
        self.assertEqual(after["files"], 2)
        self.assertEqual(after["payload_bytes"], result["payload_bytes"])
        self.assertEqual(after["result"],
                         "BYTE_MATCH_ONLY_REQUIRES_AUTHENTICATED_DRIVE_FETCH")

    def test_readback_wrong_zip_sha_fails(self):
        self.seal_private()
        record = self.make_bundle()
        self.copy_to_readback()
        with self.readback.open("ab") as fd:
            fd.write(b"TAMPER")
        with self.assertRaisesRegex(EvidenceError, "private-bundle-readback-sha-mismatch"):
            verify_readback(self.readback, record["bundle_sha256"], self.receipt_sha)

    def test_source_changed_after_seal_rejected(self):
        self.seal_private()
        (self.source / "guest.log").write_bytes(b"ALTERED SOURCE DATA\n")
        with self.assertRaisesRegex(EvidenceError, "source-changed-since-seal"):
            self.make_bundle()
        self.assertFalse(self.bundle.exists())

    def test_wrong_pinned_receipt_does_not_write(self):
        self.seal_private()
        self.receipt_sha = "a" * 64
        with self.assertRaisesRegex(EvidenceError, "pinned-receipt-sha-mismatch"):
            self.make_bundle()
        self.assertFalse(self.bundle.exists())

    def test_cannot_overwrite_existing_zip(self):
        self.seal_private()
        self.bundle.write_bytes(b"PRIVATE IMMUTABLE")
        with self.assertRaisesRegex(EvidenceError, "bundle-version-already-exists"):
            self.make_bundle()
        self.assertEqual(self.bundle.read_bytes(), b"PRIVATE IMMUTABLE")

    def test_zip_cannot_be_generated_within_evidence_tree(self):
        self.seal_private()
        with self.assertRaisesRegex(EvidenceError, "bundle-inside-private-evidence"):
            pack(self.source, self.manifest, self.receipt_sha,
                 self.source / "RIKITU_forbidden.zip")
        self.assertFalse((self.source / "RIKITU_forbidden.zip").exists())

    def test_zip_with_extra_member_is_rejected_even_with_recomputed_digest(self):
        self.seal_private()
        self.make_bundle()
        self.copy_to_readback()
        with zipfile.ZipFile(self.readback, "a") as archive:
            archive.writestr("unexpected.txt", b"fake")
        forged_zip_sha = hashlib.sha256(self.readback.read_bytes()).hexdigest()
        with self.assertRaisesRegex(EvidenceError, "unexpected-bundle-member"):
            verify_readback(self.readback, forged_zip_sha, self.receipt_sha)

    def test_symlink_source_and_readback_zip_rejected(self):
        self.seal_private()
        leak = self.root / "outside.dat"
        leak.write_bytes(b"NO FOLLOW")
        (self.source / "linked.png").symlink_to(leak)
        with self.assertRaisesRegex(EvidenceError, "symlink-in-private-evidence"):
            self.make_bundle()
        (self.source / "linked.png").unlink()
        result = self.make_bundle()
        self.readback.symlink_to(self.bundle)
        with self.assertRaisesRegex(EvidenceError, "missing-or-symlink-private-bundle"):
            verify_readback(self.readback, result["bundle_sha256"], self.receipt_sha)

    def test_manifest_rewrite_not_accepted(self):
        self.seal_private()
        record = json.loads(self.manifest.read_text())
        record["entries"][0]["sha256"] = "f" * 64
        self.manifest.write_text(json.dumps(record))
        with self.assertRaisesRegex(EvidenceError, "pinned-receipt-sha-mismatch"):
            self.make_bundle()

    def test_same_bytes_same_zip_sha(self):
        self.seal_private()
        first = self.make_bundle()
        second_path = self.bundle.parent / "RIKITU_capture-v2.zip"
        second = pack(self.source, self.manifest, self.receipt_sha, second_path)
        self.assertEqual(first["bundle_sha256"], second["bundle_sha256"])
        self.assertEqual(first["bundle_bytes"], second["bundle_bytes"])


if __name__ == "__main__":
    unittest.main()
