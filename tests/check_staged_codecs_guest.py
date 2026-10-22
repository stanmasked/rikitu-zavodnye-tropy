#!/usr/bin/env python3
"""Stage125 independent NeXium guest audit: real staged RKS1/RKC2/RKL3.

Checks exact record lengths, schema magic, CRC32, serial generations,
family isolation and absence of staging leftovers on an isolated SD root.
Emulator evidence is NOT a guarantee of power-cut durability on hardware.
"""
from __future__ import annotations

import hashlib
from pathlib import Path
import sys
import tempfile
import zlib

FAMILIES = {
    "map": (b"RKS1", 44, 32, 1),
    "campaign": (b"RKC2", 32, 16, 4),
    "level": (b"RKL3", 64, 32, 4),
}
PHASES = {"seed": (1, 2), "advance": (3, 2), "verify": (3, 2)}


def check_record(data: bytes, family: str, expected_generation: int) -> None:
    magic, size, generation_at, level = FAMILIES[family]
    if len(data) != size or data[:4] != magic:
        raise AssertionError(f"{family}: invalid size/magic: {len(data)} {data[:4]!r}")
    if data[5] != level:
        raise AssertionError(f"{family}: unexpected map/chapter {data[5]}")
    generation = int.from_bytes(data[generation_at:generation_at + 4], "little")
    if generation != expected_generation:
        raise AssertionError(f"{family}: generation {generation} != {expected_generation}")
    actual_crc = int.from_bytes(data[-4:], "little")
    calculated_crc = zlib.crc32(data[:-4]) & 0xFFFFFFFF
    if actual_crc != calculated_crc:
        raise AssertionError(f"{family}: CRC32 mismatch")
    if family == "campaign":
        wallet = int.from_bytes(data[12:16], "little")
        if wallet != 157 + generation - 1:
            raise AssertionError(f"RKC2 wallet mismatch: {wallet}")
    if family == "level":
        wallet = int.from_bytes(data[36:40], "little")
        if wallet != 157:
            raise AssertionError(f"RKL3 wallet mismatch: {wallet}")


def verify_files(root: Path, phase: str) -> None:
    a_generation, b_generation = PHASES[phase]
    base = root / "sdmc" / "switch" / "RIKITU"
    for family in FAMILIES:
        for slot, generation in (("a", a_generation), ("b", b_generation)):
            path = base / f"stage125-{family}-{slot}.{dict(map='rks',campaign='rkc',level='rkl')[family]}"
            data = path.read_bytes()
            check_record(data, family, generation)
            print(f"PASS Stage125 {phase} {family} {slot}: gen={generation} "
                  f"sha256={hashlib.sha256(data).hexdigest()[:12]}")
            if (base / (path.name + ".pending")).is_symlink() or (
                base / (path.name + ".pending")).exists():
                raise AssertionError(f"Stale pending path: {path}.pending")
    print(f"PASS Stage125 {phase}: 6 CRC-verified genuine codec slot records")


def verify_image(path: Path, phase: str) -> None:
    from PIL import Image
    with Image.open(path) as im:
        rgb = im.convert("RGB")
    if rgb.size != (1280, 720):
        raise AssertionError(f"Unexpected NeXium image dimensions {rgb.size}")
    for i, label in enumerate(("SDMC mount", "private directory", "cold-boot state",
                               "staged writes/verification")):
        left = 280 + 200 * i
        greens = sum(
            g > 170 and r < 100 and b < 100
            for x in range(left - 24, left + 100, 8)
            for y in range(108, 200, 8)
            for r, g, b in [rgb.getpixel((x, y))]
        )
        if greens < 32:
            raise AssertionError(f"{path}: {label} failed: {greens} green pixels")
    print(f"PASS Stage125 {phase} four green guest signals {path.name}")


def selftest() -> None:
    with tempfile.TemporaryDirectory() as temp:
        base = Path(temp) / "sdmc" / "switch" / "RIKITU"
        base.mkdir(parents=True)
        for family, (magic, size, generation_at, level) in FAMILIES.items():
            ext = dict(map="rks", campaign="rkc", level="rkl")[family]
            for slot, generation in (("a", 1), ("b", 2)):
                data = bytearray(size)
                data[:4] = magic
                data[5] = level
                data[generation_at:generation_at + 4] = generation.to_bytes(4, "little")
                if family == "campaign":
                    data[12:16] = (157 + generation - 1).to_bytes(4, "little")
                if family == "level":
                    data[36:40] = (157).to_bytes(4, "little")
                data[-4:] = zlib.crc32(data[:-4]).to_bytes(4, "little")
                (base / f"stage125-{family}-{slot}.{ext}").write_bytes(data)
        verify_files(Path(temp), "seed")
        for family in FAMILIES:
            ext = dict(map="rks", campaign="rkc", level="rkl")[family]
            path = base / f"stage125-{family}-a.{ext}"
            data = bytearray(path.read_bytes())
            gen_at = FAMILIES[family][2]
            data[gen_at:gen_at + 4] = (3).to_bytes(4, "little")
            if family == "campaign":
                data[12:16] = (159).to_bytes(4, "little")
            data[-4:] = zlib.crc32(data[:-4]).to_bytes(4, "little")
            path.write_bytes(data)
        verify_files(Path(temp), "advance")
        verify_files(Path(temp), "verify")
        bad = base / "stage125-map-a.rks"
        raw = bytearray(bad.read_bytes())
        raw[7] ^= 1
        bad.write_bytes(raw)
        try:
            verify_files(Path(temp), "verify")
        except AssertionError:
            pass
        else:
            raise AssertionError("Corrupted CRC was not detected")
        print("PASS Stage125 host checker selftest: CRC tampering rejected")


def main() -> None:
    args = sys.argv[1:]
    if args == ["--selftest"]:
        selftest()
    elif len(args) == 4 and args[0] in PHASES:
        phase, before, after, root = args
        verify_image(Path(before), phase)
        verify_image(Path(after), phase)
        verify_files(Path(root), phase)
    else:
        raise SystemExit("Usage: check_staged_codecs_guest.py --selftest | seed|advance|verify BEFORE AFTER DATA_ROOT")


if __name__ == "__main__":
    main()
