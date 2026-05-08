#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Debug write and read process
"""

import os
import sys
import struct
import tempfile

# Add tools directory to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from gen_cn_font import load_gb2312_chars, load_cn_chars_append, CN_CHARS_500, parse_bdf, bitmap_to_uint16

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

font_data = []
char_map = []
for i, ch in enumerate(valid_chars):
    code = ord(ch)
    bitmap = bdf_chars[code]
    idx = len(font_data)
    char_map.append((code, idx))
    font_data.extend(bitmap_to_uint16(bitmap))

print(f"char_map: {len(char_map)} entries")

# Write to temp file
with tempfile.NamedTemporaryFile(delete=False, suffix='.bin') as f:
    temp_path = f.name
    
    # Write font data
    for val in font_data:
        f.write(struct.pack('<H', val))
    
    # Write index
    print(f"\nWriting index table...")
    for i, (unicode_val, idx) in enumerate(char_map):
        entry = (unicode_val << 16) | idx
        f.write(struct.pack('<I', entry))
        
        # Check specific entries
        if unicode_val in [0x8ECE, 0x709C, 0x7168, 0x9C94]:
            char = chr(unicode_val)
            print(f"  Writing [{i}] U+{unicode_val:04X} ('{char}') -> entry=0x{entry:08X}")
    
    f.flush()

# Read back and check
with open(temp_path, 'rb') as f:
    data = f.read()

print(f"\nTemp file size: {len(data)} bytes")

# Read index table byte by byte
index_start = len(font_data) * 2
print(f"Index start: {index_start}")

# Check specific positions
print(f"\nReading specific entries:")
for i, (unicode_val, idx) in enumerate(char_map):
    if unicode_val in [0x8ECE, 0x709C, 0x7168, 0x9C94]:
        entry_offset = index_start + i * 4
        entry_bytes = data[entry_offset:entry_offset+4]
        entry = struct.unpack('<I', entry_bytes)[0]
        read_unicode = entry >> 16
        read_idx = entry & 0xFFFF
        char = chr(unicode_val)
        print(f"  [{i}] U+{unicode_val:04X} ('{char}') -> offset={entry_offset}")
        print(f"       Written: entry=0x{(unicode_val << 16) | idx:08X}")
        print(f"       Read:    entry=0x{entry:08X}, unicode=0x{read_unicode:04X}, idx={read_idx}")

os.unlink(temp_path)
