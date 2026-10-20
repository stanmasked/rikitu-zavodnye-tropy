#!/usr/bin/env python3
"""Validate minimal NRO0 container metadata without requiring a Switch SDK.

This verifies the file header, not emulator execution or game correctness.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import struct

NRO_MAGIC_OFFSET = 0x10
NRO_SIZE_OFFSET = 0x18
MIN_NRO_SIZE = 0x50


def validate_nro(data: bytes) -> tuple[int, int]:
    """Return (declared program bytes, actual bytes), or raise ValueError."""
    actual = len(data)
    if actual < MIN_NRO_SIZE:
        raise ValueError(f"NRO file too small: {actual} bytes")
    if data[NRO_MAGIC_OFFSET : NRO_MAGIC_OFFSET + 4] != b"NRO0":
        raise ValueError("missing NRO0 signature at offset 0x10")
    declared = struct.unpack_from("<I", data, NRO_SIZE_OFFSET)[0]
    if not MIN_NRO_SIZE <= declared <= actual:
        raise ValueError(
            f"invalid NRO program size {declared} for {actual}-byte file"
        )
    return declared, actual


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("nro", type=Path, help="NRO produced by devkitA64")
    options = parser.parse_args()
    program, total = validate_nro(options.nro.read_bytes())
    print(f"PASS NRO0 header: program={program} bytes, file={total} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
