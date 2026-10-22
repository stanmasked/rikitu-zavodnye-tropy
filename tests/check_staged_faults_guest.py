#!/usr/bin/env python3
"""Stage126: independent SDMC interruption and obstruction evidence audit.

Six cold guest runs: seed, partial, observe, block, recover, verify.
All six authentic RKS1/RKC2/RKL3 live slots must retain valid CRC.
Hash snapshots prove unchanged live bytes across interruptions/refusals.
"""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import zlib

from check_staged_codecs_guest import FAMILIES, check_record, verify_image

PHASES = {
    "seed": (1, 2), "partial": (1, 2),
    "observe": (3, 2), "block": (3, 2),
    "recover": (3, 4), "verify": (3, 4),
}
EXTENSIONS = {"map": "rks", "campaign": "rkc", "level": "rkl"}
SAME_AS = {"partial": "seed", "block": "observe", "verify": "recover"}
CHECKPOINT = {"seed", "observe", "recover"}


def record_path(base: Path, family: str, slot: str) -> Path:
    return base / f"stage126-{family}-{slot}.{EXTENSIONS[family]}"


def verify_files(root: Path, phase: str) -> None:
    base = root / "sdmc" / "switch" / "RIKITU"
    a_gen, b_gen = PHASES[phase]
    hashes: dict[str, str] = {}
    for family, (magic, size, _generation_offset, _level) in FAMILIES.items():
        for slot, generation in (("a", a_gen), ("b", b_gen)):
            path = record_path(base, family, slot)
            if path.is_symlink() or not path.is_file():
                raise AssertionError(f"Missing or unsafe live slot: {path}")
            data = path.read_bytes()
            check_record(data, family, generation)
            hashes[path.name] = hashlib.sha256(data).hexdigest()
            pending = base / f"{path.name}.pending"
            if phase == "partial" and slot == "a":
                if pending.is_symlink() or not pending.is_file():
                    raise AssertionError(f"Missing genuine partial staging file: {pending}")
                piece = pending.read_bytes()
                if len(piece) != size // 2 or piece[:4] != magic:
                    raise AssertionError(f"Not a half-written {family} record: {pending}")
            elif phase == "block" and slot == "b":
                if pending.is_symlink() or not pending.is_dir():
                    raise AssertionError(f"Blocked staging directory absent: {pending}")
                if any(pending.iterdir()):
                    raise AssertionError(f"Foreign files in pending obstruction: {pending}")
            elif os.path.lexists(pending):
                raise AssertionError(f"Unexpected staged path: {pending}")
            print(f"PASS Stage126 {phase} {family} {slot}: gen={generation} "
                  f"sha256={hashes[path.name][:12]}")

    manifest_path = root / "stage126-live-hashes.json"
    if phase in CHECKPOINT:
        stored = json.loads(manifest_path.read_text()) if manifest_path.exists() else {}
        stored[phase] = hashes
        manifest_path.write_text(json.dumps(stored, indent=2, sort_keys=True))
    else:
        if not manifest_path.is_file():
            raise AssertionError("Previous cold-boot hash checkpoint missing")
        stored = json.loads(manifest_path.read_text())
        earlier = SAME_AS[phase]
        if stored.get(earlier) != hashes:
            raise AssertionError(f"{phase}: live-slot bytes changed since {earlier}")
    print(f"PASS Stage126 {phase}: 6 authentic records, pending-path layout and byte-invariance")


def make_test_record(family: str, generation: int) -> bytes:
    magic, size, offset, level = FAMILIES[family]
    data = bytearray(size)
    data[:4] = magic
    data[5] = level
    data[offset:offset + 4] = generation.to_bytes(4, "little")
    if family == "campaign":
        data[12:16] = (156 + generation).to_bytes(4, "little")
    elif family == "level":
        data[36:40] = (157).to_bytes(4, "little")
    data[-4:] = zlib.crc32(data[:-4]).to_bytes(4, "little")
    return bytes(data)


def selftest() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        base = root / "sdmc" / "switch" / "RIKITU"
        base.mkdir(parents=True)
        def set_record(slot: str, generation: int) -> None:
            for family in FAMILIES:
                record_path(base, family, slot).write_bytes(
                    make_test_record(family, generation))
        set_record("a", 1)
        set_record("b", 2)
        verify_files(root, "seed")
        for family, (_magic, size, _, _) in FAMILIES.items():
            record_path(base, family, "a").with_name(
                record_path(base, family, "a").name + ".pending").write_bytes(
                    make_test_record(family, 3)[:size // 2])
        verify_files(root, "partial")
        bad = record_path(base, "map", "a")
        pending = bad.with_name(bad.name + ".pending")
        valid_piece = pending.read_bytes()
        pending.write_bytes(valid_piece[:-1])
        try:
            verify_files(root, "partial")
        except AssertionError:
            pass
        else:
            raise AssertionError("Incomplete staging size was not rejected")
        pending.write_bytes(valid_piece)
        for family in FAMILIES:
            path = record_path(base, family, "a")
            (base / (path.name + ".pending")).unlink()
        set_record("a", 3)
        verify_files(root, "observe")
        for family in FAMILIES:
            path = record_path(base, family, "b")
            (base / (path.name + ".pending")).mkdir()
        verify_files(root, "block")
        for family in FAMILIES:
            path = record_path(base, family, "b")
            (base / (path.name + ".pending")).rmdir()
        set_record("b", 4)
        verify_files(root, "recover")
        verify_files(root, "verify")
        altered = bytearray(record_path(base, "map", "a").read_bytes())
        altered[7] ^= 1
        record_path(base, "map", "a").write_bytes(altered)
        try:
            verify_files(root, "verify")
        except AssertionError:
            pass
        else:
            raise AssertionError("Corrupted live CRC was not rejected")
    print("PASS Stage126 host selftest: six phases, obstruction, byte invariance, CRC tamper")


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
        raise SystemExit("Usage: check_staged_faults_guest.py --selftest | "
                         "seed|partial|observe|block|recover|verify BEFORE AFTER DATA_ROOT")


if __name__ == "__main__":
    main()
