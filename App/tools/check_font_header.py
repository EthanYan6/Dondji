#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Check first 4 bytes of cn_font.bin
"""

import struct

bin_path = '../../docs/font/cn_font.bin'
with open(bin_path, 'rb') as f:
    data = f.read()

print(f"File size: {len(data)} bytes")

# Check first 4 bytes
first_4_bytes = data[:4]
print(f"First 4 bytes: {first_4_bytes.hex()}")

# Parse as two uint16_t (little-endian)
word0 = struct.unpack('<H', first_4_bytes[0:2])[0]
word1 = struct.unpack('<H', first_4_bytes[2:4])[0]
print(f"word0: 0x{word0:04X} (expected: 0x1100)")
print(f"word1: 0x{word1:04X} (expected: 0x2100)")

# Check version byte
version_offset = 218613
if len(data) > version_offset:
    version_byte = data[version_offset]
    print(f"Version byte at offset {version_offset}: 0x{version_byte:02X} (expected: 0x03)")
else:
    print(f"ERROR: File too short for version offset {version_offset}")

# Check if this is the first character bitmap
# The first character is '的' (U+7684), bitmap should start at offset 0
print(f"\nFirst character bitmap (offset 0-23 bytes):")
bitmap_bytes = data[0:24]
print(f"  {bitmap_bytes.hex()}")

# Parse as 12 uint16_t
bitmap_words = struct.unpack('<12H', bitmap_bytes)
print(f"  As uint16_t: {[hex(w) for w in bitmap_words]}")
