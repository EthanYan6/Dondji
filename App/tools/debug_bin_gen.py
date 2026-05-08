#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Debug generate_bin function
"""

import os
import sys
import struct
import shutil

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
for ch in valid_chars:
    code = ord(ch)
    bitmap = bdf_chars[code]
    char_map.append((code, len(font_data)))
    font_data.extend(bitmap_to_uint16(bitmap))

print(f"font_data: {len(font_data)} uint16_t")
print(f"char_map: {len(char_map)} entries")

# Check char_map for duplicates
char_map_dict = {}
for code, idx in char_map:
    if code in char_map_dict:
        print(f"  Duplicate in char_map: U+{code:04X}")
    char_map_dict[code] = idx

print(f"Unique in char_map: {len(char_map_dict)}")

# Check missing 'wei' chars
print(f"\nChecking missing 'wei' characters in char_map:")
missing_wei = [0x8ECE, 0x709C, 0x7168, 0x9C94]
for unicode_val in missing_wei:
    char = chr(unicode_val)
    if unicode_val in char_map_dict:
        idx = char_map_dict[unicode_val]
        print(f"  U+{unicode_val:04X} ('{char}') -> idx={idx} [OK]")
    else:
        print(f"  U+{unicode_val:04X} ('{char}') -> NOT FOUND [MISSING]")

# Now check the actual binary file
print(f"\nChecking actual cn_font.bin:")
bin_path = os.path.join(script_dir, "..", "..", "docs", "font", "cn_font.bin")
with open(bin_path, 'rb') as f:
    data = f.read()

# Read index table from binary
index_offset = len(font_data) * 2
print(f"Index offset: {index_offset}")

bin_index_map = {}
for i in range(len(char_map)):
    entry_offset = index_offset + i * 4
    if entry_offset + 4 <= len(data):
        entry = struct.unpack('<I', data[entry_offset:entry_offset+4])[0]
        unicode_val = entry >> 16
        font_idx = entry & 0xFFFF
        bin_index_map[unicode_val] = font_idx

print(f"Binary index entries: {len(bin_index_map)}")

# Check missing 'wei' chars in binary
print(f"\nChecking missing 'wei' characters in binary:")
for unicode_val in missing_wei:
    char = chr(unicode_val)
    if unicode_val in bin_index_map:
        idx = bin_index_map[unicode_val]
        print(f"  U+{unicode_val:04X} ('{char}') -> idx={idx} [OK]")
    else:
        print(f"  U+{unicode_val:04X} ('{char}') -> NOT FOUND [MISSING]")
