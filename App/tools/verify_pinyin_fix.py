#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Verify the fixed pinyin table format
"""

import re

# Read cn_font_data.h
with open('../cn_font_data.h', 'r', encoding='utf-8') as f:
    content = f.read()

# Find pinyin table
match = re.search(r'static const uint8_t cn_pinyin_data\[\d+\] = \{([^}]+)\}', content, re.DOTALL)
if match:
    pinyin_data = match.group(1)
    # Extract bytes
    bytes_list = [int(b, 16) for b in re.findall(r'0x([0-9A-Fa-f]{2})', pinyin_data)]
    
    print(f"Total bytes: {len(bytes_list)}")
    
    # Parse first few entries
    offset = 0
    entry_count = 0
    print(f"\nFirst 5 pinyin entries:")
    while offset < len(bytes_list) and entry_count < 5:
        str_len = bytes_list[offset]
        offset += 1
        
        syllable = ''.join(chr(b) for b in bytes_list[offset:offset+str_len])
        offset += str_len
        
        char_count = bytes_list[offset]
        offset += 1
        
        print(f"  {entry_count+1}. '{syllable}' -> {char_count} chars")
        
        # Read first 3 Unicode values
        for i in range(min(3, char_count)):
            unicode_val = (bytes_list[offset] << 8) | bytes_list[offset+1]
            offset += 2
            char = chr(unicode_val)
            print(f"      U+{unicode_val:04X} ('{char}')")
        
        # Skip remaining Unicode values
        offset += (char_count - min(3, char_count)) * 2
        
        entry_count += 1
    
    # Check 'wei' specifically
    print(f"\nSearching for 'wei'...")
    offset = 0
    while offset < len(bytes_list):
        str_len = bytes_list[offset]
        offset += 1
        
        syllable = ''.join(chr(b) for b in bytes_list[offset:offset+str_len])
        offset += str_len
        
        char_count = bytes_list[offset]
        offset += 1
        
        if syllable == 'wei':
            print(f"  Found 'wei' -> {char_count} chars")
            print(f"  First 6 Unicode values:")
            for i in range(min(6, char_count)):
                unicode_val = (bytes_list[offset] << 8) | bytes_list[offset+1]
                offset += 2
                char = chr(unicode_val)
                print(f"    {i+1}. U+{unicode_val:04X} ('{char}')")
            break
        
        # Skip Unicode values
        offset += char_count * 2
