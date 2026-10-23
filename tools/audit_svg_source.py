#!/usr/bin/env python3
"""RIKITU Stage146: private SVG source structural audit (no asset uploads).

Run this locally against an exact-byte private source. Public CI executes only
the synthetic self-tests in tests/test_svg_source_audit.py.
"""
import argparse
import collections
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

NUMBER = r"[-+]?(?:(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?)"
TOKENS = re.compile(rf"[MmZzLlHhVvCcSsQqTtAa]|{NUMBER}")
COMMANDS = set("MmZzLlHhVvCcSsQqTtAa")
ARITY = {"M": 2, "L": 2, "H": 1, "V": 1, "C": 6, "S": 4,
         "Q": 4, "T": 2, "A": 7, "Z": 0}
URL_REF = re.compile(r"url\(\s*['\"]?#([A-Za-z_][\w:.-]*)['\"]?\s*\)")
BANNED_XML = re.compile(br"<!\s*(?:DOCTYPE|ENTITY)\b", re.I)
SVG_NS = "{http://www.w3.org/2000/svg}"


def local_name(name):
    return name.rsplit("}", 1)[-1]


def check_path(d):
    """Return None if path command arities and token stream are valid."""
    if not d.strip():
        return "empty-path"
    found = list(TOKENS.finditer(d))
    if not found or d[:found[0].start()].strip(" \t\r\n,"):
        return "invalid-path-token"
    last = 0
    for m in found:
        if d[last:m.start()].strip(" \t\r\n,"):
            return "invalid-path-token"
        last = m.end()
    if d[last:].strip(" \t\r\n,"):
        return "invalid-path-token"
    tokens = [m.group(0) for m in found]
    if tokens[0] not in "Mm":
        return "path-must-start-with-move"
    i = 0
    draws = False
    while i < len(tokens):
        cmd = tokens[i]
        if cmd not in COMMANDS:
            return "invalid-path-command"
        i += 1
        j = i
        while j < len(tokens) and tokens[j] not in COMMANDS:
            j += 1
        count = j - i
        size = ARITY[cmd.upper()]
        if size == 0 and count:
            return "closepath-has-arguments"
        if size and (count == 0 or count % size):
            return "incomplete-path-command"
        if size and cmd.upper() != "M":
            draws = True
        if cmd.upper() == "M" and count > 2:
            draws = True  # additional move pairs imply line segments
        if cmd.upper() == "Z":
            draws = True
        i = j
    if not draws:
        return "move-only-path"
    return None


# This intentionally flags only a strong structural hint, not a semantic error.
# It uses synthetic-only public fixtures; private SVG data is never included.
ABSOLUTE_VERTICAL_CLOSURE = re.compile(
    rf"[TL]\s*({NUMBER})[\s,]+({NUMBER})\s*"
    rf"L\s*({NUMBER})[\s,]+({NUMBER})\s*"
    rf"H\s*({NUMBER})\s*[zZ]\s*$"
)


def has_interior_bottom_cliff(path_data, view_box):
    """Detect a long abrupt polygon closure to the bottom INSIDE the image.

    Warning only: a vertical internal cliff may be intentional architecture.
    Relative commands or arbitrary curves are out of scope; never auto-fix.
    """
    if view_box is None:
        return False
    left, top, width, height = view_box
    match = ABSOLUTE_VERTICAL_CLOSURE.search(path_data)
    if not match:
        return False
    x_before, y_before, x_end, y_end, _ = map(float, match.groups())
    margin = max(12.0, width * 0.003)
    bottom = top + height
    return (
        left + margin < x_end < left + width - margin
        and abs(x_before - x_end) <= 0.5
        and abs(y_end - bottom) <= 1.0
        and y_end - y_before >= max(24.0, height * 0.04)
    )



# Complementary warning for near-vertical *opening* edges. A path may begin at
# the stream surface, terminate along the bottom edge and close straight back
# up to its initial M point. The drawing is valid SVG but can look cut off.
ABSOLUTE_START_MOVE = re.compile(
    rf"^\s*M\s*({NUMBER})[\s,]+({NUMBER})"
)
ABSOLUTE_BOTTOM_CLOSE = re.compile(
    rf"L\s*({NUMBER})[\s,]+({NUMBER})\s*H\s*({NUMBER})\s*[zZ]\s*$"
)


def has_interior_opening_cliff(path_data, view_box):
    """Warn about a tall near-vertical *first* fill edge inside the canvas.

    The heuristic recognizes a restricted absolute M...L...H...Z form only.
    It does not prove a visual defect; architecture can have vertical edges.
    """
    if view_box is None:
        return False
    start = ABSOLUTE_START_MOVE.search(path_data)
    end = ABSOLUTE_BOTTOM_CLOSE.search(path_data)
    if not start or not end:
        return False
    start_x, start_y = map(float, start.groups())
    _, bottom_y, closure_x = map(float, end.groups())
    left, top, width, height = view_box
    margin = max(12.0, width * 0.003)
    return (
        left + margin < start_x < left + width - margin
        and abs(start_x - closure_x) <= max(24.0, height * 0.035)
        and abs(bottom_y - (top + height)) <= 1.0
        and bottom_y - start_y >= max(64.0, height * 0.12)
    )

