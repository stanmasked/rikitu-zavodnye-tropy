#!/usr/bin/env python3
"""Stage157: manifest and compare private CI evidence without publishing bytes.

This tool does NOT upload to or authenticate Google Drive. Its verify command
is meaningful as a durability gate ONLY when readback-dir was created from an
independent authenticated Drive download by the trusted operator. Never print
private filenames, paths, payloads or manifest bodies in public CI logs.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import stat
import sys

FORMAT = "rikitu-private-evidence-v1"
MAX_FILES = 256
MAX_TOTAL_BYTES = 512 * 1024 * 1024
CHUNK_SIZE = 1024 * 1024


class EvidenceError(Exception):
    """Only a constant error code, never a path or private content."""


def _safe_root(path):
    if path.is_symlink() or not path.is_dir():
        raise EvidenceError("invalid-private-directory")
    root = path.resolve()
    for ancestor in (root, *root.parents):
        if (ancestor / ".git").exists():
            raise EvidenceError("private-directory-inside-git-checkout")
    return root


def _safe_rel(name):
    if not isinstance(name, str) or not name or "\\" in name or "\x00" in name:
        raise EvidenceError("invalid-relative-entry")
    p = PurePosixPath(name)
    if p.is_absolute() or p.as_posix() != name or any(
            part in (".", "..") for part in name.split("/")):
        raise EvidenceError("invalid-relative-entry")
    return p


def _inventory(root):
    entries = []
    total = 0
    for folder, subdirs, filenames in os.walk(root, followlinks=False):
        directory = Path(folder)
        if any((directory / x).is_symlink() for x in subdirs + filenames):
            raise EvidenceError("symlink-in-private-evidence")
        for name in sorted(filenames):
            file = directory / name
            info = file.lstat()
            if not stat.S_ISREG(info.st_mode):
                raise EvidenceError("nonregular-evidence-entry")
            rel = file.relative_to(root).as_posix()
            _safe_rel(rel)
            total += info.st_size
            if info.st_size > MAX_TOTAL_BYTES or total > MAX_TOTAL_BYTES:
                raise EvidenceError("evidence-size-limit")
            if len(entries) >= MAX_FILES:
                raise EvidenceError("evidence-file-count-limit")
            hasher = hashlib.sha256()
            count = 0
            with file.open("rb") as stream:
                while True:
                    block = stream.read(CHUNK_SIZE)
                    if not block:
                        break
                    count += len(block)
                    hasher.update(block)
            after = file.stat()
            if count != info.st_size or info.st_mtime_ns != after.st_mtime_ns or (
                    info.st_size != after.st_size):
                raise EvidenceError("evidence-changed-during-read")
            entries.append({"path": rel, "bytes": count, "sha256": hasher.hexdigest()})
    if not entries:
        raise EvidenceError("empty-evidence-folder")
    entries.sort(key=lambda v: v["path"])
    return entries, total


def _safe_manifest_file(path):
    if path.is_symlink():
        raise EvidenceError("manifest-is-symlink")
    parent = _safe_root(path.parent)
    return parent / path.name


def _expected_manifest(data):
    if not isinstance(data, dict) or set(data) != {
            "format", "status", "entries", "total_bytes"}:
        raise EvidenceError("invalid-manifest-schema")
    if data["format"] != FORMAT or data["status"] != "PENDING_DRIVE_READBACK":
        raise EvidenceError("invalid-manifest-status")
    entries = data["entries"]
    if not isinstance(entries, list) or not 1 <= len(entries) <= MAX_FILES:
        raise EvidenceError("invalid-manifest-entries")
    checked = []
    total = 0
    for entry in entries:
        if not isinstance(entry, dict) or set(entry) != {"path", "bytes", "sha256"}:
            raise EvidenceError("invalid-manifest-entry")
        path = _safe_rel(entry["path"]).as_posix()
        size, digest = entry["bytes"], entry["sha256"]
        if type(size) is not int or not 0 <= size <= MAX_TOTAL_BYTES:
            raise EvidenceError("invalid-manifest-size")
        if not isinstance(digest, str) or len(digest) != 64 or any(
                x not in "0123456789abcdef" for x in digest):
            raise EvidenceError("invalid-manifest-digest")
        checked.append((path, size, digest))
        total += size
    if len(set(x[0] for x in checked)) != len(checked) or total > MAX_TOTAL_BYTES:
        raise EvidenceError("invalid-manifest-duplicates-or-total")
    if type(data["total_bytes"]) is not int or data["total_bytes"] != total:
        raise EvidenceError("invalid-manifest-total")
    if [v[0] for v in checked] != sorted(v[0] for v in checked):
        raise EvidenceError("unsorted-manifest")
    return checked


def seal(source_dir: Path, output_manifest: Path):
    root = _safe_root(source_dir)
    destination = _safe_manifest_file(output_manifest)
    if destination == root or root in destination.parents:
        raise EvidenceError("manifest-must-be-outside-evidence-tree")
    if destination.exists() or destination.is_symlink():
        raise EvidenceError("manifest-exists")
    entries, total = _inventory(root)
    record = {
        "format": FORMAT,
        "status": "PENDING_DRIVE_READBACK",
        "entries": entries,
        "total_bytes": total,
    }
    content = (json.dumps(record, ensure_ascii=False, sort_keys=True,
                          separators=(",", ":")) + "\n").encode("utf-8")
    # Exclusive create: never clobber private manifests or historical versions.
    try:
        fd = os.open(destination, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, "wb") as output:
            output.write(content)
            output.flush()
            os.fsync(output.fileno())
    except FileExistsError as exc:
        raise EvidenceError("manifest-exists") from exc
    except OSError as exc:
        raise EvidenceError("manifest-write-error") from exc
    return {"result": "PRIVATE_MANIFEST_SEALED_NOT_ARCHIVED",
            "files": len(entries), "total_bytes": total,
            "manifest_sha256": hashlib.sha256(content).hexdigest()}


def verify(manifest: Path, readback_dir: Path):
    path = _safe_manifest_file(manifest)
    root = _safe_root(readback_dir)
    try:
        raw = path.read_bytes()
        if len(raw) > 128 * 1024:
            raise EvidenceError("manifest-too-large")
        data = json.loads(raw)
    except (OSError, ValueError, UnicodeError) as exc:
        raise EvidenceError("manifest-read-error") from exc
    expected = _expected_manifest(data)
    actual, total = _inventory(root)
    observed = [(x["path"], x["bytes"], x["sha256"]) for x in actual]
    if observed != expected:
        raise EvidenceError("private-readback-mismatch")
    return {
        "result": "BYTE_MATCH_ONLY_REQUIRES_AUTHENTICATED_DRIVE_FETCH",
        "files": len(expected), "total_bytes": total,
        "manifest_sha256": hashlib.sha256(raw).hexdigest(),
    }


def main():
    parser = argparse.ArgumentParser(description="Private-only CI evidence receipt")
    sub = parser.add_subparsers(dest="command", required=True)
    s = sub.add_parser("seal")
    s.add_argument("--source-dir", required=True, type=Path)
    s.add_argument("--manifest", required=True, type=Path)
    v = sub.add_parser("verify")
    v.add_argument("--manifest", required=True, type=Path)
    v.add_argument("--readback-dir", required=True, type=Path)
    args = parser.parse_args()
    try:
        result = (seal(args.source_dir, args.manifest)
                  if args.command == "seal"
                  else verify(args.manifest, args.readback_dir))
    except EvidenceError as error:
        print("RESULT: FAIL " + str(error), file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
