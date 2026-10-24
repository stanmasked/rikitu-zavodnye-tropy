#!/usr/bin/env python3
"""Stage158: privately bundle sealed CI evidence for authenticated Drive handoff.

Never uploads data, prints source paths or claims that local verification
proves authenticated Google Drive storage. Public CI uses fake bytes only.
"""
import argparse
import hashlib
import hmac
import json
import os
from pathlib import Path
import re
import sys
import zipfile

from private_evidence_readback import (
    EvidenceError, _expected_manifest, _inventory, _safe_manifest_file,
    _safe_root, _safe_rel, CHUNK_SIZE, MAX_TOTAL_BYTES,
)

RECEIPT_MEMBER = "PRIVATE_EVIDENCE_MANIFEST.json"
CONTENT_PREFIX = "evidence/"
MAX_MANIFEST_BYTES = 128 * 1024


def _digest(value, error_code):
    if not isinstance(value, str) or not re.fullmatch(r"[0-9A-Fa-f]{64}", value):
        raise EvidenceError(error_code)
    return value.lower()


def _sha_file(path):
    sha = hashlib.sha256()
    with path.open("rb") as fd:
        while True:
            chunk = fd.read(CHUNK_SIZE)
            if not chunk:
                break
            sha.update(chunk)
    return sha.hexdigest()


def _load_receipt(path, expected_digest):
    if path.is_symlink() or not path.is_file():
        raise EvidenceError("missing-or-symlink-receipt")
    try:
        if path.stat().st_size > MAX_MANIFEST_BYTES:
            raise EvidenceError("oversized-receipt")
        raw = path.read_bytes()
        if not hmac.compare_digest(hashlib.sha256(raw).hexdigest(),
                                   _digest(expected_digest, "invalid-pinned-receipt-sha")):
            raise EvidenceError("pinned-receipt-sha-mismatch")
        receipt = json.loads(raw)
        listed = _expected_manifest(receipt)
    except (OSError, ValueError, UnicodeError) as exc:
        raise EvidenceError("invalid-private-receipt") from exc
    return raw, listed


def _safe_target(path, source_root):
    parent = _safe_root(path.parent)
    if parent == source_root or source_root in parent.parents:
        raise EvidenceError("bundle-inside-private-evidence")
    if path.suffix.lower() != ".zip" or not path.name.startswith("RIKITU_"):
        raise EvidenceError("unsafe-private-bundle-name")
    if path.is_symlink() or path.exists():
        raise EvidenceError("bundle-version-already-exists")
    return parent / path.name


def _validate_archive(path, expected_receipt_sha):
    receipt_sha = _digest(expected_receipt_sha, "invalid-pinned-receipt-sha")
    if path.is_symlink() or not path.is_file():
        raise EvidenceError("missing-or-symlink-private-bundle")
    try:
        with zipfile.ZipFile(path, "r") as archive:
            members = archive.infolist()
            if not members or len(members) > 257:
                raise EvidenceError("invalid-bundle-entry-count")
            if members[0].filename != RECEIPT_MEMBER:
                raise EvidenceError("missing-bundle-receipt")
            seen = set()
            for info in members:
                if info.is_dir() or info.filename in seen or info.file_size > MAX_TOTAL_BYTES:
                    raise EvidenceError("invalid-bundle-member")
                if info.compress_type != zipfile.ZIP_STORED:
                    raise EvidenceError("compressed-private-member")
                if info.filename != RECEIPT_MEMBER:
                    if not info.filename.startswith(CONTENT_PREFIX):
                        raise EvidenceError("unexpected-bundle-member")
                    _safe_rel(info.filename[len(CONTENT_PREFIX):])
                seen.add(info.filename)
            receipt = archive.read(RECEIPT_MEMBER)
            if len(receipt) > MAX_MANIFEST_BYTES or not hmac.compare_digest(
                    hashlib.sha256(receipt).hexdigest(), receipt_sha):
                raise EvidenceError("bundle-receipt-digest-mismatch")
            try:
                expected = _expected_manifest(json.loads(receipt))
            except (UnicodeError, ValueError) as exc:
                raise EvidenceError("invalid-bundle-receipt") from exc
            if len(members) != len(expected) + 1:
                raise EvidenceError("unexpected-bundle-file-count")
            actual = []
            total = 0
            for info in members[1:]:
                name = info.filename[len(CONTENT_PREFIX):]
                sha = hashlib.sha256()
                count = 0
                with archive.open(info, "r") as fd:
                    while True:
                        data = fd.read(CHUNK_SIZE)
                        if not data:
                            break
                        count += len(data)
                        total += len(data)
                        if total > MAX_TOTAL_BYTES:
                            raise EvidenceError("bundle-exceeds-size-limit")
                        sha.update(data)
                if count != info.file_size:
                    raise EvidenceError("bundle-size-mismatch")
                actual.append((name, count, sha.hexdigest()))
            if sorted(actual) != sorted(expected):
                raise EvidenceError("bundle-content-manifest-mismatch")
            return len(expected), total
    except (zipfile.BadZipFile, EOFError, OSError, RuntimeError) as exc:
        raise EvidenceError("corrupt-private-bundle") from exc


