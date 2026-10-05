"""Planning-only points in a yaw-Z then pitch-Y mechanism; no actuator commands."""

import math


def rotate(point, origin, axis, degrees):
    if axis not in ("Y", "Z"):
        raise ValueError("only the declared Y and Z axes are supported")
    if (
        len(point) != 3
        or len(origin) != 3
        or not all(
            type(v) in (int, float) and math.isfinite(v)
            for v in (*point, *origin, degrees)
        )
    ):
        raise ValueError("finite three-dimensional coordinates and angle required")
    x, y, z = (a - b for a, b in zip(point, origin))
    c, s = math.cos(math.radians(degrees)), math.sin(math.radians(degrees))
    value = (
        (c * x - s * y, s * x + c * y, z)
        if axis == "Z"
        else (c * x + s * z, y, -s * x + c * z)
    )
    return [a + b for a, b in zip(value, origin)]


def camera_envelope_centre(point, params, yaw, pitch):
    # Pitch is relative to the yaw carrier, so rotate locally before world yaw.
    pitched = rotate(point, params["pitch_axis_origin_mm"], "Y", pitch)
    return rotate(pitched, params["yaw_axis_origin_mm"], "Z", yaw)
