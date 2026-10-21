#!/usr/bin/env python3
"""Stage97: inject reproducible damaged SDMC B slots; prove guest repairs them.

The fixture is created in the *actual NeXium guest* by two independent NRO
launches. Host-side injection changes only the NEWER B save files. A third
independent guest must load valid generation-1 A and rewrite generation-2 B,
then a fourth independent guest reads the repaired generation-2 state.
This models CRC corruption and an incomplete write, NOT real power loss.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import zlib

# (base name, extension, signature, size, crc-offset, generation-offset, fault)
FAMILIES = {
    "rks1": [("map01", "rks", b"RKS1", 44, 40, 32, "crc")],
    "campaign": [
        ("campaign", "rkc", b"RKC2", 32, 28, 16, "crc"),
        ("level", "rkl", b"RKL3", 64, 60, 32, "truncate"),
    ],
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def verified(path: Path, signature: bytes, size: int,
             crc_index: int, generation_index: int) -> bytes:
    data = path.read_bytes()
    if len(data) != size or data[:4] != signature:
        raise AssertionError(
            f"{path}: invalid {signature!r} record length={len(data)}, "
            f"expected={size}"
        )
    crc = struct.unpack_from("<I", data, crc_index)[0]
    if zlib.crc32(data[:crc_index]) & 0xffffffff != crc:
        raise AssertionError(f"{path}: failed CRC32")
    gen = struct.unpack_from("<I", data, generation_index)[0]
    if gen not in (1, 2):
        raise AssertionError(f"{path}: unexpected generation {gen}")
    return data


def paths(root: Path, base: str, ext: str) -> tuple[Path, Path]:
    folder = root / "sdmc" / "switch" / "RIKITU"
    return folder / f"{base}-a.{ext}", folder / f"{base}-b.{ext}"


def proof_path(root: Path, family: str) -> Path:
    return root / f"stage97-{family}-recovery-proof.json"


def inject(root: Path, family: str) -> None:
    proof: dict[str, object] = {"family": family, "records": []}
    for base, ext, sig, size, crc, gen_at, fault in FAMILIES[family]:
        a, b = paths(root, base, ext)
        good_a = verified(a, sig, size, crc, gen_at)
        good_b = verified(b, sig, size, crc, gen_at)
        if struct.unpack_from("<I", good_a, gen_at)[0] != 1:
            raise AssertionError(f"{a}: requires intact older generation 1")
        if struct.unpack_from("<I", good_b, gen_at)[0] != 2:
            raise AssertionError(f"{b}: requires newer generation 2")
        proof["records"].append({
            "base": base,
            "sha_a": sha(good_a),
            "sha_b": sha(good_b),
            "fault": fault,
        })
        if fault == "crc":
            damaged = bytearray(good_b)
            damaged[crc] ^= 0x01  # Same size, invalid checksum
            b.write_bytes(damaged)
        else:
            b.write_bytes(good_b[:13])  # Simulate interrupted partial write
        try:
            verified(b, sig, size, crc, gen_at)
        except AssertionError:
            pass
        else:
            raise AssertionError(f"{b}: fault was not detected")
        if sha(verified(a, sig, size, crc, gen_at)) != sha(good_a):
            raise AssertionError(f"{a}: damaged the surviving backup")
        print(f"STAGE97 INJECT {base}: newer B damaged by {fault}, A gen1 intact")
    proof_path(root, family).write_text(
        json.dumps(proof, indent=2) + "\n", encoding="utf-8"
    )


def check_repair(root: Path, family: str) -> None:
    proof = json.loads(proof_path(root, family).read_text(encoding="utf-8"))
    if proof["family"] != family or len(proof["records"]) != len(FAMILIES[family]):
        raise AssertionError("Stage97 recovery manifest mismatch")
    for (base, ext, sig, size, crc, gen_at, _), rec in zip(
        FAMILIES[family], proof["records"]
    ):
        if rec["base"] != base:
            raise AssertionError("Stage97 record ordering mismatch")
        a, b = paths(root, base, ext)
        good_a = verified(a, sig, size, crc, gen_at)
        good_b = verified(b, sig, size, crc, gen_at)
        if (struct.unpack_from("<I", good_a, gen_at)[0],
            struct.unpack_from("<I", good_b, gen_at)[0]) != (1, 2):
            raise AssertionError(f"{base}: guest did not restore both generations")
        if sha(good_a) != rec["sha_a"] or sha(good_b) != rec["sha_b"]:
            raise AssertionError(
                f"{base}: guest repair changed stored game progress or backup"
            )
        print(
            f"PASS Stage97 NeXium {base}: B repaired from A, "
            "CRC/backup and generation 1->2 verified"
        )


def selftest() -> None:
    def fake(sig: bytes, size: int, crc_at: int, gen_at: int, gen: int) -> bytes:
        blob = bytearray(size)
        blob[:4] = sig
        blob[4] = 2 if sig == b"RKC2" else (3 if sig == b"RKL3" else 1)
        struct.pack_into("<I", blob, gen_at, gen)
        struct.pack_into("<I", blob, crc_at,
                         zlib.crc32(blob[:crc_at]) & 0xffffffff)
        return bytes(blob)
    for family in FAMILIES:
        with tempfile.TemporaryDirectory(prefix="rikitu-stage97-") as d:
            root = Path(d)
            for base, ext, sig, size, crc, gen_at, _ in FAMILIES[family]:
                a, b = paths(root, base, ext)
                a.parent.mkdir(parents=True, exist_ok=True)
                a.write_bytes(fake(sig, size, crc, gen_at, 1))
                b.write_bytes(fake(sig, size, crc, gen_at, 2))
            inject(root, family)
            try:
                check_repair(root, family)
            except AssertionError:
                pass  # Without a real guest repair, it MUST fail
            else:
                raise AssertionError("Unrepaired corrupted slots passed")
            for base, ext, sig, size, crc, gen_at, _ in FAMILIES[family]:
                _, b = paths(root, base, ext)
                b.write_bytes(fake(sig, size, crc, gen_at, 2))
            check_repair(root, family)
    print("PASS Stage97 fault injector selftest: rejects damaged B, accepts repair")


def main() -> None:
    if len(sys.argv) == 2 and sys.argv[1] == "--selftest":
        selftest()
        return
    if (len(sys.argv) != 4 or sys.argv[1] not in ("inject", "check") or
        sys.argv[3] not in FAMILIES):
        raise SystemExit(
            "Usage: check_torn_guest_slots.py inject|check DATA_ROOT rks1|campaign\n"
            "       check_torn_guest_slots.py --selftest"
        )
    action, root, family = sys.argv[1], Path(sys.argv[2]), sys.argv[3]
    if action == "inject":
        inject(root, family)
    else:
        check_repair(root, family)


if __name__ == "__main__":
    main()
