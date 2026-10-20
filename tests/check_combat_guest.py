#!/usr/bin/env python3
"""Verify four C99 combat regressions executed inside a real NeXium guest.

Each 64x56 status square is emitted by a QA-only Switch NRO *after* it runs
native/rikitu_combat_guest_qa.c. Green means the exact guest C99 check passed.
This is a smoke test of deterministic game states, not a full playthrough.
"""
from __future__ import annotations
from pathlib import Path
import sys
from PIL import Image

NAMES = ("43/s patrol", "contact/immunity", "stomp bounce", "checkpoint respawn")


def check(path: Path) -> None:
    with Image.open(path) as src:
        screen = src.convert("RGB")
    if screen.size != (1280, 720):
        raise AssertionError(f"Expected 1280x720 NeXium capture, got {screen.size}")
    for index, name in enumerate(NAMES):
        left = 280 + 200 * index
        # NeXium screenshot includes window borders and arbitrary
        # offsets. Scan the square's full projected neighborhood, rather
        # than assuming the guest framebuffer starts at (0,0).
        samples = [screen.getpixel((x, y))
                   for x in range(left - 24, left + 100, 8)
                   for y in range(108, 200, 8)]
        green = sum(g >= 170 and r < 100 and b < 100
                    for r, g, b in samples)
        # A real 64x56 filled guest square gives ~56 sampled green
        # pixels in the current NeXium viewport; require >=32.
        if green < 32:
            raise AssertionError(
                f"NeXium guest {name} did not report success: "
                f"green={green}/{len(samples)}; pixels={samples[:6]}"
            )
        print(f"PASS guest MAP01 {name}: {green} green samples ({path.name})")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("Usage: check_combat_guest.py <before.png> <after.png>")
    check(Path(sys.argv[1]))
    check(Path(sys.argv[2]))
    print("PASS all four true guest-side MAP01 combat checks in two NeXium frames")
