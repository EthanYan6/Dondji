#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Check actual 'wei' data in cn_font.bin
"""

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

# Pinyin table offset
py_offset = 189448

# Search for 'wei'
print(f"\nSearching for 'wei' in pinyin table...")
offset = py_offset
while offset < len(data) - 10:
    str_len = data[offset]
    if str_len == 0 or str_len > 10:
        break
    
    syllable = data[offset+1:offset+1+str_len].decode('ascii', errors='ignore')
    char_count = data[offset+1+str_len]
    
    if syllable == 'wei':
        print(f"Found 'wei' at offset {offset - py_offset}")
        print(f"char_count: {char_count}")
        
        # Read all Unicode values
        unicode_offset = offset + 1 + str_len + 1
        print(f"\nAll {char_count} Unicode values:")
        
        for i in range(char_count):
            if unicode_offset + i*2 + 1 < len(data):
                unicode_val = (data[unicode_offset + i*2] << 8) | data[unicode_offset + i*2 + 1]
                try:
                    char = chr(unicode_val)
                    print(f"  [{i:2d}] U+{unicode_val:04X} ('{char}')")
                except:
                    print(f"  [{i:2d}] U+{unicode_val:04X} (invalid)")
        
        # Check specific pages
        print(f"\nPage 1 (indices 0-5):")
        for i in range(6):
            unicode_val = (data[unicode_offset + i*2] << 8) | data[unicode_offset + i*2 + 1]
            print(f"  [{i}] U+{unicode_val:04X} ('{chr(unicode_val)}')")
        
        print(f"\nPage 2 (indices 6-11):")
        for i in range(6, 12):
            unicode_val = (data[unicode_offset + i*2] << 8) | data[unicode_offset + i*2 + 1]
            print(f"  [{i}] U+{unicode_val:04X} ('{chr(unicode_val)}')")
        
        print(f"\nPage 3 (indices 12-17):")
        for i in range(12, 18):
            unicode_val = (data[unicode_offset + i*2] << 8) | data[unicode_offset + i*2 + 1]
            print(f"  [{i}] U+{unicode_val:04X} ('{chr(unicode_val)}')")
        
        break
    
    offset += 1 + str_len + 1 + char_count * 2
else:
    print(f"'wei' not found!")
