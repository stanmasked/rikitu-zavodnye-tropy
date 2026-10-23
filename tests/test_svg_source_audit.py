"""Synthetic SVG-only regression: never loads private character or background art."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_svg_source import audit_svg, check_path  # noqa: E402


def doc(body):
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="1280" height="720"'
            ' viewBox="0 0 1280 720">' + body + '</svg>').encode()


class SvgAuditTest(unittest.TestCase):
    def test_valid_private_safe_synthetic(self):
        s, issues = audit_svg(doc('<defs><linearGradient id="sky"/></defs>'
                                  '<g><path id="ridge" d="M0 20 L20 30 40 21 Z"'
                                  ' fill="url(#sky)"/>'
                                  '<use href="#ridge" x="80"/></g>'))
        self.assertEqual(s["uses"], 1)
        self.assertEqual(issues, [])

    def test_detects_duplicate_ids_and_broken_refs(self):
        _, issues = audit_svg(doc('<g id="same"/><path id="same" d="M0 0 L10 10"'
                                  ' fill="url(#gone)"/><use href="#missing"/>'))
        codes = [x[1] for x in issues]
        self.assertIn("duplicate-id", codes)
        self.assertIn("broken-url-reference", codes)
        self.assertIn("broken-href", codes)

    def test_incomplete_geometry_and_script(self):
        _, issues = audit_svg(doc('<path d="M 0 0 L 10"/>'
                                  '<script>bad</script><use href="https://invalid.invalid/a"/>'))
        codes = [x[1] for x in issues]
        self.assertIn("incomplete-path-command", codes)
        self.assertIn("executable-or-foreign-element", codes)
        self.assertIn("external-href", codes)

    def test_identical_use_warns_but_is_not_fatal(self):
        _, issues = audit_svg(doc('<path id="a" d="M0 0 L1 1"/>'
                                  '<g><use href="#a" x="3"/><use href="#a" x="3"/></g>'))
        self.assertEqual(issues, [("warning", "identical-use-in-same-layer")])

    def test_dtd_malformed_and_wrong_namespace(self):
        self.assertEqual(audit_svg(b'<!DOCTYPE svg><svg/>')[1][0][1],
                         "xml-dtd-or-entity-forbidden")
        self.assertEqual(audit_svg(b'<svg')[1][0][1], "malformed-svg-xml")
        self.assertEqual(audit_svg(b'<svg/>')[1][0][1], "wrong-root-namespace")

    def test_root_event_and_external_css_url(self):
        _, issues = audit_svg(b'<svg xmlns="http://www.w3.org/2000/svg" '
                              b'viewBox="0 0 1280 720" onload="alert(1)">'
                              b'<path d="M0 0 L1 1" '
                              b'fill="url(https://example.invalid/external)"/>'
                              b'</svg>')
        self.assertIn(("error", "inline-event-handler"), issues)
        self.assertIn(("error", "external-url-reference"), issues)

    def test_missing_or_zero_extents(self):
        _, issues = audit_svg(doc('<rect width="5"/>'
                                  '<circle r="0"/>'
                                  '<ellipse rx="1" ry="2"/>'))
        self.assertIn(("error", "shape-missing-extent"), issues)
        self.assertIn(("error", "invalid-shape-extent"), issues)

    def test_external_entities_and_oversized_input_rejected(self):
        self.assertIn(("error", "xml-dtd-or-entity-forbidden"),
                      audit_svg(b'<!ENTITY x "y"><svg/>')[1])
        self.assertIn(("error", "oversized-svg"),
                      audit_svg(b'x' * (20 * 1024 * 1024 + 1))[1])

    def test_detects_interior_vertical_bottom_cliff(self):
        # Synthetic analogue of a fill abruptly ending inside a panorama.
        _, issues = audit_svg(doc('<path d="M200 540 T620 560 L620 720 H200Z"'
                                  ' fill="#5eb3be"/>'))
        self.assertEqual(issues, [("warning", "interior-bottom-vertical-fill-edge")])

    def test_does_not_flag_normal_canvas_edge_closure(self):
        _, issues = audit_svg(doc('<path d="M200 540 T1280 560 L1280 720 H200Z"'
                                  ' fill="#5eb3be"/>'))
        self.assertEqual(issues, [])

    def test_does_not_flag_nonfilled_or_small_vertical_edges(self):
        for attr, path in (
            ('fill="none"', "M200 540 T620 560 L620 720 H200Z"),
            ('fill="#77c"', "M200 690 T620 703 L620 720 H200Z"),
            ('fill="#77c"', "M200 540 T620 560 L621 720 H200Z"),
        ):
            with self.subTest(attr=attr, path=path):
                _, issues = audit_svg(doc(f'<path d="{path}" {attr}/>'))
                self.assertNotIn(
                    ("warning", "interior-bottom-vertical-fill-edge"), issues)

    def test_flags_tall_interior_left_closing_bank(self):
        # Synthetic-only analogue of a filled stream beginning vertically.
        _, issues = audit_svg(doc('<path d="M200 540 Q350 530 500 560'
                                  ' L660 720 H180Z" fill="#69b9c7"/>'))
        self.assertEqual(
            issues, [("warning", "interior-start-near-vertical-fill-edge")])

    def test_left_bank_taper_no_longer_warns(self):
        _, issues = audit_svg(doc('<path d="M130 718 Q170 640 200 540'
                                  ' Q350 530 500 560 L660 720 H130Z"'
                                  ' fill="#69b9c7"/>'))
        self.assertEqual(issues, [])

    def test_left_edge_warning_ignores_open_canvas_border(self):
        _, issues = audit_svg(doc('<path d="M0 530 Q300 520 500 540'
                                  ' L660 720 H0Z" fill="#69b9c7"/>'))
        self.assertEqual(issues, [])

    def test_left_bank_warning_requires_fill_and_sharp_slope(self):
        cases = (
            ('fill="none"', "M200 540 Q350 530 500 560 L660 720 H180Z"),
            ('fill="#69b9c7"', "M200 690 Q350 690 500 699 L660 720 H180Z"),
            ('fill="#69b9c7"', "M260 540 Q350 530 500 560 L660 720 H180Z"),
        )
        for style, path in cases:
            with self.subTest(style=style, path=path):
                _, issues = audit_svg(doc(f'<path d="{path}" {style}/>'))
                self.assertNotIn(
                    ("warning", "interior-start-near-vertical-fill-edge"),
                    issues)

    def test_path_arities_and_exponent(self):
        self.assertIsNone(check_path("M1e2,2 L3.0,-4.5 z"))
        self.assertIsNone(check_path("M0 0 C1 2 3 4 5 6 Z"))
        self.assertEqual(check_path("M0 0 Q1 2 3"), "incomplete-path-command")
        self.assertEqual(check_path("M0 0 $ 3"), "invalid-path-token")
        self.assertEqual(check_path("M0 0"), "move-only-path")


if __name__ == "__main__":
    unittest.main()
