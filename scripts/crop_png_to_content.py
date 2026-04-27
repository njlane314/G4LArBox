#!/usr/bin/env python3
"""Crop white margins from simple ROOT-produced PNG files.

This intentionally avoids external image libraries so it works in the thesis
build environment used here. It supports the 8-bit RGB/RGBA, non-interlaced PNGs
written by ROOT canvases.
"""

from __future__ import annotations

import argparse
import binascii
import struct
import sys
import zlib
from pathlib import Path


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def paeth(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def read_chunks(data: bytes):
    if not data.startswith(PNG_SIGNATURE):
        raise ValueError("not a PNG file")
    offset = len(PNG_SIGNATURE)
    while offset < len(data):
        length = struct.unpack(">I", data[offset : offset + 4])[0]
        kind = data[offset + 4 : offset + 8]
        payload = data[offset + 8 : offset + 8 + length]
        yield kind, payload
        offset += 12 + length


def unfilter_scanlines(raw: bytes, width: int, height: int, channels: int) -> list[bytearray]:
    stride = width * channels
    rows: list[bytearray] = []
    offset = 0
    previous = bytearray(stride)
    for _ in range(height):
        filter_type = raw[offset]
        offset += 1
        row = bytearray(raw[offset : offset + stride])
        offset += stride

        if filter_type == 0:
            pass
        elif filter_type == 1:
            for i in range(stride):
                left = row[i - channels] if i >= channels else 0
                row[i] = (row[i] + left) & 0xFF
        elif filter_type == 2:
            for i in range(stride):
                row[i] = (row[i] + previous[i]) & 0xFF
        elif filter_type == 3:
            for i in range(stride):
                left = row[i - channels] if i >= channels else 0
                up = previous[i]
                row[i] = (row[i] + ((left + up) // 2)) & 0xFF
        elif filter_type == 4:
            for i in range(stride):
                left = row[i - channels] if i >= channels else 0
                up = previous[i]
                upper_left = previous[i - channels] if i >= channels else 0
                row[i] = (row[i] + paeth(left, up, upper_left)) & 0xFF
        else:
            raise ValueError(f"unsupported PNG filter type {filter_type}")

        rows.append(row)
        previous = row
    return rows


def load_png(path: Path):
    data = path.read_bytes()
    idat = bytearray()
    width = height = bit_depth = color_type = interlace = None
    for kind, payload in read_chunks(data):
        if kind == b"IHDR":
            width, height, bit_depth, color_type, _, _, interlace = struct.unpack(">IIBBBBB", payload)
        elif kind == b"IDAT":
            idat.extend(payload)

    if width is None or height is None or bit_depth is None or color_type is None:
        raise ValueError("missing IHDR")
    if bit_depth != 8 or interlace != 0:
        raise ValueError("only 8-bit non-interlaced PNGs are supported")
    channels_by_type = {2: 3, 6: 4}
    if color_type not in channels_by_type:
        raise ValueError(f"unsupported PNG color type {color_type}")

    channels = channels_by_type[color_type]
    rows = unfilter_scanlines(zlib.decompress(bytes(idat)), width, height, channels)
    return width, height, bit_depth, color_type, channels, rows


def content_bounds(rows: list[bytearray], width: int, height: int, channels: int, threshold: int):
    xmin, ymin = width, height
    xmax, ymax = -1, -1
    for y, row in enumerate(rows):
        for x in range(width):
            i = x * channels
            r, g, b = row[i], row[i + 1], row[i + 2]
            alpha = row[i + 3] if channels == 4 else 255
            if alpha > 0 and min(r, g, b) < threshold:
                xmin = min(xmin, x)
                xmax = max(xmax, x)
                ymin = min(ymin, y)
                ymax = max(ymax, y)
    if xmax < xmin or ymax < ymin:
        return 0, 0, width - 1, height - 1
    return xmin, ymin, xmax, ymax


def chunk(kind: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + kind
        + payload
        + struct.pack(">I", binascii.crc32(kind + payload) & 0xFFFFFFFF)
    )


def save_png(path: Path, width: int, height: int, color_type: int, rows: list[bytearray]):
    raw = bytearray()
    for row in rows:
        raw.append(0)
        raw.extend(row)
    payload = struct.pack(">IIBBBBB", width, height, 8, color_type, 0, 0, 0)
    encoded = (
        PNG_SIGNATURE
        + chunk(b"IHDR", payload)
        + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + chunk(b"IEND", b"")
    )
    path.write_bytes(encoded)


def crop_one(path: Path, threshold: int, padding: int, dry_run: bool = False) -> bool:
    width, height, _, color_type, channels, rows = load_png(path)
    xmin, ymin, xmax, ymax = content_bounds(rows, width, height, channels, threshold)
    xmin = max(0, xmin - padding)
    ymin = max(0, ymin - padding)
    xmax = min(width - 1, xmax + padding)
    ymax = min(height - 1, ymax + padding)

    new_width = xmax - xmin + 1
    new_height = ymax - ymin + 1
    if new_width == width and new_height == height:
        print(f"{path}: unchanged {width}x{height}")
        return False

    cropped = [row[xmin * channels : (xmax + 1) * channels] for row in rows[ymin : ymax + 1]]
    print(f"{path}: {width}x{height} -> {new_width}x{new_height}")
    if not dry_run:
        save_png(path, new_width, new_height, color_type, cropped)
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("images", nargs="+", type=Path)
    parser.add_argument("--threshold", type=int, default=252)
    parser.add_argument("--padding", type=int, default=8)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    status = 0
    for image in args.images:
        try:
            crop_one(image, args.threshold, args.padding, args.dry_run)
        except Exception as exc:
            print(f"{image}: {exc}", file=sys.stderr)
            status = 1
    return status


if __name__ == "__main__":
    raise SystemExit(main())
