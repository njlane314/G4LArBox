#!/usr/bin/env python3
"""
Patch a GDML file by adding sparse fast-dSiPM/DPSU optical marker nodes.

The intended first-pass DUNE VD use is to place tiny LAr-marker optical nodes
as daughters of volTPCActive. If that logical volume is repeated, the node
pattern is repeated with it, which is useful for early timing studies.
"""

from __future__ import annotations

import argparse
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Optional


LENGTH_TO_CM = {
    "mm": 0.1,
    "cm": 1.0,
    "m": 100.0,
}


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Add fast-dSiPM/DPSU optical node volumes to a GDML file.")
    p.add_argument("--input", required=True, help="Input GDML file")
    p.add_argument("--output", required=True, help="Output patched GDML file")
    p.add_argument("--parent", default="volTPCActive", help="Logical volume receiving the nodes")
    p.add_argument("--nx", type=int, default=1, help="Number of nodes along local x of parent volume")
    p.add_argument("--ny", type=int, default=1, help="Number of nodes along local y of parent volume")
    p.add_argument("--nz", type=int, default=1, help="Number of nodes along local z of parent volume")
    p.add_argument("--margin-cm", type=float, default=10.0, help="Margin from each parent-volume boundary")
    p.add_argument("--housing-x-cm", type=float, default=3.0, help="DPSU housing x-size")
    p.add_argument("--housing-y-cm", type=float, default=3.0, help="DPSU housing y-size")
    p.add_argument("--housing-z-cm", type=float, default=3.0, help="DPSU housing z-size")
    p.add_argument("--sensor-x-cm", type=float, default=0.10, help="Sensitive face x-size")
    p.add_argument("--sensor-y-cm", type=float, default=1.0, help="Sensitive face y-size")
    p.add_argument("--sensor-z-cm", type=float, default=1.0, help="Sensitive face z-size")
    p.add_argument("--housing-material", default="LAr", help="Material ref for housing")
    p.add_argument("--sensor-material", default="LAr", help="Material ref for sensitive volume")
    p.add_argument("--name-prefix", default="FastDPSU", help="Prefix for new GDML names")
    p.add_argument("--copy-number-base", type=int, default=1, help="First housing physvol copy number")
    p.add_argument("--dry-run", action="store_true", help="Do not write output; print summary only")
    return p.parse_args()


def require_child(root: ET.Element, tag: str) -> ET.Element:
    elem = root.find(tag)
    if elem is None:
        raise RuntimeError(f"Could not find <{tag}> in GDML")
    return elem


def find_volume(root: ET.Element, name: str) -> Optional[ET.Element]:
    for vol in root.findall(".//volume"):
        if vol.get("name") == name:
            return vol
    return None


def find_solid(root: ET.Element, name: str) -> Optional[ET.Element]:
    solids = require_child(root, "solids")
    for solid in list(solids):
        if solid.get("name") == name:
            return solid
    return None


def length_to_cm(value: str, unit: Optional[str]) -> float:
    scale = LENGTH_TO_CM.get(unit or "cm")
    if scale is None:
        raise RuntimeError(f"Unsupported GDML length unit {unit!r}; expected one of {sorted(LENGTH_TO_CM)}")
    return float(value) * scale


def get_parent_box_dimensions_cm(root: ET.Element, parent_volume: ET.Element) -> tuple[float, float, float]:
    solidref = parent_volume.find("solidref")
    if solidref is None or not solidref.get("ref"):
        raise RuntimeError("Parent volume has no <solidref ref=...>")

    solid_name = solidref.get("ref")
    solid = find_solid(root, solid_name)
    if solid is None:
        raise RuntimeError(f"Could not find parent solid {solid_name!r}")
    if solid.tag != "box":
        raise RuntimeError(
            f"Parent solid {solid_name!r} is a <{solid.tag}>, not a <box>. "
            "This simple patcher currently handles box parents only."
        )

    try:
        unit = solid.get("lunit", "cm")
        return (
            length_to_cm(solid.get("x"), unit),
            length_to_cm(solid.get("y"), unit),
            length_to_cm(solid.get("z"), unit),
        )
    except Exception as exc:
        raise RuntimeError(f"Could not parse dimensions for box {solid_name!r}") from exc


def ensure_box(solids: ET.Element, name: str, x: float, y: float, z: float) -> None:
    for solid in solids.findall("box"):
        if solid.get("name") == name:
            return
    ET.SubElement(solids, "box", {
        "name": name,
        "lunit": "cm",
        "x": f"{x:.8g}",
        "y": f"{y:.8g}",
        "z": f"{z:.8g}",
    })


def find_volume_index(structure: ET.Element, name: str) -> Optional[int]:
    for index, child in enumerate(list(structure)):
        if child.tag == "volume" and child.get("name") == name:
            return index
    return None


def ensure_volume_before(structure: ET.Element, name: str, material: str, solid: str, before_name: str) -> ET.Element:
    for volume in structure.findall("volume"):
        if volume.get("name") == name:
            return volume

    volume = ET.SubElement(structure, "volume", {"name": name})
    ET.SubElement(volume, "materialref", {"ref": material})
    ET.SubElement(volume, "solidref", {"ref": solid})
    structure.remove(volume)

    before_index = find_volume_index(structure, before_name)
    if before_index is None:
        structure.append(volume)
    else:
        structure.insert(before_index, volume)
    return volume


