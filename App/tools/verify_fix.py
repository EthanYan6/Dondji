#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Verify the fixed cn_font.bin file
"""

import os
import struct

bin_path = '../../docs/font/cn_font.bin'
with open(bin_path, 'rb') as f:
    data = f.read()

print(f"File size: {len(data)} bytes")

# Expected sizes
bitmap_size = 162384
index_count = 6766
index_size = index_count * 6  # 6 bytes per entry
py_size = 15633

print(f"\nExpected sizes:")
print(f"  Bitmap: {bitmap_size} bytes")
print(f"  Index:  {index_size} bytes ({index_count} entries x 6 bytes)")
print(f"  Pinyin: {py_size} bytes")
print(f"  Total:  {bitmap_size + index_size + py_size} bytes")

# Read index table
print(f"\nReading index table:")
index_start = bitmap_size
index_map = {}

for i in range(index_count):
    entry_offset = index_start + i * 6
    if entry_offset + 6 <= len(data):
        # Read unicode (2 bytes LE)
        unicode_bytes = data[entry_offset:entry_offset+2]
        unicode_val = struct.unpack('<H', unicode_bytes)[0]
        
        # Read index (4 bytes LE)
        index_bytes = data[entry_offset+2:entry_offset+6]
        idx = struct.unpack('<I', index_bytes)[0]
        
        index_map[unicode_val] = idx

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

# Check pinyin table
print(f"\nChecking pinyin table:")
py_offset = bitmap_size + index_size
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
        
        # Check first 6 Unicode values
        unicode_offset = offset + 1 + str_len + 1
        print(f"  First 6 Unicode values:")
        for i in range(min(6, char_count)):
            unicode_val = (data[unicode_offset + i*2] << 8) | data[unicode_offset + i*2 + 1]
            char = chr(unicode_val)
            print(f"    [{i}] U+{unicode_val:04X} ('{char}')")
        
        # Check last 6 Unicode values
        print(f"  Last 6 Unicode values:")
        for i in range(max(0, char_count-6), char_count):
            unicode_val = (data[unicode_offset + i*2] << 8) | data[unicode_offset + i*2 + 1]
            char = chr(unicode_val)
            print(f"    [{i}] U+{unicode_val:04X} ('{char}')")
        
        break
    
    offset += 1 + str_len + 1 + char_count * 2

# Verify all 'wei' chars are in index
print(f"\nVerifying all 'wei' chars are in index:")
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
                print(f"  [{i:2d}] U+{unicode_val:04X} ('{char}') -> NOT in index table")
        
        if missing_count == 0:
            print(f"  All {char_count} characters found in index table [OK]")
        else:
            print(f"  Missing: {missing_count}/{char_count} characters")
        break
    
    offset += 1 + str_len + 1 + char_count * 2
