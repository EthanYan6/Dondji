#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
提取原始字库（CN_CHARS_500）并生成CSV文件
"""

import csv

# 从 gen_cn_font.py 中提取的 CN_CHARS_500
CN_CHARS_500 = (
    "的是一不了在人有我他这中大来上个国到说们为子和你地出会也时要就能下行对着生里年前面后东西南北小高多少长短快慢好新旧远近安危黑吉辽冀鲁豫晋陕甘川鄂湘皖赣苏浙闽"
    "粤滇黔琼京津沪渝蒙宁藏疆青桂呼沈哈杭合福济郑武沙广深成昆贵兰银厦珠佛莞惠州泉烟台威海徐温嘉绍金柳梧三亚泸绵德阳乐遵义毕铜仁顺都匀王李张刘陈杨赵黄周吴孙马朱胡"
    "郭何林罗梁谢宋唐韩冯董程蔡曹袁邓许傅曾彭吕卢蒋贾丁魏薛叶阎余潘杜左右内外主副正偏收发守听叫紧急救援消防医疗保维修检查测试报警信号连接断开射频道波段功率音量关"
    "启停暂继返确认取二四五六七八九十百千万半点零两几第每各区栋层排座间组队套天水火风山河路桥港村站门口车船街巷坪坝湾坡岭峰谷洞溪池湖江洲岛红蓝绿白紫铁明亮暗清强"
    "弱轻重热冷凉干湿稳满空真假坏进退升降增减存删添加更换移动扩缩显示输入读写播通阻逆常异优先工厂楼房屋舍院场园馆心室厅窗弹墙壁柱顶底板梯走廊电器油汽煤雷光声色形宽"
    "窄浅低厚薄粗细疏密级种类样机飞潜爬跳跑立坐卧倒拿放送迎还买卖借看想思愿意欲需必须可该应被把给让向从过无没又花草树木竹米麦豆瓜果菜茶药鱼鸟兽虫龙凤虎狮熊狼猫狗"
    "鸡鸭鹅铝钢玉石玻璃陶瓷漆布纸皮毛棉麻丝绸缎呢绒春夏秋冬晨昏早晚日夜朝夕阴晴雨雪霜露雾云霞虹赤橙灰褐粉彩浓淡巅昌平协战斗班陵蔚县苑儋范围盒才啊闫党隘服曙炉索蛟"
    "载登鬼朋鼎誉倌猪佩荒柏庭业临于仅仪作侧候储全其典准分切列制力包占即压双反叮名命克咚响噪回型复它完定导尾差带度式待忙念恢息户手打扫描持按控拟提文最条校模止步比"
    "版现画益监盘直码禁称端等简精系繁纪经统置联自航背节英表视觉解语言设调请跨部铃锁键限除静页魅默字数方用省丘丰丹丽之乌乡乾互亓井交亭亳什介仓仔仙令任伊伍休佳依俱"
    "偃傲元兄充兆兖公兴兵冈冕农冠冶凌刊利剑务劲劳勇勒化华博卡卫原及友古句同吾咸哥商喀善嘴团固圆圈土圣圳坊坛坦坻垣垦垫垭城埗埠基堂堡堰塔塘塞墨墩士壶备太头夷奇奉奎"
    "如姑姚姜娄嫂孔孝孟学宏宜宝实审宣宫家容宾宿富寨寮寺寿封尉尔尖尚尤尧居展屯屿岑岗岩岳岸峙峡峪崂崇崎崖嵊嵩巢巨巩巴市师帕帮帽幸庄庆庐库店庙府康延建弓弟当征徒微徽"
    "志忠忻怀恩悟悦感慈扎托扬扶承技抚抢拉拍拔招括挑振授掖搜摩攀故敦整斯施族旗旺易昔昝星昭普景暨曲月朐朔朗望未本术朵权杞松极枝枣柴标栏株格栾桃桐桑桓桦梅梓梨椒楚榄"
    "榆榕榜樟横樵歌氏民永汇汉汕汝汤汨汾沁沂沅沛沟沧沭治沽沾泊法泗泮泰泽泾洈洋洛洪洱流浏浦浩浪浮涉涞涟涡润涿淄淮淼渑渠渭湄湛溆源溧滁滋滑滔滕滦滨滩演漠漯漳潍潢潭潮"
    "潼澜澧澳濮瀣灌灯灵炬烽焦煌照熙熟燕爱牛牟牡特狐独猛献玛环球理瑞瓶甪田由甸界番皇皋盐盖盛盟盱相眉眙眼睢矿砀砂碑碧磐社祁神祥禅禹禺秀私科租秦稷章符綦纵线绛绥网美"
    "群羽老考者耳聊聚肃肇肥胜胶脚腊腿至致舞舟艇良艺芒芜芦芬芳芷苍茂荆荔荣荷莆莎莒莘莱莲获菁菏萝营萧落葛葫蒲蓥蓬虞蚌蜀蜂蟠衡衢褒襄覆观览角讯诸贝贡贤贺资赫超越转轿"
    "辉辛边达迁运逊速遂邑邡邢邯邱邳邵邹郁郊郎郏郓郡郫郯郴郸鄄鄱酒醴野钟钦链锡锦镇闵闾阁阆阜阡阿陀陂际陆陉隆随障雄雅集霆霍霸靖鞍韶项颍饶首香驰驳驻驾骅验骑鲅鲍鸿鹤"
    "鹰鹿麓黎鼓齐局乘执事迪刷址共窑装旅眠"
)

def main():
    print("正在分析原始字库 CN_CHARS_500...")
    
    # 提取所有汉字（Unicode >= 0x4E00）
    chars = [ch for ch in CN_CHARS_500 if ord(ch) >= 0x4E00]
    
    # 去重（按首次出现顺序）
    seen = set()
    unique_chars = []
    for ch in chars:
        if ch not in seen:
            seen.add(ch)
            unique_chars.append(ch)
    
    print(f"原始字库总字符数: {len(chars)}")
    print(f"去重后汉字数: {len(unique_chars)}")
    
    # 生成CSV文件
    output_csv = "原始字库汉字清单_CN_CHARS_500.csv"
    
    print(f"\n正在生成CSV文件: {output_csv}")
    
    with open(output_csv, 'w', encoding='utf-8-sig', newline='') as csvfile:
        fieldnames = ['序号', '汉字', 'Unicode', 'Unicode十六进制', '拼音首字母', '笔画数估算']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        
        writer.writeheader()
        
        for idx, char in enumerate(unique_chars, 1):
            unicode_val = ord(char)
            writer.writerow({
                '序号': idx,
                '汉字': char,
                'Unicode': unicode_val,
                'Unicode十六进制': f'U+{unicode_val:04X}',
                '拼音首字母': get_pinyin_initial(char),
                '笔画数估算': estimate_strokes(char)
            })
    
    print(f"CSV文件生成完成！")
    print(f"文件路径: {output_csv}")
    
    # 统计信息
    print(f"\n统计信息:")
    print(f"总汉字数: {len(unique_chars)}")
    print(f"Unicode范围: U+{min(ord(ch) for ch in unique_chars):04X} - U+{max(ord(ch) for ch in unique_chars):04X}")
    
    # 按拼音首字母统计
    pinyin_stats = {}
    for ch in unique_chars:
        initial = get_pinyin_initial(ch)
        pinyin_stats[initial] = pinyin_stats.get(initial, 0) + 1
    
    print(f"\n按拼音首字母分布:")
    for initial in sorted(pinyin_stats.keys()):
        print(f"  {initial}: {pinyin_stats[initial]} 个汉字")

def get_pinyin_initial(char):
    """获取汉字的拼音首字母（简化版）"""
    # 这是一个简化的映射，实际应该使用pypinyin库
    pinyin_map = {
        '的是一不了在人有我他这中大来上个国到说们为子和你地出会也时要就能下行对着生里年': 'dysbzlnrwtzdldysgdssmnwhendxzsdzcssln',
        '前面后东西南北小高多少长短快慢好新旧远近安危黑吉辽冀鲁豫晋陕甘川鄂湘皖赣苏浙闽': 'qmhdxnbxgdsdckmhxjyjahjlljysgcexawgszm',
        '粤滇黔琼京津沪渝蒙宁藏疆青桂呼沈哈杭合福济郑武沙广深成昆贵兰银厦珠佛莞惠州泉': 'ydqqjjhymngzqghshhhbfjzwsgsckglyszfghzq',
        '烟台威海徐温嘉绍金柳梧三亚泸绵德阳乐遵义毕铜仁顺都匀王李张刘陈杨赵黄周吴孙马朱胡': 'ytwexwjsjlwsyslmdaylzybtrsdywlzlczyzhzwsmzh',
    }
    
    # 尝试从映射中查找
    for chars, initials in pinyin_map.items():
        if char in chars:
            idx = chars.index(char)
            if idx < len(initials):
                return initials[idx].upper()
    
    # 默认返回 '?'
    return '?'

def estimate_strokes(char):
    """估算汉字笔画数（简化版，返回范围）"""
    # 这是一个简化的估算，实际应该查询笔画字典
    unicode_val = ord(char)
    
    # 根据Unicode范围粗略估算
    if unicode_val < 0x4E00:
        return 0
    elif unicode_val < 0x5000:
        return '5-15'
    elif unicode_val < 0x6000:
        return '8-20'
    elif unicode_val < 0x7000:
        return '10-25'
    else:
        return '12-30'

if __name__ == '__main__':
    main()