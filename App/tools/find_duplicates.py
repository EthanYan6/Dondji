#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Find duplicate entries in cn_font.bin index table
"""

import os
import struct
from collections import Counter

bin_path = '../../docs/font/cn_font.bin'
with open(bin_path, 'rb') as f:
    data = f.read()

# Read index table
bitmap_size = 162384
index_count = 6766
index_start = bitmap_size

entries = []
unicode_list = []

for i in range(index_count):
    entry_offset = index_start + i * 4
    if entry_offset + 4 <= len(data):
        entry = struct.unpack('<I', data[entry_offset:entry_offset+4])[0]
        unicode_val = entry >> 16
        font_idx = entry & 0xFFFF
        entries.append((unicode_val, font_idx))
        unicode_list.append(unicode_val)

print(f"Total entries: {len(entries)}")
print(f"Unique Unicode values: {len(set(unicode_list))}")

# Find duplicates
counter = Counter(unicode_list)
duplicates = [(u, c) for u, c in counter.items() if c > 1]

print(f"\nFound {len(duplicates)} Unicode values with duplicates")

if duplicates:
    print(f"\nFirst 20 duplicates:")
    for unicode_val, count in duplicates[:20]:
        char = chr(unicode_val)
        # Find all entries with this Unicode
        matching = [(i, idx) for i, (u, idx) in enumerate(entries) if u == unicode_val]
        print(f"  U+{unicode_val:04X} ('{char}') appears {count} times:")
        for pos, font_idx in matching:
            print(f"    Position {pos}: font_idx={font_idx}")

# Check if missing 'wei' chars are in the list
print(f"\nChecking missing 'wei' characters in unicode_list:")
missing_wei = [0x8ECE, 0x709C, 0x7168, 0x9C94]
for unicode_val in missing_wei:
    char = chr(unicode_val)
    count = counter.get(unicode_val, 0)
    if count > 0:
        print(f"  U+{unicode_val:04X} ('{char}') appears {count} times")
    else:
        print(f"  U+{unicode_val:04X} ('{char}') NOT in unicode_list")
