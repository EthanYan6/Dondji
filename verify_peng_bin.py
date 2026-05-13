#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Verify peng pinyin data in generated font"""

import struct

# 读取二进制文件
with open('docs/font/cn_font.bin', 'rb') as f:
    data = f.read()

# 拼音表偏移量
PY_OFFSET = 36652  # CN_FONT_PY_OFFSET

# 解析拼音表
offset = PY_OFFSET
syllables = {}

while offset < len(data) - 1:
    # 读取拼音长度
    py_len = data[offset]
    if py_len == 0 or offset + py_len >= len(data):
        break
    
    # 读取拼音
    py = data[offset+1:offset+1+py_len].decode('ascii', errors='ignore')
    
    # 读取字符数量
    char_count = data[offset+1+py_len]
    
    # 读取字符索引
    indices = []
    for i in range(char_count):
        idx_offset = offset+1+py_len+1 + i*2
        if idx_offset+1 < len(data):
            idx = (data[idx_offset] << 8) | data[idx_offset+1]
            indices.append(idx)
    
    syllables[py] = indices
    offset += 1 + py_len + 1 + char_count * 2

# 检查"peng"
if 'peng' in syllables:
    print("'peng'音节:")
    print(f"  索引：{syllables['peng']}")
    
    # 从索引区读取 Unicode 码点
    INDEX_OFFSET = 31416  # CN_FONT_BITMAP_SIZE
    for idx in syllables['peng']:
        entry_offset = INDEX_OFFSET + idx * 4
        entry_value = struct.unpack('<I', data[entry_offset:entry_offset+4])[0]
        unicode_val = (entry_value >> 16) & 0xFFFF
        ch = chr(unicode_val)
        print(f"  索引 {idx}: {ch} (U+{unicode_val:04X})")
else:
    print("错误：'peng'不在拼音表中！")
