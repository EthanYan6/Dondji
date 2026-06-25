"""Generate the mokuyu bitmap header and updated mokuyu.c source files."""

from PIL import Image

# --- 1. Generate bitmap data from image ---
TARGET_W = 100
TARGET_H = 48

img = Image.open(r"C:\Users\EthanYan\Downloads\mokuyu.jpg").convert("L")

# Use NEAREST to preserve pure black/white values
orig_w, orig_h = img.size
scale = min(TARGET_W / orig_w, TARGET_H / orig_h)
new_w = int(orig_w * scale)
new_h = int(orig_h * scale)
img = img.resize((new_w, new_h), Image.NEAREST)

canvas = Image.new("L", (TARGET_W, TARGET_H), 0)  # black background
ox = (TARGET_W - new_w) // 2
oy = (TARGET_H - new_h) // 2
canvas.paste(img, (ox, oy))

pixels = canvas.load()
ROWS = 6
COLS = 128

data = []
for row in range(ROWS):
    row_bytes = []
    for col in range(COLS):
        byte_val = 0
        for bit in range(8):
            py = row * 8 + bit
            px = col
            if 0 <= px < TARGET_W and 0 <= py < TARGET_H:
                # White shape pixels (>128) = LCD ON (1)
                # Black background pixels (<=128) = LCD OFF (0)
                if pixels[px, py] > 128:
                    byte_val |= (1 << bit)
        row_bytes.append(byte_val)
    data.append(row_bytes)

# --- 2. Write bitmap header file ---
bitmap_h = """#pragma once
#include <stdint.h>

// mokuyu image bitmap: 100x48 centered in 128x48 area
// framebuffer rows 0-5 (y=0..47), image starts at x=14
// White shape pixels = 1 (LCD on), black background = 0 (LCD off)
const uint8_t BITMAP_MOKUYU[6][128] = {
"""
for i, row in enumerate(data):
    hex_vals = ",".join(f"0x{b:02X}" for b in row)
    comma = "," if i < len(data) - 1 else ""
    bitmap_h += f"    {{{hex_vals}}}{comma}\n"
bitmap_h += "};\n"

with open(r"d:\File\code\uvk1\Dondji\App\app\mokuyu_bitmap.h", "w") as f:
    f.write(bitmap_h)

print("Generated mokuyu_bitmap.h")
