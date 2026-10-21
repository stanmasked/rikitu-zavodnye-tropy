#!/usr/bin/env python3
"""Validate real guest RKC2 + RKL3 files after independent NeXium starts."""
from __future__ import annotations

import struct
from pathlib import Path
import sys
import zlib

from check_save_guest import validate_frame

EXPECTED = {"seed": (1, 1), "resume": (2, 2), "verify": (2, 2)}


def read_and_check(path: Path, magic: bytes, length: int, crc_offset: int) -> bytes:
    blob = path.read_bytes()
    if len(blob) != length or blob[:4] != magic:
        raise AssertionError(
            f"Invalid guest {magic.decode()} save {path}: "
            f"magic={blob[:4]!r} size={len(blob)}, expected {length}"
        )
    crc = struct.unpack_from("<I", blob, crc_offset)[0]
    computed = zlib.crc32(blob[:crc_offset]) & 0xFFFFFFFF
    if crc != computed:
        raise AssertionError(f"Corrupted CRC in {path}: {crc:#x}!={computed:#x}")
    return blob


def validate_pair(root: Path, phase: str) -> None:
    if phase not in EXPECTED:
        raise ValueError(f"Unsupported phase {phase}")
    folder = root / "sdmc" / "switch" / "RIKITU"
    expected_count, latest_generation = EXPECTED[phase]
    for ext, magic, size, checksum, generation_at in [
        ("rkc", b"RKC2", 32, 28, 16),
        ("rkl", b"RKL3", 64, 60, 32),
    ]:
        stem = "campaign" if ext == "rkc" else "level"
        files = sorted(folder.glob(f"{stem}-*.{ext}"))
        if len(files) != expected_count:
            raise AssertionError(
                f"Expected {expected_count} independent {ext} slots, "
                f"got {files}"
            )
        observed = []
        for file in files:
            content = read_and_check(file, magic, size, checksum)
            generation = struct.unpack_from("<I", content, generation_at)[0]
            observed.append(generation)
            if content[5] != 4:
                raise AssertionError(f"{file}: wrong MAP04 level: {content[5]}")
            if ext == "rkc":
                wallet = struct.unpack_from("<I", content, 12)[0]
                if not (
                    content[6] == 1 and content[7] == 1
                    and content[8] == 4 and wallet == 157
                ):
                    raise AssertionError(f"{file}: wrong campaign/shop state")
            else:
                index = 1 if generation == 1 else 2
                hearts = 3 if generation == 1 else 2
                boss = 3 if generation == 1 else 2
                fragments = 1 if generation == 1 else 5
                enemies = 1 if generation == 1 else 33
                wallet = struct.unpack_from("<I", content, 36)[0]
                seconds = struct.unpack_from("<I", content, 28)[0]
                first_coin = bool(content[12] & 1)
                last_coin = bool(content[23] & 2)
                if not (
                    content[6] == index and content[7] == hearts
                    and content[8] == 4 and content[9] == boss
                    and content[10] == fragments and content[11] == enemies
                    and first_coin and (last_coin == (generation == 2))
                    and content[41] == 1 and wallet == 157
                    and seconds == (25000 if generation == 1 else 55000)
                ):
                    raise AssertionError(
                        f"{file}: wrong checkpoint/boss/fragments/"
                        "coins/enemies/elapsed-time state"
                    )
        if sorted(observed) != list(range(1, latest_generation + 1)):
            raise AssertionError(
                f"{ext} generations {observed}, "
                f"expected 1..{latest_generation}"
            )
        print(f"PASS NeXium SDMC {phase}: {ext} CRC and generations {observed}")


def main():
    if len(sys.argv) != 5:
        raise SystemExit(
            "Usage: check_campaign_guest.py "
            "<seed|resume|verify> before.png after.png data-root"
        )
    phase = sys.argv[1]
    if phase not in EXPECTED:
        raise ValueError(phase)
    for path in (Path(sys.argv[2]), Path(sys.argv[3])):
        validate_frame(path, phase)
    validate_pair(Path(sys.argv[4]), phase)


if __name__ == "__main__":
    main()
