#!/usr/bin/env python3
"""Write a standalone concept GDML for one fiber-coupled FastDPSU device.

The paper gives a functional block diagram rather than a mechanical drawing, so
this GDML is an engineering abstraction of the stated components: dSiPM photo
sensors, readout electronics, optical power conversion, silicon photonics, and
dielectric fiber coupling.  Dimensions are deliberately small and explicit so
they can be revised once a real mechanical design exists.
"""

from __future__ import annotations

import argparse
import xml.etree.ElementTree as ET
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("gdml/fastdpsu_device_concept.gdml"),
        help="Output GDML path",
    )
    return parser.parse_args()


def add_box(solids: ET.Element, name: str, x: float, y: float, z: float) -> None:
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


def add_sphere(solids: ET.Element, name: str, rmax: float) -> None:
    ET.SubElement(
        solids,
        "sphere",
        {
            "name": name,
            "lunit": "cm",
            "aunit": "deg",
            "rmin": "0",
            "rmax": f"{rmax:.8g}",
            "startphi": "0",
            "deltaphi": "360",
            "starttheta": "0",
            "deltatheta": "180",
        },
    )


def add_volume(structure: ET.Element, name: str, material: str, solid: str, sensitive: bool = False) -> ET.Element:
    volume = ET.SubElement(structure, "volume", {"name": name})
    ET.SubElement(volume, "materialref", {"ref": material})
    ET.SubElement(volume, "solidref", {"ref": solid})
    if sensitive:
        ET.SubElement(volume, "auxiliary", {"auxtype": "SensDet", "auxvalue": "PhotonDetector"})
    return volume


