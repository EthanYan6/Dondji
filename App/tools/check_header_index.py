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
Check cn_font_data.h index table
"""

import re

# Read cn_font_data.h
with open('../cn_font_data.h', 'r', encoding='utf-8') as f:
    content = f.read()

# Find index table
match = re.search(r'static const uint32_t cn_font_index\[(\d+)\] = \{([^}]+)\}', content, re.DOTALL)
if match:
    count = int(match.group(1))
    index_data = match.group(2)
    entries = re.findall(r'0x([0-9A-Fa-f]{8})', index_data)
    
    print(f"Declared count: {count}")
    print(f"Actual entries: {len(entries)}")
    
    # Build map
    index_map = {}
    for entry in entries:
        unicode_val = int(entry[:4], 16)
        font_idx = int(entry[4:], 16)
        if unicode_val in index_map:
            print(f"  Duplicate: U+{unicode_val:04X}")
        index_map[unicode_val] = font_idx
    
    print(f"Unique Unicode values: {len(index_map)}")
    
    # Check missing 'wei' chars
    print(f"\nChecking missing 'wei' characters:")
    missing_wei = [0x8ECE, 0x709C, 0x7168, 0x9C94]
    for unicode_val in missing_wei:
        char = chr(unicode_val)
        if unicode_val in index_map:
            font_idx = index_map[unicode_val]
            print(f"  U+{unicode_val:04X} ('{char}') -> font_idx={font_idx} [OK]")
        else:
            print(f"  U+{unicode_val:04X} ('{char}') -> NOT FOUND [MISSING]")
