"""Synthetic-only Stage153 QA: never load or upload actual private GFX-01 art."""
import hashlib
from io import BytesIO
from pathlib import Path
import sys
import tempfile
import unittest

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.export_private_png_rgba8 import ExportError, export_png


class PrivateRgbaExportTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.private_dir = self.root / "private"
        self.private_dir.mkdir()
        self.source = self.root / "synthetic.png"

    def image(self, mode="RGBA", size=(4, 2), color=(17, 34, 51, 255)):
        pic = Image.new(mode, size, color if mode == "RGBA" else color[:3])
        pic.save(self.source, format="PNG")
        return hashlib.sha256(self.source.read_bytes()).hexdigest()

    def export(self, digest, **kwargs):
        return export_png(
            kwargs.get("source", self.source),
            kwargs.get("private_dir", self.private_dir),
            kwargs.get("name", "candidate-v1.rgba8"),
            digest,
            kwargs.get("width", 4),
            kwargs.get("height", 2),
        )

    def test_rgba_bytes_and_manifest_are_exact(self):
        sha = self.image()
        result = self.export(sha)
        self.assertEqual(result["pixel_bytes"], 32)
        self.assertEqual(result["source_png_sha256"], sha)
        self.assertEqual(result["stride_pixels"], 4)
        payload = (self.private_dir / "candidate-v1.rgba8").read_bytes()
        self.assertEqual(payload, bytes((17, 34, 51, 255)) * 8)
        self.assertEqual(result["derived_rgba8_sha256"],
                         hashlib.sha256(payload).hexdigest())
        self.assertFalse(any(self.private_dir.glob(".rikitu-rgba8-*")))

    def test_rgb_is_exported_as_opaque_rgba(self):
        sha = self.image(mode="RGB")
        self.export(sha)
        payload = (self.private_dir / "candidate-v1.rgba8").read_bytes()
        self.assertEqual(payload[:4], bytes((17, 34, 51, 255)))

    def test_wrong_sha_and_dimensions_cannot_write(self):
        sha = self.image()
        with self.assertRaisesRegex(ExportError, "source-sha256-mismatch"):
            self.export("f" * 64)
        with self.assertRaisesRegex(ExportError, "unexpected-image"):
            self.export(sha, width=5)
        self.assertEqual(list(self.private_dir.iterdir()), [])

    def test_nonopaque_pixels_rejected(self):
        sha = self.image(color=(1, 2, 3, 128))
        with self.assertRaisesRegex(ExportError, "nonopaque-background"):
            self.export(sha)
        self.assertEqual(list(self.private_dir.iterdir()), [])

    def test_existing_private_derivative_never_overwritten(self):
        sha = self.image()
        out = self.private_dir / "candidate-v1.rgba8"
        out.write_bytes(b"IMMUTABLE EXISTING")
        with self.assertRaisesRegex(ExportError, "output-already-exists"):
            self.export(sha)
        self.assertEqual(out.read_bytes(), b"IMMUTABLE EXISTING")

    def test_refuse_output_inside_git_checkout(self):
        sha = self.image()
        (self.private_dir / ".git").mkdir()
        with self.assertRaisesRegex(ExportError, "output-inside-git-checkout"):
            self.export(sha)
        self.assertFalse((self.private_dir / "candidate-v1.rgba8").exists())

    def test_invalid_magic_and_mode(self):
        self.source.write_bytes(b"not an image")
        digest = hashlib.sha256(self.source.read_bytes()).hexdigest()
        with self.assertRaisesRegex(ExportError, "invalid-png-magic"):
            self.export(digest)
        picture = Image.new("P", (4, 2))
        picture.save(self.source, format="PNG")
        digest = hashlib.sha256(self.source.read_bytes()).hexdigest()
        with self.assertRaisesRegex(ExportError, "unsupported-png-mode"):
            self.export(digest)

    def test_no_path_traversal_and_no_symlink_input(self):
        digest = self.image()
        with self.assertRaisesRegex(ExportError, "invalid-private-output-name"):
            self.export(digest, name="../escape.rgba8")
        link = self.root / "linked.png"
        link.symlink_to(self.source)
        with self.assertRaisesRegex(ExportError, "invalid-source"):
            self.export(digest, source=link)


if __name__ == "__main__":
    unittest.main()
