import importlib.util
import math
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "frame_math", ROOT / "tools/frame_math.py"
)
frames = importlib.util.module_from_spec(spec)
spec.loader.exec_module(frames)


class FramesTest(unittest.TestCase):
    def test_right_hand_axes(self):
        for actual, expected in zip(
            frames.rotate([1, 0, 0], [0, 0, 0], "Z", 90), [0, 1, 0]
        ):
            self.assertAlmostEqual(actual, expected)
        for actual, expected in zip(
            frames.rotate([1, 0, 0], [0, 0, 0], "Y", 90), [0, 0, -1]
        ):
            self.assertAlmostEqual(actual, expected)

    def test_pivot_and_distance_preserved(self):
        origin = [3, -5, 7]
        for axis in ("Y", "Z"):
            for angle in (-30, 0, 20):
                self.assertEqual(frames.rotate(origin, origin, axis, angle), origin)
                result = frames.rotate([10, 4, -3], origin, axis, angle)
                self.assertAlmostEqual(
                    math.dist(result, origin), math.dist([10, 4, -3], origin)
                )

    def test_pitch_then_yaw_not_reverse(self):
        params = {"pitch_axis_origin_mm": [0, 0, 0], "yaw_axis_origin_mm": [0, 0, 0]}
        result = frames.camera_envelope_centre([0, 0, 1], params, 90, 90)
        for actual, expected in zip(result, [0, 1, 0]):
            self.assertAlmostEqual(actual, expected)

    def test_invalid_input(self):
        for value in (float("nan"), float("inf"), True, "10"):
            with self.assertRaises(ValueError):
                frames.rotate([value, 0, 0], [0, 0, 0], "Y", 1)
        with self.assertRaises(ValueError):
            frames.rotate([0, 0, 0], [0, 0, 0], "X", 1)


if __name__ == "__main__":
    unittest.main()
