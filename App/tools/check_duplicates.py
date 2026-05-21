#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""检测字库中是否有重复汉字"""

import sys
import os

script_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, script_dir)

from gen_cn_font import CN_CHARS_500, load_cn_chars_append

def check_duplicates():
    append_chars = load_cn_chars_append(script_dir)
    all_chars = CN_CHARS_500 + append_chars
    
    seen = {}
    duplicates = []
    
    for i, char in enumerate(all_chars):
        if char in seen:
            duplicates.append({
                'char': char,
                'first_pos': seen[char],
                'second_pos': i,
                'in_base': seen[char] < len(CN_CHARS_500),
                'in_append': i >= len(CN_CHARS_500)
            })
        else:
            seen[char] = i
    
    if duplicates:
        print("=" * 60)
        print("发现重复汉字：")
        print("=" * 60)
        for dup in duplicates:
            print(f"汉字: {dup['char']}")
            print(f"  首次出现位置: {dup['first_pos']} (在{'基础字库' if dup['in_base'] else '追加字库'})")
            print(f"  重复出现位置: {dup['second_pos']} (在{'基础字库' if not dup['in_append'] else '追加字库'})")
            print()
        print(f"总计发现 {len(duplicates)} 个重复汉字")
        return False
    else:
        print("=" * 60)
        print("[OK] 未发现重复汉字")
        print("=" * 60)
        print(f"基础字库: {len(CN_CHARS_500)} 个汉字")
        print(f"追加字库: {len(append_chars)} 个汉字")
        print(f"总计: {len(all_chars)} 个汉字 (去重后)")
        return True

if __name__ == "__main__":
    success = check_duplicates()
    sys.exit(0 if success else 1)
