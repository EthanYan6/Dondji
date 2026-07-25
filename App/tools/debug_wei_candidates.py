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
Debug pinyin candidate lookup for 'wei'
"""

from gen_cn_font import load_gb2312_chars, load_cn_chars_append, CN_CHARS_500, build_pinyin_table
import os

# Load and merge characters
gb2312_chars = load_gb2312_chars('.')
existing_chars_text = ''.join(CN_CHARS_500)
append_segment = load_cn_chars_append('.')
existing_chars_text += append_segment

seen = set()
existing_chars = []
for char in existing_chars_text:
    if char not in seen and ord(char) >= 0x4E00:
        seen.add(char)
        existing_chars.append(char)

unique_chars = list(existing_chars)
existing_set = set(existing_chars)
for char in gb2312_chars:
    if char not in existing_set:
        unique_chars.append(char)
        existing_set.add(char)

# Build pinyin table
pinyin_table = build_pinyin_table(unique_chars)

# Check "wei" candidates
print("Pinyin 'wei' candidates:")
if 'wei' in pinyin_table:
    indices = pinyin_table['wei']
    print(f"Total: {len(indices)} characters")
    print(f"\nFirst 6 (page 1):")
    for i, idx in enumerate(indices[:6]):
        char = unique_chars[idx]
        print(f"  {i+1}. index={idx}, char='{char}' (U+{ord(char):04X})")
    
    print(f"\nNext 6 (page 2, offset=6):")
    for i, idx in enumerate(indices[6:12]):
        char = unique_chars[idx]
        print(f"  {i+1}. index={idx}, char='{char}' (U+{ord(char):04X})")
    
    print(f"\nLast 6 (page 10, offset=54):")
    for i, idx in enumerate(indices[54:60]):
        char = unique_chars[idx]
        print(f"  {i+1}. index={idx}, char='{char}' (U+{ord(char):04X})")
    
    # Check if any index is out of range
    print(f"\nIndex range check:")
    print(f"  Max index: {max(indices)}")
    print(f"  Total chars: {len(unique_chars)}")
    if max(indices) >= len(unique_chars):
        print(f"  ERROR: Some indices are out of range!")
    else:
        print(f"  OK: All indices are valid")
