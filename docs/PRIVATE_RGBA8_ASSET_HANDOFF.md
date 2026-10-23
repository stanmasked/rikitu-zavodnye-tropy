# Private PNG to RGBA8 handoff — Stage153

Status: **offline tooling only; no art approval, no Switch runtime asset loader**.

## Why this exists

The native 1280×720 framebuffer uses little-endian uint32 pixels
`0xAABBGGRR`. In memory the same pixel is ordered **R, G, B, A**.
The Stage148–151 code can copy a caller-owned RGBA8 panorama, and Stage149
can draw gameplay and HUD on top. It does not decode PNG or load assets.

This opt-in converter turns a **locally supplied, explicitly approved**
opaque RGB/RGBA PNG into a tightly packed RGBA8 byte stream. It keeps
all real source and derived media outside the public repository.

For a 5120×720 private panorama, expect **14,745,600 raw bytes** and
`stride_pixels=5120`; these bytes can be sliced through the existing
bounded camera helper. Raw conversion is not itself a texture loader,
nor evidence that the source art has been approved.

## Strict gates

- Source must be a PNG, RGB or fully opaque RGBA, correct expected dimensions,
  bounded to 32 MiB input and 16 million decoded pixels.
- Caller must supply the **previously verified exact source SHA-256** via
  `--expected-sha256`. No matching digest means no output.
- The private output directory must already exist and must not be inside any
  detected Git checkout. A pre-existing version is never overwritten.
  Output names must be safe local basenames with `.rgba8` suffix.
- An atomic link publishes a fully written derived file with private
  permissions; on errors, temporary material is cleaned up.
- JSON stdout contains only pixel dimensions, byte counts, hashes, stride,
  and an explicit draft/unapproved status. Do not put source paths, images,
  pixel bytes, raw screenshots or private Drive URLs in CI logs.
- Raw exported pixels, original image, JSON receipt, private CI captures and
  checksum manifest belong in the private Google Drive project folders.
  Confirm size, SHA-256 and an authenticated independent Drive **readback**
  before discarding any working copy.

Example **for a private operator environment**, not for public CI:

```bash
python tools/export_private_png_rgba8.py \
  --input /private/MAP01-approved.png \
  --private-output-dir /private/derived \
  --output-name MAP01-approved-v1.rgba8 \
  --expected-sha256 YOUR_VERIFIED_64_HEX_DIGEST \
  --width 5120 --height 720
```

This command is only appropriate after the visual approval gate. The current
StreamBank-taper candidate A remains **DRAFT / NOT APPROVED**, so it must
not be silently named approved, baked into a public build, or substituted for
the immutable source ZIP. GFX-02 issue #126 continues to track that approval,
private archiving, real Switch texture loading and verified private NeXium
captures.

## Public regression

The Stage153 workflow creates only a tiny 4×2 **synthetic** PNG in ephemeral
runner memory and validates RGBA byte order, SHA mismatch rejection, alpha
opacity checks, version immutability, unsafe path refusal and decoder errors.
No source art or raw PNG/texture artifact is uploaded publicly.