def add_placement(parent: ET.Element, ref: str, name: str, x: float, y: float, z: float) -> None:
    physvol = ET.SubElement(parent, "physvol")
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
    gdml = ET.Element(
        "gdml",
        {
            "xmlns:xsi": "http://www.w3.org/2001/XMLSchema-instance",
            "xsi:noNamespaceSchemaLocation": "http://service-spi.web.cern.ch/service-spi/app/releases/GDML/schema/gdml.xsd",
        },
    )
    define = ET.SubElement(gdml, "define")
    ET.SubElement(define, "position", {"name": "center", "unit": "cm", "x": "0", "y": "0", "z": "0"})

    materials = ET.SubElement(gdml, "materials")
    for name, formula, z, atom in [
        ("hydrogen", "H", "1", "1.00794"),
        ("carbon", "C", "6", "12.0107"),
        ("nitrogen", "N", "7", "14.0067"),
        ("oxygen", "O", "8", "15.999"),
        ("argon", "Ar", "18", "39.948"),
        ("silicon", "Si", "14", "28.0855"),
        ("copper", "Cu", "29", "63.546"),
    ]:
        element = ET.SubElement(materials, "element", {"name": name, "formula": formula, "Z": z})
        ET.SubElement(element, "atom", {"value": atom})

    air = ET.SubElement(materials, "material", {"name": "Air", "state": "gas"})
    ET.SubElement(air, "D", {"value": "0.001205", "unit": "g/cm3"})
    ET.SubElement(air, "fraction", {"n": "0.781154", "ref": "nitrogen"})
    ET.SubElement(air, "fraction", {"n": "0.209476", "ref": "oxygen"})
    ET.SubElement(air, "fraction", {"n": "0.00937", "ref": "argon"})

    epoxy = ET.SubElement(materials, "material", {"name": "FR4_Approx"})
    ET.SubElement(epoxy, "D", {"value": "1.85", "unit": "g/cm3"})
    ET.SubElement(epoxy, "fraction", {"n": "0.38", "ref": "carbon"})
    ET.SubElement(epoxy, "fraction", {"n": "0.08", "ref": "hydrogen"})
    ET.SubElement(epoxy, "fraction", {"n": "0.30", "ref": "oxygen"})
    ET.SubElement(epoxy, "fraction", {"n": "0.24", "ref": "silicon"})

    silica = ET.SubElement(materials, "material", {"name": "Silica"})
    ET.SubElement(silica, "D", {"value": "2.20", "unit": "g/cm3"})
    ET.SubElement(silica, "fraction", {"n": "0.4674", "ref": "silicon"})
    ET.SubElement(silica, "fraction", {"n": "0.5326", "ref": "oxygen"})

    silicon_mat = ET.SubElement(materials, "material", {"name": "Silicon"})
    ET.SubElement(silicon_mat, "D", {"value": "2.33", "unit": "g/cm3"})
    ET.SubElement(silicon_mat, "composite", {"n": "1", "ref": "silicon"})

    copper_mat = ET.SubElement(materials, "material", {"name": "Copper"})
    ET.SubElement(copper_mat, "D", {"value": "8.96", "unit": "g/cm3"})
    ET.SubElement(copper_mat, "composite", {"n": "1", "ref": "copper"})

    solids = ET.SubElement(gdml, "solids")
    add_box(solids, "WorldSolid", 12.0, 6.0, 6.0)
    add_box(solids, "PowerFiberSolid", 8.0, 0.08, 0.08)
    add_box(solids, "SignalFiberSolid", 8.0, 0.06, 0.06)
    add_sphere(solids, "SphericalNodeEnvelopeSolid", 1.75)
    add_box(solids, "CarrierPlateSolid", 2.5, 0.18, 1.35)
    add_box(solids, "ReadoutElectronicsSolid", 1.05, 0.10, 0.70)
    add_box(solids, "SensitiveTileSolid", 0.75, 0.10, 0.65)
    add_box(solids, "OpticalPowerConverterSolid", 0.48, 0.14, 0.28)
    add_box(solids, "SiliconPhotonicsSolid", 0.48, 0.14, 0.28)
    add_box(solids, "FiberFerruleSolid", 0.62, 0.20, 0.24)
    add_box(solids, "FiberClipSolid", 3.6, 0.08, 0.12)

    structure = ET.SubElement(gdml, "structure")
    add_volume(structure, "volFastDPSUPowerFiber", "Silica", "PowerFiberSolid")
    add_volume(structure, "volFastDPSUSignalFiber", "Silica", "SignalFiberSolid")
    add_volume(structure, "volFastDPSUSphericalNode", "Silica", "SphericalNodeEnvelopeSolid")
    add_volume(structure, "volFastDPSUCarrierPlate", "FR4_Approx", "CarrierPlateSolid")
    add_volume(structure, "volFastDPSUReadoutElectronics", "Silicon", "ReadoutElectronicsSolid")
    add_volume(structure, "volOpDetSensitive_FastDPSU", "Silicon", "SensitiveTileSolid", sensitive=True)
    add_volume(structure, "volFastDPSUOpticalPowerConverter", "Silicon", "OpticalPowerConverterSolid")
    add_volume(structure, "volFastDPSUSiliconPhotonics", "Silicon", "SiliconPhotonicsSolid")
    add_volume(structure, "volFastDPSUFiberFerrule", "Silica", "FiberFerruleSolid")
    add_volume(structure, "volFastDPSUFiberClip", "FR4_Approx", "FiberClipSolid")

    world = add_volume(structure, "volWorld", "Air", "WorldSolid")
    add_placement(world, "volFastDPSUPowerFiber", "posPowerFiber", 0.0, -0.34, -0.82)
    add_placement(world, "volFastDPSUSignalFiber", "posSignalFiber", 0.0, -0.34, 0.82)
    add_placement(world, "volFastDPSUSphericalNode", "posSphericalNode", 0.0, 0.0, 0.0)
    add_placement(world, "volFastDPSUCarrierPlate", "posCarrierPlate", 0.0, 0.0, 0.0)
    add_placement(world, "volFastDPSUReadoutElectronics", "posReadoutElectronics", -0.15, -0.22, 0.0)
    add_placement(world, "volOpDetSensitive_FastDPSU", "posSensitiveTile", 0.0, 0.22, 0.0)
    add_placement(world, "volFastDPSUOpticalPowerConverter", "posOpticalPowerConverter", -0.58, 0.25, -0.42)
    add_placement(world, "volFastDPSUSiliconPhotonics", "posSiliconPhotonics", 0.58, 0.25, 0.42)
    add_placement(world, "volFastDPSUFiberFerrule", "posPowerFiberFerrule", -0.72, 0.05, -0.78)
    add_placement(world, "volFastDPSUFiberFerrule", "posSignalFiberFerrule", 0.72, 0.05, 0.78)
    add_placement(world, "volFastDPSUFiberClip", "posPowerFiberClip", 0.0, 0.42, -0.82)
    add_placement(world, "volFastDPSUFiberClip", "posSignalFiberClip", 0.0, 0.42, 0.82)

    setup = ET.SubElement(gdml, "setup", {"name": "Default", "version": "1.0"})
    ET.SubElement(setup, "world", {"ref": "volWorld"})

    indent(gdml)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    ET.ElementTree(gdml).write(args.output, encoding="utf-8", xml_declaration=True)
    print(f"Wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
