#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Check pinyin coverage for all characters
"""

from gen_cn_font import load_gb2312_chars, load_cn_chars_append, CN_CHARS_500, get_pinyin, PINYIN_MAP_CLEAN
import os

# Load characters
gb2312_chars = load_gb2312_chars('.')
existing_chars_text = ''.join(CN_CHARS_500)
append_segment = load_cn_chars_append('.')
existing_chars_text += append_segment

# Deduplicate
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

print(f'Total characters: {len(unique_chars)}')

# Check pinyin coverage
has_pinyin = 0
no_pinyin = []
for ch in unique_chars:
    py = PINYIN_MAP_CLEAN.get(ch)
    if py is None:
        py = get_pinyin(ch)
    if py:
        has_pinyin += 1
    else:
        no_pinyin.append(ch)

print(f'Characters with pinyin: {has_pinyin}')
print(f'Characters without pinyin: {len(no_pinyin)}')
if no_pinyin:
    print(f'First 20 without pinyin: {"".join(no_pinyin[:20])}')
