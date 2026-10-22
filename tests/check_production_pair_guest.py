#!/usr/bin/env python3
"""Stage131 guest audit for production RKS1/RKC2/RKL3 save-pair writers.

Audits the persistent SDMC image after each independent NeXium launch.
This is emulator evidence, not a physical Switch power-loss guarantee.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sys
import tempfile
import zlib

FAMILIES = {
    "map": (b"RKS1", 44, 32, 1),
    "campaign": (b"RKC2", 32, 16, 4),
    "level": (b"RKL3", 64, 32, 4),
}
EXT = {"map": "rks", "campaign": "rkc", "level": "rkl"}
PHASE_MARKER = {
    "seed": "seeded",
    "advance": "ready-to-block",
    "block": "ready-to-recover",
    "recover": "ready-to-verify",
    "verify": "verified",
}
GENERATIONS = {
    "seed": (1, None),
    "advance": (1, 2),
    "block": (1, 2),
    "recover": (3, 2),
    "verify": (3, 2),
}


def check_record(data: bytes, family: str, generation: int) -> None:
    magic, size, generation_at, level = FAMILIES[family]
    if len(data) != size or data[:4] != magic:
        raise AssertionError(f"{family}: invalid size/magic {len(data)} {data[:4]!r}")
    if data[5] != level:
        raise AssertionError(f"{family}: unexpected chapter/map {data[5]}")
    actual_generation = int.from_bytes(data[generation_at:generation_at + 4], "little")
    if actual_generation != generation:
        raise AssertionError(
            f"{family}: generation {actual_generation} != expected {generation}"
        )
    if int.from_bytes(data[-4:], "little") != (zlib.crc32(data[:-4]) & 0xFFFFFFFF):
        raise AssertionError(f"{family}: CRC32 mismatch")
    expected_checkpoint = 0 if generation == 1 else 1
    expected_hearts = 3 if generation == 1 else 2 if generation == 2 else 1
    if family in ("map", "level"):
        if data[6] != expected_checkpoint or data[7] != expected_hearts:
            raise AssertionError(
                f"{family}: checkpoint/hearts {data[6]}/{data[7]} "
                f"do not match generation {generation}"
            )
    if family == "campaign":
        wallet = int.from_bytes(data[12:16], "little")
        if wallet != 156 + generation:
            raise AssertionError(f"RKC2 wallet {wallet} != {156 + generation}")
    if family == "level":
        wallet = int.from_bytes(data[36:40], "little")
        if wallet != 157:
            raise AssertionError(f"RKL3 wallet {wallet} != 157")
        coin_bit = bool(data[12] & 1)
        if coin_bit != (generation >= 2):
            raise AssertionError(f"RKL3 coin bit inconsistent with generation {generation}")


def verify_files(root: Path, phase: str) -> None:
    base = root / "sdmc" / "switch" / "RIKITU"
    marker = base / "stage131-phase.txt"
    if marker.read_text().strip() != PHASE_MARKER[phase]:
        raise AssertionError(f"{phase}: unexpected phase marker {marker.read_text()!r}")
    escaped_target = root / "sdmc" / "switch" / "stage131-escape.rks"
    if escaped_target.exists() or escaped_target.is_symlink():
        raise AssertionError(f"{phase}: traversal target was unexpectedly created")
    a_generation, b_generation = GENERATIONS[phase]
    hashes: dict[str, str] = {}
    for family in FAMILIES:
        for slot, generation in (("a", a_generation), ("b", b_generation)):
            path = base / f"stage131-{family}-{slot}.{EXT[family]}"
            if generation is None:
                if path.exists() or path.is_symlink():
                    raise AssertionError(f"{phase}: unexpected slot {path.name}")
            else:
                data = path.read_bytes()
                check_record(data, family, generation)
                hashes[path.name] = hashlib.sha256(data).hexdigest()
                print(
                    f"PASS Stage131 {phase} {family} {slot}: gen={generation} "
                    f"sha256={hashes[path.name][:12]}"
                )
            pending = base / (path.name + ".pending")
            if pending.exists() or pending.is_symlink():
                raise AssertionError(f"{phase}: stale pending path {pending.name}")
        hold = base / f"stage131-{family}-a.hold"
        if hold.exists() or hold.is_symlink():
            raise AssertionError(f"{phase}: backup was not restored: {hold.name}")

    # Hash continuity proves that an interrupted/blocked transaction did not
    # rewrite either live record. Codec validity alone would miss changes to
    # otherwise-valid gameplay fields. The manifest lives outside guest SDMC.
    manifest_path = root / "stage131-slot-hashes.json"
    if phase == "seed":
        if manifest_path.exists():
            raise AssertionError("seed: stale hash manifest in a supposedly fresh root")
        manifest: dict[str, dict[str, str]] = {}
    else:
        if not manifest_path.is_file():
            raise AssertionError(f"{phase}: missing prior hash manifest")
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))

    if phase == "seed":
        manifest["seed"] = hashes
    elif phase == "advance":
        previous = manifest.get("seed")
        if not previous:
            raise AssertionError("advance: seed hashes are missing")
        for family in FAMILIES:
            name = f"stage131-{family}-a.{EXT[family]}"
            if hashes.get(name) != previous.get(name):
                raise AssertionError(f"advance: older A slot changed: {name}")
        manifest["advance"] = hashes
    elif phase == "block":
        previous = manifest.get("advance")
        if not previous or hashes != previous:
            raise AssertionError("block: obstructed transaction changed live slot bytes")
        manifest["block"] = hashes
    elif phase == "recover":
        previous = manifest.get("advance")
        if not previous:
            raise AssertionError("recover: advance hashes are missing")
        for family in FAMILIES:
            a_name = f"stage131-{family}-a.{EXT[family]}"
            b_name = f"stage131-{family}-b.{EXT[family]}"
            if hashes.get(b_name) != previous.get(b_name):
                raise AssertionError(f"recover: surviving B backup changed: {b_name}")
            if hashes.get(a_name) == previous.get(a_name):
                raise AssertionError(f"recover: A slot was not advanced to generation 3: {a_name}")
        manifest["recover"] = hashes
    elif phase == "verify":
        previous = manifest.get("recover")
        if not previous or hashes != previous:
            raise AssertionError("verify: cold-start verification changed committed bytes")
        manifest["verify"] = hashes

    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(f"PASS Stage131 {phase}: production save-pair state, hash continuity and sidecars")


def verify_image(path: Path, phase: str) -> None:
    from PIL import Image
    with Image.open(path) as im:
        rgb = im.convert("RGB")
    if rgb.size != (1280, 720):
        raise AssertionError(f"Unexpected NeXium image dimensions {rgb.size}")
    for i, label in enumerate((
        "SDMC mount", "private directory", "recognized cold-start phase",
        "production save-pair assertions",
    )):
        left = 280 + 200 * i
        greens = sum(
            g > 170 and r < 100 and b < 100
            for x in range(left - 24, left + 100, 8)
            for y in range(108, 200, 8)
            for r, g, b in [rgb.getpixel((x, y))]
        )
        if greens < 32:
            raise AssertionError(f"{path}: {label} failed: {greens} green pixels")
    print(f"PASS Stage131 {phase} four green guest signals {path.name}")


def fake_record(family: str, generation: int) -> bytes:
    magic, size, generation_at, level = FAMILIES[family]
    data = bytearray(size)
    data[:4] = magic
    data[5] = level
    data[generation_at:generation_at + 4] = generation.to_bytes(4, "little")
    if family in ("map", "level"):
        data[6] = 0 if generation == 1 else 1
        data[7] = 3 if generation == 1 else 2 if generation == 2 else 1
    if family == "campaign":
        data[12:16] = (156 + generation).to_bytes(4, "little")
    if family == "level":
        data[36:40] = (157).to_bytes(4, "little")
        if generation >= 2:
            data[12] = 1
    data[-4:] = (zlib.crc32(data[:-4]) & 0xFFFFFFFF).to_bytes(4, "little")
    return bytes(data)


def selftest() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        base = root / "sdmc" / "switch" / "RIKITU"
        base.mkdir(parents=True)
        for phase in ("seed", "advance", "block", "recover", "verify"):
            a_generation, b_generation = GENERATIONS[phase]
            for family in FAMILIES:
                for slot, generation in (("a", a_generation), ("b", b_generation)):
                    path = base / f"stage131-{family}-{slot}.{EXT[family]}"
                    if generation is None:
                        path.unlink(missing_ok=True)
                    else:
                        path.write_bytes(fake_record(family, generation))
            (base / "stage131-phase.txt").write_text(PHASE_MARKER[phase])
            verify_files(root, phase)
        print("PASS Stage131 checker selftest: five phases and all three record formats")


def main() -> None:
    args = sys.argv[1:]
    if args == ["--selftest"]:
        selftest()
    elif len(args) == 4 and args[0] in PHASE_MARKER:
        phase, before, after, root = args
        verify_image(Path(before), phase)
        verify_image(Path(after), phase)
        verify_files(Path(root), phase)
    else:
        raise SystemExit(
            "Usage: check_production_pair_guest.py --selftest | "
            "seed|advance|block|recover|verify BEFORE AFTER DATA_ROOT"
        )


if __name__ == "__main__":
    main()
