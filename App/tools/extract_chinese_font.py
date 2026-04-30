#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Extract Chinese characters from BDF font file for UV-K5 menu
"""

import sys
import os

# 嵌入中文字模：仅包含 App 下 .c/.h 字符串字面量中出现的 CJK（见 tools/scan_used_chinese.py）。
# 增删中文 UI 文案后请运行: python App/tools/scan_used_chinese.py 核对，再 python App/tools/extract_chinese_font.py
MENU_CHARS = """
一上下业中临主于亚京亮人仅仪优余作侧保信候停先光克入全关其典准出分切列删制前力功加动北即厂压
双反发叮右号名向听启呼命咚响噪回场型增声复外字存它守定宽密对导射尾左差带应度开式待忙恢息户手扩
扫报拟持按换接控描提收数文方无时显暗更最机条校模止步段比池测消率现用电画百益监盘直省短码确示禁
称移窄端等简类精系繁经统继置联背自航色英行表视觉解言警认设试语请调跨输过进道部量铃锁键长间限除静
面五念版纪节页音频风高魅鸡麦默完成打包储占
"""

def parse_bdf(filename):
    """Parse BDF font file and extract character bitmaps"""
    chars = {}
    
    with open(filename, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        
        if line.startswith('STARTCHAR'):
            encoding = None
            bbx = None
            bitmap = []
            
            i += 1
            while i < len(lines) and not lines[i].strip().startswith('ENDCHAR'):
                line = lines[i].strip()
                
                if line.startswith('ENCODING'):
                    encoding = int(line.split()[1])
                elif line.startswith('BBX'):
                    parts = line.split()
                    bbx = (int(parts[1]), int(parts[2]), int(parts[3]), int(parts[4]))
                elif line.startswith('BITMAP'):
                    i += 1
                    while i < len(lines) and not lines[i].strip().startswith('ENDCHAR'):
                        hex_line = lines[i].strip()
                        if hex_line and all(c in '0123456789ABCDEFabcdef' for c in hex_line):
                            bitmap.append(int(hex_line, 16))
                        i += 1
                    break
                i += 1
            
            if encoding is not None and bbx is not None:
                chars[encoding] = {
                    'bbx': bbx,
                    'bitmap': bitmap
                }
        
        i += 1
    
    return chars

def char_to_unicode(char):
    """Convert character to Unicode code point"""
    return ord(char)

def generate_font_data(chars, output_file):
    """Generate C header file with font data"""
    
    unique_chars = list(set(MENU_CHARS.replace('\n', '').replace(' ', '')))
    unique_chars.sort()
    
    print(f"Total unique characters: {len(unique_chars)}")
    print(f"Characters: {''.join(unique_chars)}")
    
    font_data = []
    char_map = {}
    
    for char in unique_chars:
        code = char_to_unicode(char)
        if code in chars:
            char_info = chars[code]
            bbx = char_info['bbx']
            bitmap = char_info['bitmap']
            
            char_map[char] = {
                'index': len(font_data),
                'width': bbx[0],
                'height': bbx[1],
                'x_offset': bbx[2],
                'y_offset': bbx[3]
            }
            
            padded_bitmap = []
            for row in bitmap:
                padded_bitmap.append(row)
            
            while len(padded_bitmap) < 12:
                padded_bitmap.append(0x0000)
            
            font_data.extend(padded_bitmap)
        else:
            print(f"Warning: Character '{char}' (U+{code:04X}) not found in font")
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("/* Auto-generated Chinese font data for UV-K5 menu */\n")
        f.write("/* Font: WenQuanYi Bitmap Song 9pt (12x12) */\n\n")
        f.write("#ifndef FONT_CHINESE_H\n")
        f.write("#define FONT_CHINESE_H\n\n")
        f.write("#include <stdint.h>\n\n")
        
        f.write(f"// Total characters: {len(char_map)}\n")
        f.write(f"// Font data size: {len(font_data)} bytes\n\n")
        f.write(f"const uint16_t gFontChinese[{len(font_data)}] = {{\n")
        
        for i, data in enumerate(font_data):
            if i % 8 == 0:
                f.write("    ")
            f.write(f"0x{data:04X}")
            if i < len(font_data) - 1:
                f.write(", ")
            if i % 8 == 7:
                f.write("\n")
        
        f.write("\n};\n\n")
        
        f.write("// Character to index mapping\n")
        f.write("typedef struct {\n")
        f.write("    uint16_t unicode;\n")
        f.write("    uint16_t index;\n")
        f.write("} t_chinese_char;\n\n")
        
        f.write(f"const t_chinese_char gChineseCharMap[{len(char_map)}] = {{\n")
        
        for i, (char, info) in enumerate(sorted(char_map.items(), key=lambda x: ord(x[0]))):
            f.write(f"    {{0x{ord(char):04X}, {info['index']}}},  // {char}\n")
        
        f.write("};\n\n")
        
        f.write("// Find character index in font data\n")
        f.write("static inline int16_t ChineseCharToIndex(uint16_t unicode) {\n")
        f.write("    for (uint16_t i = 0; i < sizeof(gChineseCharMap) / sizeof(gChineseCharMap[0]); i++) {\n")
        f.write("        if (gChineseCharMap[i].unicode == unicode) {\n")
        f.write("            return gChineseCharMap[i].index;\n")
        f.write("        }\n")
        f.write("    }\n")
        f.write("    return -1;  // Character not found\n")
        f.write("}\n\n")
        
        f.write("#endif  // FONT_CHINESE_H\n")
    
    print(f"Font data written to: {output_file}")
    print(f"Font data size: {len(font_data) * 2} bytes")

def main():
    bdf_file = "bdf/wenquanyi_9pt.bdf"
    output_file = "font_chinese.h"
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    bdf_path = os.path.join(script_dir, "..", bdf_file)
    output_path = os.path.join(script_dir, "..", output_file)
    
    if not os.path.exists(bdf_path):
        print(f"Error: BDF file not found: {bdf_path}")
        sys.exit(1)
    
    print(f"Parsing BDF file: {bdf_path}")
    chars = parse_bdf(bdf_path)
    print(f"Total characters in font: {len(chars)}")
    
    print(f"Generating font data...")
    generate_font_data(chars, output_path)

if __name__ == '__main__':
    main()
