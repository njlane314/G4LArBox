#!/usr/bin/env python3
"""
Patch a full DUNE VD GDML with the sparse FastDPSU string layout described in
arXiv:2502.09729v2, Figure 2.

The target layout is a global 8 x 39 grid of suspended strings, with 8 DPSUs
on each string.  In the refactored DUNE VD GDML used here, the full 60 m
detector length is local Z, the transverse detector width is local Y, and the
vertical/string direction is local X.

This is a deliberately compact visualization/early-study patch.  It places
global FastDPSU strings under volCryostat, centered on volEnclosureTPC, and
models each string as two suspended half-strings split around the central VD
plane.  Each DPSU is a small tag-like package on the support/fiber pair, with
a carrier, readout board, sensitive tile, clamps, and fiber-interface
placeholders.  A production geometry contribution should move this into the
DUNE VD generator and handle overlap-free placement against the segmented TPC
volumes.
"""

from __future__ import annotations

import argparse
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Optional


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Add the paper-style 8x39x8 FastDPSU string lattice to DUNE VD GDML."
    )
    parser.add_argument("--input", required=True, help="Input DUNE VD GDML")
    parser.add_argument("--output", required=True, help="Output patched GDML")
    parser.add_argument("--parent", default="volCryostat", help="Logical parent receiving global strings")
    parser.add_argument("--reference-volume", default="volEnclosureTPC", help="Volume whose box centers the lattice")
    parser.add_argument("--strings-y", type=int, default=8, help="Number of string columns across detector Y")
    parser.add_argument("--strings-z", type=int, default=39, help="Number of string columns along detector Z")
    parser.add_argument("--nodes-per-string", type=int, default=8, help="Number of DPSUs along each vertical string")
    parser.add_argument("--pitch-cm", type=float, default=150.0, help="Nominal string/node pitch from the paper")
    parser.add_argument("--string-margin-x-cm", type=float, default=30.0, help="Clearance from reference X faces")
    parser.add_argument("--central-gap-cm", type=float, default=30.0, help="Gap around the central VD plane")
    parser.add_argument("--string-radius-cm", type=float, default=0.20, help="Legacy visual string half-thickness")
    parser.add_argument("--housing-x-cm", type=float, default=4.0, help="Invisible LAr node-envelope X size")
    parser.add_argument("--housing-y-cm", type=float, default=1.10, help="Invisible LAr node-envelope Y size")
    parser.add_argument("--housing-z-cm", type=float, default=2.20, help="Invisible LAr node-envelope Z size")
    parser.add_argument("--carrier-x-cm", type=float, default=3.20, help="DPSU carrier plate X size")
    parser.add_argument("--carrier-y-cm", type=float, default=0.22, help="DPSU carrier plate Y size")
    parser.add_argument("--carrier-z-cm", type=float, default=1.50, help="DPSU carrier plate Z size")
    parser.add_argument("--sensor-x-cm", type=float, default=0.90, help="Sensitive tile X size")
    parser.add_argument("--sensor-y-cm", type=float, default=0.12, help="Sensitive tile Y size")
    parser.add_argument("--sensor-z-cm", type=float, default=0.80, help="Sensitive tile Z size")
    parser.add_argument("--board-x-cm", type=float, default=1.30, help="Digital readout ASIC/board X size")
    parser.add_argument("--board-y-cm", type=float, default=0.12, help="Digital readout ASIC/board Y size")
    parser.add_argument("--board-z-cm", type=float, default=0.90, help="Digital readout ASIC/board Z size")
    parser.add_argument("--coupler-x-cm", type=float, default=0.55, help="Fiber interface X size")
    parser.add_argument("--coupler-y-cm", type=float, default=0.20, help="Fiber interface Y size")
    parser.add_argument("--coupler-z-cm", type=float, default=0.25, help="Fiber interface Z size")
    parser.add_argument("--clamp-x-cm", type=float, default=3.60, help="Node clamp X size")
    parser.add_argument("--clamp-y-cm", type=float, default=0.08, help="Node clamp Y size")
    parser.add_argument("--clamp-z-cm", type=float, default=0.12, help="Node clamp Z size")
    parser.add_argument("--core-cable-y-cm", type=float, default=0.20, help="Power/support fiber Y size")
    parser.add_argument("--core-cable-z-cm", type=float, default=0.20, help="Power/support fiber Z size")
    parser.add_argument("--core-cable-offset-y-cm", type=float, default=0.0, help="Support cable Y offset")
    parser.add_argument("--core-cable-offset-z-cm", type=float, default=-1.35, help="Support/power fiber Z offset")
    parser.add_argument("--fiber-y-cm", type=float, default=0.12, help="Optical fiber Y size")
    parser.add_argument("--fiber-z-cm", type=float, default=0.12, help="Optical fiber Z size")
    parser.add_argument("--fiber-offset-y-cm", type=float, default=0.0, help="Optical fiber Y offset")
    parser.add_argument("--fiber-offset-z-cm", type=float, default=1.35, help="Signal fiber Z offset")
    parser.add_argument("--material", default="LAr", help="Legacy placeholder material")
    parser.add_argument("--sensor-material", default="SILICON_Si", help="Sensitive tile material")
    parser.add_argument("--housing-material", default="LAr", help="Housing envelope material")
    parser.add_argument("--carrier-material", default="FR4", help="DPSU carrier plate material")
    parser.add_argument("--board-material", default="FR4", help="Readout board material")
    parser.add_argument("--cable-material", default="FR4", help="Dielectric support cable material")
    parser.add_argument("--fiber-material", default="SiO2", help="Optical fiber placeholder material")
    parser.add_argument("--clamp-material", default="FR4", help="Clamp material")
    parser.add_argument("--copy-number-base", type=int, default=250200, help="First string copy number")
    parser.add_argument("--force", action="store_true", help="Remove existing FastDPSU paper placements first")
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


