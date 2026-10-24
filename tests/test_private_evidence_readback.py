"""Stage157 synthetic-only evidence receipt tests; no private CI images or saves."""
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from private_evidence_readback import EvidenceError, seal, verify


class PrivateEvidenceReadbackTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.runner = self.root / "runner-only"
        self.runner.mkdir()
        self.readback = self.root / "authenticated-download"
        self.readback.mkdir()
        self.private_receipts = self.root / "private-manifests"
        self.private_receipts.mkdir()
        self.manifest = self.private_receipts / "receipt-v1.json"
        (self.runner / "frames").mkdir()
        (self.runner / "frames" / "capture-01.png").write_bytes(b"FAKE-PNG-PIXELS")
        (self.runner / "guest.log").write_bytes(b"synthetic-marker\n")

    def download_simulated(self):
        shutil.copytree(self.runner, self.readback, dirs_exist_ok=True)

    def test_seal_and_independent_readback(self):
        receipt = seal(self.runner, self.manifest)
        self.assertEqual(receipt["files"], 2)
        self.assertEqual(receipt["result"], "PRIVATE_MANIFEST_SEALED_NOT_ARCHIVED")
        self.assertEqual(self.manifest.stat().st_mode & 0o777, 0o600)
        record = json.loads(self.manifest.read_bytes())
        self.assertEqual(
            [x["path"] for x in record["entries"]],
            ["frames/capture-01.png", "guest.log"])
        self.assertEqual(record["status"], "PENDING_DRIVE_READBACK")
        self.download_simulated()
        verified = verify(self.manifest, self.readback)
        self.assertEqual(verified["files"], 2)
        self.assertEqual(
            verified["result"],
            "BYTE_MATCH_ONLY_REQUIRES_AUTHENTICATED_DRIVE_FETCH")
        self.assertEqual(verified["manifest_sha256"], receipt["manifest_sha256"])

    def test_mismatched_bytes_rejected_even_when_length_same(self):
        seal(self.runner, self.manifest)
        self.download_simulated()
        (self.readback / "guest.log").write_bytes(b"synthetic-markerX")
        with self.assertRaisesRegex(EvidenceError, "private-readback-mismatch"):
            verify(self.manifest, self.readback)

    def test_missing_and_extra_files_are_failures(self):
        seal(self.runner, self.manifest)
        self.download_simulated()
        (self.readback / "guest.log").unlink()
        with self.assertRaisesRegex(EvidenceError, "private-readback-mismatch"):
            verify(self.manifest, self.readback)
        (self.readback / "guest.log").write_bytes(b"synthetic-marker\n")
        (self.readback / "extra.dat").write_bytes(b"unexpected")
        with self.assertRaisesRegex(EvidenceError, "private-readback-mismatch"):
            verify(self.manifest, self.readback)

    def test_immutable_version_no_overwrite(self):
        seal(self.runner, self.manifest)
        before = self.manifest.read_bytes()
        with self.assertRaisesRegex(EvidenceError, "manifest-exists"):
            seal(self.runner, self.manifest)
        self.assertEqual(self.manifest.read_bytes(), before)

    def test_symlink_never_followed(self):
        leak = self.root / "other.bin"
        leak.write_bytes(b"do-not-follow")
        (self.runner / "linked.png").symlink_to(leak)
        with self.assertRaisesRegex(EvidenceError, "symlink-in-private-evidence"):
            seal(self.runner, self.manifest)
        (self.runner / "linked.png").unlink()
        seal(self.runner, self.manifest)
        self.download_simulated()
        (self.readback / "linked.png").symlink_to(leak)
        with self.assertRaisesRegex(EvidenceError, "symlink-in-private-evidence"):
            verify(self.manifest, self.readback)

    def test_manifest_rejects_path_traversal_even_on_identical_checksum(self):
        seal(self.runner, self.manifest)
        payload = json.loads(self.manifest.read_text())
        payload["entries"][0]["path"] = "../guest.log"
        self.manifest.write_text(json.dumps(payload))
        self.download_simulated()
        with self.assertRaisesRegex(EvidenceError, "invalid-relative-entry"):
            verify(self.manifest, self.readback)

    def test_empty_and_git_checkout_rejected(self):
        empty = self.root / "empty"
        empty.mkdir()
        with self.assertRaisesRegex(EvidenceError, "empty-evidence-folder"):
            seal(empty, self.manifest)
        (self.runner / ".git").mkdir()
        with self.assertRaisesRegex(EvidenceError, "private-directory-inside-git-checkout"):
            seal(self.runner, self.manifest)
        (self.runner / ".git").rmdir()
        (self.private_receipts / ".git").mkdir()
        with self.assertRaisesRegex(EvidenceError, "private-directory-inside-git-checkout"):
            seal(self.runner, self.manifest)

    def test_receipt_must_not_live_inside_captured_tree(self):
        with self.assertRaisesRegex(EvidenceError, "manifest-must-be-outside-evidence-tree"):
            seal(self.runner, self.runner / "receipt-v1.json")
        self.assertFalse((self.runner / "receipt-v1.json").exists())

    def test_manifest_wrong_status_or_duplicate_rejected(self):
        seal(self.runner, self.manifest)
        payload = json.loads(self.manifest.read_text())
        payload["status"] = "ARCHIVED"
        self.manifest.write_text(json.dumps(payload))
        self.download_simulated()
        with self.assertRaisesRegex(EvidenceError, "invalid-manifest-status"):
            verify(self.manifest, self.readback)
        payload["status"] = "PENDING_DRIVE_READBACK"
        payload["entries"].append(payload["entries"][-1])
        payload["total_bytes"] += payload["entries"][-1]["bytes"]
        self.manifest.write_text(json.dumps(payload))
        with self.assertRaisesRegex(EvidenceError, "invalid-manifest-duplicates-or-total"):
            verify(self.manifest, self.readback)

    def test_rejects_more_than_256_evidence_files(self):
        for n in range(257):
            (self.runner / ("extra-%03d.bin" % n)).write_bytes(b"0")
        with self.assertRaisesRegex(EvidenceError, "evidence-file-count-limit"):
            seal(self.runner, self.manifest)


if __name__ == "__main__":
    unittest.main()
