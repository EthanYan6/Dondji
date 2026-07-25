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
Merge existing font characters with GB2312 complete character set
"""

import os

def load_existing_chars():
    """Load existing 1281 characters from gen_cn_font.py"""
    from gen_cn_font import CN_CHARS_500, load_cn_chars_append
    
    # Load base characters
    base_chars = ''.join(CN_CHARS_500)
    
    # Load append characters
    append_chars = load_cn_chars_append(os.path.dirname(__file__))
    
    # Combine and deduplicate while preserving order
    combined = base_chars + append_chars
    seen = set()
    unique_chars = []
    for char in combined:
        if char not in seen:
            seen.add(char)
            unique_chars.append(char)
    
    return unique_chars

def load_gb2312_chars():
    """Load GB2312 characters from file"""
    with open('gb2312_chars.txt', 'r', encoding='utf-8') as f:
        return list(f.read().strip())

def merge_chars(existing_chars, gb2312_chars):
    """Merge existing chars with GB2312, preserving existing order"""
    # Start with existing characters (they keep their order)
    merged = list(existing_chars)
    existing_set = set(existing_chars)
    
    # Add GB2312 characters that are not in existing set
    for char in gb2312_chars:
        if char not in existing_set:
            merged.append(char)
            existing_set.add(char)
    
    return merged

if __name__ == '__main__':
    print("Loading existing characters...")
    existing = load_existing_chars()
    print(f"Existing characters: {len(existing)}")
    
    print("\nLoading GB2312 characters...")
    gb2312 = load_gb2312_chars()
    print(f"GB2312 characters: {len(gb2312)}")
    
    print("\nMerging character sets...")
    merged = merge_chars(existing, gb2312)
    print(f"Merged total: {len(merged)}")
    print(f"New characters added: {len(merged) - len(existing)}")
    
    # Save merged character list
    with open('merged_chars.txt', 'w', encoding='utf-8') as f:
        f.write(''.join(merged))
    
    print(f"\nSaved to merged_chars.txt")
    
    # Show some statistics
    print(f"\nFirst 20 chars: {''.join(merged[:20])}")
    print(f"Last 20 chars: {''.join(merged[-20:])}")
