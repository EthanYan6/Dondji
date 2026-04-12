#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Extract Chinese characters from BDF font file for UV-K5 menu
"""

import sys
import os

# Menu item Chinese characters to support
# 一级菜单: MR信道 VFO信道 收发设置 按键显示 DTMF控制 系统设置 功能选择 系统信息
# 二级菜单和子选项根据Excel表格
MENU_CHARS = """
步进功率宽窄带调制繁忙锁定语音压扩频偏方向值段外发射接收发射存储删除信道名扫描列表
静噪级别声控发射限时省电模式启用频段锁尾音消除中继扫描恢复优先应急
短按长设置自动键盘信道显示电池麦克风增益条背光时间最暗亮收发按键音音量对比度反色锁定仪表收信提醒
DTMF实时监听分隔码组呼码上行下行前导解码响应保持编辑
开机画面类型校准导航恢复出厂
一键对频哑音频谱仪收音机
系统信息
语言切换
中文英文
无手电筒监听扫描报警锁键盘模式背光临时关接收仅主信道静音
经典关开
窄带更窄带
停止载波超时
全部声音消息电压
默认业余禁用解锁
ROGER MDC
频率号名称加
百分比
视觉
精简
宽窄
双频守候跨段转发主发双收
更
移除偏移测试
作右回左常择能选钟铃
信道设置其它关于语言英文界面菜单删存列表亚音段外锁加密模式
叮咚鸡
开机提示魅力北京
接收发射频偏宽窄繁忙压扩扫描优先恢复短按长按键盘电池麦克风
对比度仪表提醒实时分隔组呼上行下行前导保持校准导航出厂
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
