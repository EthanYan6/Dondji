#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Debug generate_bin step by step
"""

import os
import sys
import struct
import tempfile

# Add tools directory to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from gen_cn_font import load_gb2312_chars, load_cn_chars_append, CN_CHARS_500, parse_bdf, build_pinyin_table, bitmap_to_uint16

script_dir = os.path.dirname(os.path.abspath(__file__))

# Load all chars
gb2312_chars = load_gb2312_chars(script_dir)
existing_chars_text = "".join(CN_CHARS_500)
append_segment = load_cn_chars_append(script_dir)
existing_chars_text += append_segment

seen = set()
existing_chars = []
for char in existing_chars_text:
    if char not in seen and ord(char) >= 0x4E00:
        seen.add(char)
        existing_chars.append(char)

unique_chars = list(existing_chars)
existing_set = set(existing_chars)
for char in gb2312_chars:
    if char not in existing_set:
        unique_chars.append(char)
        existing_set.add(char)

# Parse BDF
bdf_path = os.path.join(script_dir, "..", "bdf", "wenquanyi_9pt.bdf")
bdf_chars = parse_bdf(bdf_path)

# Generate like generate_bin does
valid_chars = [ch for ch in unique_chars if ord(ch) in bdf_chars]
print(f"valid_chars: {len(valid_chars)}")

font_data = []
char_map = []
for i, ch in enumerate(valid_chars):
    code = ord(ch)
    bitmap = bdf_chars[code]
    idx = len(font_data)
    char_map.append((code, idx))
    font_data.extend(bitmap_to_uint16(bitmap))
    
    # Check for issues
    if i < 10:
        print(f"  [{i}] U+{code:04X} ('{ch}') -> idx={idx}, bitmap_len={len(bitmap)}")

print(f"\nfont_data: {len(font_data)} uint16_t")
print(f"char_map: {len(char_map)} entries")

# Check char_map for duplicates
from collections import Counter
codes = [c for c, _ in char_map]
counter = Counter(codes)
dups = [(c, n) for c, n in counter.items() if n > 1]
if dups:
    print(f"\nDuplicates in char_map: {len(dups)}")
    for code, n in dups[:5]:
        char = chr(code)
        print(f"  U+{code:04X} ('{char}') appears {n} times")
else:
    print("\nNo duplicates in char_map")

# Write to temp file and check
with tempfile.NamedTemporaryFile(delete=False, suffix='.bin') as f:
    temp_path = f.name
    
    # Write font data
    for val in font_data:
        f.write(struct.pack('<H', val))
    
    # Write index
    for unicode_val, idx in char_map:
        f.write(struct.pack('<I', (unicode_val << 16) | idx))
    
    # Write pinyin (skip for now)
    
    f.flush()

# Read back and check
with open(temp_path, 'rb') as f:
    data = f.read()

print(f"\nTemp file size: {len(data)} bytes")

# Read index table
index_start = len(font_data) * 2
index_map = {}
for i in range(len(char_map)):
    entry_offset = index_start + i * 4
    if entry_offset + 4 <= len(data):
        entry = struct.unpack('<I', data[entry_offset:entry_offset+4])[0]
        unicode_val = entry >> 16
        font_idx = entry & 0xFFFF
        index_map[unicode_val] = font_idx

print(f"Index entries read: {len(index_map)}")

# Check missing 'wei' chars
print(f"\nChecking missing 'wei' characters:")
missing_wei = [0x8ECE, 0x709C, 0x7168, 0x9C94]
for unicode_val in missing_wei:
    char = chr(unicode_val)
    if unicode_val in index_map:
        idx = index_map[unicode_val]
        print(f"  U+{unicode_val:04X} ('{char}') -> idx={idx} [OK]")
    else:
        print(f"  U+{unicode_val:04X} ('{char}') -> NOT FOUND [MISSING]")

os.unlink(temp_path)
