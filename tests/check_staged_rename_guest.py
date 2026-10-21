#!/usr/bin/env python3
"""Stage106: verify NeXium guest-provided green checks and exact SDMC files.

Three independent QA NRO starts share the SAME private NeXium SDMC root:
seed A=OLD/B=NEW, advance A=LATEST/B=NEW, verify is read-only.
This proves emulator guest rename-over-existing semantics only, not power-cut
atomicity, fsync durability or physical Nintendo Switch filesystem behavior.
"""
from __future__ import annotations

import hashlib
from pathlib import Path
import sys
import tempfile

def record(seed: int) -> bytes:
    return bytes((seed+i)&255 for i in range(32))

def verify_files(root: Path,phase: str) -> None:
    if phase not in ("seed","advance","verify"):
        raise AssertionError(f"Invalid guest phase: {phase}")
    base=root/"sdmc"/"switch"/"RIKITU"
    a,b=base/"stage106-a.bin",base/"stage106-b.bin"
    expected_a=record(17 if phase=="seed" else 180)
    expected_b=record(120)
    got_a,got_b=a.read_bytes(),b.read_bytes()
    if got_a!=expected_a or got_b!=expected_b:
        raise AssertionError(
            f"Guest staged rename wrong: phase={phase} A={got_a.hex()} "
            f"B={got_b.hex()} expected A={expected_a.hex()} B={expected_b.hex()}"
        )
    if (base/"stage106-a.bin.pending").exists() or (base/"stage106-b.bin.pending").exists():
        raise AssertionError("Unremoved staged file remains after guest commit")
    print(f"PASS Stage106 SDMC {phase}: A={hashlib.sha256(got_a).hexdigest()[:12]} "
          f"B={hashlib.sha256(got_b).hexdigest()[:12]}, no pending file")

def verify_image(path: Path) -> None:
    from PIL import Image
    with Image.open(path) as im:
        rgb=im.convert("RGB")
    if rgb.size!=(1280,720):
        raise AssertionError(f"Unexpected NeXium frame dimensions {rgb.size}")
    for i,name in enumerate(("sdmc mounted","directory present","prior state","rename verified")):
        left=280+200*i
        pixels=(rgb.getpixel((x,y))
                for x in range(left-24,left+100,8)
                for y in range(108,200,8))
        greens=sum(g>170 and r<100 and b<100 for r,g,b in pixels)
        if greens<32:
            raise AssertionError(f"{path}: guest '{name}' is not green ({greens})")
    print(f"PASS Stage106 guest four green checks in {path.name}")

def selftest() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root=Path(temp)
        base=root/"sdmc"/"switch"/"RIKITU"
        base.mkdir(parents=True)
        a,b=base/"stage106-a.bin",base/"stage106-b.bin"
        a.write_bytes(record(17))
        b.write_bytes(record(120))
        verify_files(root,"seed")
        try:
            verify_files(root,"advance")
        except AssertionError:
            pass
        else:
            raise AssertionError("Host failed to detect incomplete transition")
        a.write_bytes(record(180))
        verify_files(root,"advance")
        verify_files(root,"verify")
        tmp=base/"stage106-a.bin.pending"
        tmp.write_bytes(b"interrupted")
        try:
            verify_files(root,"verify")
        except AssertionError:
            pass
        else:
            raise AssertionError("Host failed to reject stale pending")
        tmp.unlink()
        b.write_bytes(record(70))
        try:
            verify_files(root,"verify")
        except AssertionError:
            pass
        else:
            raise AssertionError("Host failed to reject stale target")
    print("PASS Stage106 host audit selftest")

def main() -> None:
    args=sys.argv[1:]
    if args==["--selftest"]:
        selftest()
    elif len(args)==4 and args[0] in ("seed","advance","verify"):
        phase,before,after,root=args
        verify_image(Path(before))
        verify_image(Path(after))
        verify_files(Path(root),phase)
    else:
        raise SystemExit("Usage: check_staged_rename_guest.py --selftest | seed|advance|verify BEFORE AFTER DATA_ROOT")

if __name__=="__main__":
    main()