def pack(source_dir, manifest, pinned_manifest_sha, destination):
    source_root = _safe_root(source_dir)
    raw_receipt, expected = _load_receipt(manifest, pinned_manifest_sha)
    actual, total = _inventory(source_root)
    observed = [(v["path"], v["bytes"], v["sha256"]) for v in actual]
    if observed != expected:
        raise EvidenceError("source-changed-since-seal")
    dest = _safe_target(destination, source_root)
    try:
        fd = os.open(dest, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, "wb") as out:
            with zipfile.ZipFile(out, mode="w", allowZip64=True) as archive:
                for name, path in [(RECEIPT_MEMBER, None)] + [
                        (CONTENT_PREFIX + entry["path"], source_root / entry["path"])
                        for entry in actual]:
                    item = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
                    item.compress_type = zipfile.ZIP_STORED
                    item.external_attr = 0o600 << 16
                    if path is None:
                        archive.writestr(item, raw_receipt)
                    else:
                        before = path.stat()
                        with archive.open(item, "w", force_zip64=True) as target:
                            with path.open("rb") as private_file:
                                while True:
                                    chunk = private_file.read(CHUNK_SIZE)
                                    if not chunk:
                                        break
                                    target.write(chunk)
                        after = path.stat()
                        if before.st_size != after.st_size or (
                                before.st_mtime_ns != after.st_mtime_ns):
                            raise EvidenceError("private-evidence-changed-during-pack")
            out.flush()
            os.fsync(out.fileno())
        verified_files, verified_bytes = _validate_archive(dest, pinned_manifest_sha)
        if verified_files != len(expected) or verified_bytes != total:
            raise EvidenceError("private-bundle-local-verify-mismatch")
        return {
            "result": "PRIVATE_BUNDLE_CREATED_NOT_UPLOADED",
            "files": verified_files,
            "payload_bytes": verified_bytes,
            "bundle_bytes": dest.stat().st_size,
            "bundle_sha256": _sha_file(dest),
            "receipt_sha256": _digest(pinned_manifest_sha,
                                      "invalid-pinned-receipt-sha"),
        }
    except (OSError, EvidenceError, zipfile.LargeZipFile) as exc:
        dest.unlink(missing_ok=True)
        if isinstance(exc, EvidenceError):
            raise
        raise EvidenceError("private-bundle-create-failed") from exc


def verify_readback(readback_zip, expected_zip_sha, expected_receipt_sha):
    _digest(expected_zip_sha, "invalid-pinned-bundle-sha")
    _digest(expected_receipt_sha, "invalid-pinned-receipt-sha")
    if readback_zip.is_symlink() or not readback_zip.is_file():
        raise EvidenceError("missing-or-symlink-private-bundle")
    digest = _sha_file(readback_zip)
    if not hmac.compare_digest(digest, expected_zip_sha.lower()):
        raise EvidenceError("private-bundle-readback-sha-mismatch")
    files, bytes_total = _validate_archive(readback_zip, expected_receipt_sha)
    return {
        "result": "BYTE_MATCH_ONLY_REQUIRES_AUTHENTICATED_DRIVE_FETCH",
        "files": files,
        "payload_bytes": bytes_total,
        "bundle_sha256": digest,
        "receipt_sha256": expected_receipt_sha.lower(),
    }


def main():
    parser = argparse.ArgumentParser(description="Private evidence ZIP handoff")
    commands = parser.add_subparsers(dest="command", required=True)
    seal = commands.add_parser("pack")
    seal.add_argument("--source-dir", required=True, type=Path)
    seal.add_argument("--manifest", required=True, type=Path)
    seal.add_argument("--expected-manifest-sha256", required=True)
    seal.add_argument("--private-zip", required=True, type=Path)
    check = commands.add_parser("verify-readback")
    check.add_argument("--private-zip", required=True, type=Path)
    check.add_argument("--expected-bundle-sha256", required=True)
    check.add_argument("--expected-manifest-sha256", required=True)
    args = parser.parse_args()
    try:
        result = (pack(args.source_dir, args.manifest,
                       args.expected_manifest_sha256, args.private_zip)
                  if args.command == "pack" else
                  verify_readback(args.private_zip, args.expected_bundle_sha256,
                                  args.expected_manifest_sha256))
    except EvidenceError as exc:
        print("RESULT: FAIL " + str(exc), file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
