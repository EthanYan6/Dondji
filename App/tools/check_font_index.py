#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Check if all 'wei' characters exist in the font index table
"""

import os
import struct

# Find cn_font.bin
bin_paths = [
    '../../docs/font/cn_font.bin',
    '../../docs/fonts/cn_font.bin',
]

bin_path = None
for path in bin_paths:
    if os.path.exists(path):
        bin_path = path
        break

if not bin_path:
    print("ERROR: cn_font.bin not found!")
    exit(1)

print(f"Reading: {bin_path}")

# Read cn_font.bin
with open(bin_path, 'rb') as f:
    data = f.read()

# Layout
bitmap_size = 162384
index_offset = bitmap_size
index_count = 6766

print(f"\nChecking index table at offset {index_offset}")
print(f"Total entries: {index_count}")

# Build index map
index_map = {}
for i in range(index_count):
    entry_offset = index_offset + i * 4
    if entry_offset + 4 <= len(data):
        entry = struct.unpack('<I', data[entry_offset:entry_offset+4])[0]
        unicode_val = entry >> 16
        font_idx = entry & 0xFFFF
        index_map[unicode_val] = font_idx

print(f"Index map built: {len(index_map)} entries")

# Check 'wei' characters
wei_chars = [
    0x4E3A, 0x5371, 0x5A01, 0x9B4F, 0x7EF4, 0x851A,  # Page 1
    0x56F4, 0x5C3E, 0x536B, 0x5C09, 0x5FAE, 0x672A,  # Page 2
    0x6D08, 0x6E2D, 0x6F4D, 0x5DCD, 0x97E6, 0x8FDD,  # Page 3
]

print(f"\nChecking 'wei' characters:")
for i, unicode_val in enumerate(wei_chars):
    char = chr(unicode_val)
    if unicode_val in index_map:
        font_idx = index_map[unicode_val]
        print(f"  [{i:2d}] U+{unicode_val:04X} ('{char}') -> font_idx={font_idx} [OK]")
    else:
        print(f"  [{i:2d}] U+{unicode_val:04X} ('{char}') -> NOT FOUND [MISSING]")

# Check all 60 'wei' characters
print(f"\nChecking all 60 'wei' characters from pinyin table:")
py_offset = 189448
offset = py_offset
while offset < len(data) - 10:
    str_len = data[offset]
    if str_len == 0 or str_len > 10:
        break
    
    syllable = data[offset+1:offset+1+str_len].decode('ascii', errors='ignore')
    char_count = data[offset+1+str_len]
    
    if syllable == 'wei':
        unicode_offset = offset + 1 + str_len + 1
        
        missing_count = 0
        for i in range(char_count):
            unicode_val = (data[unicode_offset + i*2] << 8) | data[unicode_offset + i*2 + 1]
            if unicode_val not in index_map:
                missing_count += 1
                char = chr(unicode_val)
                print(f"  [{i:2d}] U+{unicode_val:04X} ('{char}') -> NOT FOUND [MISSING]")
        
        if missing_count == 0:
            print(f"  All {char_count} characters found in index table [OK]")
        else:
            print(f"  Missing: {missing_count}/{char_count} characters")
        break
    
    offset += 1 + str_len + 1 + char_count * 2
