#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Check for duplicate characters in unique_chars
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

# Merge
unique_chars = list(existing_chars)
existing_set = set(existing_chars)
for char in gb2312_chars:
    if char not in existing_set:
        unique_chars.append(char)
        existing_set.add(char)

print(f"Total characters: {len(unique_chars)}")
print(f"Unique characters: {len(set(unique_chars))}")

# Check for duplicates
counter = Counter(unique_chars)
duplicates = [(char, count) for char, count in counter.items() if count > 1]

if duplicates:
    print(f"\nFound {len(duplicates)} duplicate characters:")
    for char, count in duplicates[:20]:
        print(f"  U+{ord(char):04X} ('{char}') appears {count} times")
else:
    print("\nNo duplicate characters found")

# Check if missing 'wei' chars are in the list
print(f"\nChecking missing 'wei' characters:")
missing_wei = [0x8ECE, 0x709C, 0x7168, 0x9C94]
for unicode_val in missing_wei:
    char = chr(unicode_val)
    count = counter.get(char, 0)
    print(f"  U+{unicode_val:04X} ('{char}') appears {count} times")
