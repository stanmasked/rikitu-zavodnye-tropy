"""Test actual keyboard controls, without teleport helpers or artificial state."""
from pathlib import Path
import os
import struct
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[1]
HTML = (ROOT / "RIKITU_play_now.html").read_text(encoding="utf-8")

with sync_playwright() as pw:
    exe = os.environ.get("RIKITU_CHROMIUM_PATH")
    if not exe and Path("/usr/bin/chromium").exists():
        exe = "/usr/bin/chromium"
    browser = pw.chromium.launch(headless=True, executable_path=exe, args=["--no-sandbox"])
    page = browser.new_page(viewport={"width": 1200, "height": 760})
    errors = []
    page.on("pageerror", lambda err: errors.append(str(err)))
    page.set_content(HTML, wait_until="load")

    state = lambda: page.evaluate("RIKITU_TEST_STATE()")
    assert state()["mode"] == "title"
    page.keyboard.press("Enter")
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='playing'", timeout=5000)
    start = state()
    assert start["level"] == 1

    # Actual movement: keep the arrow key down, don't call RIKITU_TEST_MOVE.
    page.keyboard.down("ArrowRight")
    page.wait_for_function(
        "(x)=>RIKITU_TEST_STATE().playerX > x+65",
        arg=start["playerX"],
        timeout=5000,
    )
    page.keyboard.up("ArrowRight")
    moved = state()
    assert moved["playerX"] > start["playerX"] + 65, moved
    assert moved["level"] == 1

    # Save a real rendered frame (not a simulated UI) for visual inspection.
    images = ROOT / "artifacts" / "real-controls"
    images.mkdir(parents=True, exist_ok=True)
    moving_png = page.locator("#game").screenshot(path=str(images / "after-move.png"))
    assert moving_png.startswith(b"\x89PNG\r\n\x1a\n")
    width, height = struct.unpack(">II", moving_png[16:24])
    assert width >= 400 and height >= 220 and len(moving_png) > 3000
    # Jump from the ground using the real Space key.
    page.wait_for_timeout(130)
    ground_y = state()["playerY"]
    page.keyboard.down("Space")
    page.wait_for_function(
        "(y)=>RIKITU_TEST_STATE().playerY < y-25",
        arg=ground_y,
        timeout=2500,
    )
    page.keyboard.up("Space")
    assert state()["playerY"] < ground_y

    # Pause must freeze the in-game timer; unpause must resume it.
    page.keyboard.press("p")
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='paused'", timeout=5000)
    paused_at = state()["elapsedSeconds"]
    paused_png = page.locator("#game").screenshot(path=str(images / "paused.png"))
    assert paused_png.startswith(b"\x89PNG\r\n\x1a\n")
    assert paused_png != moving_png, "Pause overlay did not change the canvas image"
    page.wait_for_timeout(330)
    assert abs(state()["elapsedSeconds"] - paused_at) < 0.05, state()
    page.keyboard.press("p")
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='playing'", timeout=5000)
    page.wait_for_function(
        "(t)=>RIKITU_TEST_STATE().elapsedSeconds>t+0.12",
        arg=paused_at,
        timeout=5000,
    )

    # Restart must restore stage 1, position and three initial hearts.
    page.keyboard.press("r")
    page.wait_for_function(
        "()=>RIKITU_TEST_STATE().playerX<=110 && RIKITU_TEST_STATE().level===1",
        timeout=5000,
    )
    restarted = state()
    assert restarted["mode"] == "playing"
    assert restarted["maxHearts"] == 3
    assert restarted["stageRecords"] == []
    assert not errors, errors

    print("PASS real keyboard: move, jump, pause timer, resume, restart and 2 actual frame screenshots")
    browser.close()
