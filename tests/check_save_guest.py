#!/usr/bin/env python3
"""Confirm actual NeXium guest SDMC save/resume across independent launches."""
from pathlib import Path
import sys
import zlib
import struct
from PIL import Image

PHASE_COLORS = {
    "seed": lambda r,g,b: r>170 and g>130 and b<90,
    "resume": lambda r,g,b: r<90 and g>90 and b>170,
    "verify": lambda r,g,b: r>160 and g<100 and b>150,
}
EXPECTED = {"seed":(1,1),"resume":(2,2),"verify":(2,2)}

def validate_frame(path:Path, phase:str) -> None:
    with Image.open(path) as image:
        rgb=image.convert("RGB")
    if rgb.size!=(1280,720):
        raise AssertionError(f"Unexpected guest window {rgb.size}")
    points=(rgb.getpixel((x,y)) for x in range(1080,1200,8)
            for y in range(108,200,8))
    matched=sum(PHASE_COLORS[phase](*pixel) for pixel in points)
    if matched<32:
        r,g,b=rgb.getpixel((1128,238))
        mount_result=(r|(g<<8)|(b<<16))
        raise AssertionError(
            f"NeXium did not reach {phase}: {matched} phase-color samples "
            f"on {path}; guest fsdevMountSdmc Result low24=0x{mount_result:06x}. "
            "A red mount flag is a guest filesystem/IPC failure, not "
            "proof that the RKS1 save codec failed.")
    for i in range(4):
        left=280+i*200
        points=(rgb.getpixel((x,y)) for x in range(left-24,left+100,8)
                for y in range(108,200,8))
        green=sum(g>170 and r<100 and b<100 for r,g,b in points)
        if green<32:
            center=rgb.getpixel((left+32,140))
            root=path.parent.parent / "save-root" / "sdmc" / "switch" / "RIKITU"
            diagnostic=("Guest SDMC mount/initialization failed" if i==0
                        else f"Guest persistence check bit {i} failed")
            raise AssertionError(
                f"{diagnostic}: phase={phase}, screenshot={path}, "
                f"green={green}, marker RGB={center}, SD files="
                f"{sorted(str(p) for p in root.glob('map01-*.rks')) if root.exists() else 'SD folder absent'}")
    print(f"PASS NeXium {phase}: four green guest checks, correct stage marker, {path.name}")

def validate_slots(root:Path,phase:str) -> None:
    folder=root/"sdmc"/"switch"/"RIKITU"
    files=list(folder.glob("map01-*.rks"))
    expected_count, expected_cp=EXPECTED[phase]
    if len(files)!=expected_count:
        raise AssertionError(f"Expected {expected_count} SDMC save slots, found {files}")
    generations=[]
    for path in files:
        contents=path.read_bytes()
        if len(contents)!=44 or contents[:4]!=b"RKS1":
            raise AssertionError(f"Invalid guest SDMC save file: {path}")
        crc=struct.unpack_from("<I",contents,40)[0]
        if (zlib.crc32(contents[:40])&0xffffffff)!=crc:
            raise AssertionError(f"Guest save checksum invalid: {path}")
        generations.append((struct.unpack_from("<I",contents,32)[0],contents[6]))
    highest=max(generations)
    if highest!=(expected_count,expected_cp):
        raise AssertionError(f"Wrong newest persisted save: {highest} expected {(expected_count,expected_cp)}")
    print(f"PASS host SDMC files after independent {phase}: {generations}")

def main() -> None:
    if len(sys.argv)!=5:
        raise SystemExit("Usage: check_save_guest.py <seed|resume|verify> <before.png> <after.png> <data_root>")
    phase=sys.argv[1]
    if phase not in EXPECTED:
        raise ValueError(f"Unsupported phase: {phase}")
    for path in [Path(sys.argv[2]),Path(sys.argv[3])]:
        validate_frame(path,phase)
    validate_slots(Path(sys.argv[4]),phase)

if __name__=="__main__":
    main()
