#!/usr/bin/env python3
"""Verify sixteen distinct live NeXium chapter HUDs and wrap to chapter one.

The images MUST be captured from the guest NRO after distinct gamepad Y
presses, not generated from the host rasterizer. This pixel-based check does
not use OCR and does not claim to validate art or real Joy-Cons.
"""
from __future__ import annotations

import hashlib
from pathlib import Path
import sys

from PIL import Image, ImageChops


def title_band(path: Path, expected_size: tuple[int, int] | None):
    with Image.open(path) as f:
        rgb = f.convert("RGB")
    if rgb.width < 960 or rgb.height < 540:
        raise ValueError(f"{path.name}: NeXium screenshot too small: {rgb.size}")
    if expected_size is not None and rgb.size != expected_size:
        raise ValueError(
            f"{path.name}: inconsistent screenshot size {rgb.size}; expected {expected_size}"
        )
    # libnx renders the chapter title in the fixed top HUD panel. Exclude
    # the moving level geometry and cyan player marker below y=84.
    sx = rgb.width / 1280.0
    sy = rgb.height / 720.0
    band = rgb.crop(
        (int(20 * sx), int(9 * sy), int(480 * sx), int(46 * sy))
    )
    return rgb.size, band


def check(root: Path):
    seen: dict[str, int] = {}
    first = None
    size = None
    for stage in range(1, 17):
        screenshot = root / f"map-{stage:02d}.png"
        if not screenshot.is_file():
            raise ValueError(f"missing actual NeXium screenshot: {screenshot}")
        size, band = title_band(screenshot, size)
        if first is None:
            first = band
        digest = hashlib.sha256(band.tobytes()).hexdigest()
        if digest in seen:
            raise ValueError(
                f"MAP{stage:02d}: title HUD unchanged from MAP{seen[digest]:02d}; "
                "Y input may not be mapped to Nintendo Y or the chapter did not change"
            )
        seen[digest] = stage
    assert first is not None
    _, wrap = title_band(root / "map-wrap-01.png", size)
    if wrap.size != first.size:
        raise ValueError("wrap chapter title band size changed")
    pixels = ImageChops.difference(wrap, first).convert("RGB")
    # On the same level, the debug HUD is flat and deterministic; allow
    # a small margin for emulator capture/compositing differences.
    changed = sum(
        1 for pixel in pixels.getdata() if max(pixel) > 24
    )
    if changed > 60:
        raise ValueError(
            f"Y after MAP16 failed to wrap back to MAP01 HUD: "
            f"{changed} changed title pixels"
        )
    print(
        f"PASS NeXium real NRO: 16 unique chapter title HUDs from Y input, "
        f"MAP16 -> MAP01 wrap, {size[0]}x{size[1]}, "
        f"wrap mismatch pixels={changed}"
    )


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: check_emulator_map_cycle.py <map-cycle-evidence-directory>")
    check(Path(sys.argv[1]))