def add_sensitive_aux(volume: ET.Element) -> None:
    for aux in volume.findall("auxiliary"):
        if aux.get("auxtype") == "SensDet" and aux.get("auxvalue") == "PhotonDetector":
            return
    ET.SubElement(volume, "auxiliary", {"auxtype": "SensDet", "auxvalue": "PhotonDetector"})


def linspace_centers(n: int, full: float, margin: float) -> list[float]:
    if n <= 0:
        raise ValueError("grid counts must be positive")

    half = 0.5 * full
    usable_min = -half + margin
    usable_max = half - margin
    if usable_min > usable_max:
        raise ValueError(f"margin {margin} cm is too large for dimension {full} cm")
    if n == 1:
        return [0.5 * (usable_min + usable_max)]

    step = (usable_max - usable_min) / (n - 1)
    return [usable_min + i * step for i in range(n)]


def make_physvol(
    volume_ref: str,
    pos_name: str,
    x: float,
    y: float,
    z: float,
    copy_number: Optional[int] = None,
    rot_ref: Optional[str] = None,
) -> ET.Element:
    attrs = {}
    if copy_number is not None:
        attrs["copynumber"] = str(copy_number)
    pv = ET.Element("physvol", attrs)
    ET.SubElement(pv, "volumeref", {"ref": volume_ref})
    ET.SubElement(pv, "position", {
        "name": pos_name,
        "unit": "cm",
        "x": f"{x:.8g}",
        "y": f"{y:.8g}",
        "z": f"{z:.8g}",
    })
    if rot_ref:
        ET.SubElement(pv, "rotationref", {"ref": rot_ref})
    return pv


def indent(elem: ET.Element, level: int = 0) -> None:
    i = "\n" + level * "  "
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "  "
        for child in elem:
            indent(child, level + 1)
        if not child.tail or not child.tail.strip():
            child.tail = i
    if level and (not elem.tail or not elem.tail.strip()):
        elem.tail = i


def main() -> int:
    args = parse_args()
    tree = ET.parse(args.input)
    root = tree.getroot()

    solids = require_child(root, "solids")
    structure = require_child(root, "structure")
    parent = find_volume(root, args.parent)
    if parent is None:
        raise RuntimeError(f"Could not find parent volume {args.parent!r}")

    px, py, pz = get_parent_box_dimensions_cm(root, parent)
    xs = linspace_centers(args.nx, px, args.margin_cm)
    ys = linspace_centers(args.ny, py, args.margin_cm)
    zs = linspace_centers(args.nz, pz, args.margin_cm)

    prefix = args.name_prefix
    housing_solid = f"{prefix}HousingSolid"
    sensor_solid = f"{prefix}SensitiveSolid"
    housing_vol = f"vol{prefix}Housing"
    sensor_vol = f"volOpDetSensitive_{prefix}"

    ensure_box(solids, housing_solid, args.housing_x_cm, args.housing_y_cm, args.housing_z_cm)
    ensure_box(solids, sensor_solid, args.sensor_x_cm, args.sensor_y_cm, args.sensor_z_cm)

    svol = ensure_volume_before(structure, sensor_vol, args.sensor_material, sensor_solid, args.parent)
    add_sensitive_aux(svol)
    hvol = ensure_volume_before(structure, housing_vol, args.housing_material, housing_solid, args.parent)

    if not any(
        pv.find("volumeref") is not None and pv.find("volumeref").get("ref") == sensor_vol
        for pv in hvol.findall("physvol")
    ):
        face_x = 0.5 * args.housing_x_cm - 0.5 * args.sensor_x_cm - 1e-4
        hvol.append(make_physvol(sensor_vol, f"pos{prefix}SensitiveFace", face_x, 0.0, 0.0))

    existing_names = set()
    for pv in parent.findall("physvol"):
        pos = pv.find("position")
        if pos is not None and pos.get("name"):
            existing_names.add(pos.get("name"))

    added = 0
    for ix, x in enumerate(xs):
        for iy, y in enumerate(ys):
            for iz, z in enumerate(zs):
                pos_name = f"pos{prefix}_{args.parent}_{ix}_{iy}_{iz}"
                if pos_name in existing_names:
                    continue
                copy_number = args.copy_number_base + ix * args.ny * args.nz + iy * args.nz + iz
                parent.append(make_physvol(housing_vol, pos_name, x, y, z, copy_number=copy_number))
                added += 1

    print(f"Parent volume: {args.parent}")
    print(f"Parent dimensions: x={px:g} cm, y={py:g} cm, z={pz:g} cm")
    print(f"Grid per logical parent: nx={args.nx}, ny={args.ny}, nz={args.nz}; added {added} physvols")
    print("Note: if the parent logical volume is placed many times, this pattern is replicated in every instance.")

    if not args.dry_run:
        indent(root)
        out = Path(args.output)
        out.parent.mkdir(parents=True, exist_ok=True)
        tree.write(out, encoding="utf-8", xml_declaration=True)
        print(f"Wrote {out}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(2)
