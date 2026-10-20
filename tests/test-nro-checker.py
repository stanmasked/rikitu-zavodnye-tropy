#!/usr/bin/env python3
"""Host-only negative and positive fixtures for the NRO0 structure check."""
from __future__ import annotations
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from check_nro import validate_nro


def fixture(declared: int = 0x80, total: int = 0x90) -> bytearray:
    contents = bytearray(total)
    contents[0x10:0x14] = b"NRO0"
    struct.pack_into("<I", contents, 0x18, declared)
    return contents


class NroBinaryValidation(unittest.TestCase):
    def test_normal_nro(self) -> None:
        self.assertEqual(validate_nro(bytes(fixture())), (0x80, 0x90))

    def test_header_inside_exact_minimum(self) -> None:
        self.assertEqual(validate_nro(bytes(fixture(0x50, 0x50))), (0x50, 0x50))

    def test_missing_magic(self) -> None:
        bad = fixture()
        bad[0x10:0x14] = b"ELF0"
        with self.assertRaisesRegex(ValueError, "NRO0"):
            validate_nro(bytes(bad))

    def test_short_file(self) -> None:
        with self.assertRaisesRegex(ValueError, "too small"):
            validate_nro(bytes(fixture()[:0x4F]))

    def test_declared_larger_than_file(self) -> None:
        with self.assertRaisesRegex(ValueError, "invalid NRO program size"):
            validate_nro(bytes(fixture(0xA0, 0x90)))

    def test_declared_smaller_than_header(self) -> None:
        with self.assertRaisesRegex(ValueError, "invalid NRO program size"):
            validate_nro(bytes(fixture(0x20, 0x90)))

    def test_empty_file(self) -> None:
        with self.assertRaises(ValueError):
            validate_nro(b"")


if __name__ == "__main__":
    unittest.main(verbosity=2)