def find_box_dimensions(root: ET.Element, volume: ET.Element) -> tuple[float, float, float]:
    solidref = volume.find("solidref")
    if solidref is None or not solidref.get("ref"):
        raise RuntimeError(f"Volume {volume.get('name')} has no solidref")
    solid = find_solid(root, solidref.get("ref"))
    if solid is None:
        raise RuntimeError(f"Could not find solid {solidref.get('ref')}")
    if solid.tag != "box":
        raise RuntimeError(f"Expected {solidref.get('ref')} to be a box, got {solid.tag}")
    if solid.get("lunit", "cm") != "cm":
        raise RuntimeError("Only cm box units are supported")
    return float(solid.get("x")), float(solid.get("y")), float(solid.get("z"))


def find_child_position(parent: ET.Element, child_volume_name: str) -> tuple[float, float, float]:
    for physvol in parent.findall("physvol"):
        volumeref = physvol.find("volumeref")
        if volumeref is None or volumeref.get("ref") != child_volume_name:
            continue
        position = physvol.find("position")
        if position is None:
            positionref = physvol.find("positionref")
            if positionref is not None:
                raise RuntimeError(
                    f"Reference volume {child_volume_name} uses a positionref; "
                    "inline positions are required by this simple patcher"
                )
            return 0.0, 0.0, 0.0
        if position.get("unit", "cm") != "cm":
            raise RuntimeError("Only cm child positions are supported")
        return (
            float(position.get("x", "0")),
            float(position.get("y", "0")),
            float(position.get("z", "0")),
        )
    raise RuntimeError(f"Could not find {child_volume_name} as a child of {parent.get('name')}")


def ensure_box(solids: ET.Element, name: str, x: float, y: float, z: float) -> None:
    for box in solids.findall("box"):
        if box.get("name") == name:
            box.set("x", f"{x:.8g}")
            box.set("y", f"{y:.8g}")
            box.set("z", f"{z:.8g}")
            box.set("lunit", "cm")
            return
    ET.SubElement(
        solids,
        "box",
        {
            "name": name,
            "lunit": "cm",
            "x": f"{x:.8g}",
            "y": f"{y:.8g}",
            "z": f"{z:.8g}",
        },
    )


def volume_index(structure: ET.Element, name: str) -> Optional[int]:
    for index, child in enumerate(list(structure)):
        if child.tag == "volume" and child.get("name") == name:
            return index
    return None


