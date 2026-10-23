"""Convert a privately held *approved* PNG into opt-in Switch RGBA8 pixels.

Only the converter/tests belong in public GitHub. Never commit its image input,
raw RGBA8 output or a real-art test fixture. The caller chooses a private dir.
"""
import argparse
import hashlib
import hmac
import io
import json
import os
from pathlib import Path
import re
import sys
import tempfile

from PIL import Image, UnidentifiedImageError

PNG_MAGIC = b"\x89PNG\r\n\x1a\n"
MAX_PNG_BYTES = 32 * 1024 * 1024
MAX_PIXELS = 16_000_000
Image.MAX_IMAGE_PIXELS = MAX_PIXELS


class ExportError(Exception):
    """Redacted diagnostic: no private filenames, paths or pixels."""


def _inside_checkout(path: Path) -> bool:
    return any((parent / ".git").exists() for parent in
               (path, *path.parents))


def export_png(source: Path, private_dir: Path, output_name: str,
               expected_sha256: str, width: int, height: int) -> dict:
    """Create a non-overwriting private .rgba8, return safe hash metadata."""
    if not re.fullmatch(r"[0-9a-fA-F]{64}", expected_sha256):
        raise ExportError("invalid-expected-sha256")
    if not (1 <= width <= 16384 and 1 <= height <= 2160 and
            width * height <= MAX_PIXELS):
        raise ExportError("invalid-pixel-dimensions")
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]{0,127}\\.rgba8",
                        output_name):
        raise ExportError("invalid-private-output-name")
    if private_dir.is_symlink() or not private_dir.is_dir():
        raise ExportError("invalid-private-output-directory")
    root = private_dir.resolve()
    if _inside_checkout(root):
        raise ExportError("output-inside-git-checkout")
    output = root / output_name
    if output.exists() or output.is_symlink():
        raise ExportError("output-already-exists")
    if source.is_symlink() or not source.is_file():
        raise ExportError("invalid-source")
    if source.stat().st_size > MAX_PNG_BYTES:
        raise ExportError("oversized-source")
    raw = source.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if not hmac.compare_digest(digest, expected_sha256.lower()):
        raise ExportError("source-sha256-mismatch")
    if not raw.startswith(PNG_MAGIC):
        raise ExportError("invalid-png-magic")
    try:
        with Image.open(io.BytesIO(raw)) as pic:
            if (pic.format != "PNG" or pic.size != (width, height) or
                    getattr(pic, "n_frames", 1) != 1):
                raise ExportError("unexpected-image-format-or-size")
            if pic.mode not in ("RGB", "RGBA"):
                raise ExportError("unsupported-png-mode")
            pic.load()  # decoder verifies compressed PNG stream
            rgba = pic.convert("RGBA")
            if rgba.getchannel("A").getextrema() != (255, 255):
                raise ExportError("nonopaque-background")
            pixels = rgba.tobytes()
    except (OSError, ValueError, UnidentifiedImageError) as exc:
        raise ExportError("png-decode-failed") from exc
    if len(pixels) != width * height * 4:
        raise ExportError("invalid-rgba-byte-count")
    output_digest = hashlib.sha256(pixels).hexdigest()
    temp_path = None
    try:
        with tempfile.NamedTemporaryFile(mode="wb", dir=root,
                                         prefix=".rikitu-rgba8-",
                                         delete=False) as tmp:
            temp_path = Path(tmp.name)
            os.chmod(tmp.name, 0o600)
            tmp.write(pixels)
            tmp.flush()
            os.fsync(tmp.fileno())
        # Hard-link creation fails instead of overwriting an existing version.
        os.link(temp_path, output)
    except FileExistsError as exc:
        raise ExportError("output-already-exists") from exc
    except OSError as exc:
        raise ExportError("private-output-write-failed") from exc
    finally:
        if temp_path is not None:
            temp_path.unlink(missing_ok=True)
    return {
        "format": "RGBA8888-little-endian-AABBGGRR",
        "width": width,
        "height": height,
        "stride_pixels": width,
        "pixel_bytes": len(pixels),
        "source_png_sha256": digest,
        "derived_rgba8_sha256": output_digest,
        "status": "PRIVATE_DERIVED_UNAPPROVED_UNTIL_EXPLICIT_APPROVAL"
    }


def main() -> int:
    p = argparse.ArgumentParser(
        description="Export an approved PRIVATE PNG to private raw RGBA8")
    p.add_argument("--input", type=Path, required=True)
    p.add_argument("--private-output-dir", type=Path, required=True)
    p.add_argument("--output-name", required=True)
    p.add_argument("--expected-sha256", required=True)
    p.add_argument("--width", type=int, required=True)
    p.add_argument("--height", type=int, required=True)
    args = p.parse_args()
    try:
        summary = export_png(args.input, args.private_output_dir,
                             args.output_name, args.expected_sha256,
                             args.width, args.height)
    except (ExportError, OSError) as exc:
        code = str(exc) if isinstance(exc, ExportError) else "private-io-failed"
        print("RESULT: FAIL:", code, file=sys.stderr)
        return 1
    print(json.dumps(summary, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
