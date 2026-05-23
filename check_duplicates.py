#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import re
from collections import Counter

content = open('App/cn_font_data.h', 'r', encoding='utf-8').read()
pattern = r"/\* \[\s*\d+\] '([^']+)' U\+([0-9A-Fa-f]+) \*/"
matches = re.findall(pattern, content)

chars = {}
duplicates = []
for char, code in matches:
    code_int = int(code, 16)
    if code_int in chars:
        duplicates.append((char, code, chars[code_int]))
    else:
        chars[code_int] = char

print(f'总字符数: {len(matches)}')
print(f'唯一字符数: {len(chars)}')

if duplicates:
    print(f'\n发现重复字符: {len(duplicates)} 个')
    print('-' * 40)
    for char, code, first in duplicates:
        print(f'  重复: "{char}" (U+{code}) 首次出现: "{first}"')
else:
    print('\n[OK] 无重复字符')

char_counter = Counter([m[0] for m in matches])
char_dups = [(c, n) for c, n in char_counter.items() if n > 1]
if char_dups:
    print(f'\n按字符本身统计重复: {len(char_dups)} 个')
    for c, n in char_dups:
        print(f'  "{c}" 出现 {n} 次')
