#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Check cn_font_data.h index table format
"""

import re

# Read cn_font_data.h
with open('../cn_font_data.h', 'r', encoding='utf-8') as f:
    content = f.read()

# Find index table
match = re.search(r'static const uint32_t cn_font_index\[\d+\] = \{([^}]+)\}', content, re.DOTALL)
if match:
    index_data = match.group(1)
    # Extract first few entries
    entries = re.findall(r'0x([0-9A-Fa-f]{8})', index_data)
    print(f"Total entries: {len(entries)}")
    print(f"\nFirst 10 entries:")
    for i, entry in enumerate(entries[:10]):
        unicode_val = int(entry[:4], 16)
        font_idx = int(entry[4:], 16)
        char = chr(unicode_val)
        print(f"  [{i}] 0x{entry} -> Unicode: U+{unicode_val:04X} ('{char}'), Index: {font_idx}")
    
    # Check 'wei' characters
    print(f"\n'wei' characters (from debug_wei_candidates.py):")
    wei_indices = [20, 60, 118, 181, 210, 530]
    for idx in wei_indices:
        if idx < len(entries):
            entry = entries[idx]
            unicode_val = int(entry[:4], 16)
            font_idx = int(entry[4:], 16)
            char = chr(unicode_val)
            print(f"  Index {idx}: 0x{entry} -> U+{unicode_val:04X} ('{char}'), bitmap_idx: {font_idx}")
