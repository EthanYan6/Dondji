#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Dondji Firmware
#
# Copyright (c) 2026 BD1AHN
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
#
# You may obtain a copy of the License at:
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Project:
#     叮咚鸡 (Dondji)
#
# Maintainer:
#     BD1AHN
#
# Commercial products using the Dondji brand require separate authorization.

"""
Debug font generation - check why some characters are missing
"""

import os
import sys

# Add tools directory to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from gen_cn_font import load_gb2312_chars, load_cn_chars_append, CN_CHARS_500, parse_bdf, build_pinyin_table

script_dir = os.path.dirname(os.path.abspath(__file__))

# Load GB2312
gb2312_chars = load_gb2312_chars(script_dir)
print(f"GB2312 characters: {len(gb2312_chars)}")

# Load existing
existing_chars_text = "".join(CN_CHARS_500)
append_segment = load_cn_chars_append(script_dir)
existing_chars_text += append_segment

seen = set()
existing_chars = []
for char in existing_chars_text:
    if char not in seen and ord(char) >= 0x4E00:
        seen.add(char)
        existing_chars.append(char)
print(f"Existing custom characters: {len(existing_chars)}")

# Merge
unique_chars = list(existing_chars)
existing_set = set(existing_chars)
for char in gb2312_chars:
    if char not in existing_set:
        unique_chars.append(char)
        existing_set.add(char)

print(f"Merged total: {len(unique_chars)} characters")

# Parse BDF
bdf_path = os.path.join(script_dir, "..", "bdf", "wenquanyi_9pt.bdf")
bdf_chars = parse_bdf(bdf_path)
print(f"Total characters in BDF: {len(bdf_chars)}")

# Check valid chars
valid_chars = [ch for ch in unique_chars if ord(ch) in bdf_chars]
print(f"Valid characters (in BDF): {len(valid_chars)}")

# Check missing chars
missing_chars = [ch for ch in unique_chars if ord(ch) not in bdf_chars]
print(f"Missing characters (not in BDF): {len(missing_chars)}")

if missing_chars:
    print(f"\nFirst 20 missing characters:")
    for i, ch in enumerate(missing_chars[:20]):
        print(f"  [{i}] U+{ord(ch):04X} ('{ch}')")

# Check specific missing 'wei' chars
print(f"\nChecking missing 'wei' characters:")
missing_wei = [0x8ECE, 0x709C, 0x7168, 0x9C94]
for unicode_val in missing_wei:
    char = chr(unicode_val)
    if char in unique_chars:
        print(f"  U+{unicode_val:04X} ('{char}') -> in unique_chars")
    else:
        print(f"  U+{unicode_val:04X} ('{char}') -> NOT in unique_chars")
    
    if unicode_val in bdf_chars:
        print(f"    -> in BDF")
    else:
        print(f"    -> NOT in BDF")

# Check if they are in valid_chars
print(f"\nChecking if missing 'wei' chars are in valid_chars:")
for unicode_val in missing_wei:
    char = chr(unicode_val)
    if char in valid_chars:
        print(f"  U+{unicode_val:04X} ('{char}') -> in valid_chars")
    else:
        print(f"  U+{unicode_val:04X} ('{char}') -> NOT in valid_chars")
