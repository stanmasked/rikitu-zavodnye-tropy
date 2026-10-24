#!/usr/bin/env python3
"""RIKITU Stage145: public Actions artifacts must be transient, safe handoffs.

Scans ALL public repository workflows; only seven allow explicit handoffs.
no access to private Drive, original art, captures, or game resources.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORKFLOWS = ROOT / ".github" / "workflows"

# (expected NRO handoffs, expected corresponding downloads)
EXPECTED = {
    "switch-graphics-emulator.yml": (4, 4),
    "stage106-staged-rename.yml": (1, 1),
    "stage125-staged-codecs.yml": (1, 1),
    "stage126-staged-faults.yml": (1, 1),
    "stage131-production-pair.yml": (1, 1),
    "switch-graphics.yml": (0, 0),
    "switch-nro.yml": (0, 0),
}


def upload_blocks(yaml_text: str):
    lines = yaml_text.splitlines()
    for i, line in enumerate(lines):
        if "uses: actions/upload-artifact@v4" not in line:
            continue
        indentation = len(line) - len(line.lstrip(" "))
        assert line.strip() in {
            "uses: actions/upload-artifact@v4",
            "- uses: actions/upload-artifact@v4",
        }
        end = i + 1
        while end < len(lines):
            line_next = lines[end]
            if line_next.strip():
                leading = len(line_next) - len(line_next.lstrip(" "))
                if leading < indentation or (
                    leading == indentation and line_next.lstrip().startswith("- ")
                ):
                    break
            end += 1
        yield "\n".join(lines[i:end])


def check_policy_workflow():
    """Keep both primary and recovery audits manually recoverable."""
    for name in ("ci-storage-policy.yml", "stage180-storage-policy-recovery.yml"):
        policy = (WORKFLOWS / name).read_text(encoding="utf-8")
        assert "workflow_dispatch:" in policy, name
        assert "permissions:\n  contents: read" in policy, name
        assert "pull_request:" in policy and "push:" in policy, name
        assert "      - '.github/workflows/**'" in policy, name
        assert "      - 'tests/test_ci_artifact_policy.py'" in policy, name
        assert "timeout-minutes: 3" in policy, name
        assert "runs-on: ubuntu-24.04" in policy, name
        assert "run: python3 tests/test_ci_artifact_policy.py" in policy, name
    print("PASS Stage180: primary and recovery storage audits share guarded triggers")


def main():
    check_policy_workflow()
    total = 0
    seen = set()
    for workflow in sorted(WORKFLOWS.iterdir()):
        if workflow.suffix not in {".yml", ".yaml"}:
            continue
        name = workflow.name
        data = workflow.read_text(encoding="utf-8")
        all_uploads = data.count("uses: actions/upload-artifact@")
        if name not in EXPECTED:
            assert all_uploads == 0, f"Unexpected artifact publishing workflow: {name}"
            continue
        seen.add(name)
        uploads, downloads = EXPECTED[name]
        assert all_uploads == uploads, (name, all_uploads, uploads)
        blocks = list(upload_blocks(data))
        assert len(blocks) == uploads, (name, len(blocks), uploads)
        actual_downloads = data.count("uses: actions/download-artifact@v4")
        assert actual_downloads == downloads, (name, actual_downloads, downloads)
        for block in blocks:
            assert "retention-days: 1" in block, name
            assert "if-no-files-found: error" in block, name
            path_lines = [
                ln.strip() for ln in block.splitlines()
                if ln.strip().startswith("path:")
            ]
            assert len(path_lines) == 1 and path_lines[0].endswith(".nro"), (name, block)
            assert "evidence/" not in block, name
            assert "artifacts/" not in block, name
        total += len(blocks)
        print(f"PASS {name}: {uploads} one-day NRO handoffs, {downloads} downloads")
    assert seen == set(EXPECTED), (seen, set(EXPECTED))
    assert total == 8, total
    print("PASS Stage145: no public raw evidence or standalone NRO uploads")


if __name__ == "__main__":
    main()
