# RIKITU Stage146 — private SVG source inspection

The Stage146 auditor is a **code-only quality gate** for environment/vector art, not a replacement for visual approval. The public repository contains synthetic fixtures and the checking program, **not** any canonical RIKITU character art or GFX-01 draft SVG bytes.

## Local-only review

1. Retrieve the exact-byte source from the private RIKITU Project Evidence Archive. Verify its recorded size and SHA-256 before extracting to a private workspace.
2. Run `python3 tools/audit_svg_source.py <private-local-file.svg>` against each candidate source. This prints only issue codes and aggregate element counts, not the source path or vector geometry; do not paste raw SVG contents into public GitHub issues or CI logs.
3. Resolve errors: malformed XML/invalid viewBox, duplicate IDs, unresolved internal references, script/foreign content, remote URLs, incomplete path commands, and missing or invalid geometric extents.
4. Review warnings manually: repeated `use` elements in one parent, move-only paths, and non-pixel extents. Some legitimate designs may reuse objects deliberately.
5. Inspect full-size raster renders and overlapping 1280×720 camera crops separately: an SVG can be syntactically valid but have **hard cut-offs, seams, doubled-looking shapes, missing fills, or wrong proportions**. Compare against approved source art without redrawing it.
6. Create a *new derived file*, keeping the original source ZIP immutable. Archive derived bytes privately and independently verify the Drive copy by size, SHA-256 and ZIP/readback before discarding the temporary working copy.

The public GitHub workflow runs only `tests/test_svg_source_audit.py` with synthetic markup. It never fetches private Drive originals, artifacts, images, level screenshots, or extracted media.

## Known current blocker

The unapproved MAP01 GFX-01 draft panorama has a visible abrupt stream boundary near source `x=2960`. This is a visual issue beyond the structural SVG auditor; correcting it and approving the revised art require their own review. GFX-01 parallax geometry in `main` is *not* proof that the background has been integrated or approved.

Canonical Rikitu, Moki, Reina, and other selected character designs must not be regenerated, altered or replaced by this quality-control work.
