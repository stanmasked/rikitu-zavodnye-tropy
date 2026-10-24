"""Public regression for the GFX-01 art approval gate.

This checks only public source structure and synthetic renderer contracts.
It never downloads, decodes, embeds or approves private GFX-01 art.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    draw = (ROOT / "native" / "rikitu_draw.c").read_text(encoding="utf-8")
    draw_header = (ROOT / "native" / "rikitu_draw.h").read_text(encoding="utf-8")
    blit = (ROOT / "native" / "rikitu_background_blit.c").read_text(encoding="utf-8")
    blit_header = (ROOT / "native" / "rikitu_background_blit.h").read_text(encoding="utf-8")
    pipeline = (ROOT / "docs" / "GRAPHICS_PIPELINE.md").read_text(encoding="utf-8")
    readme = (ROOT / "assets" / "graphics" / "world-01" / "README.md").read_text(
        encoding="utf-8"
    )

    # The production/default scene remains the legacy synthetic debug renderer.
    assert (
        "return draw_scene_impl(pixels,width,height,stride_pixels,s,0);" in draw
    ), "default scene must retain the legacy debug background"
    assert (
        "return draw_scene_impl(pixels,width,height,stride_pixels,s,1);" in draw
    ), "background compositing must remain an explicit caller-owned opt-in"
    assert "rikitu_background_blit" not in draw
    assert "rikitu_background_blit" not in draw_header

    # The framebuffer copier must stay asset-agnostic: no filesystem or decoder.
    for marker in (
        "fopen(",
        "fread(",
        "stbi_load",
        "flower-trail-background.svg",
        "tileset-world01.svg",
    ):
        assert marker not in blit, f"draft-art loading marker entered blit module: {marker}"
    assert "no file io" in blit_header.lower()

    # Private draft sources must remain outside the public checkout.
    draft_paths = (
        ROOT / "assets" / "graphics" / "world-01" / "flower-trail-background.svg",
        ROOT / "assets" / "graphics" / "world-01" / "tileset-world01.svg",
    )
    assert all(not path.exists() for path in draft_paths), (
        "unapproved GFX-01 source assets must remain in private Drive"
    )

    assert "no file loader is activated" in pipeline.lower()
    assert "no gfx-01 source/raster pixels are compiled into the binary" in pipeline.lower()
    assert "not approved final art and not integrated into the game" in readme.lower()
    assert "preserve the exact private archive unchanged" in readme.lower()

    print(
        "PASS GFX-01 approval gate: private draft absent, default debug renderer "
        "unchanged, and no asset loader enabled"
    )


if __name__ == "__main__":
    main()
