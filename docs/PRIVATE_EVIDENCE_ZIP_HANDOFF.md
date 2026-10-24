# Stage158 — private evidence ZIP handoff

Stage157 already builds per-file `SHA-256`/length receipts with a pinned
manifest digest. Stage158 packages the **sealed source tree** into one
immutable, privately generated, uncompressed ZIP containing the exact
`PRIVATE_EVIDENCE_MANIFEST.json` and an `evidence/` subtree.

This solves the **single-file handoff / replayability** problem for the
existing authenticated private Google Drive connector. It **does not**
configure Drive access in GitHub Actions or prove that a user uploaded it.

## Private-only operating sequence

1. Use Stage157 `seal` in a private temporary directory, with a
   non-overwriting manifest outside the captured evidence root. Securely pin
   the manifest SHA-256 in a separate versioned private handoff record.
2. Pack it into a newly named private ZIP outside the Git checkout:

```bash
python tools/private_evidence_bundle.py pack \
  --source-dir /private/run3999/evidence \
  --manifest /private/receipts/run3999-manifest-v1.json \
  --expected-manifest-sha256 PREVIOUSLY_PINNED_64_HEX_SHA \
  --private-zip /private/bundles/RIKITU_run3999_v1.zip
```

3. Record ZIP byte count and SHA-256 separately, then upload **that exact
   ZIP** with the authorized Google Drive connector into the restricted
   RIKITU project `02_Raw_Traces_and_Captures` folder; put the manifest
   and receipt record in `00_Manifest_and_SHA256`. Do not post the ZIP
   to public GitHub artifacts or logs.
4. Confirm Drive metadata (object ID, exact size, `shared=false`,
   parent folder). Then **download that object independently using the
   authorized connector** into a fresh private directory and run:

```bash
python tools/private_evidence_bundle.py verify-readback \
  --private-zip /private/drive-readback/RIKITU_run3999_v1.zip \
  --expected-bundle-sha256 PREVIOUSLY_PINNED_ZIP_SHA \
  --expected-manifest-sha256 PREVIOUSLY_PINNED_MANIFEST_SHA
```

5. Only after matching whole-ZIP hash **and** independently checking all
   archive members against the separately pinned receipt may the journal
   mark this specific Drive file as privately archived. Never bulk-delete
   GitHub artifacts: require exact-ID allowlist and separate authorization.

Both CLI operations print **hashes, byte counts and status only**; no
private filenames, contents, paths or generated binary excerpts.
ZIP entries have fixed timestamps, predictable order, no compression
and no symlinks. Existing ZIP versions are never overwritten.

## Status, limits and authentication

- The ZIP is **not an encrypted format**. Its confidentiality comes from
  the private filesystem and Google Drive access controls. Never upload it
  to the public repository, GH artifacts or open issue comments.
- A local copy named “readback” is **not sufficient evidence** of a Drive
  upload. The trusted operator must prove that the bytes came from an
  independently authenticated Drive download.
- The current permanent archive sits in a normal private Google My Drive.
  Google Cloud service accounts alone cannot own/upload files in personal
  My Drive. Automatic keyless GitHub OIDC would need a **properly configured
  Shared Drive** or a safe delegated user OAuth design with provider consent,
  as well as secrets/environment access isolation. These capabilities
  have **not** been configured in the GitHub runner and this PR does not
  bypass that requirement.
- Cap each sealed bundle at 256 files and 512 MiB via Stage157. Split
  oversized runs into independently versioned receipts.
- The storage issue #121 stays open until an authenticated, restricted
  and end-to-end independently verified automatic runner transfer exists.

The public CI workflow tests only short **synthetic byte strings** and
never supplies source art, real NeXium captures or original game files.
Game logic, Nintendo Switch binary and the still-unapproved GFX-01
panorama are not changed.
