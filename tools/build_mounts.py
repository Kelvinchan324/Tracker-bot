"""Generate editable candidate cradles, not a released pan/tilt mechanism.

Requires cadquery==2.8.0. Reads the revision's actual envelope positions.
Validates single solids, no envelope intrusion and STEP round trips.
"""

import json
from pathlib import Path

import cadquery as cq

ROOT = Path(__file__).resolve().parents[1]


def box(x, y, z, center):
    return cq.Workplane("XY").box(x, y, z).translate(tuple(center))


def build():
    revision = json.loads((ROOT / "hardware/rev-a.json").read_text(encoding="utf-8"))
    params = json.loads((ROOT / "mechanical/mount-parameters.json").read_text())
    for key in (
        "clearance_per_side",
        "wall",
        "base",
        "yaw_wall_height",
        "camera_wall_height",
        "mount_slot_length",
        "mount_slot_width",
    ):
        if not 0 < params[key] < 30:
            raise ValueError("invalid dimension: " + key)
    if params["mount_slot_length"] < params["mount_slot_width"]:
        raise ValueError("slot length must exceed width")
    components = {c["ref"]: c for c in revision["components"]}
    clearance, wall, base = (params[k] for k in ("clearance_per_side", "wall", "base"))
    assembly = cq.Assembly(name="CANDIDATE_MOUNTS_NOT_RELEASED")
    records = []
    for ref, name, height in (
        ("M1", "yaw-seat-draft", params["yaw_wall_height"]),
        ("U1", "camera-saddle-draft", params["camera_wall_height"]),
    ):
        c = components[ref]
        sx, sy, sz = c["size"]
        cx, cy, cz = c["position"]
        floor = cz - sz / 2
        inner_x, inner_y = sx + 2 * clearance, sy + 2 * clearance
        outer_x, outer_y = inner_x + 2 * wall, inner_y + 2 * wall
        if (
            params["mount_slot_width"] >= 6
            or params["mount_slot_length"] >= outer_y - 2
        ):
            raise ValueError("mount slot leaves insufficient tab margin")
        # Local floor is z=0. Positive Z walls never intrude on the given envelope.
        part = box(outer_x, outer_y, base, (0, 0, -base / 2))
        part = part.union(box(outer_x, outer_y, height, (0, 0, height / 2)))
        part = part.cut(box(inner_x, inner_y, height + 2, (0, 0, height / 2 + 1)))
        if ref == "U1":
            # Open both X ends; connector/lens/antenna keepouts still require measurement.
            part = part.cut(
                box(outer_x + 2, inner_y, height + 2, (0, 0, height / 2 + 1))
            )
        else:
            # Candidate cable exit, explicitly not a measured SG90 interface.
            part = part.cut(box(wall + 2, 6, 8, (-outer_x / 2, 0, 4)))
        tab_centers = []
        for sign in (-1, 1):
            x = sign * (outer_x / 2 + 3)
            part = part.union(box(8, outer_y, base, (x, 0, -base / 2)))
            slot = (
                cq.Workplane("XY")
                .center(x, 0)
                .slot2D(params["mount_slot_length"], params["mount_slot_width"], 90)
                .extrude(base + 2)
                .translate((0, 0, -base - 1))
            )
            part = part.cut(slot)
            tab_centers.append([round(cx + x, 3), cy, floor - base / 2])
        part = part.translate((cx, cy, floor))
        shape = part.val()
        envelope = box(sx, sy, sz, c["position"])
        if len(part.solids().vals()) != 1 or not shape.isValid() or shape.Volume() <= 0:
            raise ValueError("invalid connected mount: " + name)
        intrusion = part.intersect(envelope).val()
        if intrusion is not None and intrusion.Volume() > 1e-6:
            raise ValueError("mount intrudes into component envelope: " + name)
        cq.exporters.export(part, str(ROOT / "mechanical" / (name + ".step")))
        cq.exporters.export(part, str(ROOT / "mechanical" / (name + ".stl")))
        recovered = cq.importers.importStep(str(ROOT / "mechanical" / (name + ".step")))
        if len(recovered.solids().vals()) != 1 or not recovered.val().isValid():
            raise ValueError("STEP validation failed: " + name)
        assembly.add(part, name=name, color=cq.Color(0.6, 0.65, 0.7))
        assembly.add(
            envelope,
            name=ref + "_ASSUMED_ENVELOPE",
            color=cq.Color(0.2, 0.65, 0.8, 0.4),
        )
        records.append(
            {
                "name": name,
                "component": ref,
                "floor_z_mm": floor,
                "slot_centres_world_mm": tab_centers,
                "volume_mm3": round(shape.Volume(), 3),
                "release": "NOT APPROVED; unresolved interfaces in mount-parameters.json",
            }
        )
    assembly.save(str(ROOT / "mechanical/mounting-drafts.step"))
    recovered_assembly = cq.importers.importStep(
        str(ROOT / "mechanical/mounting-drafts.step")
    )
    if len(recovered_assembly.solids().vals()) != 4:
        raise ValueError("combined STEP lost mount or envelope bodies")
    (ROOT / "mechanical/mount-checks.json").write_text(
        json.dumps(
            {
                "status": "CAD-only checks; no physical fit or load validation",
                "parts": records,
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )
    print(json.dumps(records, indent=2))


if __name__ == "__main__":
    build()
