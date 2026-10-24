"""Stage157 synthetic-only evidence receipt tests; no private CI images or saves."""
import json
import hashlib
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
        self.sealed_digest = None
        (self.runner / "frames").mkdir()
        (self.runner / "frames" / "capture-01.png").write_bytes(b"FAKE-PNG-PIXELS")
        (self.runner / "guest.log").write_bytes(b"synthetic-marker\n")

    def _seal(self):
        result = self._seal()
        self.sealed_digest = result["manifest_sha256"]
        return result

    def _verify(self, digest=None):
        return verify(self.manifest, self.readback,
                      self.sealed_digest if digest is None else digest)

    def download_simulated(self):
        shutil.copytree(self.runner, self.readback, dirs_exist_ok=True)

    def test_seal_and_independent_readback(self):
        receipt = self._seal()
        self.assertEqual(receipt["files"], 2)
        self.assertEqual(receipt["result"], "PRIVATE_MANIFEST_SEALED_NOT_ARCHIVED")
        self.assertEqual(self.manifest.stat().st_mode & 0o777, 0o600)
        record = json.loads(self.manifest.read_bytes())
        self.assertEqual(
            [x["path"] for x in record["entries"]],
            ["frames/capture-01.png", "guest.log"])
        self.assertEqual(record["status"], "PENDING_DRIVE_READBACK")
        self.download_simulated()
        verified = self._verify()
        self.assertEqual(verified["files"], 2)
        self.assertEqual(
            verified["result"],
            "BYTE_MATCH_ONLY_REQUIRES_AUTHENTICATED_DRIVE_FETCH")
        self.assertEqual(verified["manifest_sha256"], receipt["manifest_sha256"])

    def test_mismatched_bytes_rejected_even_when_length_same(self):
        self._seal()
        self.download_simulated()
        (self.readback / "guest.log").write_bytes(b"synthetic-markerX")
        with self.assertRaisesRegex(EvidenceError, "private-readback-mismatch"):
            self._verify()

    def test_missing_and_extra_files_are_failures(self):
        self._seal()
        self.download_simulated()
        (self.readback / "guest.log").unlink()
        with self.assertRaisesRegex(EvidenceError, "private-readback-mismatch"):
            self._verify()
        (self.readback / "guest.log").write_bytes(b"synthetic-marker\n")
        (self.readback / "extra.dat").write_bytes(b"unexpected")
        with self.assertRaisesRegex(EvidenceError, "private-readback-mismatch"):
            self._verify()

    def test_immutable_version_no_overwrite(self):
        self._seal()
        before = self.manifest.read_bytes()
        with self.assertRaisesRegex(EvidenceError, "manifest-exists"):
            self._seal()
        self.assertEqual(self.manifest.read_bytes(), before)

    def test_symlink_never_followed(self):
        leak = self.root / "other.bin"
        leak.write_bytes(b"do-not-follow")
        (self.runner / "linked.png").symlink_to(leak)
        with self.assertRaisesRegex(EvidenceError, "symlink-in-private-evidence"):
            self._seal()
        (self.runner / "linked.png").unlink()
        self._seal()
        self.download_simulated()
        (self.readback / "linked.png").symlink_to(leak)
        with self.assertRaisesRegex(EvidenceError, "symlink-in-private-evidence"):
            self._verify()

    def test_manifest_rejects_path_traversal_even_on_identical_checksum(self):
        self._seal()
        payload = json.loads(self.manifest.read_text())
        payload["entries"][0]["path"] = "../guest.log"
        self.manifest.write_text(json.dumps(payload))
        self.download_simulated()
        with self.assertRaisesRegex(EvidenceError, "invalid-relative-entry"):
            self._verify(digest=hashlib.sha256(
                self.manifest.read_bytes()).hexdigest())

    def test_empty_and_git_checkout_rejected(self):
        empty = self.root / "empty"
        empty.mkdir()
        with self.assertRaisesRegex(EvidenceError, "empty-evidence-folder"):
            seal(empty, self.manifest)
        (self.runner / ".git").mkdir()
        with self.assertRaisesRegex(EvidenceError, "private-directory-inside-git-checkout"):
            self._seal()
        (self.runner / ".git").rmdir()
        (self.private_receipts / ".git").mkdir()
        with self.assertRaisesRegex(EvidenceError, "private-directory-inside-git-checkout"):
            self._seal()

    def test_receipt_must_not_live_inside_captured_tree(self):
        with self.assertRaisesRegex(EvidenceError, "manifest-must-be-outside-evidence-tree"):
            seal(self.runner, self.runner / "receipt-v1.json")
        self.assertFalse((self.runner / "receipt-v1.json").exists())

    def test_manifest_wrong_status_or_duplicate_rejected(self):
        self._seal()
        payload = json.loads(self.manifest.read_text())
        payload["status"] = "ARCHIVED"
        self.manifest.write_text(json.dumps(payload))
        self.download_simulated()
        with self.assertRaisesRegex(EvidenceError, "invalid-manifest-status"):
            self._verify(digest=hashlib.sha256(
                self.manifest.read_bytes()).hexdigest())
        payload["status"] = "PENDING_DRIVE_READBACK"
        payload["entries"].append(payload["entries"][-1])
        payload["total_bytes"] += payload["entries"][-1]["bytes"]
        self.manifest.write_text(json.dumps(payload))
        with self.assertRaisesRegex(EvidenceError, "invalid-manifest-duplicates-or-total"):
            self._verify(digest=hashlib.sha256(
                self.manifest.read_bytes()).hexdigest())

    def test_manifest_and_payload_replacement_cannot_fake_readback(self):
        self._seal()
        self.download_simulated()
        (self.readback / "guest.log").write_bytes(b"new-synthetic-log")
        payload = json.loads(self.manifest.read_text())
        for entry in payload["entries"]:
            if entry["path"] == "guest.log":
                altered = (self.readback / "guest.log").read_bytes()
                entry["sha256"] = hashlib.sha256(altered).hexdigest()
                payload["total_bytes"] += len(altered) - entry["bytes"]
                entry["bytes"] = len(altered)
        self.manifest.write_text(json.dumps(payload))
        # Without independent pinned digest, replacing BOTH would pass.
        with self.assertRaisesRegex(EvidenceError, "manifest-sha256-mismatch"):
            self._verify()

    def test_invalid_pinned_sha_is_rejected(self):
        self._seal()
        self.download_simulated()
        with self.assertRaisesRegex(EvidenceError, "invalid-expected-manifest-digest"):
            self._verify(digest="bad")

    def test_rejects_more_than_256_evidence_files(self):
        for n in range(257):
            (self.runner / ("extra-%03d.bin" % n)).write_bytes(b"0")
        with self.assertRaisesRegex(EvidenceError, "evidence-file-count-limit"):
            self._seal()


if __name__ == "__main__":
    unittest.main()
