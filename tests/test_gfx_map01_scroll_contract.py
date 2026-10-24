"""RIKITU GFX-01: synthetic MAP01 panorama/source-crop contract.

No private approved character art, draft SVG, image rendering or asset
downloads are needed. This only tests that a proposed parallax transform
could cover the existing native MAP01 route at a 1280x720 viewport.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "native" / "rikitu_flower_data.h"
VIEW_W, VIEW_H = 1280, 720
BG_W, BG_H = 5120, 720
STRIP_W, STRIP_H, CELL_W, CELL_H, CELLS = 1280, 64, 64, 64, 20


def read_world_width():
    data = SOURCE.read_text(encoding="utf-8")
    m = re.search(r"^#define\s+RIKITU_FLOWER_WORLD_WIDTH\s+(\d+(?:\.\d+)?)f?\s*$", data, re.M)
    assert m, "MAP01 world-width macro missing"
    return float(m.group(1))


def map_scroll(camera_world, world_width):
    assert world_width > VIEW_W and BG_W >= VIEW_W
    camera_world = max(0.0, min(float(camera_world), world_width - VIEW_W))
    return camera_world * (BG_W - VIEW_W) / (world_width - VIEW_W)


def check_workflow_runner():
    """Keep every public GFX-01 synthetic contract on a pinned runner."""
    workflow_dir = ROOT / ".github" / "workflows"
    expected = {
        "stage148-background-blit.yml": ("timeout-minutes: 4", "native/test_background_blit.c"),
        "stage149-background-overlay.yml": ("timeout-minutes: 5", "native/test_background_overlay.c"),
        "stage150-map01-five-cameras.yml": ("timeout-minutes: 5", "native/test_map01_background_composite.c"),
        "gfx-map01-contract.yml": ("timeout-minutes: 3", "native/test_parallax_map01.c"),
    }
    for name, required in expected.items():
        workflow = (workflow_dir / name).read_text(encoding="utf-8")
        assert "runs-on: ubuntu-24.04" in workflow, f"{name}: runner must be pinned"
        assert "runs-on: ubuntu-latest" not in workflow, f"{name}: floating runner is forbidden"
        assert required[0] in workflow, f"{name}: bounded timeout missing"
        assert required[1] in workflow, f"{name}: required synthetic test missing"
    assert "python3 tests/test_gfx_map01_scroll_contract.py" in (
        workflow_dir / "gfx-map01-contract.yml").read_text(encoding="utf-8")
    print("PASS GFX-01 runner contract: four pinned workflows, bounded timeouts and synthetic test gates")

def main():
    check_workflow_runner()
    world_width = read_world_width()
    assert world_width == 7900.0, f"Re-review the draft panorama if MAP01 world width changes: {world_width}"
    assert BG_H == VIEW_H
    assert STRIP_W == CELLS * CELL_W and STRIP_H == CELL_H
    max_camera = world_width - VIEW_W
    assert abs(map_scroll(0, world_width)) < 1e-8
    assert abs(map_scroll(max_camera, world_width) - (BG_W - VIEW_W)) < 1e-7
    previous = -1
    for step in range(1001):
        camera = max_camera * step / 1000
        x = map_scroll(camera, world_width)
        assert 0 <= x <= BG_W - VIEW_W + 1e-7
        assert x >= previous
        assert x + VIEW_W <= BG_W + 1e-7
        previous = x
    print("PASS GFX-01 MAP01 synthetic panorama scroll: 7900 world to 5120 source with 1280 viewport")
    print("PASS tileset strip: 20 x 64 source cells; NO asset approval or renderer integration implied")


if __name__ == "__main__":
    main()
