#!/usr/bin/env python3
"""Stage98: inject a fault in NEWEST A slot, require NeXium fallback to B.

Two actual independent guest executions produce A/gen1 and B/gen2.
After those guests exit, the host exchanges the complete slot records:
A/gen2 is newest, B/gen1 is the intact older fallback. It damages ONLY
the new A (simulated torn write/CRC error). A third guest must restore B
and rewrite A/gen2; a fourth loads the repaired A without changing files.
These are synthetic media faults, not sudden hardware power cuts.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import zlib

from check_torn_guest_slots import FAMILIES, paths, verified

# Invert Stage97's corruption modes so *both* slot orders and fault types
# are exercised by guest-run tests.
FAULTS = {"rks": "truncate", "rkc": "truncate", "rkl": "crc"}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def proof_path(root: Path, family: str) -> Path:
    return root / f"stage98-{family}-reverse-recovery-proof.json"


def inject(root: Path, family: str) -> None:
    rows: list[dict[str, str]] = []
    for base, ext, sig, size, crc, gen_at, _ in FAMILIES[family]:
        a, b = paths(root, base, ext)
        older = verified(a, sig, size, crc, gen_at)
        newer = verified(b, sig, size, crc, gen_at)
        if struct.unpack_from("<I", older, gen_at)[0] != 1:
            raise AssertionError(f"{a}: require original gen1")
        if struct.unpack_from("<I", newer, gen_at)[0] != 2:
            raise AssertionError(f"{b}: require original gen2")
        # This tests a legal slot orientation opposite to the initial
        # seed/resume sequence, without altering the actual game codec.
        a.write_bytes(newer)
        b.write_bytes(older)
        fault = FAULTS[ext]
        if fault == "truncate":
            a.write_bytes(newer[:13])
        else:
            broken = bytearray(newer)
            broken[crc] ^= 0x80
            a.write_bytes(broken)
        try:
            verified(a, sig, size, crc, gen_at)
        except AssertionError:
            pass
        else:
            raise AssertionError(f"{a}: failed to simulate {fault} fault")
        if sha(verified(b, sig, size, crc, gen_at)) != sha(older):
            raise AssertionError(f"{b}: damaged older backup")
        rows.append({
            "name": base, "older_b_sha": sha(older),
            "newer_a_sha": sha(newer), "fault": fault,
        })
        print(
            f"STAGE98 INJECT {base}: newest A gen2 {fault}, "
            "older B gen1 intact"
        )
    proof_path(root, family).write_text(
        json.dumps({"family": family, "records": rows}, indent=2) + "\n",
        encoding="utf-8",
    )


def check(root: Path, family: str) -> None:
    proof = json.loads(proof_path(root, family).read_text(encoding="utf-8"))
    if proof.get("family") != family or len(proof.get("records", [])) != len(FAMILIES[family]):
        raise AssertionError("Stage98 reverse-slot proof mismatch")
    for (base, ext, sig, size, crc, gen_at, _), row in zip(
        FAMILIES[family], proof["records"]
    ):
        if row["name"] != base or row["fault"] != FAULTS[ext]:
            raise AssertionError("Stage98 slot order mismatch")
        a, b = paths(root, base, ext)
        repaired_a = verified(a, sig, size, crc, gen_at)
        older_b = verified(b, sig, size, crc, gen_at)
        if (struct.unpack_from("<I", repaired_a, gen_at)[0],
            struct.unpack_from("<I", older_b, gen_at)[0]) != (2, 1):
            raise AssertionError(f"{base}: wrong recovery generations A2/B1")
        if sha(repaired_a) != row["newer_a_sha"] or sha(older_b) != row["older_b_sha"]:
            raise AssertionError(
                f"{base}: guest didn't reproduce original records "
                "or changed surviving B backup"
            )
        print(
            f"PASS Stage98 NeXium {base}: damaged newest A repaired, "
            "B/gen1 preserved, A/gen2 byte-exact and CRC valid"
        )


def fake(sig: bytes, size: int, crc_offset: int, gen_offset: int, gen: int) -> bytes:
    data = bytearray(size)
    data[:4] = sig
    data[4] = 2 if sig == b"RKC2" else (3 if sig == b"RKL3" else 1)
    struct.pack_into("<I", data, gen_offset, gen)
    struct.pack_into(
        "<I", data, crc_offset, zlib.crc32(data[:crc_offset]) & 0xffffffff
    )
    return bytes(data)


def selftest() -> None:
    for family in FAMILIES:
        with tempfile.TemporaryDirectory(prefix="rikitu-stage98-") as temp:
            root = Path(temp)
            for base, ext, sig, size, crc, gen_at, _ in FAMILIES[family]:
                a, b = paths(root, base, ext)
                a.parent.mkdir(parents=True, exist_ok=True)
                a.write_bytes(fake(sig, size, crc, gen_at, 1))
                b.write_bytes(fake(sig, size, crc, gen_at, 2))
            inject(root, family)
            try:
                check(root, family)
            except AssertionError:
                pass
            else:
                raise AssertionError("Damaged latest A unexpectedly passed")
            for base, ext, sig, size, crc, gen_at, _ in FAMILIES[family]:
                a, _ = paths(root, base, ext)
                a.write_bytes(fake(sig, size, crc, gen_at, 2))
            check(root, family)
    print("PASS Stage98 reverse-slot fault selftest: rejects broken A, accepts repair")


def main() -> None:
    if len(sys.argv) == 2 and sys.argv[1] == "--selftest":
        selftest()
        return
    if (len(sys.argv) != 4 or sys.argv[1] not in ("inject", "check") or
            sys.argv[3] not in FAMILIES):
        raise SystemExit(
            "Usage: check_reverse_guest_slots.py inject|check ROOT rks1|campaign "
            "or --selftest"
        )
    action, root, family = sys.argv[1], Path(sys.argv[2]), sys.argv[3]
    if action == "inject":
        inject(root, family)
    else:
        check(root, family)


if __name__ == "__main__":
    main()
