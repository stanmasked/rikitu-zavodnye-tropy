# Graphics pipeline — RIKITU: «Заводные тропы»

Status: **Stage GFX-01 — art direction and environment source draft**. The current environment art is not approved final art and is not integrated into the game.

Scope: parallel art track; no gameplay, level geometry, balance, save formats, or canonical character designs are changed.

## Non-negotiable rules

- Approved RIKITU character designs are immutable. Do not redraw, replace, reinterpret, or “improve” Rikitu, Moki, Reina, or other approved characters.
- No newly generated character image is a canonical asset. Until the exact approved source sprite is located and verified, environment work must remain character-free.
- Browser and Nintendo Switch technical markers remain technical placeholders; they are not visual design references.
- Environment art must fit the established 16-level campaign and current map data. It must not imply new collision geometry or require gameplay changes.
- The first target is MAP01, **«Цветочная тропа»**, in **Солнечная долина**.

## Visual direction for the first pass

- Family-friendly, warm, storybook 2D illustration; clear silhouettes and calm value structure.
- Bright morning light, sky blues, leaf greens, meadow yellow-greens, warm wood and stone, restrained brass accents for the clockwork mystery.
- Flowers, a stream, distant homes and a far-off mill can establish the valley and foreshadow MAP02 without changing the MAP01 route.
- Keep the central gameplay corridor low-detail. Foreground decoration should not obscure the player, platforms, collectibles, checkpoints, gate, or HUD.
- Avoid photorealism, 3D-rendered materials, heavy bloom, excessive visual noise, and any new character designs.

## First environment draft — private archive

The exact GFX-01 draft source archive is stored in the private Google Drive project archive under **06_Approved_Art_and_Design / GFX-01_Draft_Sources_NOT_APPROVED**. Its private manifest records source hashes and dimensions.

The archived draft contains:
- An environment-only vector panorama for MAP01, 5120 × 720.
- A transparent environment tileset strip, 1280 × 64, with 20 unique 64 × 64 source cells.

These are draft source assets, not approved final art or integrated game resources. Their exact source files and manifest are intentionally excluded from this public repository. The archive must remain unchanged; use derived copies for review. Do not recreate the art from a description or treat the draft as canonical.

The drafts do not define platform collision, collectible placement, checkpoints, exit-gate location, or existing MAP01 geometry.

Before any integration, render at 1280 × 720, inspect legibility at actual play scale, check scrolling/cropping against the existing MAP01, and confirm no decoration competes with gameplay.

## Stage149 — synthetic scene compositing, no art integration

The optional `rikitu_draw_scene_over_background` entry point draws the existing solids, collectibles, enemies, player hitbox and HUD **over a caller-prefilled frame**. It skips only the legacy synthetic sky gradient and far-distance rectangles. The original `rikitu_draw_scene` still calls the same implementation with legacy sky enabled, so existing games and NRO builds keep the identical default rendering path.

Stage149's public regression uses a tiny, synthetic panorama transferred through `rikitu_background_blit_rgba8888` before rendering the unchanged gameplay/HUD layers. It checks visible background pixels remain untouched away from interactive geometry, while player and platform pixels are still drawn on top. GCC/Clang/sanitizer and the full renderer/browser build gates must pass.

**No file loader is activated, and no GFX-01 source/raster pixels are compiled into the binary.** Approved art and a verified private-loading pipeline are separate GFX-02 acceptance gates. Never treat Stage149 as art approval or a screenshot from the real game.

## Stage148 — opt-in RGBA8 source-frame transfer (synthetic only)

The *technical* Stage148 change adds `native/rikitu_background_blit.{h,c}`: a bounded, side-effect-free transfer of a pre-rendered **caller-owned** RGBA8888 panorama into a destination framebuffer. The crop position is calculated by the existing `rikitu_parallax_crop_x` geometry contract, preserving the 7900px MAP01 world and 5120px draft panorama relationship.

The destination and source memory spans are validated (dimensions, per-row strides, last accessible pixel, camera and world-width finiteness) **before any pixel is written**. Row padding remains unchanged, the existing source buffer is never modified, and malformed/short buffers are rejected without partial screen updates. Callers are required to supply independent, non-overlapping memory areas and fully decoded source pixels. There is no decoder, filesystem access, SVG parsing or character art linked into the library.

GCC, Clang and Clang ASan/UBSan tests operate only on a small synthetic 80×8 panorama. **No original/derived SVG, screenshot, real sprite or confidential source pixels are stored in public GitHub**. The existing `rikitu_draw_scene` and Switch scene remain unchanged: actual background rendering is gated on separate visual approval, a verified private asset-loading strategy and NeXium capture QA.

The current GFX-01 stream taper is still an **unapproved private draft** even though it has five independently archived 1280×720 source-only review crops. Do not mistake the Stage148 framebuffer copy for in-game asset integration or a decision on the art direction.

## Production sequence

1. Review and lock the environment style using MAP01 only.
2. Split the environment into independently renderable parallax layers and prepare a low-detail gameplay corridor.
3. Create a reusable MAP01 tileset while keeping collision data separate from art.
4. Create environment props and non-character effects: flowers, bushes, stream details, bridge dressing, gears and light glints.
5. Locate the exact approved Rikitu source art and prepare sprite/animation exports without altering silhouette, costume, face, proportions, or colors.
6. Keep asset manifests, exact hashes, original sources, raw captures, and approval evidence in private Drive.
7. Integrate only after the current renderer has a verified image/texture path; run visual regression tests without publishing raw screenshots as permanent public artifacts.

