#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
提取字库汉字并生成CSV文件
"""

import os
import csv
from collections import Counter

def read_text_file(filepath):
    """读取文本文件，返回字符列表"""
    if not os.path.exists(filepath):
        return []
    
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 提取所有汉字（Unicode >= 0x4E00）
    chars = [ch for ch in content if ord(ch) >= 0x4E00]
    return chars

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # 读取各个字库文件
    print("正在读取字库文件...")
    
    # 1. 追加字库
    append_path = os.path.join(script_dir, "App", "tools", "cn_chars_append.txt")
    append_chars = read_text_file(append_path)
    print(f"追加字库: {len(append_chars)} 个汉字")
    
    # 2. 合并字库
    merged_path = os.path.join(script_dir, "App", "tools", "merged_chars.txt")
    merged_chars = read_text_file(merged_path)
    print(f"合并字库: {len(merged_chars)} 个汉字")
    
    # 3. GB2312字库
    gb2312_path = os.path.join(script_dir, "App", "tools", "gb2312_chars.txt")
    gb2312_chars = read_text_file(gb2312_path)
    print(f"GB2312字库: {len(gb2312_chars)} 个汉字")
    
    # 4. gen_cn_font.py 中的 CN_CHARS_500
    # 手动提取（因为这是代码中的字符串）
    print("\n分析 gen_cn_font.py 中的 CN_CHARS_500...")
    
    # 创建一个字典来统计每个汉字的来源
    char_sources = {}
    
    # 从各个来源添加汉字
    for ch in append_chars:
        if ch not in char_sources:
            char_sources[ch] = []
        char_sources[ch].append("追加字库")
    
    for ch in merged_chars:
        if ch not in char_sources:
            char_sources[ch] = []
        char_sources[ch].append("合并字库")
    
    for ch in gb2312_chars:
        if ch not in char_sources:
            char_sources[ch] = []
        char_sources[ch].append("GB2312")
    
    print(f"\n总共有 {len(char_sources)} 个唯一汉字")
    
    # 生成CSV文件
    output_csv = os.path.join(script_dir, "字库汉字清单.csv")
    
    print(f"\n正在生成CSV文件: {output_csv}")
    
    with open(output_csv, 'w', encoding='utf-8-sig', newline='') as csvfile:
        fieldnames = ['序号', '汉字', 'Unicode', 'Unicode十六进制', '来源', '出现次数']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        
        writer.writeheader()
        
        # 按Unicode排序
        sorted_chars = sorted(char_sources.items(), key=lambda x: ord(x[0]))
        
        for idx, (char, sources) in enumerate(sorted_chars, 1):
            unicode_val = ord(char)
            writer.writerow({
                '序号': idx,
                '汉字': char,
                'Unicode': unicode_val,
                'Unicode十六进制': f'U+{unicode_val:04X}',
                '来源': '; '.join(sources),
                '出现次数': len(sources)
            })
    
    print(f"CSV文件生成完成！")
    print(f"文件路径: {output_csv}")
    
    # 统计信息
    print(f"\n统计信息:")
    print(f"总汉字数: {len(char_sources)}")
    print(f"Unicode范围: U+{min(ord(ch) for ch in char_sources.keys()):04X} - U+{max(ord(ch) for ch in char_sources.keys()):04X}")
    
    # 按来源统计
    source_counts = Counter()
    for sources in char_sources.values():
        for source in sources:
            source_counts[source] += 1
    
    print(f"\n各字库汉字数量:")
    for source, count in source_counts.most_common():
        print(f"  {source}: {count} 个汉字")

if __name__ == '__main__':
    main()