#!/usr/bin/env python3
"""Audit stage geometry and theoretical straight-run reference times.

IMPORTANT: This is NOT a gameplay-duration test. It never claims that a stage
can be cleared in the reported seconds. Real runs include jumping, collectible
detours, hazards, boss fights, retries and route exploration.
"""
import argparse
import json
import os
from pathlib import Path
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[1]


def collect():
    html = (ROOT / "RIKITU_play_now.html").read_text(encoding="utf-8")
    binary = os.environ.get("RIKITU_CHROMIUM_PATH")
    if not binary and Path("/usr/bin/chromium").exists():
        binary = "/usr/bin/chromium"

    with sync_playwright() as pw:
        browser = pw.chromium.launch(
            headless=True, executable_path=binary, args=["--no-sandbox"]
        )
        page = browser.new_page(viewport={"width": 1200, "height": 760})
        page_errors = []
        page.on("pageerror", lambda err: page_errors.append(str(err)))
        page.set_content(html, wait_until="load")
        levels = page.evaluate("RIKITU_TEST_LEVELS()")
        speed = page.evaluate("RikituCore.SETTINGS.runSpeed")
        browser.close()

    assert not page_errors, f"Browser script errors: {page_errors}"
    assert isinstance(speed, (int, float)) and speed > 0
    assert len(levels) > 0

    records = []
    for i, d in enumerate(levels, start=1):
        expected_boss = i % 2 == 0
        assert len(d["fragments"]) == 3, f"Stage {i}: fragment count"
        assert d["coins"] == 90, f"Stage {i}: coin budget"
        assert len(d["checkpoints"]) == 3, f"Stage {i}: checkpoint count"
        assert len(d["sections"]) == 4, f"Stage {i}: section count"
        assert bool(d["boss"]) == expected_boss, f"Stage {i}: boss placement"
        assert 105 < d["gate"] <= d["width"], f"Stage {i}: invalid exit"
        assert all(0 <= part[0] < d["gate"] for part in d["fragments"]), (
            f"Stage {i}: fragment outside exit"
        )
        thresholds = [section[0] for section in d["sections"]]
        assert thresholds[0] == 0 and thresholds == sorted(thresholds), (
            f"Stage {i}: section order"
        )

        # A distance/speed *reference*, not a proven lower bound:
        # conveyors and jumps can speed traversal; obstacles and searches delay it.
        nominal = round((d["gate"] - 105) / speed, 1)
        target = (480, 600) if expected_boss else (360, 480)
        records.append({
            "stage": i,
            "world": (i - 1) // 4 + 1,
            "width_px": d["width"],
            "exit_px": d["gate"],
            "boss": expected_boss,
            "sections": [item[1] for item in d["sections"]],
            "fragment_count": 3,
            "coin_count": 90,
            "checkpoint_count": 3,
            "nominal_straight_run_seconds": nominal,
            "target_play_seconds": {"min": target[0], "max": target[1]},
            "target_to_nominal_ratio": round(target[0] / nominal, 1),
        })
    return {
        "version": 1,
        "description": "Geometry-only reference, NOT observed gameplay duration",
        "run_speed_px_per_second": speed,
        "stages": records,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="Write JSON report")
    args = parser.parse_args()
    report = collect()
    print("RIKITU PACING / GEOMETRY AUDIT")
    print("NOT A GAMEPLAY TIME MEASUREMENT")
    for r in report["stages"]:
        label = "boss" if r["boss"] else "normal"
        t = r["target_play_seconds"]
        print(
            f"Stage {r['stage']:02} | {label:6} | {r['width_px']:5} px"
            f" | nominal straight run {r['nominal_straight_run_seconds']:5.1f}s"
            f" | target {t['min']//60}-{t['max']//60}m"
        )
    print(
        "To validate the target, complete each stage with ordinary controller "
        "input and review the saved stage-results journal."
    )
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(
            json.dumps(report, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )
        print(f"Wrote {args.output}")


if __name__ == "__main__":
    main()
