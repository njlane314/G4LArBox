#!/usr/bin/env python3
"""
Patch a DUNE VD GDML file with sparse FastDPSU nodes that dangle from the
readout/anode side into each repeated active TPC volume.

In the DUNE VD refactored generator the "below anode" direction is represented
by decreasing local X. This script therefore anchors each drop near the
positive-X face of volTPCActive and extends it along -X.
"""

from __future__ import annotations

import argparse
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Optional


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Add dangling FastDPSU nodes to DUNE VD GDML.")
    parser.add_argument("--input", required=True, help="Input DUNE VD GDML")
    parser.add_argument("--output", required=True, help="Output patched GDML")
    parser.add_argument("--parent", default="volTPCActive", help="Logical volume receiving dangling drops")
    parser.add_argument("--ny", type=int, default=1, help="Number of drops across local Y per active logical volume")
    parser.add_argument("--nz", type=int, default=1, help="Number of drops across local Z per active logical volume")
    parser.add_argument("--anchor-margin-cm", type=float, default=8.0, help="Margin from positive-X/anode face")
    parser.add_argument("--lateral-margin-cm", type=float, default=20.0, help="Margin from local Y/Z faces")
    parser.add_argument("--drop-length-cm", type=float, default=180.0, help="Length of dangling string along -X")
    parser.add_argument("--cable-y-cm", type=float, default=0.6, help="Cable visual thickness in Y")
    parser.add_argument("--cable-z-cm", type=float, default=0.6, help="Cable visual thickness in Z")
    parser.add_argument("--housing-x-cm", type=float, default=3.0, help="Node housing X size")
    parser.add_argument("--housing-y-cm", type=float, default=3.0, help="Node housing Y size")
    parser.add_argument("--housing-z-cm", type=float, default=3.0, help="Node housing Z size")
    parser.add_argument("--sensor-x-cm", type=float, default=0.10, help="Sensitive face X size")
    parser.add_argument("--sensor-y-cm", type=float, default=1.0, help="Sensitive face Y size")
    parser.add_argument("--sensor-z-cm", type=float, default=1.0, help="Sensitive face Z size")
    parser.add_argument("--material", default="LAr", help="Placeholder material for the visual node volumes")
    parser.add_argument("--copy-number-base", type=int, default=10000, help="First drop copy number")
    parser.add_argument("--dry-run", action="store_true", help="Print summary without writing")
    return parser.parse_args()


def require_child(root: ET.Element, tag: str) -> ET.Element:
    child = root.find(tag)
    if child is None:
        raise RuntimeError(f"Could not find <{tag}>")
    return child


def find_volume(root: ET.Element, name: str) -> Optional[ET.Element]:
    for volume in root.findall(".//volume"):
        if volume.get("name") == name:
            return volume
    return None


def find_solid(root: ET.Element, name: str) -> Optional[ET.Element]:
    solids = require_child(root, "solids")
    for solid in list(solids):
        if solid.get("name") == name:
            return solid
    return None


def parent_box_dimensions(root: ET.Element, parent: ET.Element) -> tuple[float, float, float]:
    solidref = parent.find("solidref")
    if solidref is None or not solidref.get("ref"):
        raise RuntimeError("Parent volume has no solidref")

    solid = find_solid(root, solidref.get("ref"))
    if solid is None:
        raise RuntimeError(f"Could not find solid {solidref.get('ref')}")
    if solid.tag != "box":
        raise RuntimeError(f"Parent solid must be a box, got {solid.tag}")
    if solid.get("lunit", "cm") != "cm":
        raise RuntimeError("Only cm parent solids are supported")

    return float(solid.get("x")), float(solid.get("y")), float(solid.get("z"))


def ensure_box(solids: ET.Element, name: str, x: float, y: float, z: float) -> None:
    for box in solids.findall("box"):
        if box.get("name") == name:
            return
    ET.SubElement(solids, "box", {
        "name": name,
        "lunit": "cm",
        "x": f"{x:.8g}",
        "y": f"{y:.8g}",
        "z": f"{z:.8g}",
    })


def volume_index(structure: ET.Element, name: str) -> Optional[int]:
    for index, child in enumerate(list(structure)):
        if child.tag == "volume" and child.get("name") == name:
            return index
    return None


def ensure_volume_before(structure: ET.Element, name: str, material: str, solid: str, before: str) -> ET.Element:
    for volume in structure.findall("volume"):
        if volume.get("name") == name:
            return volume

    volume = ET.Element("volume", {"name": name})
    ET.SubElement(volume, "materialref", {"ref": material})
    ET.SubElement(volume, "solidref", {"ref": solid})

    index = volume_index(structure, before)
    if index is None:
        structure.append(volume)
    else:
        structure.insert(index, volume)
    return volume


def add_sensor_aux(volume: ET.Element) -> None:
    for aux in volume.findall("auxiliary"):
        if aux.get("auxtype") == "SensDet" and aux.get("auxvalue") == "PhotonDetector":
            return
    ET.SubElement(volume, "auxiliary", {"auxtype": "SensDet", "auxvalue": "PhotonDetector"})


