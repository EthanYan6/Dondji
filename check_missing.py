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
