#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Correctly check cn_font.bin index table
"""

import os
import struct

bin_path = '../../docs/font/cn_font.bin'
with open(bin_path, 'rb') as f:
    data = f.read()

print(f"File size: {len(data)} bytes")

# Expected sizes
bitmap_size = 162384  # 81192 * 2
index_count = 6766
index_size = index_count * 6  # 40596
py_size = 15633

print(f"\nExpected sizes:")
print(f"  Bitmap: {bitmap_size} bytes")
print(f"  Index:  {index_size} bytes ({index_count} entries)")
print(f"  Pinyin: {py_size} bytes")
print(f"  Total:  {bitmap_size + index_size + py_size} bytes")

# Read index table
print(f"\nReading index table:")
index_start = bitmap_size
index_map = {}

for i in range(index_count):
    entry_offset = index_start + i * 4
    if entry_offset + 4 <= len(data):
        entry = struct.unpack('<I', data[entry_offset:entry_offset+4])[0]
        unicode_val = entry >> 16
        font_idx = entry & 0xFFFF
        index_map[unicode_val] = font_idx

print(f"  Total entries read: {index_count}")
print(f"  Unique Unicode values: {len(index_map)}")

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

# Find which entries are missing
print(f"\nFinding missing Unicode values:")
all_wei_unicode = []
py_offset = bitmap_size + index_size
offset = py_offset

while offset < len(data) - 10:
    str_len = data[offset]
    if str_len == 0 or str_len > 10:
        break
    
    syllable = data[offset+1:offset+1+str_len].decode('ascii', errors='ignore')
    char_count = data[offset+1+str_len]
    
    if syllable == 'wei':
        unicode_offset = offset + 1 + str_len + 1
        
        for i in range(char_count):
            unicode_val = (data[unicode_offset + i*2] << 8) | data[unicode_offset + i*2 + 1]
            all_wei_unicode.append(unicode_val)
            
            if unicode_val not in index_map:
                char = chr(unicode_val)
                print(f"  [{i:2d}] U+{unicode_val:04X} ('{char}') -> NOT in index table")
        
        break
    
    offset += 1 + str_len + 1 + char_count * 2

print(f"\nTotal 'wei' characters: {len(all_wei_unicode)}")
print(f"Missing from index: {len([u for u in all_wei_unicode if u not in index_map])}")
