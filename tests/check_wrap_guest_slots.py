#!/usr/bin/env python3
"""Stage99: prove uint32 generation rollover in real NeXium SDMC save pairs.

Guest #1 seeds A/gen1. The host only rewrites its generation to UINT32_MAX,
recomputing CRC, optionally moving that seed to B. Guest #2 must restore
that seed and write the opposite slot as gen1 (zero generation is forbidden).
Guest #3 must select the new gen1 over old UINT32_MAX and retain both slots.
The guest itself validates loaded game state with four green frame markers.
Synthetic generation modification is not an actual long-running game session.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import zlib

from check_torn_guest_slots import FAMILIES, paths
from check_save_guest import validate_frame

MAX = 0xFFFFFFFF


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def record(path: Path, sig: bytes, size: int, crc_at: int, gen_at: int) -> bytes:
    contents = path.read_bytes()
    if len(contents) != size or contents[:4] != sig:
        raise AssertionError(f"{path}: invalid signature/size")
    if struct.unpack_from('<I', contents, crc_at)[0] != (
        zlib.crc32(contents[:crc_at]) & MAX
    ):
        raise AssertionError(f"{path}: CRC mismatch")
    if struct.unpack_from('<I', contents, gen_at)[0] == 0:
        raise AssertionError(f"{path}: forbidden zero generation")
    return contents


def generation(contents: bytes, at: int) -> int:
    return struct.unpack_from('<I', contents, at)[0]


def reissue(contents: bytes, gen: int, crc_at: int, gen_at: int) -> bytes:
    modified = bytearray(contents)
    struct.pack_into('<I', modified, gen_at, gen)
    struct.pack_into('<I', modified, crc_at, zlib.crc32(modified[:crc_at]) & MAX)
    return bytes(modified)


def proof_file(root: Path, family: str, older: str) -> Path:
    return root / f"stage99-{family}-older-{older}-rollover-proof.json"


def prepare(root: Path, family: str, older: str) -> None:
    if family not in FAMILIES or older not in ('a', 'b'):
        raise ValueError('Unsupported family/slot')
    result = {'family': family, 'older': older, 'records': []}
    for base, ext, sig, size, crc_at, gen_at, _ in FAMILIES[family]:
        a, b = paths(root, base, ext)
        if b.exists():
            raise AssertionError(f"{b}: test requires only seeded A")
        seed = record(a, sig, size, crc_at, gen_at)
        if generation(seed, gen_at) != 1:
            raise AssertionError(f"{a}: seed generation must be 1")
        if ext in ('rks', 'rkl') and seed[6] != 1:
            raise AssertionError(f"{a}: seed must contain checkpoint 1")
        near_wrap = reissue(seed, MAX, crc_at, gen_at)
        old_path = a if older == 'a' else b
        old_path.write_bytes(near_wrap)
        if older == 'b':
            a.unlink()
        record(old_path, sig, size, crc_at, gen_at)
        result['records'].append({
            'base': base, 'ext': ext, 'old_sha': digest(near_wrap)
        })
        print(f"STAGE99 PREPARE {base}: older {older.upper()} gen=0xffffffff; other slot absent")
    proof_file(root, family, older).write_text(
        json.dumps(result, indent=2) + '\n', encoding='utf-8')


def audit(root: Path, family: str, older: str, verify: bool = False) -> None:
    proof_path = proof_file(root, family, older)
    result = json.loads(proof_path.read_text(encoding='utf-8'))
    if result.get('family') != family or result.get('older') != older:
        raise AssertionError('Proof family or orientation mismatch')
    if len(result.get('records', [])) != len(FAMILIES[family]):
        raise AssertionError('Proof record-count mismatch')
    for (base, ext, sig, size, crc_at, gen_at, _), entry in zip(
        FAMILIES[family], result['records']
    ):
        if entry['base'] != base or entry['ext'] != ext:
            raise AssertionError('Proof record-order mismatch')
        a, b = paths(root, base, ext)
        old_path, new_path = (a, b) if older == 'a' else (b, a)
        old_data = record(old_path, sig, size, crc_at, gen_at)
        new_data = record(new_path, sig, size, crc_at, gen_at)
        if generation(old_data, gen_at) != MAX or generation(new_data, gen_at) != 1:
            raise AssertionError(f"{base}: expected old=0xffffffff, new=1")
        if digest(old_data) != entry['old_sha']:
            raise AssertionError(f"{base}: previous checkpoint was overwritten")
        if ext in ('rks', 'rkl'):
            if old_data[6] != 1 or new_data[6] != 2:
                raise AssertionError(f"{base}: new checkpoint not loaded/saved")
        elif new_data != reissue(old_data, 1, crc_at, gen_at):
            raise AssertionError('RKC2 campaign data changed across generation wrap')
        if verify:
            if entry.get('new_sha') != digest(new_data):
                raise AssertionError(f"{base}: verification guest rewrote the latest slot")
        else:
            entry['new_sha'] = digest(new_data)
        print(
            f"PASS Stage99 {base}: old {older.upper()}/0xffffffff intact, "
            f"new {('b' if older == 'a' else 'a').upper()}/1 CRC-valid; "
            f"{'verify unchanged' if verify else 'wrap successful'}"
        )
    if not verify:
        proof_path.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')


def fake(sig: bytes, size: int, crc_at: int, gen_at: int) -> bytes:
    contents = bytearray(size)
    contents[:4] = sig
    contents[4] = {b'RKS1': 1, b'RKC2': 2, b'RKL3': 3}[sig]
    contents[5] = 4 if sig != b'RKS1' else 1
    contents[6] = 1
    return reissue(contents, 1, crc_at, gen_at)


def selftest() -> None:
    for family in FAMILIES:
        for older in ('a', 'b'):
            with tempfile.TemporaryDirectory(prefix='rikitu-stage99-') as directory:
                root = Path(directory)
                for base, ext, sig, size, crc_at, gen_at, _ in FAMILIES[family]:
                    a, _ = paths(root, base, ext)
                    a.parent.mkdir(parents=True, exist_ok=True)
                    a.write_bytes(fake(sig, size, crc_at, gen_at))
                prepare(root, family, older)
                try:
                    audit(root, family, older)
                except FileNotFoundError:
                    pass
                else:
                    raise AssertionError('Missing newer slot unexpectedly accepted')
                for base, ext, sig, size, crc_at, gen_at, _ in FAMILIES[family]:
                    a, b = paths(root, base, ext)
                    old_path, new_path = (a, b) if older == 'a' else (b, a)
                    old_data = old_path.read_bytes()
                    current = bytearray(reissue(old_data, 1, crc_at, gen_at))
                    if ext in ('rks', 'rkl'):
                        current[6] = 2
                        current = bytearray(reissue(current, 1, crc_at, gen_at))
                    new_path.write_bytes(current)
                audit(root, family, older)
                audit(root, family, older, verify=True)
                # A complete, checksummed mutation of the old backup must fail.
                first = FAMILIES[family][0]
                base, ext, sig, size, crc_at, gen_at, _ = first
                a, b = paths(root, base, ext)
                old_path = a if older == 'a' else b
                before = old_path.read_bytes()
                damaged = bytearray(before)
                damaged[7] ^= 1
                old_path.write_bytes(reissue(damaged, MAX, crc_at, gen_at))
                try:
                    audit(root, family, older, verify=True)
                except AssertionError:
                    pass
                else:
                    raise AssertionError('Modified old backup escaped SHA-256 check')
                old_path.write_bytes(before)
                # A newest generation=0 must be rejected even if CRC is valid.
                new_path = b if older == 'a' else a
                before = new_path.read_bytes()
                new_path.write_bytes(reissue(before, 0, crc_at, gen_at))
                try:
                    audit(root, family, older, verify=True)
                except AssertionError:
                    pass
                else:
                    raise AssertionError('Zero-generation slot escaped rejection')
                new_path.write_bytes(before)
                audit(root, family, older, verify=True)
    print('PASS Stage99 rollover selftest: both orientations, all three formats, negative cases')


def main() -> None:
    args = sys.argv[1:]
    if args == ['--selftest']:
        selftest()
    elif len(args) == 4 and args[0] == 'frame':
        phase = args[1]
        if phase not in ('seed', 'resume', 'verify'):
            raise SystemExit('Unsupported guest phase')
        validate_frame(Path(args[2]), phase)
        validate_frame(Path(args[3]), phase)
    elif len(args) == 4 and args[0] in ('prepare', 'audit', 'verify'):
        action, root, family, older = args
        if family not in FAMILIES or older not in ('a', 'b'):
            raise SystemExit('Invalid save family or slot orientation')
        if action == 'prepare':
            prepare(Path(root), family, older)
        else:
            audit(Path(root), family, older, verify=(action == 'verify'))
    else:
        raise SystemExit('Usage: check_wrap_guest_slots.py --selftest | frame PHASE BEFORE AFTER | prepare|audit|verify ROOT rks1|campaign a|b')


if __name__ == '__main__':
    main()
