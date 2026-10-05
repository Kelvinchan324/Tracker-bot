"""Lightweight freshness ledger checks, not CAD validation or physical evidence."""

import hashlib
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class VisualReviewTest(unittest.TestCase):
    def test_review_sources_and_image_are_current(self):
        report = json.loads((ROOT / "mechanical/carrier-visual-review.json").read_text())
        self.assertIs(report["physical_validation"], False)
        self.assertIs(report["assembly_body_match"], True)
        self.assertEqual(report["rendered_bodies"], 11)
        self.assertEqual(len(report["parts"]), 11)
        self.assertEqual(len({p["name"] for p in report["parts"]}), 11)
        expected = {
            "mechanical/carrier-fixture-plate.step", "mechanical/yaw-seat-draft.step",
            "mechanical/yaw-carrier-draft.step", "mechanical/pitch-carrier-draft.step",
            "mechanical/camera-saddle-draft.step", "mechanical/carrier-layout.step",
            "mechanical/carrier-parameters.json", "hardware/rev-a.json",
            "tools/render_carrier_review.py",
        }
        self.assertEqual(set(report["sources_sha256"]), expected)
        for relative, digest in report["sources_sha256"].items():
            with self.subTest(source=relative):
                data = (ROOT / relative).read_bytes()
                if Path(relative).suffix in {".json", ".py"}:
                    data = data.replace(b"\r\n", b"\n")
                self.assertEqual(hashlib.sha256(data).hexdigest(), digest,
                                 "Stale visual review: rerun renderer and inspect the result")
        image = (ROOT / "mechanical/carrier-visual-review.png").read_bytes()
        self.assertEqual(hashlib.sha256(image).hexdigest(), report["render_sha256"])
        self.assertEqual(image[:8], b"\x89PNG\r\n\x1a\n")
        self.assertEqual(int.from_bytes(image[16:20], "big"), 2400)
        self.assertEqual(int.from_bytes(image[20:24], "big"), 1350)


if __name__ == "__main__":
    unittest.main()
