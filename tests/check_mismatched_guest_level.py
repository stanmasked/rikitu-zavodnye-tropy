#!/usr/bin/env python3
"""Stage101: NeXium guest fallback from CRC-valid but foreign RKL3 slot.

Run separate guests for seed / upgrade / repair / verify. After upgrade,
the host changes ONLY the newest level slot's chapter or wallet and repairs
its CRC. The real guest must reject that plausible but foreign record,
load the intact first checkpoint, rewrite latest RKL3 and preserve backup.
Campaign RKC2 is never modified during injection.
"""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import zlib

from check_torn_guest_slots import paths

MAX = 0xffffffff
RECORDS = {
    "campaign": ("campaign", "rkc", b"RKC2", 32, 28, 16),
    "level": ("level", "rkl", b"RKL3", 64, 60, 32),
}

def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def generation(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]

def reissue(data: bytes, gen: int, crc_at: int, gen_at: int) -> bytes:
    d = bytearray(data)
    struct.pack_into("<I", d, gen_at, gen)
    struct.pack_into("<I", d, crc_at, zlib.crc32(d[:crc_at]) & MAX)
    return bytes(d)

def valid(path: Path, key: str) -> bytes:
    _, _, magic, size, crc_at, gen_at = RECORDS[key]
    d = path.read_bytes()
    if len(d) != size or d[:4] != magic:
        raise AssertionError(f"{path}: invalid size or magic")
    if generation(d, crc_at) != zlib.crc32(d[:crc_at]) & MAX:
        raise AssertionError(f"{path}: CRC-invalid")
    if generation(d, gen_at) == 0:
        raise AssertionError(f"{path}: forbidden zero generation")
    return d

def slot_paths(root: Path, key: str) -> tuple[Path, Path]:
    name, ext, _, _, _, _ = RECORDS[key]
    return paths(root, name, ext)

def proof_path(root: Path, fault: str, newest: str) -> Path:
    return root / f"stage101-{fault}-newest-{newest}-proof.json"

def inject(root: Path, fault: str, newest: str) -> None:
    if fault not in ("chapter", "wallet") or newest not in ("a", "b"):
        raise ValueError("Invalid fault or slot orientation")
    ca, cb = slot_paths(root, "campaign")
    la, lb = slot_paths(root, "level")
    campaign_a, campaign_b = valid(ca, "campaign"), valid(cb, "campaign")
    old_level, latest_level = valid(la, "level"), valid(lb, "level")
    if [generation(campaign_a, 16), generation(campaign_b, 16)] != [1, 2]:
        raise AssertionError("RKC2 must be original generations 1,2")
    if [generation(old_level, 32), generation(latest_level, 32)] != [1, 2]:
        raise AssertionError("RKL3 must be original generations 1,2")
    if campaign_b[5] != 4 or latest_level[5] != 4 or old_level[5] != 4:
        raise AssertionError("Require legitimate MAP04 source records")
    if generation(campaign_b, 12) != 157 or generation(old_level, 36) != 157:
        raise AssertionError("Require wallet 157 source records")
    if latest_level[6] != 2 or old_level[6] != 1:
        raise AssertionError("Require checkpoint 1->2")
    if newest == "a":
        la.write_bytes(latest_level)
        lb.write_bytes(old_level)
    broken = bytearray(latest_level)
    if fault == "chapter":
        broken[5] = 3
    else:
        struct.pack_into("<I", broken, 36, 158)
    broken_bytes = reissue(broken, 2, 60, 32)
    newer = la if newest == "a" else lb
    older = lb if newest == "a" else la
    newer.write_bytes(broken_bytes)
    checked = valid(newer, "level")  # Fault MUST retain a correct CRC32.
    if (fault == "chapter" and checked[5] != 3) or (
        fault == "wallet" and generation(checked, 36) != 158
    ):
        raise AssertionError("The requested incompatible identity was not injected")
    if sha(valid(older, "level")) != sha(old_level):
        raise AssertionError("Injection changed surviving level backup")
    if valid(ca, "campaign") != campaign_a or valid(cb, "campaign") != campaign_b:
        raise AssertionError("Injection changed campaign records")
    proof_path(root, fault, newest).write_text(json.dumps({
        "fault": fault, "newest": newest,
        "campaign_new_sha": sha(campaign_b),
        "level_old_sha": sha(old_level),
        "level_new_sha": sha(latest_level),
    }, indent=2) + "\n", encoding="utf-8")
    print(f"STAGE101 INJECT {fault} newest={newest.upper()}: CRC-valid foreign MAP04 RKL3, backup unchanged")

