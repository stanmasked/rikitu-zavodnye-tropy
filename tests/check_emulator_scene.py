#!/usr/bin/env python3
"""Check evidence from a real NeXium rendering/input session, not a host preview.

The exact marker color is grounded in native/rikitu_draw.c's neutral player.
"""
from __future__ import annotations
from pathlib import Path
import sys
from PIL import Image

MARKER = (78, 220, 233)
TOLERANCE = 22

def marker_centroid(image: Image.Image) -> tuple[float, int]:
    rgb = image.convert("RGB")
    count = 0
    sum_x = 0
    for y in range(rgb.height):
        for x in range(rgb.width):
            p = rgb.getpixel((x, y))
            if max(abs(p[k]-MARKER[k]) for k in range(3)) <= TOLERANCE:
                count += 1
                sum_x += x
    if count < 130:
        raise ValueError(f"missing neutral cyan player marker: {count} pixels")
    return sum_x/count, count

def check(before_path: Path, after_path: Path) -> None:
    before = Image.open(before_path)
    after = Image.open(after_path)
    if before.size != after.size:
        raise ValueError(f"NeXium frame sizes differ: {before.size} vs {after.size}")
    if before.width < 960 or before.height < 540:
        raise ValueError(f"unexpectedly small NeXium window: {before.size}")
    start, count_start = marker_centroid(before)
    end, count_end = marker_centroid(after)
    moved = end-start
    print(f"NeXium: screenshot dimensions {before.size}, "
          f"cyan marker pixels {count_start}/{count_end}, "
          f"horizontal centroid {start:.1f} -> {end:.1f}, delta={moved:.1f}")
    if moved < 12.0:
        raise ValueError("rightward controller input did not visibly move player")
    print("PASS NeXium real graphical NRO: cyan player marker rendered and moved")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("Usage: check_emulator_scene.py <start.png> <after-right.png>")
    check(Path(sys.argv[1]), Path(sys.argv[2]))