## Private evidence archive — standard layout

RIKITU uses **Project Evidence Archive — Private Google Drive**. Public access must remain disabled.

- `00_Manifest_and_SHA256`: inventory of source names, byte sizes, SHA-256, acquisition provenance, source revision, tools, and archive location.
- `01_Private_Originals`: unmodified originals and approved reference files; immutable.
- `02_Raw_Traces_and_Captures`: raw emulator traces, screenshots, frame captures, and other potentially sensitive observations.
- `03_Aggregate_Reports`: derived audit results; keep analysis separate from raw inputs.
- `04_Logs_and_CI`: retained CI logs and receipts, with exact workflow/run ID, commit SHA, test conditions, and result.
- `05_Handoff_and_Stage_Notes`: stage scope, status, acceptance gates, unresolved defects, and reproducibility instructions.
- `06_Approved_Art_and_Design`: canonical art and segregated draft sources clearly labelled NOT_APPROVED.
- `07_Builds_and_Test_Artifacts`: private binaries, builds, and retained test outputs.

**Transfer and retention gate:** before removing an existing CI artifact, record its source run/artifact identity, byte size and SHA-256 in the private manifest; verify a private Drive readback against the exact bytes. Deletion is permitted only for specifically verified IDs. Never use a broad cleanup by file name, age, repository, or prefix as a substitute. Keep an immutable original and make changes only to derived copies. Public PR text may describe completion and safe test results, but must not contain private Drive URLs, raw captures, approved art, or secret material.

## GFX-01 raster review — open visual defect (draft only)

The verified source ZIP renders as a 5120 × 720 vector panorama. Reviewing three true-size 1280 × 720 crops (left, middle, right) revealed a visible hard rectangular cut-off at the right side of the stream in the middle crop. The source `layer-stream` paths end vertically at `x=2960`, which is not a convincing natural shoreline. This is an **open draft-art defect**, not an approved result.

Repair only a *derived* copy of the environment SVG: taper/curve the stream or cover its terminus with appropriate banks and vegetation, then independently review the entire panorama and overlapping camera crops. Preserve the original archived ZIP and its checksum unchanged. No character source or existing gameplay geometry may be edited as part of this repair.

A successful synthetic scroll-contract test does not dismiss this visual defect, prove renderer integration or grant art approval.

## MAP01 visual-scroll contract (technical review, not art approval)

The current native MAP01 uses `RIKITU_FLOWER_WORLD_WIDTH = 7900.0f`. Its verified draft panorama is `5120 × 720`, whereas the active scene viewport is `1280 × 720`. Direct one-to-one camera scrolling would expose pixels beyond the panorama at the right end of the 7900-pixel route. **Do not scale the background to 7900 pixels or change gameplay world geometry to conceal this.**

Once an image renderer is explicitly implemented and the environment design approved, the proposed background sampling model is:

```text
camera_world  = clamp(player_x - 0.37 * viewport_width,
                      0, world_width - viewport_width)
camera_source = camera_world
                * (panorama_width - viewport_width)
                / (world_width - viewport_width)

MAP01 draft values: camera_source = camera_world * 3840 / 6620
source crop: [camera_source, camera_source + 1280] x [0, 720]
```

For these dimensions the proportional factor is approximately `0.5800604`: left edge = 0, rightmost source offset = 3840. A source crop must never leave the exact 5120 × 720 panorama. Keep the foreground, interactive objects, HUD and collision geometry in their existing coordinate systems; **do not** apply this parallax transform to platforms, collectibles, player or UI.

The tile strip contract is `20 × 64 = 1280` horizontal source pixels, each tile 64 × 64. The strip is an artist reference only; it is not a new collision definition.

`tests/test_gfx_map01_scroll_contract.py` checks the synthetic relationship, and `native/rikitu_parallax.{h,c}` provides a reusable side-effect-free crop function whose C regression `native/test_parallax_map01.c` runs under GCC and Clang. The function intentionally does not draw, read, package, or approve an art asset, and has no effect on existing MAP01 camera or gameplay. Passing the test does **not** prove rendering or game integration. That still requires texture support, an approved style review and real captured 1280 × 720 frame QA stored only in the verified private archive.

## Technical handoff conventions

- Editable art sources, manifests, approved originals, and raw captures live in private Drive; public GitHub contains safe documentation, code, and tests.
- Never overwrite approved originals. Preserve exact sources and create derived versions for edits.
- Raster exports should use PNG with alpha where needed. Preserve lossless source files and record exact export dimensions in the private manifest.
- Backgrounds and foreground decoration must be separate from collision/platform geometry.
- Do not claim a sprite, animation, or background is integrated until it is actually loaded by the browser or native renderer and verified in a rendered capture.


## Stage181 — enforce the GFX-01 approval gate (code-only)

A new synthetic-only regression checks that the default native scene still uses the legacy debug background, that the optional background overlay is caller-owned and opt-in, and that the RGBA8 copier contains no filesystem/decoder path. It also asserts that the private GFX-01 SVG sources remain absent from the public checkout and that the documented draft/approval state stays explicit.

The GFX-01 contract workflow pins Ubuntu 24.04 and runs this gate alongside the existing synthetic scroll contract and GCC/Clang crop tests. The test contains no private source bytes, Drive IDs, hashes, screenshots or public links. Passing it proves only that draft art is not silently integrated; it does not approve StreamBank v2. Full-size visual approval must remain separate from runtime integration and NeXium screenshot QA.
