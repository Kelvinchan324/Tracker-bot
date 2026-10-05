"""Render existing STEP geometry for review; never infer fit or optical frames."""

import hashlib
import json
from pathlib import Path

import cadquery as cq
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.patches import Patch  # noqa: E402
from mpl_toolkits.mplot3d.art3d import Poly3DCollection  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
MECHANICAL = ROOT / "mechanical"


def source_digest(path):
    data = path.read_bytes()
    if path.suffix in {".json", ".py"}:
        data = data.replace(b"\r\n", b"\n")  # Git text checkout must not stale a review.
    return hashlib.sha256(data).hexdigest()


def render():
    manifest = json.loads((ROOT / "hardware/rev-a.json").read_text())
    parameters = json.loads((MECHANICAL / "carrier-parameters.json").read_text())
    files = {
        "Fixture": ("carrier-fixture-plate.step", "#acb6c2"),
        "Yaw seat": ("yaw-seat-draft.step", "#67788d"),
        "Yaw carrier": ("yaw-carrier-draft.step", "#d98a3d"),
        "Pitch carrier": ("pitch-carrier-draft.step", "#f2c34e"),
        "Camera saddle": ("camera-saddle-draft.step", "#2b968e"),
    }
    records, shapes, sources = [], [], []
    for label, (filename, color) in files.items():
        path = MECHANICAL / filename
        shape = cq.importers.importStep(str(path)).val()
        if len(shape.Solids()) != 1 or not shape.isValid() or shape.Volume() <= 0:
            raise ValueError("Invalid review solid: " + filename)
        shapes.append((label, shape, color, 1.0))
        sources.append(path)
    for component in manifest["components"]:
        if "position" not in component:
            continue
        ref = component["ref"]
        shape = cq.Workplane("XY").box(*component["size"]).translate(
            tuple(component["position"])
        ).val()
        color = "#477cbd" if ref.startswith("M") else "#8acbd0" if ref == "U1" else "#aa88b9"
        shapes.append((ref + " envelope", shape, color, 0.85))

    assembly_path = MECHANICAL / "carrier-layout.step"
    saved = cq.importers.importStep(str(assembly_path)).val()
    if len(saved.Solids()) != len(shapes):
        raise ValueError("Review parts disagree with assembly solid count")
    # Match every individual body, not only total volume; reject a stale STEP set.
    def signature(shape):
        b = shape.BoundingBox()
        return (shape.Volume(), b.xmin, b.xmax, b.ymin, b.ymax, b.zmin, b.zmax)

    unmatched = [signature(s) for s in saved.Solids()]
    for label, shape, _, _ in shapes:
        sig = signature(shape)
        match = next((i for i, other in enumerate(unmatched)
                      if all(abs(a - b) < 1e-4 for a, b in zip(sig, other))), None)
        if match is None:
            raise ValueError("Review geometry disagrees with assembly: " + label)
        unmatched.pop(match)

    meshes = []
    for label, shape, color, alpha in shapes:
        exact_signature = signature(shape)  # Before tessellation can cache faceted bounds.
        vertices, triangles = shape.tessellate(0.15, 0.2)
        points = [v.toTuple() for v in vertices]
        meshes.append((label, [[points[i] for i in tri] for tri in triangles], color, alpha))
        records.append({"name": label, "triangles": len(triangles),
                        "volume_mm3": shape.Volume(), "bounds_signature": exact_signature})

    fig = plt.figure(figsize=(16, 9), facecolor="#f7f8fb")
    fig.suptitle("TRACKER BOT  /  CARRIER GEOMETRY REVIEW", x=0.045, ha="left",
                 y=0.96, fontsize=21, fontweight="bold", color="#233247")
    fig.text(0.045, 0.905, "DRAFT ONLY  |  Zero pose  |  Dimensions in mm  |  Physical validation: NONE",
             fontsize=12, color="#a64329")
    for rect, title, elev, azim in [
        ([0.035, 0.28, 0.38, 0.54], "A  /  Isometric assembly", 25, -55),
        ([0.41, 0.28, 0.34, 0.54], "B  /  Side projection (+X view)", 0, 0),
    ]:
        ax = fig.add_axes(rect, projection="3d", facecolor="#f7f8fb")
        ax.set_proj_type("ortho")
        for _, faces, color, alpha in meshes:
            ax.add_collection3d(Poly3DCollection(faces, facecolors=color, alpha=alpha,
                                                edgecolors="none", zsort="average"))
        for name, color in [("yaw", "#be3c82"), ("pitch", "#008378")]:
            origin = parameters[name + "_axis_origin_mm"]
            direction = parameters[name + "_axis_direction"]
            start = [origin[i] - 14 * direction[i] for i in range(3)]
            ax.quiver(*start, *direction, length=35, color=color, linewidth=2.5,
                      arrow_length_ratio=0.16)
            ax.scatter(*origin, color=color, s=22, depthshade=False)
        ax.set(xlim=(-55, 55), ylim=(-45, 45), zlim=(0, 105),
               xlabel="X / mm", ylabel="Y / mm", zlabel="Z / mm")
        ax.set_box_aspect((110, 90, 105))
        ax.view_init(elev=elev, azim=azim)
        if elev == 0:
            ax.set_xticks([])  # Depth axis collapses in this orthographic view.
            ax.set_xlabel("")
        ax.tick_params(labelsize=8)
        ax.set_title(title, fontsize=13, loc="left", color="#233247", pad=15)

    legend = fig.add_axes([0.78, 0.37, 0.21, 0.47])
    legend.axis("off")
    legend.legend(handles=[Patch(facecolor=c, label=label) for label, _, c, _ in shapes],
                  loc="upper left", frameon=False, fontsize=10, labelspacing=1.1)
    fig.text(0.79, 0.34, "Solid colors: candidate STEP parts\nTranslucent boxes: component envelopes\n"
             "M1: yaw servo / M2: pitch servo\nU1: camera + controller envelope",
             fontsize=9, linespacing=1.6, color="#334155")
    fig.text(0.05, 0.20, "ASSUMED JOINT FRAMES", fontsize=11, weight="bold", color="#233247")
    yaw = parameters["yaw_axis_origin_mm"]
    pitch = parameters["pitch_axis_origin_mm"]
    fig.text(0.05, 0.105, f"Yaw: {yaw}, +Z (magenta)\nPitch: {pitch}, local +Y (teal)\n"
             "Camera optical frame: UNKNOWN; U1 centre is not the optical centre.",
             fontsize=10, linespacing=1.6)
    fig.text(0.53, 0.20, "UNRESOLVED LOAD PATH", fontsize=11, weight="bold", color="#a64329")
    fig.text(0.53, 0.105, "Horn lands are undrilled: no rigid servo-output attachment.\n"
             "Body/board retention, pitch bearing and cable sweep are unfinished.\n"
             "No fasteners, optical rays, stress analysis or fabrication approval shown.",
             fontsize=10, linespacing=1.6)
    fig.text(0.05, 0.035, "Generated from committed STEP parts + rev-a component envelopes. "
             "Read carrier-review.md before changing parameters or operating motors.",
             fontsize=9, color="#58667a")
    output = MECHANICAL / "carrier-visual-review.png"
    fig.savefig(output, dpi=150, facecolor=fig.get_facecolor())
    plt.close(fig)
    sources += [assembly_path, ROOT / "hardware/rev-a.json",
                MECHANICAL / "carrier-parameters.json", Path(__file__).resolve()]
    report = {"physical_validation": False, "assembly_body_match": True,
              "match_basis": "one-to-one volume within 1e-4 mm3 and bounds within 1e-4 mm",
              "not_proven": ["topological identity", "clearance", "physical fit", "load capacity"],
              "rendered_bodies": len(shapes), "parts": records,
              "render_sha256": hashlib.sha256(output.read_bytes()).hexdigest(),
              "generator_versions": {"cadquery": cq.__version__, "matplotlib": matplotlib.__version__},
              "hash_policy": "STEP/PNG native bytes; JSON/Python CRLF normalized to LF",
              "sources_sha256": {p.relative_to(ROOT).as_posix(): source_digest(p)
                                 for p in sources}}
    (MECHANICAL / "carrier-visual-review.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"Rendered {len(shapes)} matched bodies: {output}")


if __name__ == "__main__":
    render()
