#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Check pinyin syllable with most characters
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

# Find syllables with more than 255 characters
print("Syllables with > 255 characters (will overflow uint8_t):")
overflow_count = 0
for py, indices in sorted(pinyin_table.items(), key=lambda x: -len(x[1])):
    if len(indices) > 255:
        overflow_count += 1
        chars = [unique_chars[idx] for idx in indices[:10]]
        print(f"  {py}: {len(indices)} chars, example: {''.join(chars)}...")

if overflow_count == 0:
    print("  None found")
else:
    print(f"\nTotal overflow syllables: {overflow_count}")

# Check "wei" specifically
print(f"\n'wei' syllable:")
if 'wei' in pinyin_table:
    indices = pinyin_table['wei']
    chars = [unique_chars[idx] for idx in indices]
    print(f"  Total chars: {len(indices)}")
    print(f"  First 20: {''.join(chars[:20])}")
    print(f"  Last 20: {''.join(chars[-20:])}")
