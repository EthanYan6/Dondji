#!/usr/bin/env python3
"""
将开机 Logo PNG 转为 ST7565 / gFrameBuffer 字节布局（每字节 8 个垂直像素，
bit0 对应组内最上一行像素，与 UI_DrawPixelBuffer 一致）。
"""
from __future__ import annotations

import argparse
import os
import sys

try:
    from PIL import Image
except ImportError:
    print("需要 Pillow: pip install pillow", file=sys.stderr)
    sys.exit(1)


def pack_framebuffer_rows(
    pixels_1bit: list[list[int]], width: int, height: int
) -> list[list[int]]:
    """pixels_1bit[y][x] 非零表示点亮（黑）。返回 FRAME_LINES 行，每行 width 字节。"""
    if height % 8 != 0:
        raise ValueError("height 必须为 8 的倍数")

    frame_lines = height // 8
    rows_out: list[list[int]] = []
    for line in range(frame_lines):
        row_bytes: list[int] = []
        for x in range(width):
            byte_val = 0
            for bit in range(8):
                y = line * 8 + bit
                if y < height and pixels_1bit[y][x]:
                    byte_val |= 1 << bit
            row_bytes.append(byte_val)
        rows_out.append(row_bytes)
    return rows_out


def image_to_bitmap(
    img: Image.Image,
    out_w: int,
    out_h: int,
    zoom: float = 1.0,
    inset: int = 0,
) -> list[list[int]]:
    gray = img.convert("L")
    # 黑字白底：暗于阈值视为前景
    threshold = 128
    bbox = None
    try:
        bw = gray.point(lambda p: 0 if p < threshold else 255)
        bbox = bw.getbbox()
    except Exception:
        bbox = None
    if bbox:
        gray = gray.crop(bbox)

    inner_w = out_w - 2 * inset
    inner_h = out_h - 2 * inset
    if inner_w < 1 or inner_h < 1:
        raise ValueError("inset 过大，有效绘制区域为空")

    gw, gh = gray.size
    scale = min(inner_w / gw, inner_h / gh) * zoom
    nw = max(1, int(gw * scale))
    nh = max(1, int(gh * scale))
    resized = gray.resize((nw, nh), Image.Resampling.LANCZOS)

    if nw >= inner_w and nh >= inner_h:
        left = (nw - inner_w) // 2
        top = (nh - inner_h) // 2
        inner_canvas = resized.crop((left, top, left + inner_w, top + inner_h))
    else:
        inner_canvas = Image.new("L", (inner_w, inner_h), color=255)
        ox = (inner_w - nw) // 2
        oy = (inner_h - nh) // 2
        inner_canvas.paste(resized, (ox, oy))

    canvas = Image.new("L", (out_w, out_h), color=255)
    canvas.paste(inner_canvas, (inset, inset))

    bw = canvas.point(lambda p: 0 if p < threshold else 255)
    pixels_1bit: list[list[int]] = []
    for y in range(out_h):
        row = []
        for x in range(out_w):
            p = bw.getpixel((x, y))
            row.append(1 if p < threshold else 0)
        pixels_1bit.append(row)
    return pack_framebuffer_rows(pixels_1bit, out_w, out_h)


def emit_c_source(path: str, arr_name: str, rows: list[list[int]]) -> None:
    lines = [
        "/* 由 App/tools/gen_boot_logo.py 生成 — 请勿手改 */",
        "",
        '#include "ui/boot_logo_bitmap.h"',
        "",
        f"const uint8_t {arr_name}[BOOT_LOGO_FRAME_LINES][128] = {{",
    ]
    for ri, row in enumerate(rows):
        parts = ", ".join(f"0x{b:02X}" for b in row)
        sep = "," if ri < len(rows) - 1 else ""
        lines.append(f"    {{ {parts} }}{sep}")
    lines.append("};")
    lines.append("")
    out_text = "\n".join(lines)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(out_text)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input_png", help="输入 PNG 路径")
    parser.add_argument(
        "-o",
        "--output",
        default="boot_logo_bitmap.c",
        help="输出 .c 路径（默认 boot_logo_bitmap.c）",
    )
    parser.add_argument("--width", type=int, default=128)
    parser.add_argument("--height", type=int, default=48)
    parser.add_argument(
        "--zoom",
        type=float,
        default=2.0,
        help="在「适配画布」的比例上再乘 zoom（2=宽高视觉放大一倍，超出部分中心裁剪）",
    )
    parser.add_argument(
        "--inset",
        type=int,
        default=5,
        help="四边留白（像素）：图案仅在中间区域缩放绘制，相当于四周各缩小 inset",
    )
    args = parser.parse_args()

    img = Image.open(args.input_png)
    rows = image_to_bitmap(img, args.width, args.height, args.zoom, args.inset)
    emit_c_source(args.output, "gBootLogoBitmap", rows)
    print(f"写入 {args.output}，高度 {len(rows) * 8}px")


if __name__ == "__main__":
    main()
