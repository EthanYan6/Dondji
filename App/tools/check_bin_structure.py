#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Check cn_font.bin file structure
"""

import os
import struct

bin_path = '../../docs/font/cn_font.bin'
with open(bin_path, 'rb') as f:
    data = f.read()

print(f"File size: {len(data)} bytes")

# Expected sizes
bitmap_size = 162384  # 81192 * 2
index_size = 40596     # 6766 * 6
py_size = 15633

print(f"\nExpected sizes:")
print(f"  Bitmap: {bitmap_size} bytes")
print(f"  Index:  {index_size} bytes")
print(f"  Pinyin: {py_size} bytes")
print(f"  Total:  {bitmap_size + index_size + py_size} bytes")

# Check actual index table
print(f"\nChecking index table:")
index_start = bitmap_size
index_end = index_start + index_size
print(f"  Index range: {index_start} - {index_end}")

# Count actual entries
actual_index_count = (len(data) - bitmap_size) // 4
print(f"  Actual index entries (based on file size): {actual_index_count}")

# Check if file is truncated
if len(data) < bitmap_size + index_size:
    print(f"\n  ERROR: File is truncated!")
    print(f"  Missing: {bitmap_size + index_size - len(data)} bytes")
    print(f"  Missing index entries: {(bitmap_size + index_size - len(data)) // 4}")
    
    # Check what's at the end of the file
    print(f"\n  Last 100 bytes of file:")
    print(f"  {data[-100:].hex()}")
else:
    print(f"  File size OK")

# Check pinyin table offset
py_offset = bitmap_size + index_size
print(f"\nPinyin table offset: {py_offset}")
if py_offset < len(data):
    print(f"  Pinyin table exists")
    # Read first few bytes
    py_data = data[py_offset:py_offset+20]
    print(f"  First 20 bytes: {py_data.hex()}")
else:
    print(f"  ERROR: Pinyin table offset beyond file size!")
