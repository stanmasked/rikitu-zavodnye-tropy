"""Synthetic Gamepad API regression: D-pad, jump, run and Start pause.

This is a software control test, not human-playtest evidence.
"""
from pathlib import Path
import os
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[1]
HTML = (ROOT / "RIKITU_play_now.html").read_text(encoding="utf-8")
MARKER = "  const C=window.RikituCore;"
assert HTML.count(MARKER) == 1, "standalone game script marker changed"
HTML = HTML.replace(
    MARKER,
    "  Object.defineProperty(navigator,'getGamepads',{configurable:true,value:()=>window.__testGamepads||[]});\n" + MARKER,
    1,
)

with sync_playwright() as pw:
    exe = os.environ.get("RIKITU_CHROMIUM_PATH")
    if not exe and Path("/usr/bin/chromium").exists():
        exe = "/usr/bin/chromium"
    browser = pw.chromium.launch(headless=True, executable_path=exe, args=["--no-sandbox"])
    page = browser.new_page(viewport={"width": 1200, "height": 760})
    errors = []
    page.on("pageerror", lambda err: errors.append(str(err)))
    page.set_content(HTML, wait_until="load")
    page.evaluate("""() => {
      const buttons = Array.from({length: 17}, () => ({pressed: false, value: 0}));
      window.__testPad = {connected: true, axes: [0, 0], buttons, mapping: 'standard'};
      window.__testGamepads = [window.__testPad];
    }""")
    state = lambda: page.evaluate("RIKITU_TEST_STATE()")
    pad = lambda script: page.evaluate(script)

    assert state()["mode"] == "title"
    # Start must launch the game without keyboard or mouse assistance.
    pad("window.__testPad.buttons[9].pressed=true")
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='playing'", timeout=5000)
    pad("window.__testPad.buttons[9].pressed=false")
    page.wait_for_timeout(100)

    # Standard Gamepad button 15 is D-pad right. No teleport/test movement helper is used.
    before = state()["playerX"]
    pad("window.__testPad.buttons[15].pressed=true")
    page.wait_for_function("(x)=>RIKITU_TEST_STATE().playerX>x+30", arg=before, timeout=3000)
    dpad_distance = state()["playerX"] - before
    pad("window.__testPad.buttons[15].pressed=false")
    page.wait_for_timeout(100)

    # A button (index 0) must trigger a real jump through the normal update loop.
    ground_y = state()["playerY"]
    pad("window.__testPad.buttons[0].pressed=true")
    page.wait_for_function("(y)=>RIKITU_TEST_STATE().playerY<y-25", arg=ground_y, timeout=2500)
    pad("window.__testPad.buttons[0].pressed=false")
    page.wait_for_function("(y)=>RIKITU_TEST_STATE().playerY>=y-1", arg=ground_y, timeout=3000)

    # Compare equal-duration analog-stick movement with and without RB/R1 (button 5).
    walk_start = state()["playerX"]
    pad("window.__testPad.axes[0]=1")
    page.wait_for_timeout(500)
    walk_distance = state()["playerX"] - walk_start
    pad("window.__testPad.axes[0]=0")
    page.wait_for_timeout(100)

    run_start = state()["playerX"]
    pad("window.__testPad.axes[0]=1;window.__testPad.buttons[5].pressed=true")
    page.wait_for_timeout(500)
    run_distance = state()["playerX"] - run_start
    pad("window.__testPad.axes[0]=0;window.__testPad.buttons[5].pressed=false")
    assert run_distance > walk_distance + 8, {
        "walk_distance": walk_distance, "run_distance": run_distance
    }

    # Start (button 9) toggles pause and freezes active gameplay time.
    pad("window.__testPad.buttons[9].pressed=true")
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='paused'", timeout=3000)
    paused_at = state()["elapsedSeconds"]
    page.wait_for_timeout(250)
    assert abs(state()["elapsedSeconds"] - paused_at) < 0.05
    pad("window.__testPad.buttons[9].pressed=false")
    page.wait_for_timeout(100)
    pad("window.__testPad.buttons[9].pressed=true")
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='playing'", timeout=3000)
    pad("window.__testPad.buttons[9].pressed=false")

    assert dpad_distance > 30
    assert walk_distance > 70
    assert not errors, errors
    print("PASS synthetic gamepad: D-pad, A jump, analog walk, RB/R1 run and Start pause")
    browser.close()
