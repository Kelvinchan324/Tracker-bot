"""Generate reviewable wiring, BOM, placement drawing and STEP from one revision manifest.
Run from the repository: python tools/build_engineering.py [--cad]
CAD requires cadquery==2.8.0. Geometry envelopes are not vendor CAD or fit approval.
"""
import argparse
import json
import math
from collections import defaultdict
from html import escape
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def validate(d):
    refs = set()
    nets = defaultdict(list)
    for c in d["components"]:
        if c["ref"] in refs:
            raise ValueError("duplicate reference: " + c["ref"])
        refs.add(c["ref"])
        if not c.get("selection") or not c.get("source"):
            raise ValueError("missing source/selection: " + c["ref"])
        for pin, net in c.get("pins", {}).items():
            if net != "NC":
                nets[net].append(c["ref"] + "." + pin)
        if "position" in c:
            values = c["position"] + c["size"]
            if len(c["position"]) != 3 or len(c["size"]) != 3 or not all(math.isfinite(v) for v in values):
                raise ValueError("invalid geometry: " + c["ref"])
            if any(v <= 0 for v in c["size"]):
                raise ValueError("nonpositive envelope: " + c["ref"])
    singles = {n: pins for n, pins in nets.items() if len(pins) < 2}
    if singles:
        raise ValueError("unconnected nets: " + repr(singles))
    return nets


def text(x, y, value, size=15, color="#172d40"):
    return f'<text x="{x}" y="{y}" font-family="Arial,sans-serif" font-size="{size}" fill="{color}">{escape(str(value))}</text>'


def svg(body, width, height):
    return f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}"><rect width="100%" height="100%" fill="#f7fafc"/>' + "".join(body) + "</svg>"


def build(d, cad=False):
    nets = validate(d)
    hw, mech = ROOT / "hardware", ROOT / "mechanical"
    hw.mkdir(exist_ok=True); mech.mkdir(exist_ok=True)
    lines = [f'# {d["title"]} — bill of materials', '', d["scope"], '',
             'Prices and procurement approval are not supplied. Quantities are per prototype.',
             '', '| Ref | Qty | Selection / value | Selection status | Source |',
             '| --- | ---: | --- | --- | --- |']
    for c in d["components"]:
        lines.append(f'| {c["ref"]} | {c.get("qty", 1)} | {c["name"]} | {c["selection"]} | {c["source"]} |')
    lines += ['', '## Net connections', '', 'Identical labels in the schematic are electrically connected.',
              'NC means intentionally unconnected. Supply and connector ratings need physical verification.', '']
    for net, endpoints in nets.items():
        lines.append(f'- **{net}**: ' + ', '.join(endpoints))
    (hw / "BOM.md").write_text("\n".join(lines) + "\n", encoding="utf-8")

    body = [text(30, 35, d["title"] + " / module wiring schematic", 23),
            text(30, 62, "DRAFT — equal net labels connect; module internals excluded; no PCB release", 15)]
    columns = [100, 100]
    for i, c in enumerate(d["components"]):
        if not c.get("pins"):
            continue
        col = 0 if columns[0] <= columns[1] else 1
        x, y = 30 + col * 610, columns[col]
        h = 74 + len(c["pins"]) * 26
        body += [f'<rect x="{x}" y="{y}" width="570" height="{h}" rx="6" fill="white" stroke="#8fa9bb"/>',
                 text(x+16, y+25, c["ref"] + "  " + c["name"][:57], 16),
                 text(x+16, y+48, c["selection"][:72], 12, "#536779")]
        for j, (pin, net) in enumerate(c["pins"].items()):
            py = y + 75 + j * 26
            body += [text(x+16, py, pin, 14),
                     f'<path d="M {x+210} {py-5} h 54" stroke="#187b97" stroke-width="2"/>',
                     f'<circle cx="{x+210}" cy="{py-5}" r="3" fill="#187b97"/>',
                     text(x+274, py, net, 13)]
        columns[col] += h + 20
    (hw / "schematic.svg").write_text(svg(body, 1250, max(columns)+20), encoding="utf-8")

    placed = [c for c in d["components"] if "position" in c]
    (mech / "placements.json").write_text(json.dumps({
        "units":"mm", "frame":"plate centre XY; top of bench Z=0; +X forward; +Y left; +Z up",
        "status":"assumed bounding envelopes unless cited otherwise", "components":placed
    }, indent=2)+"\n", encoding="utf-8")
    w, depth, thickness = d["plate"]
    scale = min(850/w, 430/depth)
    body = [text(30, 35, d["title"] + " / draft top view", 23),
            text(30, 62, f"Plate {w} x {depth} x {thickness} mm; +X right, +Y up; envelopes only", 15)]
    ox, oy = 500, 340
    body.append(f'<rect x="{ox-w*scale/2}" y="{oy-depth*scale/2}" width="{w*scale}" height="{depth*scale}" fill="#e7edf3" stroke="#456"/>')
    for c in placed:
        x,y,z = c["position"]; sx,sy,sz = c["size"]
        body.append(f'<rect x="{ox+(x-sx/2)*scale}" y="{oy-(y+sy/2)*scale}" width="{sx*scale}" height="{sy*scale}" fill="#55aacd" fill-opacity=".28" stroke="#187b97"/>')
        body.append(text(ox+x*scale, oy-y*scale, c["ref"], 14))
    for i,c in enumerate(placed):
        body.append(text(30, 595+i*23, f'{c["ref"]}: {c["name"]}  centre={c["position"]}  envelope={c["size"]}', 13))
    body.append(text(30, 620+len(placed)*23, "Mount patterns, cable loops, thermal clearance and motion sweeps require measured fit checks.", 13))
    (mech / "placement.svg").write_text(svg(body, 1000, 650+len(placed)*23), encoding="utf-8")
    if cad:
        import cadquery as cq
        plate = cq.Workplane("XY").box(w, depth, thickness).translate((0,0,thickness/2))
        for x,y in d["fixture_holes"]:
            tool = cq.Workplane("XY").center(x,y).circle(d["fixture_hole_d"]/2).extrude(thickness+2).translate((0,0,-1))
            plate = plate.cut(tool)
        if not plate.val().isValid() or plate.val().Volume() <= 0:
            raise ValueError("invalid base plate")
        assembly = cq.Assembly(name=d["cad_name"])
        assembly.add(plate, name="fixture_plate", color=cq.Color(.55,.6,.65))
        for c in placed:
            shape=cq.Workplane("XY").box(*c["size"]).translate(tuple(c["position"]))
            if not shape.val().isValid():
                raise ValueError("invalid envelope: " + c["ref"])
            assembly.add(shape, name=c["ref"]+"_ENVELOPE", color=cq.Color(.2,.65,.8,.5))
        assembly.save(str(mech / "layout-draft.step"))
        cq.exporters.export(plate, str(mech / "fixture-plate.step"))
        cq.exporters.export(plate, str(mech / "fixture-plate.stl"))
        # Reimport the actual exports to verify readable BREP and expected solid count.
        recovered = cq.importers.importStep(str(mech / "layout-draft.step"))
        if len(recovered.solids().vals()) != 1 + len(placed):
            raise ValueError("STEP round-trip lost bodies")
        print(f"CAD verified: {len(placed)+1} solids; base plate volume {plate.val().Volume():.2f} mm3")
    print(f'{d["title"]}: {len(d["components"])} BOM rows, {len(nets)} nets, {len(placed)} placed envelopes')


if __name__ == "__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cad", action="store_true")
    args=parser.parse_args()
    build(json.loads((ROOT/"hardware/rev-a.json").read_text(encoding="utf-8")), args.cad)
