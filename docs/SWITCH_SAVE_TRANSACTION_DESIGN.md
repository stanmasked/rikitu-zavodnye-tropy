# Nintendo Switch Save Transaction Design — Stage127

**Status:** design proposal only. No shipping save writer is changed by this document.

## Why this work is needed

The RKS1, RKC2, and RKL3 codecs already validate fixed-size records and CRCs. The remaining gap is the storage transaction that writes those records to the two SDMC slots.

Current production paths:

- `native/rikitu_save_io.c` — RKS1
- `native/rikitu_campaign_save_io.c` — RKC2
- `native/rikitu_level_save_io.c` — RKL3

Stage130 implements this classification in Draft PR
[114](https://github.com/stanmasked/rikitu-zavodnye-tropy/pull/114): the shared
reader distinguishes inspection errors from absent/malformed fixed-size
records, and all three production load/store paths fail closed on an I/O error.
Its host test injects read-only `EACCES` while allowing write access, checks
both A/B orientations for RKS1, RKC2 and RKL3, and asserts that failed loads
leave caller state unchanged and failed stores preserve both slots byte for
byte. This is still a Draft PR; GCC/Clang CI has not yet completed, so the
change is not considered validated or merged. The host injection does not
prove physical Switch SDMC durability.

Each production store routine selects a target slot and opens it with `fopen(target, "wb")`. That truncates the selected file before the new record has been completely written and checked. The other slot is intended to remain as fallback, but the writer itself does not stage and verify the replacement.

The experimental `native/rikitu_staged_slot.c` writes a sibling `.pending` file, closes it, checks exact-byte readback, then calls `rename()`. Its Linux path adds advisory locking and inode/path checks. The non-Linux path used by the Switch QA build does not yet have equivalent locking and path-identity protections.

The experimental `native/rikitu_staged_pair.c` serializes pair selection and commit with Linux `flock`; it deliberately fails closed outside Linux. It is not a production Switch backend and is not called by the shipping save writers.

Stage125/126 guest proofs are useful evidence for their tested cases in NeXium. They do **not** establish concurrent-writer safety or physical SDMC power-loss durability.

## Required invariants

1. **Never target the newest valid slot.** Choose the invalid/older slot only after reading and validating both records inside the same serialized transaction.
2. **Preserve the backup on every pre-commit failure.** A failed open, short write, flush/close error, readback mismatch, codec rejection, or replacement failure must not damage the other valid slot.
3. **Use real codec validation.** A staged record must pass the correct RKS1/RKC2/RKL3 decoder, expected size, CRC, generation, and applicable campaign identity checks before it can replace a slot.
4. **Fail closed on ambiguous pairs.** Do not choose a winner when two valid records have the same generation but different bytes, or when serial-number arithmetic cannot order them unambiguously.
5. **Serialize selection through commit.** The lock covers reading A/B, selecting the destination and generation, staging, verification, replacement, and the final filesystem commit request. Locking only the individual file write is insufficient.
6. **Never silently fall back to direct truncation.** If the target backend cannot provide its documented transaction semantics, return failure and preserve the existing valid slot.
7. **Keep format codecs independent of storage.** Do not change RKS1/RKC2/RKL3 layouts or weaken CRC/generation validation to make the backend easier to implement.
8. **Report durability precisely.** A successful `fwrite`, `fflush`, `fclose`, or `rename` alone is not proof that the bytes survive sudden power loss.

## Proposed architecture

### 1. Shared pair-store policy

Keep the codecs as-is and consolidate duplicated pair-write policy behind one shared internal transaction API. The API should accept:

- A and B slot paths.
- Record size and family-specific validate/encode callbacks.
- Any required expected campaign identity or stale-snapshot generation.
- A storage backend that implements staging, exact readback, replacement, and filesystem commit.

The shared policy must classify **absent/invalid records** separately from **filesystem/path I/O errors**. A permission, directory, or read error must not be treated as an empty slot and then overwritten.

### 2. Host backend

Retain the Linux host backend as the place to prove concurrent-process behavior. It must continue to use a stable pair lock and fail closed on lock-path replacement, ambiguous generations, and stale snapshots. Existing Linux tests are evidence for the host backend only.

### 3. Switch/libnx backend

Implement a distinct backend for the SDMC filesystem. Do not reuse Linux `flock`, `realpath`, `O_NOFOLLOW`, inode checks, or assumptions about POSIX rename replacement as if they were automatically available on Switch.

Before selecting the final replacement algorithm, verify against the exact libnx/toolchain revision used by the build and on the target platform:

- Whether `rename(temp, existing_target)` replaces an existing file or fails.
- What `fflush()` and `fclose()` report for the mounted `sdmc:` device.
- The return/error behavior of `fsdevCommitDevice("sdmc")` and whether it is appropriate after the staged replacement.
- Whether the required same-directory staging, readback, rename, and commit sequence preserves the old destination on each reported failure.
- Which synchronization primitive protects all in-process save callers. Do not silently substitute an unlocked writer.

The current `switch_graphical/source/main.c` mounts/reuses `sdmc` and unmounts it on shutdown when it mounted it locally. No explicit `fsdevCommitDevice("sdmc")` call appears in the inspected save/shutdown path. Review that lifecycle together with the backend; adding a commit call must still be treated as a request to the filesystem, not a blanket power-loss guarantee.

Reference for the libnx fsdev implementation and API surface:
- https://github.com/switchbrew/libnx/blob/master/nx/source/runtime/devices/fs_dev.c
- https://github.com/switchbrew/libnx/blob/master/nx/include/switch/runtime/devices/fs_dev.h

Pin or record the exact libnx/devkitPro revision before treating implementation details from a moving upstream branch as the contract.

### Source audit: libnx snapshot, not yet the build contract

I inspected the public `switchbrew/libnx` source at commit
`b156b4774fb2df7b93402169509266da7547fc4c` on 2026-10-22:

- In [`fsdev_rename`](https://github.com/switchbrew/libnx/blob/b156b4774fb2df7b93402169509266da7547fc4c/nx/source/runtime/devices/fs_dev.c),
  libnx determines the source entry type and dispatches regular files to
  `fsFsRenameFile`. It does not explicitly unlink the destination first.
  This wrapper does **not** prove whether the underlying HOS filesystem
  replaces an existing destination; that remains a runtime/platform question.
- `fsdevCommitDevice("sdmc")` returns the result of
  `fsFsCommit(&device->fs)`. The [header](https://github.com/switchbrew/libnx/blob/b156b4774fb2df7b93402169509266da7547fc4c/nx/include/switch/runtime/devices/fs_dev.h)
  says commit is not automatic at unmount and should be used after file
  writes. The current `switch_graphical/source/main.c` shutdown path can
  unmount SDMC but contains no explicit `fsdevCommitDevice` call.
  A commit failure after rename is therefore a distinct, ambiguous
  post-replacement outcome; it must not be reported as though the previous
  target were certainly unchanged.
- `fsFileFlush` returns a native `Result`, and libnx's `fsdev_fsync`
  translates that result into the POSIX return value. In contrast,
  `fsFileClose` returns `void`; the inspected `fsdev_close` cannot
  propagate a native close result through `fclose`. For ordinary buffered
  writers, `fflush` drains stdio data through `fsdev_write`, whose normal
  path uses `fsFileWrite`; it does not itself request
  `fsFileFlush`. Therefore `fflush`/ `fclose` success alone must not be
  treated as a filesystem-commit guarantee.
- The Switch build workflows use the container tag
  `devkitpro/devkita64:20260219`, but current logs do not record the
  installed `libnx` package version. Before treating this source snapshot
  as the build contract, record the installed devkitPro/libnx package
  versions in CI and bind the audit to that exact version.

These are source-level findings, not proof of physical SDMC behavior. Keep
the existing-target rename and `fsdevCommitDevice` result checks as
separate QA cases, and do not claim power-loss durability from NeXium alone.

### Stage128–129 CI evidence: bounded dependency setup and libnx fingerprint

On commit `db5013deda904692a100643a84e337add2351451`, two guest workflows
completed successfully after adding bounded APT operations and disabling
recommended packages:

- [Stage106 rename replacement](https://github.com/stanmasked/rikitu-zavodnye-tropy/actions/runs/39839316416):
  package indexes fetched about 3 MB in roughly 3 seconds; dependency
  installation fetched 31.7 MB in about 5 seconds. Three independent cold
  starts passed, with expected A/B hashes and no leftover pending file.
- [Stage125 real-codec staged replacement](https://github.com/stanmasked/rikitu-zavodnye-tropy/actions/runs/39839316721):
  dependency installation and all three guest starts passed.

The first Stage128 attempt to fingerprint the build environment failed before
compilation because the devkitPro container's default shell was `sh`, while
the diagnostic used Bash-only `set -o pipefail`. Commit
`c31a2a1ad7d17977043a475cc468e5101b0786d1` explicitly selects Bash for that
step in the four Switch workflows.

The first retry stopped because `switch-dev` is not a package name in
this image. Commit `9eb6bbd82e78cabef92bf278c0d6ee13a0860f21` removes that
name from the strict query while retaining checks for the libnx headers,
static archive, and their SHA-256 fingerprints.

The subsequent Stage126 guest build completed the fingerprint step and
compiled the isolated QA NRO successfully in container
`devkitpro/devkita64:20260219` (image digest
`sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282`):

- `libnx 4.12.0-1`
- `devkitA64 r29.2-1`
- `devkitARM r67.1-1`
- `switch/runtime/devices/fs_dev.h` SHA-256:
  `ff7559fda745935c6cbac0dfd8bd6c9f5b1b3b9f5f81ed3d97e14773e041b507`
- `switch/services/fs.h` SHA-256:
  `afdbd18ff86a2ded14d32261394b2cdbdc5f3ae5613c77d4dabeb82009c451d6`
- `libnx.a` SHA-256:
  `12a59cdfdceab83e39ee129b0275dd803e393e83f19de3922a3b6cc95c767edd`

The run reported `PASS NRO0 header: program=196608 bytes, file=219333 bytes`.
The follow-on guest proof in
[Stage126 run 39845005538](https://github.com/stanmasked/rikitu-zavodnye-tropy/actions/runs/39845005538)
also completed successfully. Six independent NeXium starts exercised seed,
partial pending, observe, blocked commit, recovery, and final verification
with authentic RKS1/RKC2/RKL3 records. The blocked phase preserved all six
live-slot byte sequences; after removing the obstruction, the retry wrote
generation 4 to slot B while preserving generation 3 in slot A for each
format. This is evidence for the isolated staged-slot QA path under NeXium,
not proof of physical Switch SDMC power-loss durability and not yet proof
that production save functions use the staged writer.

### 4. Replacement policy

Do not implement an unlink-first fallback merely because replacement rename fails. If the target filesystem cannot replace an existing destination, design a separate two-slot-safe rotation protocol that always retains the newest valid slot across every interruption boundary. That protocol requires its own tests before integration.

## Required test matrix

### Portable host tests

For each of RKS1, RKC2, and RKL3, test both A/B orientations and inject failure at every storage boundary:

- Temporary-file create/open.
- Short write.
- Flush and close.
- Reopen/readback, including truncated, extended, and byte-mismatched records.
- Codec/CRC/generation/campaign-identity validation.
- Destination replacement.
- Final filesystem-commit result.
- Pre-existing staging obstruction and cleanup failure.
- Two writers that start from the same stale snapshot.
- Process termination before and after each commit boundary.

After every injected failure, compare exact bytes of both live slots and prove the newest previously valid record still loads.

### NeXium guest tests

Use the real NRO and real RKS1/RKC2/RKL3 codecs. Verify the target adapter's return values, slot bytes, generations, and reload result. Emulator evidence is necessary but not sufficient for a physical SDMC durability claim.

### Physical Nintendo Switch validation

Before describing the backend as power-loss-safe, test on the actual target filesystem with a disposable test profile and a recoverable fixture. Verify existing-target rename behavior, filesystem commit return codes, and recovery after controlled process termination. Any actual power-interruption experiment must be explicitly designed to avoid risking the user's real saves.

## Rollout gates

1. Document and test the exact libnx SDMC semantics.
2. Add an injectable host backend and exhaustive failure-boundary tests.
3. Implement the Switch backend without changing record formats.
4. Route RKS1 through the shared transaction API; run host tests, NRO build, and guest tests.
5. Repeat independently for RKC2 and RKL3.
6. Keep each stage in Draft until all relevant checks pass.
7. Do not claim physical power-loss durability until the target-hardware evidence exists.

## Current conclusion

The codecs and the staged prototype provide a useful foundation, but the three production writers still use direct `"wb"` writes. The next safe step is to prove the libnx/SDMC replacement and commit semantics, then integrate through a shared transaction policy. This document intentionally makes no production-code or hardware-durability claim.
