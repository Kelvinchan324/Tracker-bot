"""Candidate load-path geometry and sampled intrusion review, not hardware approval."""

import json
import math
import re
from pathlib import Path

import cadquery as cq
from frame_math import camera_envelope_centre, rotate

ROOT = Path(__file__).resolve().parents[1]


def box(size, centre):
    return cq.Workplane("XY").box(*size).translate(tuple(centre))


def overlap(a, b):
    shape = a.intersect(b).val()
    return 0 if shape is None else shape.Volume()


def turn(part, origin, axis, degrees):
    end = [origin[i] + axis[i] for i in range(3)]
    return part.rotate(tuple(origin), tuple(end), degrees)


def build():
    manifest = json.loads((ROOT / "hardware/rev-a.json").read_text())
    p = json.loads((ROOT / "mechanical/carrier-parameters.json").read_text())
    for key, value in p.items():
        if key in {"status", "unresolved", "camera_optical_frame"}:
            continue
        values = value if isinstance(value, list) else [value]
        if not values or not all(
            type(v) in (int, float) and math.isfinite(v) for v in values
        ):
            raise ValueError("invalid finite geometry parameter: " + key)
    config = (ROOT / "firmware/include/app_config.h").read_text()
    command_ranges = {}
    for axis in ("Yaw", "Pitch"):
        values = []
        for field in ("Min", "Center", "Max"):
            match = re.search(r"k" + axis + field + r"Deg\s*=\s*([\d.]+)F", config)
            if not match:
                raise ValueError("cannot read firmware command bounds")
            values.append(float(match.group(1)))
        low, centre, high = values
        samples = p["sample_" + axis.lower() + "_deg"]
        if (
            min(samples) != low - centre
            or max(samples) != high - centre
            or 0 not in samples
        ):
            raise ValueError(
                "samples must include centre and current firmware-range endpoints"
            )
        command_ranges[axis.lower()] = values
    components = {c["ref"]: c for c in manifest["components"] if "position" in c}
    envelopes = {ref: box(c["size"], c["position"]) for ref, c in components.items()}
    if overlap(envelopes["M2"], envelopes["M2"]) <= 1:
        raise ValueError("intrusion detector positive control failed")
    m2 = components["M2"]
    if (
        m2["position"][:2] != [0, 0]
        or p["yaw_axis_direction"] != [0, 0, 1]
        or p["pitch_axis_direction"] != [0, 1, 0]
    ):
        raise ValueError("this candidate is limited to the declared centred Z/Y layout")
    sx, sy, sz = m2["size"]
    floor = m2["position"][2] - sz / 2
    wall, base, clear = p["wall"], p["seat_base"], p["seat_clearance"]
    deck = p["yaw_deck_bottom_z"]
    iw, idepth = sx + 2 * clear, sy + 2 * clear
    ow, od = iw + 2 * wall, idepth + 2 * wall
    if not (
        2 <= wall <= 5
        and 2 <= base <= 5
        and 0.3 <= clear <= 2
        and floor - base > deck + wall
    ):
        raise ValueError("invalid carrier wall, clearance or stack")
    yaw = (
        cq.Workplane("XY")
        .circle(p["yaw_deck_radius"])
        .extrude(wall)
        .translate((0, 0, deck))
    )
    # Undrilled horn land: hole pattern cannot be inferred from a servo body box.
    yaw = yaw.union(box((ow, od, base), (0, 0, floor - base / 2)))
    for x in (-p["riser_x"], p["riser_x"]):
        yaw = yaw.union(
            box(
                (wall, p["riser_depth"], floor - base - deck - wall),
                (x, 0, (floor - base + deck + wall) / 2),
            )
        )
    seat = box(
        (ow, od, p["seat_wall_height"]), (0, 0, floor + p["seat_wall_height"] / 2)
    )
    seat = seat.cut(
        box(
            (iw, idepth, p["seat_wall_height"] + 2),
            (0, 0, floor + p["seat_wall_height"] / 2 + 1),
        )
    )
    # Open shaft-side (+Y) wall to leave horn-space review possible.
    seat = seat.cut(
        box(
            (iw, wall + 2, p["seat_wall_height"] + 2),
            (0, od / 2, floor + p["seat_wall_height"] / 2),
        )
    )
    yaw = yaw.union(seat)
    camera_mount = cq.importers.importStep(
        str(ROOT / "mechanical/camera-saddle-draft.step")
    )
    yaw_mount = cq.importers.importStep(str(ROOT / "mechanical/yaw-seat-draft.step"))
    camera_floor = camera_mount.val().BoundingBox().zmin
    x0, x1 = p["camera_shelf_x_bounds"]
    y0, y1 = p["camera_shelf_y_bounds"]
    pitch_x, pitch_z = p["pitch_axis_origin_mm"][0], p["pitch_axis_origin_mm"][2]
    pitch_y = p["pitch_plate_y"]
    pitch = box(
        (x1 - x0, y1 - y0, base),
        ((x0 + x1) / 2, (y0 + y1) / 2, camera_floor - base / 2),
    )
    # Candidate plate outside +Y case face, not an invented spline or horn.
    disc = (
        cq.Workplane("XZ")
        .center(pitch_x, pitch_z)
        .circle(p["pitch_disc_radius"])
        .extrude(wall / 2, both=True)
        .translate((0, pitch_y, 0))
    )
    stem = box(
        (10, wall, camera_floor - pitch_z), (pitch_x, pitch_y, (camera_floor + pitch_z) / 2)
    )
    pitch = pitch.union(stem).union(disc)
    records = json.loads((ROOT / "mechanical/mount-checks.json").read_text())["parts"]
    fixture = cq.importers.importStep(str(ROOT / "mechanical/fixture-plate.step"))
    for record in records:
        for x, y, _ in record["slot_centres_world_mm"]:
            if record["component"] == "U1":
                pitch = pitch.cut(
                    cq.Workplane("XY")
                    .center(x, y)
                    .circle(1.7)
                    .extrude(base + 2)
                    .translate((0, 0, camera_floor - base - 1))
                )
            else:
                fixture = fixture.cut(
                    cq.Workplane("XY")
                    .center(x, y)
                    .circle(1.7)
                    .extrude(manifest["plate"][2] + 2)
                    .translate((0, 0, -1))
                )
    parts = {
        "yaw-carrier-draft": yaw,
        "pitch-carrier-draft": pitch,
        "carrier-fixture-plate": fixture,
    }
    for name, part in parts.items():
        if (
            len(part.solids().vals()) != 1
            or not part.val().isValid()
            or part.val().Volume() <= 0
        ):
            raise ValueError("invalid connected part: " + name)
        for ext in ("step", "stl"):
            cq.exporters.export(part, str(ROOT / "mechanical" / (name + "." + ext)))
        recovered = cq.importers.importStep(str(ROOT / "mechanical" / (name + ".step")))
        if len(recovered.solids().vals()) != 1 or not recovered.val().isValid():
            raise ValueError("STEP roundtrip failed: " + name)
    # Fixed obstacles and pair checks use real candidate solids plus coarse boxes.
    fixed = {k: v for k, v in envelopes.items() if k not in ("M2", "U1")}
    fixed.update(fixture=fixture, yaw_seat=yaw_mount)
    samples = []
    for yaw_deg in p["sample_yaw_deg"]:
        for pitch_deg in p["sample_pitch_deg"]:
            moving = {}
            for name, part in {
                "yaw_carrier": yaw,
                "M2": envelopes["M2"],
                "pitch_carrier": pitch,
                "camera_saddle": camera_mount,
                "U1": envelopes["U1"],
            }.items():
                if name in ("pitch_carrier", "camera_saddle", "U1"):
                    part = turn(
                        part,
                        p["pitch_axis_origin_mm"],
                        p["pitch_axis_direction"],
                        pitch_deg,
                    )
                moving[name] = turn(
                    part, p["yaw_axis_origin_mm"], p["yaw_axis_direction"], yaw_deg
                )
            collisions = []
            pairs = [
                (a, b, first, second)
                for a, first in moving.items()
                for b, second in fixed.items()
            ]
            names = list(moving)
            pairs += [
                (a, b, moving[a], moving[b])
                for i, a in enumerate(names)
                for b in names[i + 1 :]
            ]
            for a, b, first, second in pairs:
                volume = overlap(first, second)
                if volume > 1e-5:
                    collisions.append(
                        {"parts": [a, b], "overlap_mm3": round(volume, 5)}
                    )
            samples.append(
                {
                    "yaw_offset_deg": yaw_deg,
                    "pitch_offset_deg": pitch_deg,
                    "pitch_axis_origin_world_mm": rotate(
                        p["pitch_axis_origin_mm"], p["yaw_axis_origin_mm"], "Z", yaw_deg
                    ),
                    "pitch_axis_direction_world": rotate(
                        p["pitch_axis_direction"], [0, 0, 0], "Z", yaw_deg
                    ),
                    "camera_envelope_centre_mm": camera_envelope_centre(
                        components["U1"]["position"], p, yaw_deg, pitch_deg
                    ),
                    "intrusions": collisions,
                }
            )
            actual = moving["U1"].val().Center().toTuple()
            expected_centre = samples[-1]["camera_envelope_centre_mm"]
            if any(abs(a - b) > 1e-6 for a, b in zip(actual, expected_centre)):
                raise ValueError("point transform disagrees with CAD transform")
    assembly = cq.Assembly(name="TRACKER_CANDIDATE_CARRIERS")
    for name, part in {
        **parts,
        "yaw_seat": yaw_mount,
        "camera_saddle": camera_mount,
        **envelopes,
    }.items():
        assembly.add(part, name=name)
    assembly.save(str(ROOT / "mechanical/carrier-layout.step"))
    recovered = cq.importers.importStep(str(ROOT / "mechanical/carrier-layout.step"))
    expected = len(parts) + 2 + len(envelopes)
    if len(recovered.solids().vals()) != expected:
        raise ValueError("assembly lost bodies")
    report = {
        "physical_validation": False,
        "status": "Sampled envelope/solid review only; no continuous clearance or hardware release",
        "assumed_joint_frames": {
            "yaw": {
                "origin_mm": p["yaw_axis_origin_mm"],
                "axis": p["yaw_axis_direction"],
            },
            "pitch_in_yaw_zero_frame": {
                "origin_mm": p["pitch_axis_origin_mm"],
                "axis": p["pitch_axis_direction"],
            },
        },
        "camera_optical_frame": None,
        "firmware_command_ranges_min_center_max": command_ranges,
        "angle_mapping": "Planning assumption: command minus centre; real motor sign and scale uncalibrated",
        "assembly_solids": expected,
        "sample_count": len(samples),
        "samples_with_intrusion": sum(bool(s["intrusions"]) for s in samples),
        "samples": samples,
        "unresolved": p["unresolved"],
    }
    (ROOT / "mechanical/carrier-checks.json").write_text(
        json.dumps(report, indent=2) + "\n"
    )
    print(json.dumps({k: v for k, v in report.items() if k != "samples"}, indent=2))
    if report["samples_with_intrusion"]:
        raise SystemExit(
            "Sampled intrusion found; inspect carrier-checks.json before further work"
        )


if __name__ == "__main__":
    build()
