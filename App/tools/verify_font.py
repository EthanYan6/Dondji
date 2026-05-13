#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Verify font characters"""

import sys
sys.path.insert(0, 'App/tools')
from gen_cn_font import CN_CHARS_500, build_pinyin_table, PINYIN_MAP_CLEAN

# 追加字符
append_chars = '免滤所值佬洁讲禾渔鹏'
all_chars = CN_CHARS_500 + append_chars

print(f"总字符数：{len(all_chars)}")

# 检查索引 1145 和 1308
indices_to_check = [175, 550, 1145, 1308]
print("\npeng 对应的字符:")
for idx in indices_to_check:
    if idx < len(all_chars):
        ch = all_chars[idx]
        # 获取拼音
        pys = PINYIN_MAP_CLEAN.get(ch)
        if pys is None:
            from gen_cn_font import get_pinyin
            py = get_pinyin(ch)
            pys = [py] if py else ['unknown']
        print(f"  索引 {idx}: {ch} (U+{ord(ch):04X}) - 拼音：{','.join(pys)}")
    else:
        print(f"  索引 {idx}: 超出范围 (总字符数：{len(all_chars)})")

# 检查'鹏'字的索引
peng_index = all_chars.index('鹏')
print(f"\n'鹏'字的实际索引：{peng_index}")

# 检查'虞'字的索引
yu_index = all_chars.index('虞')
print(f"'虞'字的实际索引：{yu_index}")

# 检查'蓬'字的索引
peng2_index = all_chars.index('蓬')
print(f"'蓬'字的实际索引：{peng2_index}")
