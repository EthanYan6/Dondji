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

"""Extract and check character list from cn_font_data.h"""

with open('App/cn_font_data.h', 'r', encoding='utf-8') as f:
    content = f.read()

# 提取字符列表
import re
match = re.search(r'/\* Character list: (.+?) \*/', content)
if match:
    char_list = match.group(1)
    print(f"总字符数：{len(char_list)}")
    
    # 找到'鹏'和'蓬'的索引
    peng_index = char_list.index('鹏')
    peng2_index = char_list.index('蓬')
    
    print(f"'鹏'字索引：{peng_index}")
    print(f"'蓬'字索引：{peng2_index}")
    
    # 检查索引 1145 和 peng_index 的字
    print(f"\n索引 1145 的字：{char_list[1145]} (U+{ord(char_list[1145]):04X})")
    print(f"索引 {peng_index} 的字：{char_list[peng_index]} (U+{ord(char_list[peng_index]):04X})")
    
    # 检查最后 15 个字
    print(f"\n最后 15 个字:")
    for i in range(len(char_list)-15, len(char_list)):
        print(f"  {i}: {char_list[i]} (U+{ord(char_list[i]):04X})")