def centers(n: int, full: float, margin: float) -> list[float]:
    if n <= 0:
        raise ValueError("counts must be positive")

    lo = -0.5 * full + margin
    hi = 0.5 * full - margin
    if lo > hi:
        raise ValueError(f"margin {margin} cm is too large for dimension {full} cm")
    if n == 1:
        return [0.5 * (lo + hi)]

    step = (hi - lo) / (n - 1)
    return [lo + i * step for i in range(n)]


def make_physvol(ref: str, name: str, x: float, y: float, z: float, copy_number: Optional[int] = None) -> ET.Element:
    attrs = {}
    if copy_number is not None:
        attrs["copynumber"] = str(copy_number)
    physvol = ET.Element("physvol", attrs)
    ET.SubElement(physvol, "volumeref", {"ref": ref})
    ET.SubElement(physvol, "position", {
        "name": name,
        "unit": "cm",
        "x": f"{x:.8g}",
        "y": f"{y:.8g}",
        "z": f"{z:.8g}",
    })
    return physvol


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
        raise RuntimeError(f"Could not find parent volume {args.parent}")

    parent_x, parent_y, parent_z = parent_box_dimensions(root, parent)
    if args.drop_length_cm + args.housing_x_cm + args.anchor_margin_cm >= parent_x:
        raise RuntimeError("Drop is too long for the active TPC X dimension")

    prefix = "FastDPSU"
    envelope_solid = f"{prefix}DropEnvelopeSolid"
    cable_solid = f"{prefix}CableSolid"
    housing_solid = f"{prefix}HousingSolid"
    sensor_solid = f"{prefix}SensitiveSolid"

    envelope_x = args.drop_length_cm + args.housing_x_cm
    envelope_y = max(args.cable_y_cm, args.housing_y_cm)
    envelope_z = max(args.cable_z_cm, args.housing_z_cm)

    ensure_box(solids, envelope_solid, envelope_x, envelope_y, envelope_z)
    ensure_box(solids, cable_solid, args.drop_length_cm, args.cable_y_cm, args.cable_z_cm)
    ensure_box(solids, housing_solid, args.housing_x_cm, args.housing_y_cm, args.housing_z_cm)
    ensure_box(solids, sensor_solid, args.sensor_x_cm, args.sensor_y_cm, args.sensor_z_cm)

    sensor = ensure_volume_before(structure, "volOpDetSensitive_FastDPSU", args.material, sensor_solid, args.parent)
    add_sensor_aux(sensor)
    housing = ensure_volume_before(structure, "volFastDPSUHousing", args.material, housing_solid, args.parent)
    cable = ensure_volume_before(structure, "volFastDPSUCable", args.material, cable_solid, args.parent)
    drop = ensure_volume_before(structure, "volFastDPSUDrop", args.material, envelope_solid, args.parent)

    if not any(pv.find("volumeref") is not None and pv.find("volumeref").get("ref") == "volOpDetSensitive_FastDPSU"
               for pv in housing.findall("physvol")):
        sensor_x = -0.5 * args.housing_x_cm + 0.5 * args.sensor_x_cm + 1.0e-4
        housing.append(make_physvol("volOpDetSensitive_FastDPSU", "posFastDPSUSensitiveFace", sensor_x, 0.0, 0.0))

    if not any(pv.find("volumeref") is not None and pv.find("volumeref").get("ref") == "volFastDPSUCable"
               for pv in drop.findall("physvol")):
        cable_x = 0.5 * args.housing_x_cm
        housing_x = -0.5 * args.drop_length_cm
        drop.append(make_physvol("volFastDPSUCable", "posFastDPSUCable", cable_x, 0.0, 0.0))
        drop.append(make_physvol("volFastDPSUHousing", "posFastDPSUHousing", housing_x, 0.0, 0.0))

    existing_positions = {
        pos.get("name")
        for pos in parent.findall("physvol/position")
        if pos is not None and pos.get("name")
    }

    anchor_x = 0.5 * parent_x - args.anchor_margin_cm
    envelope_center_x = anchor_x - 0.5 * envelope_x
    ys = centers(args.ny, parent_y, args.lateral_margin_cm)
    zs = centers(args.nz, parent_z, args.lateral_margin_cm)

    added = 0
    for iy, y in enumerate(ys):
        for iz, z in enumerate(zs):
            pos_name = f"posFastDPSUDrop_{args.parent}_{iy}_{iz}"
            if pos_name in existing_positions:
                continue
            copy_number = args.copy_number_base + iy * args.nz + iz
            parent.append(make_physvol("volFastDPSUDrop", pos_name, envelope_center_x, y, z, copy_number))
            added += 1

    print(f"Parent volume: {args.parent}")
    print(f"Parent dimensions: x={parent_x:g} cm, y={parent_y:g} cm, z={parent_z:g} cm")
    print(f"Dangling direction: local -X from anchor x={anchor_x:g} cm")
    print(f"Drops per logical active TPC: ny={args.ny}, nz={args.nz}; added {added}")
    print("The logical active TPC is repeated by the DUNE VD cryostat placements.")

    if not args.dry_run:
        indent(root)
        output = Path(args.output)
        output.parent.mkdir(parents=True, exist_ok=True)
        tree.write(output, encoding="utf-8", xml_declaration=True)
        print(f"Wrote {output}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(2)
