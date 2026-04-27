#!/usr/bin/env python3
"""Validate the FastDPSU box hierarchy in a patched GDML file.

This is intentionally narrow: it checks the component volumes introduced by the
FastDPSU patcher, where all solids are boxes and placements are unrotated.  It
reports component counts and catches sibling overlaps/containment mistakes that
are easy to miss in detector-scale ROOT renderings.
"""

from __future__ import annotations

import argparse
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Box:
    x: float
    y: float
    z: float


@dataclass(frozen=True)
class Placement:
    ref: str
    name: str
    x: float
    y: float
    z: float


FASTDPSU_VOLUMES = [
    "volFastDPSUString",
    "volFastDPSUStringCoreCable",
    "volFastDPSUOpticalFiber",
    "volFastDPSUHousing",
    "volFastDPSUCarrierPlate",
    "volFastDPSUReadoutBoard",
    "volOpDetSensitive_FastDPSU",
    "volFastDPSUFiberCoupler",
    "volFastDPSUNodeClamp",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("gdml", type=Path)
    parser.add_argument("--tolerance-cm", type=float, default=1e-5)
    return parser.parse_args()


def volumes(root: ET.Element) -> dict[str, ET.Element]:
    return {v.get("name"): v for v in root.findall(".//volume") if v.get("name")}


def solids(root: ET.Element) -> dict[str, ET.Element]:
    return {s.get("name"): s for s in root.find("solids").findall("*") if s.get("name")}


def box_for_volume(volume: ET.Element, solid_by_name: dict[str, ET.Element]) -> Box:
    solidref = volume.find("solidref")
    if solidref is None or not solidref.get("ref"):
        raise ValueError(f"{volume.get('name')} has no solidref")
    solid = solid_by_name[solidref.get("ref")]
    if solid.tag != "box":
        raise ValueError(f"{volume.get('name')} uses non-box solid {solid.get('name')}")
    return Box(float(solid.get("x")), float(solid.get("y")), float(solid.get("z")))


def placements(volume: ET.Element) -> list[Placement]:
    out: list[Placement] = []
    for physvol in volume.findall("physvol"):
        ref = physvol.find("volumeref")
        pos = physvol.find("position")
        if ref is None or pos is None:
            continue
        out.append(
            Placement(
                ref=ref.get("ref"),
                name=pos.get("name", ""),
                x=float(pos.get("x", "0")),
                y=float(pos.get("y", "0")),
                z=float(pos.get("z", "0")),
            )
        )
    return out


def interval(center: float, length: float) -> tuple[float, float]:
    half = 0.5 * length
    return center - half, center + half


def contained(child: Placement, child_box: Box, mother_box: Box, tol: float) -> bool:
    for center, child_len, mother_len in [
        (child.x, child_box.x, mother_box.x),
        (child.y, child_box.y, mother_box.y),
        (child.z, child_box.z, mother_box.z),
    ]:
        lo, hi = interval(center, child_len)
        if lo < -0.5 * mother_len - tol or hi > 0.5 * mother_len + tol:
            return False
    return True


def overlap(a: Placement, a_box: Box, b: Placement, b_box: Box, tol: float) -> bool:
    for ac, al, bc, bl in [
        (a.x, a_box.x, b.x, b_box.x),
        (a.y, a_box.y, b.y, b_box.y),
        (a.z, a_box.z, b.z, b_box.z),
    ]:
        alo, ahi = interval(ac, al)
        blo, bhi = interval(bc, bl)
        if ahi <= blo + tol or bhi <= alo + tol:
            return False
    return True


def main() -> int:
    root = ET.parse(args.gdml).getroot()
    volume_by_name = volumes(root)
    solid_by_name = solids(root)
    boxes = {name: box_for_volume(volume_by_name[name], solid_by_name) for name in FASTDPSU_VOLUMES}

    print(f"GDML: {args.gdml}")
    for name in FASTDPSU_VOLUMES:
        b = boxes[name]
        print(f"{name:32s} box {b.x:g} x {b.y:g} x {b.z:g} cm")

    cryostat = volume_by_name["volCryostat"]
    string_count = sum(1 for p in placements(cryostat) if p.ref == "volFastDPSUString")
    string_children = placements(volume_by_name["volFastDPSUString"])
    housing_children = placements(volume_by_name["volFastDPSUHousing"])
    node_count_per_string = sum(1 for p in string_children if p.ref == "volFastDPSUHousing")
    print(f"string placements in volCryostat: {string_count}")
    print(f"node housings per string: {node_count_per_string}")
    print(f"total node housings: {string_count * node_count_per_string}")

    errors: list[str] = []
    for mother_name, children in [
        ("volFastDPSUString", string_children),
        ("volFastDPSUHousing", housing_children),
    ]:
        mother_box = boxes[mother_name]
        for child in children:
            if child.ref not in boxes:
                continue
            if not contained(child, boxes[child.ref], mother_box, args.tolerance_cm):
                errors.append(f"{child.name} ({child.ref}) is outside {mother_name}")
        for i, a in enumerate(children):
            if a.ref not in boxes:
                continue
            for b in children[i + 1 :]:
                if b.ref not in boxes:
                    continue
                if overlap(a, boxes[a.ref], b, boxes[b.ref], args.tolerance_cm):
                    errors.append(f"{mother_name}: {a.name} ({a.ref}) overlaps {b.name} ({b.ref})")

    if errors:
        print("Validation failures:")
        for error in errors:
            print(f"  - {error}")
        return 1
    print("FastDPSU hierarchy validation: OK")
    return 0


if __name__ == "__main__":
    args = parse_args()
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(2)