def audit_svg(raw):
    """Return (summary, list of (severity, code)) without leaking private art."""
    issues = []
    def err(code):
        issues.append(("error", code))
    def warn(code):
        issues.append(("warning", code))

    if len(raw) > 20 * 1024 * 1024:
        return {}, [("error", "oversized-svg")]
    if BANNED_XML.search(raw):
        return {}, [("error", "xml-dtd-or-entity-forbidden")]
    try:
        root = ET.fromstring(raw)
    except ET.ParseError:
        return {}, [("error", "malformed-svg-xml")]
    if root.tag != SVG_NS + "svg":
        err("wrong-root-namespace")
    vb = root.get("viewBox", "").replace(",", " ").split()
    view_box = None
    if len(vb) != 4:
        err("missing-or-invalid-viewbox")
    else:
        try:
            vals = [float(x) for x in vb]
            if not all(map(__import__("math").isfinite, vals)) or vals[2] <= 0 or vals[3] <= 0:
                err("invalid-viewbox-extent")
            else:
                view_box = vals
        except ValueError:
            err("invalid-viewbox-extent")
    elements = list(root.iter())
    ids = [x.get("id") for x in elements if x.get("id")]
    duplicate_count = sum(n - 1 for n in collections.Counter(ids).values() if n > 1)
    for _ in range(duplicate_count):
        err("duplicate-id")
    id_set = set(ids)
    uses_by_parent = collections.defaultdict(set)

    parent_by_node = {id(child): parent for parent in elements for child in list(parent)}
    for node in elements:
        tag = local_name(node.tag)
        if tag in {"script", "foreignObject"}:
            err("executable-or-foreign-element")
        for key, value in node.attrib.items():
            key_local = local_name(key)
            if key_local.lower().startswith("on"):
                err("inline-event-handler")
            for ref in URL_REF.findall(value):
                if ref not in id_set:
                    err("broken-url-reference")
            if re.search(r"""url\(\s*['"]?(?:https?:|data:|file:|//)""", value, re.I):
                err("external-url-reference")
            if key_local == "href":
                if not value.startswith("#"):
                    err("external-href")
                elif value[1:] not in id_set:
                    err("broken-href")
        if tag == "path":
            if node.get("fill", "").strip().lower() != "none":
                d = node.get("d", "")
                if has_interior_bottom_cliff(d, view_box):
                    warn("interior-bottom-vertical-fill-edge")
                if has_interior_opening_cliff(d, view_box):
                    warn("interior-start-near-vertical-fill-edge")
            issue = check_path(node.get("d", ""))
            if issue == "move-only-path":
                warn(issue)
            elif issue:
                err(issue)
        if tag in {"rect", "circle", "ellipse"}:
            keys = {"rect": ("width", "height"), "circle": ("r",),
                    "ellipse": ("rx", "ry")}[tag]
            for key in keys:
                raw_value = node.get(key)
                if raw_value is None:
                    err("shape-missing-extent")
                    continue
                try:
                    size = float(raw_value.strip().removesuffix("px"))
                    if not __import__("math").isfinite(size) or size <= 0:
                        err("invalid-shape-extent")
                except ValueError:
                    warn("nonpixel-shape-extent-review")
        if tag == "use":
            href = node.get("href") or node.get("{http://www.w3.org/1999/xlink}href", "")
            signature = (href, node.get("x", "0"), node.get("y", "0"),
                         node.get("transform", ""))
            parent = parent_by_node.get(id(node))
            if signature in uses_by_parent[id(parent)]:
                warn("identical-use-in-same-layer")
            uses_by_parent[id(parent)].add(signature)
    summary = {"elements": len(elements), "ids": len(ids),
               "paths": sum(local_name(x.tag) == "path" for x in elements),
               "uses": sum(local_name(x.tag) == "use" for x in elements)}
    return summary, issues


def main():
    p = argparse.ArgumentParser(description="Audit private RIKITU SVGs locally")
    p.add_argument("source", type=Path, help="private local source; never CI upload")
    a = p.parse_args()
    summary, issues = audit_svg(a.source.read_bytes())
    print("RIKITU SVG audit:", summary)
    for severity, code in issues:
        print(severity.upper() + ": " + code)
    print("RESULT: ", "FAIL" if any(s == "error" for s, _ in issues) else "PASS")
    return int(any(s == "error" for s, _ in issues))


if __name__ == "__main__":
    sys.exit(main())
