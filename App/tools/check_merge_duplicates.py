#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Check if unique_chars has duplicates that are not detected
"""

import os
import sys
from collections import Counter

# Add tools directory to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from gen_cn_font import load_gb2312_chars, load_cn_chars_append, CN_CHARS_500

script_dir = os.path.dirname(os.path.abspath(__file__))

# Load GB2312
gb2312_chars = load_gb2312_chars(script_dir)
print(f"GB2312: {len(gb2312_chars)} chars")

# Check for duplicates in GB2312
gb2312_counter = Counter(gb2312_chars)
gb2312_dups = [(c, n) for c, n in gb2312_counter.items() if n > 1]
if gb2312_dups:
    print(f"GB2312 duplicates: {len(gb2312_dups)}")
    for c, n in gb2312_dups[:5]:
        print(f"  U+{ord(c):04X} ('{c}') appears {n} times")
else:
    print("GB2312: no duplicates")

# Load existing
existing_chars_text = "".join(CN_CHARS_500)
append_segment = load_cn_chars_append(script_dir)
existing_chars_text += append_segment

print(f"\nExisting text: {len(existing_chars_text)} chars")

# Check for duplicates in existing text
existing_counter = Counter(existing_chars_text)
existing_dups = [(c, n) for c, n in existing_counter.items() if n > 1]
if existing_dups:
    print(f"Existing text duplicates: {len(existing_dups)}")
    for c, n in existing_dups[:5]:
        print(f"  U+{ord(c):04X} ('{c}') appears {n} times")
else:
    print("Existing text: no duplicates")

# Merge like the script does
seen = set()
existing_chars = []
for char in existing_chars_text:
    if char not in seen and ord(char) >= 0x4E00:
        seen.add(char)
        existing_chars.append(char)

print(f"\nDeduplicated existing: {len(existing_chars)} chars")

# Merge with GB2312
unique_chars = list(existing_chars)
existing_set = set(existing_chars)
for char in gb2312_chars:
    if char not in existing_set:
        unique_chars.append(char)
        existing_set.add(char)

print(f"Merged: {len(unique_chars)} chars")

# Check final for duplicates
final_counter = Counter(unique_chars)
final_dups = [(c, n) for c, n in final_counter.items() if n > 1]
if final_dups:
    print(f"\nFinal duplicates: {len(final_dups)}")
    for c, n in final_dups[:10]:
        print(f"  U+{ord(c):04X} ('{c}') appears {n} times")
else:
    print("\nFinal: no duplicates")

# Check specific duplicates found in binary
print(f"\nChecking specific duplicates from binary:")
bin_dups = [0x80FD, 0x77ED, 0x7597, 0x8239, 0x8F7B]
for unicode_val in bin_dups:
    char = chr(unicode_val)
    count = final_counter.get(char, 0)
    print(f"  U+{unicode_val:04X} ('{char}') appears {count} times in unique_chars")
