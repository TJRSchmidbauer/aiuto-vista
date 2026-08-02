#!/usr/bin/env python3
"""Convert an 8-bit, non-interlaced RGB/RGBA PNG to an LVGL RGB565 C asset."""

import argparse
import struct
import zlib
from pathlib import Path


def read_png(path: Path):
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG file")

    position = 8
    compressed = bytearray()
    width = height = color_type = None
    while position < len(data):
        length = struct.unpack(">I", data[position:position + 4])[0]
        kind = data[position + 4:position + 8]
        payload = data[position + 8:position + 8 + length]
        position += length + 12
        if kind == b"IHDR":
            width, height, depth, color_type, compression, filtering, interlace = struct.unpack(
                ">IIBBBBB", payload
            )
            if depth != 8 or color_type not in (2, 6) or compression or filtering or interlace:
                raise ValueError("only 8-bit, non-interlaced RGB/RGBA PNGs are supported")
        elif kind == b"IDAT":
            compressed.extend(payload)
        elif kind == b"IEND":
            break

    channels = 4 if color_type == 6 else 3
    stride = width * channels
    packed = zlib.decompress(compressed)
    rows = []
    previous = bytearray(stride)
    offset = 0
    for _ in range(height):
        filter_type = packed[offset]
        source = packed[offset + 1:offset + 1 + stride]
        offset += stride + 1
        row = bytearray(stride)
        for index, byte in enumerate(source):
            left = row[index - channels] if index >= channels else 0
            above = previous[index]
            upper_left = previous[index - channels] if index >= channels else 0
            if filter_type == 0:
                predictor = 0
            elif filter_type == 1:
                predictor = left
            elif filter_type == 2:
                predictor = above
            elif filter_type == 3:
                predictor = (left + above) // 2
            elif filter_type == 4:
                estimate = left + above - upper_left
                distances = (abs(estimate - left), abs(estimate - above), abs(estimate - upper_left))
                predictor = (left, above, upper_left)[distances.index(min(distances))]
            else:
                raise ValueError(f"unsupported PNG filter {filter_type}")
            row[index] = (byte + predictor) & 0xFF
        rows.append(row)
        previous = row
    return width, height, channels, rows


def convert(input_path: Path, output_path: Path, symbol: str):
    width, height, channels, rows = read_png(input_path)
    pixels = []
    for row in rows:
        for position in range(0, len(row), channels):
            red, green, blue = row[position:position + 3]
            alpha = row[position + 3] if channels == 4 else 255
            rgb565 = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)
            pixels.extend((rgb565 & 0xFF, rgb565 >> 8, alpha))

    lines = []
    for position in range(0, len(pixels), 18):
        values = ", ".join(f"0x{value:02X}" for value in pixels[position:position + 18])
        lines.append(f"    {values},")

    output_path.write_text(
        '#include "ui.h"\n\n'
        '#ifndef LV_ATTRIBUTE_MEM_ALIGN\n#define LV_ATTRIBUTE_MEM_ALIGN\n#endif\n\n'
        f'const LV_ATTRIBUTE_MEM_ALIGN uint8_t {symbol}_data[] = {{\n'
        + "\n".join(lines)
        + f'\n}};\n\nconst lv_img_dsc_t {symbol} = {{\n'
          '    .header.always_zero = 0,\n'
        f'    .header.w = {width},\n'
        f'    .header.h = {height},\n'
        f'    .data_size = sizeof({symbol}_data),\n'
          '    .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,\n'
        f'    .data = {symbol}_data\n}};\n',
        encoding="utf-8",
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("symbol")
    arguments = parser.parse_args()
    convert(arguments.input, arguments.output, arguments.symbol)
