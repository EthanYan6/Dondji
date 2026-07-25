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

"""Read font binary and verify peng indices"""

import struct

# 读取二进制文件
with open('docs/font/cn_font.bin', 'rb') as f:
    data = f.read()

print(f"字库大小：{len(data)} 字节")

# 读取索引表：从偏移量 31416 开始，每个字符 4 字节（Unicode 码点）
index_offset = 31416
index_size = 4  # 每个索引 4 字节

# 读取所有索引
indices = []
for i in range(1309):
    offset = index_offset + i * 4
    codepoint = struct.unpack('<I', data[offset:offset+4])[0]
    indices.append(codepoint)

# 检查 peng 的索引：175, 550, 1145, 1308
peng_indices = [175, 550, 1145, 1308]
print("\npeng 对应的字符（从二进制索引表读取）:")
for idx in peng_indices:
    if idx < len(indices):
        codepoint = indices[idx]
        ch = chr(codepoint)
        print(f"  索引 {idx}: U+{codepoint:04X} = {ch}")
    else:
        print(f"  索引 {idx}: 超出范围")

# 找到所有 peng 音的字
print("\n所有 peng 音的字:")
from tools.gen_cn_font import build_pinyin_table, CN_CHARS_500
append_chars = '免滤所值佬洁讲禾渔鹏'
all_chars = CN_CHARS_500 + append_chars
syllable_to_indices = build_pinyin_table(all_chars)
for idx in syllable_to_indices.get('peng', []):
    if idx < len(all_chars):
        ch = all_chars[idx]
        print(f"  索引 {idx}: {ch} (U+{ord(ch):04X})")