def ensure_volume_before(
    structure: ET.Element,
    name: str,
    material: str,
    solid: str,
    before: str,
) -> ET.Element:
    for volume in structure.findall("volume"):
        if volume.get("name") == name:
            volume[:] = []
            ET.SubElement(volume, "materialref", {"ref": material})
            ET.SubElement(volume, "solidref", {"ref": solid})
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
    ET.SubElement(volume, "auxiliary", {"auxtype": "SensDet", "auxvalue": "PhotonDetector"})


def centered_positions(count: int, pitch: float) -> list[float]:
    if count <= 0:
        raise ValueError("counts must be positive")
    center = 0.5 * (count - 1)
    return [(index - center) * pitch for index in range(count)]


def split_string_node_positions(count: int, pitch: float, central_gap: float) -> list[float]:
    if count <= 0:
        raise ValueError("counts must be positive")
    if count % 2 != 0:
        raise ValueError("paper-style split strings require an even node count")
    per_half = count // 2
    positive = [0.5 * central_gap + 0.5 * pitch + index * pitch for index in range(per_half)]
    return [-x for x in reversed(positive)] + positive


def validate_span(name: str, count: int, pitch: float, available: float) -> None:
    span = (count - 1) * pitch
    if span > available:
        raise RuntimeError(
            f"{name} span {span:g} cm exceeds available reference dimension {available:g} cm"
        )


def span_with_offset(size: float, offset: float) -> float:
    return size + 2.0 * abs(offset)


def make_physvol(
    ref: str,
    name: str,
    x: float,
    y: float,
    z: float,
    copy_number: Optional[int] = None,
) -> ET.Element:
    attrs = {}
    if copy_number is not None:
        attrs["copynumber"] = str(copy_number)
    physvol = ET.Element("physvol", attrs)
    ET.SubElement(physvol, "volumeref", {"ref": ref})
    ET.SubElement(
        physvol,
        "position",
        {
            "name": name,
            "unit": "cm",
            "x": f"{x:.8g}",
            "y": f"{y:.8g}",
            "z": f"{z:.8g}",
        },
    )
    return physvol


