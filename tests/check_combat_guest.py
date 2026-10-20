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
        x0 = 120 + 170 * index
        # Sample well inside the solid guest-rendered square, not its edges.
        samples = [screen.getpixel((x, y))
                   for x in range(x0 + 15, x0 + 48, 8)
                   for y in range(133, 166, 8)]
        if not all(g >= 170 and r < 100 and b < 100 for r, g, b in samples):
            raise AssertionError(
                f"NeXium guest {name} did not report success: {samples[:4]}"
            )
        print(f"PASS guest MAP01 {name}: green frame markers ({path.name})")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("Usage: check_combat_guest.py <before.png> <after.png>")
    check(Path(sys.argv[1]))
    check(Path(sys.argv[2]))
    print("PASS all four true guest-side MAP01 combat checks in two NeXium frames")
