#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Dondji Firmware
#
# Copyright (c) 2026 BD1AHN
#
# Licensed under the Apache License, Version 2.0
#
# Project:
#     叮咚鸡 (Dondji)
#
# Maintainer:
#     BD1AHN
#
# Official Website:
#     https://ethanyan6.github.io/Dondji/
#
# The Dondji name, logo, and official project identity
# are protected separately from the source code license.
"""
Check if missing characters exist in BDF font file
"""

missing_chars = [
    (0x8ECE, '軎'),
    (0x709C, '炜'),
    (0x7168, '煨'),
    (0x9C94, '鲔'),
]

print("Missing characters in font index:")
for unicode_val, char in missing_chars:
    print(f"  U+{unicode_val:04X} ('{char}')")

# Read BDF file
bdf_path = '../bdf/wenquanyi_9pt.bdf'
print(f"\nReading BDF file: {bdf_path}")

with open(bdf_path, 'r', encoding='utf-8', errors='ignore') as f:
    bdf_content = f.read()

print(f"BDF file size: {len(bdf_content)} bytes")

# Check if missing chars exist in BDF
print(f"\nChecking if missing characters exist in BDF:")
for unicode_val, char in missing_chars:
    # Search for ENCODING line
    encoding_line = f"ENCODING {unicode_val}"
    if encoding_line in bdf_content:
        print(f"  U+{unicode_val:04X} ('{char}') -> FOUND in BDF")
    else:
        print(f"  U+{unicode_val:04X} ('{char}') -> NOT in BDF")

# Also check the 3rd page characters
print(f"\nChecking 3rd page characters in BDF:")
page3_chars = [
    (0x6D08, '洈'),
    (0x6E2D, '渭'),
    (0x6F4D, '潍'),
    (0x5DCD, '巍'),
    (0x97E6, '韦'),
    (0x8FDD, '违'),
]

for unicode_val, char in page3_chars:
    encoding_line = f"ENCODING {unicode_val}"
    if encoding_line in bdf_content:
        print(f"  U+{unicode_val:04X} ('{char}') -> FOUND in BDF")
    else:
        print(f"  U+{unicode_val:04X} ('{char}') -> NOT in BDF")
