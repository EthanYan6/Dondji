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

"""Test pinyin table generation"""

import sys
sys.path.insert(0, '.')
from gen_cn_font import PINYIN_MAP_CLEAN, build_pinyin_table, CN_CHARS_500

# 追加字符
append_chars = '免滤所值佬洁讲禾渔鹏'
all_chars = CN_CHARS_500 + append_chars

print(f"总字符数：{len(all_chars)}")

# 检查'虞'和'鹏'的拼音
print(f"\n'虞'在 PINYIN_MAP_CLEAN 中：{'虞' in PINYIN_MAP_CLEAN}")
print(f"'鹏'在 PINYIN_MAP_CLEAN 中：{'鹏' in PINYIN_MAP_CLEAN}")

# 检查索引
yu_index = all_chars.index('虞')
peng_index = all_chars.index('鹏')
print(f"\n'虞'字索引：{yu_index}")
print(f"'鹏'字索引：{peng_index}")

# 生成拼音表
syllable_to_indices = build_pinyin_table(all_chars)

# 检查'peng'和'yu'对应的索引
print(f"\n'peng'对应的索引：{syllable_to_indices.get('peng', [])}")
print(f"'yu'对应的索引：{syllable_to_indices.get('yu', [])}")

# 检查这些索引对应的字符
if 'peng' in syllable_to_indices:
    print("\n'peng'音节对应的字符:")
    for idx in syllable_to_indices.get('peng', []):
        if idx < len(all_chars):
            ch = all_chars[idx]
            print(f"  {idx}: {ch} (U+{ord(ch):04X})")

if 'yu' in syllable_to_indices:
    print("\n'yu'音节对应的字符:")
    for idx in syllable_to_indices.get('yu', []):
        if idx < len(all_chars):
            ch = all_chars[idx]
            print(f"  {idx}: {ch} (U+{ord(ch):04X})")
