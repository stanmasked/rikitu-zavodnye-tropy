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
MILL_NAMES = ("map02 data", "beetle stomp/recovery", "boss contact/immunity", "three hits + exit")
GROVE_NAMES = ("MAP03 data", "first spring", "second and third springs", "airborne safety + gate")
BRIDGE_NAMES = ("MAP04 geometry", "three touch levers", "guardian hit", "boss-gated exit")
WORKSHOP_NAMES = ("MAP05 geometry", "forward belts", "reverse belt", "airborne safety + gate")
DELIVERY_NAMES = ("MAP06 geometry", "three belts", "three steam vents", "boss-gated exit")


def check(path: Path) -> None:
    with Image.open(path) as src:
        screen = src.convert("RGB")
    if screen.size != (1280, 720):
        raise AssertionError(f"Expected 1280x720 NeXium capture, got {screen.size}")
    # Sixth row immediately follows MAP05 near the bottom of the 720px guest.
    # The 56px green squares occupy y=656..711, never beyond the framebuffer.
    # A stray green sample from MAP05 (< 32) cannot validate a failed MAP06.
    for index, name in enumerate(DELIVERY_NAMES):
        left = 280 + 200 * index
        samples = [screen.getpixel((x, y))
                   for x in range(left - 24, left + 100, 8)
                   for y in range(650, 720, 8)]
        green = sum(g >= 170 and r < 100 and b < 100
                    for r, g, b in samples)
        if green < 32:
            raise AssertionError(
                f"NeXium guest MAP06 {name} failed: {green}/{len(samples)} green")
        print(f"PASS guest MAP06 {name}: {green} green samples ({path.name})")
    for index, name in enumerate(WORKSHOP_NAMES):
        left = 280 + 200 * index
        samples = [screen.getpixel((x,y))
                   for x in range(left-24,left+100,8)
                   for y in range(586, 676, 8)]
        green = sum(g >= 170 and r < 100 and b < 100
                    for r,g,b in samples)
        if green < 32:
            raise AssertionError(
                f"NeXium guest MAP05 {name} failed: {green}/{len(samples)} green")
        print(f"PASS guest MAP05 {name}: {green} green samples ({path.name})")
    for index, name in enumerate(BRIDGE_NAMES):
        left = 280 + 200 * index
        samples = [screen.getpixel((x,y))
                   for x in range(left-24,left+100,8)
                   for y in range(466, 556, 8)]
        green=sum(g>=170 and r<100 and b<100 for r,g,b in samples)
        if green<32:
            raise AssertionError(
                f"NeXium guest MAP04 {name} failed: {green}/{len(samples)} green")
        print(f"PASS guest MAP04 {name}: {green} green samples ({path.name})")
    for index, name in enumerate(GROVE_NAMES):
        left = 280 + 200 * index
        samples = [screen.getpixel((x,y))
                   for x in range(left - 24, left + 100, 8)
                   for y in range(346, 436, 8)]
        green = sum(g >= 170 and r < 100 and b < 100
                    for r,g,b in samples)
        if green < 32:
            raise AssertionError(
                f"NeXium guest MAP03 {name} failed: {green}/{len(samples)} green")
        print(f"PASS guest MAP03 {name}: {green} green samples ({path.name})")
    for index, name in enumerate(MILL_NAMES):
        left = 280 + 200 * index
        samples = [screen.getpixel((x,y))
                   for x in range(left - 24, left + 100, 8)
                   for y in range(226, 316, 8)]
        green = sum(g >= 170 and r < 100 and b < 100
                    for r,g,b in samples)
        if green < 32:
            raise AssertionError(
                f"NeXium guest MAP02 {name} failed: {green}/{len(samples)} green")
        print(f"PASS guest MAP02 {name}: {green} green samples ({path.name})")
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
    print("PASS all 24 MAP01/MAP02/MAP03/MAP04/MAP05/MAP06 guest-side checks in both NeXium frames")