def audit(root: Path, fault: str, newest: str, verify: bool = False) -> None:
    info = json.loads(proof_path(root, fault, newest).read_text(encoding="utf-8"))
    if info["fault"] != fault or info["newest"] != newest:
        raise AssertionError("Proof identity mismatch")
    ca, cb = slot_paths(root, "campaign")
    la, lb = slot_paths(root, "level")
    c3, c2 = valid(ca, "campaign"), valid(cb, "campaign")
    older = lb if newest == "a" else la
    newer = la if newest == "a" else lb
    old_level, new_level = valid(older, "level"), valid(newer, "level")
    if generation(c3, 16) != 3 or generation(c2, 16) != 2:
        raise AssertionError("RKC2 did not write next generation after guest restore")
    if sha(c2) != info["campaign_new_sha"] or c3 != reissue(c2, 3, 28, 16):
        raise AssertionError("RKC2 campaign changed unexpectedly")
    if generation(old_level, 32) != 1 or generation(new_level, 32) != 2:
        raise AssertionError("RKL3 repaired generation order invalid")
    if sha(old_level) != info["level_old_sha"] or sha(new_level) != info["level_new_sha"]:
        raise AssertionError("Backup changed or guest did not restore original level contents")
    if old_level[5] != 4 or new_level[5] != 4 or (
        generation(old_level, 36), generation(new_level, 36)
    ) != (157, 157):
        raise AssertionError("RKL3 chapter/wallet mismatch after recovery")
    if old_level[6] != 1 or new_level[6] != 2:
        raise AssertionError("Wrong restored checkpoints")
    print(f"PASS Stage101 NeXium {fault} newest={newest.upper()}: "
          "old checkpoint preserved; foreign identity rejected; new RKL3 byte-exact"
          + (" after fourth guest" if verify else " after repair"))

def fake(key: str, gen: int, checkpoint: int) -> bytes:
    _, _, magic, size, crc_at, gen_at = RECORDS[key]
    d = bytearray(size)
    d[:4] = magic
    d[4] = 2 if key == "campaign" else 3
    d[5] = 4
    d[6] = 1 if key == "campaign" else checkpoint
    if key == "level":
        struct.pack_into("<I", d, 36, 157)
    else:
        struct.pack_into("<I", d, 12, 157)
    return reissue(d, gen, crc_at, gen_at)

def selftest() -> None:
    for fault in ("chapter", "wallet"):
        for newest in ("a", "b"):
            with tempfile.TemporaryDirectory(prefix="rikitu-stage101-") as temp:
                root = Path(temp)
                ca, cb = slot_paths(root, "campaign")
                la, lb = slot_paths(root, "level")
                ca.parent.mkdir(parents=True)
                ca.write_bytes(fake("campaign", 1, 1))
                cb.write_bytes(fake("campaign", 2, 2))
                la.write_bytes(fake("level", 1, 1))
                lb.write_bytes(fake("level", 2, 2))
                inject(root, fault, newest)
                try:
                    audit(root, fault, newest)
                except AssertionError:
                    pass
                else:
                    raise AssertionError("Foreign latest slot passed without guest repair")
                newer = la if newest == "a" else lb
                newer.write_bytes(fake("level", 2, 2))
                ca.write_bytes(fake("campaign", 3, 2))
                audit(root, fault, newest)
                audit(root, fault, newest, verify=True)
                old = lb if newest == "a" else la
                original = old.read_bytes()
                tampered = bytearray(original)
                tampered[6] ^= 1
                old.write_bytes(reissue(tampered, 1, 60, 32))
                try:
                    audit(root, fault, newest, verify=True)
                except AssertionError:
                    pass
                else:
                    raise AssertionError("Mutated backup passed SHA verification")
                old.write_bytes(original)
                audit(root, fault, newest, verify=True)
    print("PASS Stage101 CRC-valid foreign RKL3 injector selftest: wallet/chapter, A/B, rejection and repair")

def main() -> None:
    args = sys.argv[1:]
    if args == ["--selftest"]:
        selftest()
    elif len(args) == 4 and args[0] in ("inject", "audit", "verify"):
        action, directory, fault, newest = args
        if action == "inject":
            inject(Path(directory), fault, newest)
        else:
            audit(Path(directory), fault, newest, verify=(action == "verify"))
    else:
        raise SystemExit("Usage: check_mismatched_guest_level.py --selftest | inject|audit|verify ROOT chapter|wallet a|b")

if __name__ == "__main__":
    main()
