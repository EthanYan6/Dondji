#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Verify yu pinyin data"""

import struct

with open('docs/font/cn_font.bin', 'rb') as f:
    data = f.read()

PY_OFFSET = 36652
INDEX_OFFSET = 31416

offset = PY_OFFSET
syllables = {}

while offset < len(data) - 1:
    py_len = data[offset]
    if py_len == 0 or offset + py_len >= len(data):
        break
    
    py = data[offset+1:offset+1+py_len].decode('ascii', errors='ignore')
    char_count = data[offset+1+py_len]
    
    indices = []
    for i in range(char_count):
        idx_offset = offset+1+py_len+1 + i*2
        if idx_offset+1 < len(data):
            idx = (data[idx_offset] << 8) | data[idx_offset+1]
            indices.append(idx)
    
    syllables[py] = indices
    offset += 1 + py_len + 1 + char_count * 2

# 检查"yu"
if 'yu' in syllables:
    print("'yu'音节（前 10 个字符）:")
    for idx in syllables['yu'][:10]:
        entry_offset = INDEX_OFFSET + idx * 4
        entry_value = struct.unpack('<I', data[entry_offset:entry_offset+4])[0]
        unicode_val = (entry_value >> 16) & 0xFFFF
        ch = chr(unicode_val)
        print(f"  索引 {idx}: {ch} (U+{unicode_val:04X})")
    print(f"  ... 共 {len(syllables['yu'])} 个字符")
    
    # 检查是否包含"虞"字
    yu_indices = syllables['yu']
    # 找到"虞"字的索引
    with open('App/cn_font_data.h', 'r', encoding='utf-8') as f:
        content = f.read()
    import re
    match = re.search(r'/\* Character list: (.+?) \*/', content)
    if match:
        char_list = match.group(1)
        yu_char_index = char_list.index('虞')
        print(f"\n'虞'字索引：{yu_char_index}")
        print(f"'虞'字在'yu'音节中：{yu_char_index in yu_indices}")
else:
    print("错误：'yu'不在拼音表中！")
