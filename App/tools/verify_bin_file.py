#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Verify cn_font.bin file format
"""

import struct
import os

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

print(f"Total size: {len(data)} bytes")

# Expected layout
bitmap_size = 162384
index_size = 40596
py_offset = 202980
py_size = 15633

print(f"\nExpected layout:")
print(f"  Bitmap: 0 - {bitmap_size} ({bitmap_size} bytes)")
print(f"  Index:  {bitmap_size} - {bitmap_size + index_size} ({index_size} bytes)")
print(f"  Pinyin: {py_offset} - {py_offset + py_size} ({py_size} bytes)")
print(f"  Total expected: {py_offset + py_size + 1} bytes")

# Check pinyin table at offset
print(f"\nChecking pinyin table at offset {py_offset}:")
if len(data) > py_offset + 10:
    # Read first few bytes of pinyin table
    py_data = data[py_offset:py_offset + 20]
    print(f"  First 20 bytes: {py_data.hex()}")
    
    # Parse first entry
    str_len = py_data[0]
    print(f"  First entry str_len: {str_len}")
    if str_len > 0 and str_len < 10:
        syllable = py_data[1:1+str_len].decode('ascii')
        print(f"  First entry syllable: '{syllable}'")
        char_count = py_data[1+str_len]
        print(f"  First entry char_count: {char_count}")
        
        # Read first Unicode
        if len(py_data) > 1 + str_len + 1 + 2:
            unicode_bytes = py_data[1+str_len+1:1+str_len+1+2]
            unicode_val = (unicode_bytes[0] << 8) | unicode_bytes[1]
            print(f"  First Unicode: U+{unicode_val:04X} ('{chr(unicode_val)}')")
else:
    print(f"  ERROR: File too small!")

# Search for 'wei' in pinyin table
print(f"\nSearching for 'wei' in pinyin table...")
offset = py_offset
while offset < len(data) - 10:
    str_len = data[offset]
    if str_len == 0 or str_len > 10:
        break
    
    syllable = data[offset+1:offset+1+str_len].decode('ascii', errors='ignore')
    char_count = data[offset+1+str_len]
    
    if syllable == 'wei':
        print(f"  Found 'wei' at offset {offset - py_offset}")
        print(f"  char_count: {char_count}")
        
        # Read first 6 Unicode values
        unicode_offset = offset + 1 + str_len + 1
        print(f"  First 6 Unicode values:")
        for i in range(min(6, char_count)):
            if unicode_offset + i*2 + 1 < len(data):
                unicode_val = (data[unicode_offset + i*2] << 8) | data[unicode_offset + i*2 + 1]
                print(f"    {i+1}. U+{unicode_val:04X} ('{chr(unicode_val)}')")
        break
    
    offset += 1 + str_len + 1 + char_count * 2
else:
    print(f"  'wei' not found!")
