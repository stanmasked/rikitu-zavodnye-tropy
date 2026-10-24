# Stage157 — private evidence archive/readback integrity

Status: **local verification tooling and synthetic CI only**. This is not a
GitHub Actions → Google Drive uploader or an automatic durable archive.

## Why this gate is needed

Guest NeXium screenshots, SDMC recovery traces and other raw CI records can
vanish with ephemeral runners. Our public GitHub workflow **must not** publish
those originals as persistent Actions artifacts. A local “checksum passed”
cannot prove an authenticated private Google Drive copy exists.

The Stage157 tool supplies two explicit steps for a trusted private operator:

1. **Seal** a runner-local private evidence tree into a versioned
   `PENDING_DRIVE_READBACK` JSON manifest, recording every relative path,
   exact byte count and SHA-256. The manifest is private too.
2. Pin the sealed manifest SHA-256 printed by the seal command in a
   **separate private handoff record**. A successful readback must match the
   pinned receipt digest; editing the file list and the pixels together
   must not permit a false match.
3. Use the **connected Google Drive authenticated upload** process to store
   the raw tree (typically packaged into an immutable versioned archive) and
   receipt under RIKITU's private archive, never in the Git repository.
   Independently download the uploaded exact files with authenticated Drive
   access into a fresh private directory.
4. **Verify** the readback directory against the sealed manifest **and the
   previously pinned SHA-256 of that exact manifest**. The tool
   rejects missing/extra files, byte or hash mismatches, symlinks, unsafe
   paths, changed manifests and unexpected schemas. Only then document the
   Drive object ID, size, SHA-256, `shared=false`, verification timestamp
   and a separate receipt in the private handoff journal.

```bash
python tools/private_evidence_readback.py seal \
  --source-dir /private/runner-evidence-v1 \
  --manifest /private/receipts/stage157-unique-run-v1.json

# Perform an authenticated private Drive upload and a SEPARATE download.

python tools/private_evidence_readback.py verify \
  --manifest /private/receipts/stage157-unique-run-v1.json \
  --readback-dir /private/independently-downloaded-evidence-v1 \
  --expected-manifest-sha256 PREVIOUSLY_PINNED_64_HEX_SHA256
```

Do not treat the second command's success alone as authentication or
authorization: the operator must establish that the directory is actually
from the private Drive object, not simply a local copy. The CLI deliberately
prints only counts and hashes, not paths, filenames or private data. Error
codes are redacted.

The sealed manifest is stored outside the evidence directory and never
overwritten. The tool requires roots outside Git checkouts, rejects symlinks
and nonregular evidence, limits a receipt to 256 files and 512 MiB, and
verifies content in bounded chunks. Each new run must use a new receipt.
A larger run must be split into separate immutable and verifiable groups.

**Current limitation:** GitHub-hosted runners do not have a provisioned,
authenticated, restricted Drive transfer. Issue #121 therefore remains open.
No automatic private archive or deletion of runner artifacts may be claimed.
For a one-off authenticated operation in this chat, the connected Drive
connector can upload/read back files, but that does not automate future CI.

## Synthetic-only public CI

Unit tests use fake in-memory byte strings called `capture-01.png` and
`guest.log`, generated in a temporary directory. Tests exercise matching
and corrupt readbacks, missing/extra files, manifest tampering, symlinks,
overwriting, traversal, file count bounds and git-checkout refusal. Public
CI neither downloads private artwork nor uploads raw artifacts.

This tool does not approve GFX-01 art, alter game saves or publish any
original character designs.