def remove_existing_paper_strings(parent: ET.Element) -> int:
    removed = 0
    for physvol in list(parent.findall("physvol")):
        volumeref = physvol.find("volumeref")
        position = physvol.find("position")
        if volumeref is None or position is None:
            continue
        if volumeref.get("ref") != "volFastDPSUString":
            continue
        if not position.get("name", "").startswith("posFastDPSUString_paper_"):
            continue
        parent.remove(physvol)
        removed += 1
    return removed


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
    reference = find_volume(root, args.reference_volume)
    if reference is None:
        raise RuntimeError(f"Could not find reference volume {args.reference_volume}")

    parent_x, parent_y, parent_z = find_box_dimensions(root, parent)
    ref_x, ref_y, ref_z = find_box_dimensions(root, reference)
    ref_pos_x, ref_pos_y, ref_pos_z = find_child_position(parent, args.reference_volume)

    validate_span("Y string grid", args.strings_y, args.pitch_cm, ref_y)
    validate_span("Z string grid", args.strings_z, args.pitch_cm, ref_z)
    validate_span("X DPSU nodes", args.nodes_per_string, args.pitch_cm, ref_x)

    string_x = ref_x - 2.0 * args.string_margin_x_cm
    if args.central_gap_cm < 0.0:
        raise RuntimeError("Central gap must be non-negative")
    if args.nodes_per_string % 2 != 0:
        raise RuntimeError("The paper-style split string layout requires an even nodes-per-string value")
    node_xs = split_string_node_positions(args.nodes_per_string, args.pitch_cm, args.central_gap_cm)
    outer_node_extent = max(abs(x) for x in node_xs) + 0.5 * args.housing_x_cm
    if 2.0 * outer_node_extent > string_x:
        raise RuntimeError("String X length is too short for the requested node pitch and count")
    if args.central_gap_cm >= string_x:
        raise RuntimeError("Central gap is wider than the string envelope")
    if string_x + 2.0 * abs(ref_pos_x) > parent_x:
        raise RuntimeError("String envelope would not fit inside parent X dimension")

    prefix = "FastDPSU"
    string_y = max(2.0 * args.string_radius_cm, args.housing_y_cm)
    string_z = max(2.0 * args.string_radius_cm, args.housing_z_cm)
    string_y = max(
        string_y,
        span_with_offset(args.core_cable_y_cm, args.core_cable_offset_y_cm),
        span_with_offset(args.fiber_y_cm, args.fiber_offset_y_cm),
    )
    string_z = max(
        string_z,
        span_with_offset(args.core_cable_z_cm, args.core_cable_offset_z_cm),
        span_with_offset(args.fiber_z_cm, args.fiber_offset_z_cm),
    )
    string_segment_x = 0.5 * (string_x - args.central_gap_cm)
    string_segment_center_x = 0.5 * args.central_gap_cm + 0.5 * string_segment_x
    ensure_box(solids, f"{prefix}StringEnvelopeSolid", string_x, string_y, string_z)
    ensure_box(solids, f"{prefix}HousingSolid", args.housing_x_cm, args.housing_y_cm, args.housing_z_cm)
    ensure_box(solids, f"{prefix}CarrierPlateSolid", args.carrier_x_cm, args.carrier_y_cm, args.carrier_z_cm)
    ensure_box(solids, f"{prefix}SensitiveSolid", args.sensor_x_cm, args.sensor_y_cm, args.sensor_z_cm)
    ensure_box(solids, f"{prefix}ReadoutBoardSolid", args.board_x_cm, args.board_y_cm, args.board_z_cm)
    ensure_box(solids, f"{prefix}FiberCouplerSolid", args.coupler_x_cm, args.coupler_y_cm, args.coupler_z_cm)
    ensure_box(solids, f"{prefix}NodeClampSolid", args.clamp_x_cm, args.clamp_y_cm, args.clamp_z_cm)
    ensure_box(
        solids,
        f"{prefix}StringCoreCableSolid",
        string_segment_x,
        args.core_cable_y_cm,
        args.core_cable_z_cm,
    )
    ensure_box(solids, f"{prefix}OpticalFiberSolid", string_segment_x, args.fiber_y_cm, args.fiber_z_cm)

    sensor = ensure_volume_before(
        structure,
        "volOpDetSensitive_FastDPSU",
        args.sensor_material,
        f"{prefix}SensitiveSolid",
        args.parent,
    )
    add_sensor_aux(sensor)

    ensure_volume_before(
        structure,
        "volFastDPSUCarrierPlate",
        args.carrier_material,
        f"{prefix}CarrierPlateSolid",
        args.parent,
    )
    ensure_volume_before(
        structure,
        "volFastDPSUReadoutBoard",
        args.board_material,
        f"{prefix}ReadoutBoardSolid",
        args.parent,
    )
    ensure_volume_before(
        structure,
        "volFastDPSUFiberCoupler",
        args.fiber_material,
        f"{prefix}FiberCouplerSolid",
        args.parent,
    )
    ensure_volume_before(
        structure,
        "volFastDPSUNodeClamp",
        args.clamp_material,
        f"{prefix}NodeClampSolid",
        args.parent,
    )
    ensure_volume_before(
        structure,
        "volFastDPSUStringCoreCable",
        args.cable_material,
        f"{prefix}StringCoreCableSolid",
        args.parent,
    )
    ensure_volume_before(
        structure,
        "volFastDPSUOpticalFiber",
        args.fiber_material,
        f"{prefix}OpticalFiberSolid",
        args.parent,
    )

    housing = ensure_volume_before(
        structure,
        "volFastDPSUHousing",
        args.housing_material,
        f"{prefix}HousingSolid",
        args.parent,
    )
    front_y = 0.5 * args.carrier_y_cm + 0.5 * args.sensor_y_cm + 0.02
    back_y = -0.5 * args.carrier_y_cm - 0.5 * args.board_y_cm - 0.02
    sensor_x = 0.0
    sensor_z = 0.0
    board_x = -0.15
    board_z = 0.0
    coupler_y = front_y + 0.5 * args.sensor_y_cm + 0.5 * args.coupler_y_cm + 0.02
    coupler_z = 0.5 * args.carrier_z_cm - 0.5 * args.coupler_z_cm
    clamp_y = back_y - 0.5 * args.board_y_cm - 0.5 * args.clamp_y_cm - 0.02
    clamp_z = 0.5 * args.carrier_z_cm - 0.5 * args.clamp_z_cm
    housing.append(make_physvol("volFastDPSUCarrierPlate", "posFastDPSUCarrierPlate", 0.0, 0.0, 0.0))
    housing.append(make_physvol("volFastDPSUReadoutBoard", "posFastDPSUReadoutBoard", board_x, back_y, board_z))
    housing.append(make_physvol("volOpDetSensitive_FastDPSU", "posFastDPSUSensitiveTile", sensor_x, front_y, sensor_z))
    housing.append(
        make_physvol(
            "volFastDPSUFiberCoupler",
            "posFastDPSUPowerFiberInterface",
            -0.9,
            coupler_y,
            -coupler_z,
        )
    )
    housing.append(
        make_physvol(
            "volFastDPSUFiberCoupler",
            "posFastDPSUSignalFiberInterface",
            0.9,
            coupler_y,
            coupler_z,
        )
    )
    housing.append(
        make_physvol(
            "volFastDPSUNodeClamp",
            "posFastDPSUNodeClampPowerFiber",
            0.0,
            clamp_y,
            -clamp_z,
        )
    )
    housing.append(
        make_physvol(
            "volFastDPSUNodeClamp",
            "posFastDPSUNodeClampSignalFiber",
            0.0,
            clamp_y,
            clamp_z,
        )
    )

    string = ensure_volume_before(
        structure,
        "volFastDPSUString",
        args.material,
        f"{prefix}StringEnvelopeSolid",
        args.parent,
    )
    for side, label in [(-1.0, "negative_x"), (1.0, "positive_x")]:
        string.append(
            make_physvol(
                "volFastDPSUStringCoreCable",
                f"posFastDPSUStringCoreCable_{label}",
                side * string_segment_center_x,
                args.core_cable_offset_y_cm,
                args.core_cable_offset_z_cm,
            )
        )
        string.append(
            make_physvol(
                "volFastDPSUOpticalFiber",
                f"posFastDPSUOpticalFiber_{label}",
                side * string_segment_center_x,
                args.fiber_offset_y_cm,
                args.fiber_offset_z_cm,
            )
        )

    for inode, x in enumerate(node_xs):
        string.append(
            make_physvol(
                "volFastDPSUHousing",
                f"posFastDPSUNodeOnString_{inode:02d}",
                x,
                0.0,
                0.0,
                inode,
            )
        )

    removed = remove_existing_paper_strings(parent) if args.force else 0
    existing = {
        pos.get("name")
        for pos in parent.findall("physvol/position")
        if pos is not None and pos.get("name")
    }

    y_offsets = centered_positions(args.strings_y, args.pitch_cm)
    z_offsets = centered_positions(args.strings_z, args.pitch_cm)
    added = 0
    for iy, y_offset in enumerate(y_offsets):
        for iz, z_offset in enumerate(z_offsets):
            pos_name = f"posFastDPSUString_paper_y{iy:02d}_z{iz:02d}"
            if pos_name in existing:
                continue
            copy_number = args.copy_number_base + iy * args.strings_z + iz
            parent.append(
                make_physvol(
                    "volFastDPSUString",
                    pos_name,
                    ref_pos_x,
                    ref_pos_y + y_offset,
                    ref_pos_z + z_offset,
                    copy_number,
                )
            )
            added += 1

    total_strings = args.strings_y * args.strings_z
    total_nodes = total_strings * args.nodes_per_string
    print(f"Parent volume: {args.parent} ({parent_x:g}, {parent_y:g}, {parent_z:g}) cm")
    print(f"Reference volume: {args.reference_volume} ({ref_x:g}, {ref_y:g}, {ref_z:g}) cm")
    print(f"Reference position in parent: x={ref_pos_x:g}, y={ref_pos_y:g}, z={ref_pos_z:g} cm")
    print(f"Paper lattice: {args.strings_y} x {args.strings_z} strings, {args.nodes_per_string} nodes/string")
    print(
        f"Pitch: {args.pitch_cm:g} cm; string X length: {string_x:g} cm; "
        f"central gap: {args.central_gap_cm:g} cm"
    )
    print(f"Total intended markers: {total_strings} strings, {total_nodes} DPSUs")
    print(f"Removed existing paper string placements: {removed}; added: {added}")
    print("Note: global cryostat placement is intended for visualization/early studies; check overlaps before simulation.")

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
