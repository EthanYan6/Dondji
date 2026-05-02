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
面五念版纪节页音频风高魅鸡麦默完成打包储占七万三丘东丰丹为丽义之乌乐九乡乾二云互亓井交亭亳什
仁介从仓仔仙令任伊伍休会佛佳依修俱偃傲元兄充兆兖公六兰兴兵冀内冈冕农冠冶冷凉凌凤刊刘利剑副务
劲劳勇勒匀化区十升华协南博卡卢卫原厦县及友口古句台叶合吉同后吕吴吾周和咸哈哥唐商喀善嘉嘴四团
园固国圆圈土圣地圳坊坛坡坦坪坻垣垦垫垭城埗埠基堂堡堰塔塘塞墨墩壁士壶备夏多大天太头夷奇奉奎好
如姑姚姜威娄嫂子孔孝孟学宁安宏宜宝实审宣宫家容宾宿富寨寮寺寿封尉小少尔尖尚尤尧居展屯山屿岑岗
岛岩岭岳岸峙峡峪峰崂崇崎崖嵊嵩川州巢工巨巩巴市布师帕帮常帽干平年幸广庄庆庐库底店庙府康廊延建
弓弟张强当彭征徐徒微德徽心志忠忻怀急恩悟悦惠感愿慈我战房扎托扬扶承技抚抢拉拍拔招括挑振授掖援搜
摩攀故救敦整斗斯新施族旗日旧旺昆昌明易昔昝星春昭晋普景暨曲曹月朐朔朗望朝木未本术朵权李村杜杞来
杨杭松极林枝枣柱柳柴标栏树株格栾桂桃桐桑桓桥桦梁梅梓梧梨椒楚楼榄榆榕榜樟横樵歌武毛氏民水永汇
汉汕汝江汤汨汾沁沂沅沈沙沛沟沧沪沭河油治沽沾泉泊法泗波泮泰泸泽泾洈洋洛洞津洪洱洲流济浏浙浦浩
浪浮海涉涞涟涡润涿淄淡淮深淼清渑渝渠温渭港湄湖湘湛湾溆源溧溪滁滋滑滔滕滦滨滩演漠漯漳潍潘潜潢
潭潮潼澜澧澳濮瀣灌火灯灵炬点烟热烽焦煌照熙熟燕爱牛牟牡特狐独狮狼猛猫献玉王玛环珠班球理琼瑞瓶
甘甪田由甸界番疆白皇皋皖盐盖盛盟盱相眉眙眼睢石矿砀砂碑碧磐社祁神祥禅福禹禺秀私秋科租秦稷空章竹
符米粤綦纵线绍绛绥绵绿网罗美群羽老考者耳聊聚肃肇肥胜胶能脚腊腿至致舞舟艇良色艺芒芜芦芬花芳芷苍
苏英茂茶荆草荔荣荷莆莎莒莘莞莱莲获菁菏菜萝营萧落葛葫蒙蒲蓝蓥蓬薛藏虎虞蚌蜀蜂蟠街衡衢褒襄西要
覆观览角讯许诸谷豫贝贡贤贵贺资赣赤赫超越路车转轿辉辛边辽达迁运远连逊通速遂遵邑邓邡邢邯邱邳邵邹
郁郊郎郏郑郓郡郫郯郴郸都鄂鄄鄱酒醴里重野金钟钢钦铁铜银链锡锦镇门闵闽闾阁阆阜队阡防阳阴阿陀陂际
陆陉陕陵陶隆随障雄雅集雨雪零雷雾霆霍霞霸青靖鞍韩韶顶项顺颍飞饶馆首香马驰驳驻驾骅验骑魏鱼鲁鲅鲍
鸟鸿鹤鹰鹿麓麻黄黎黑黔鼓齐龙
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
