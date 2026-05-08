#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generate GB2312 complete character list
GB2312包含6763个汉字（一级汉字3755个 + 二级汉字3008个）
"""

def generate_gb2312_chars():
    gb2312_chars = []
    
    # GB2312汉字编码范围：B0A1-F7FE
    # 一级汉字：16-55区 (B0A1-D7FE)
    # 二级汉字：56-87区 (D8A1-F7FE)
    
    for high_byte in range(0xB0, 0xF8):  # B0-F7
        for low_byte in range(0xA1, 0xFF):  # A1-FE
            try:
                char = bytes([high_byte, low_byte]).decode('gb2312')
                if '\u4e00' <= char <= '\u9fff':  # 只保留汉字
                    gb2312_chars.append(char)
            except:
                pass
    
    return gb2312_chars

if __name__ == '__main__':
    chars = generate_gb2312_chars()
    
    print(f'GB2312汉字总数: {len(chars)}')
    print(f'前20个汉字: {"".join(chars[:20])}')
    print(f'后20个汉字: {"".join(chars[-20:])}')
    
    # 保存到文件
    with open('gb2312_chars.txt', 'w', encoding='utf-8') as f:
        f.write(''.join(chars))
    
    print(f'已保存到 gb2312_chars.txt')
