#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Check which characters are missing from BDF"""

import sys
sys.path.insert(0, 'App/tools')
from gen_cn_font import CN_CHARS_500, parse_bdf

append_chars = "免滤所值佬洁讲禾渔鹏"
all_chars = CN_CHARS_500 + append_chars

# 解析 BDF
bdf_path = 'App/bdf/wenquanyi_9pt.bdf'
print(f"Parsing BDF: {bdf_path}")
bdf_chars = parse_bdf(bdf_path)
print(f"Total BDF characters: {len(bdf_chars)}")

# 检查哪些字符不在 BDF 中
missing = []
for i, ch in enumerate(all_chars):
    code = ord(ch)
    if code not in bdf_chars:
        missing.append((i, ch, code))

print(f"\nMissing {len(missing)} characters:")
for idx, ch, code in missing:
    print(f"  Index {idx}: {ch} (U+{code:04X})")

# 检查'鹏'字是否在 BDF 中
peng_code = ord('鹏')
print(f"\n'鹏'字 (U+{peng_code:04X}) 在 BDF 中：{peng_code in bdf_chars}")
