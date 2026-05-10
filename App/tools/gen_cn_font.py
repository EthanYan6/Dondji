#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generate Chinese font bitmap + pinyin table for UV-K5 CN channel names.
CN_CHARS_500 is ordered unique Han (~1254); reads BDF and writes cn_font_data.h + cn_font.bin.
"""

import os
import shutil
import sys
from collections import Counter
from pypinyin import pinyin, Style


def load_cn_chars_append(script_dir):
    """
    追加字表：UTF-8 文本，与 CN_CHARS_500 拼接后参与去重排序。
    以 # 开头的行视为注释；可多行或一行连续写汉字。
    """
    append_path = os.path.join(script_dir, "cn_chars_append.txt")
    if not os.path.isfile(append_path):
        return ""
    raw_text = ""
    with open(append_path, "r", encoding="utf-8") as append_file:
        raw_text = append_file.read()
    kept_segments = []
    for single_line in raw_text.splitlines():
        stripped_line = single_line.strip()
        if not stripped_line:
            continue
        if stripped_line.startswith("#"):
            continue
        kept_segments.append(stripped_line)
    append_segment = "".join(kept_segments)
    return append_segment


# ── 中文频道名字表：按首次出现顺序去重（与既有 SPI 字序一致）──
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
    "鹰鹿麓黎鼓齐局乘执事迪刷址共窑装旅"
)


def parse_bdf(filename):
    """Parse BDF font file and extract character bitmaps as uint16_t rows"""
    chars = {}
    with open(filename, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    i = 0
    while i < len(lines):
        line = lines[i].strip()
        if line.startswith('STARTCHAR'):
            encoding = None
            bbx_width = 0
            bitmap = []
            i += 1
            while i < len(lines) and not lines[i].strip().startswith('ENDCHAR'):
                line = lines[i].strip()
                if line.startswith('ENCODING'):
                    encoding = int(line.split()[1])
                elif line.startswith('BBX'):
                    parts = line.split()
                    bbx_width = int(parts[1])
                elif line.startswith('BITMAP'):
                    i += 1
                    while i < len(lines) and not lines[i].strip().startswith('ENDCHAR'):
                        hex_line = lines[i].strip()
                        if hex_line and all(c in '0123456789ABCDEFabcdef' for c in hex_line):
                            # BDF rows are byte-aligned. Pad hex to full row width
                            # so int() produces the correct uint16_t value.
                            row_bytes = (bbx_width + 7) // 8
                            expected_digits = row_bytes * 2
                            if len(hex_line) < expected_digits:
                                hex_line = hex_line.zfill(expected_digits)
                            bitmap.append(int(hex_line, 16))
                        i += 1
                    break
                i += 1
            if encoding is not None:
                chars[encoding] = bitmap
        i += 1
    return chars

def bitmap_to_uint16(bitmap, rows=12):
    """Pad/truncate bitmap rows to fixed row count.
    Values are already proper uint16_t from parse_bdf."""
    result = list(bitmap)
    while len(result) < rows:
        result.append(0)
    return result[:rows]

# ── Pinyin data ──
# Format: (pinyin_syllable, [chars...])
# We'll build this dynamically from CN_CHARS_500

# Full pinyin mapping for common characters
# This is a simplified mapping - we map each character to its most common pinyin
PINYIN_MAP = {
    '的': 'de', '是': 'shi', '一': 'yi', '不': 'bu', '了': 'le',
    '在': 'zai', '人': 'ren', '有': 'you', '我': 'wo', '他': 'ta',
    '这': 'zhe', '中': 'zhong', '大': 'da', '来': 'lai', '上': 'shang',
    '个': 'ge', '国': 'guo', '到': 'dao', '说': 'shuo', '们': 'men',
    '为': 'wei', '子': 'zi', '和': 'he', '你': 'ni', '地': 'di',
    '会': 'hui', '出': 'chu', '也': 'ye', '时': 'shi2', '要': 'yao',
    '就': 'jiu', '能': 'neng', '下': 'xia', '行': 'xing,hang2', '对': 'dui',
    '着': 'zhe2', '生': 'sheng', '里': 'li', '年': 'nian', '前': 'qian',
    '面': 'mian', '后': 'hou', '东': 'dong', '西': 'xi', '南': 'nan',
    '北': 'bei', '小': 'xiao', '高': 'gao', '多': 'duo', '少': 'shao',
    '长': 'chang', '短': 'duan', '快': 'kuai', '慢': 'man', '好': 'hao',
    '新': 'xin', '旧': 'jiu2', '远': 'yuan', '近': 'jin', '安': 'an',
    '危': 'wei2',
    # 省份
    '黑': 'hei', '吉': 'ji', '辽': 'liao', '冀': 'ji2', '鲁': 'lu',
    '豫': 'yu', '晋': 'jin2', '陕': 'shan', '甘': 'gan', '川': 'chuan',
    '鄂': 'e', '湘': 'xiang', '皖': 'wan', '赣': 'gan2', '苏': 'su',
    '浙': 'zhe3', '闽': 'min', '粤': 'yue', '滇': 'dian', '黔': 'qian2',
    '琼': 'qiong',
    # 城市
    '京': 'jing', '津': 'jin3', '沪': 'hu', '渝': 'yu2', '蒙': 'meng',
    '宁': 'ning', '藏': 'zang', '疆': 'jiang', '青': 'qing', '桂': 'gui',
    '呼': 'hu2', '沈': 'shen', '哈': 'ha', '杭': 'hang', '合': 'he2',
    '福': 'fu', '济': 'ji3', '郑': 'zheng', '武': 'wu', '沙': 'sha',
    '广': 'guang', '深': 'shen2', '成': 'cheng', '昆': 'kun', '贵': 'gui2',
    '兰': 'lan', '西': 'xi2', '银': 'yin', '厦': 'xia2', '珠': 'zhu',
    '佛': 'fo', '东': 'dong2', '莞': 'guan', '惠': 'hui2', '泉': 'quan',
    '烟': 'yan', '台': 'tai', '威': 'wei3', '徐': 'xu', '温': 'wen',
    '嘉': 'jia', '绍': 'shao2', '金': 'jin4', '柳': 'liu', '梧': 'wu2',
    '海': 'hai', '三': 'san', '亚': 'ya', '泸': 'lu2', '州': 'zhou',
    '绵': 'mian2', '德': 'de2', '阳': 'yang', '乐': 'le2', '遵': 'zun',
    '义': 'yi2', '毕': 'bi', '铜': 'tong', '仁': 'ren2', '顺': 'shun',
    '都': 'du', '匀': 'yun',
    # 姓氏
    '王': 'wang', '李': 'li2', '张': 'zhang', '刘': 'liu2', '陈': 'chen',
    '杨': 'yang2', '赵': 'zhao', '黄': 'huang', '周': 'zhou2', '吴': 'wu3',
    '孙': 'sun', '马': 'ma', '朱': 'zhu2', '胡': 'hu3', '郭': 'guo2',
    '何': 'he3', '林': 'lin', '罗': 'luo', '梁': 'liang', '谢': 'xie',
    '宋': 'song', '唐': 'tang', '韩': 'han', '冯': 'feng', '董': 'dong3',
    '程': 'cheng2', '蔡': 'cai', '曹': 'cao', '袁': 'yuan2', '邓': 'deng',
    '许': 'xu2', '傅': 'fu2', '曾': 'zeng', '彭': 'peng', '吕': 'lv',
    '卢': 'lu3', '蒋': 'jiang2', '贾': 'jia2', '丁': 'ding', '魏': 'wei4',
    '薛': 'xue', '叶': 'ye2', '阎': 'yan2', '余': 'yu3', '潘': 'pan',
    '杜': 'du2',
    # 方位功能
    '下': 'xia2', '左': 'zuo', '右': 'you2', '内': 'nei', '外': 'wai',
    '主': 'zhu3', '副': 'fu3', '正': 'zheng2', '偏': 'pian',
    '收': 'shou', '发': 'fa', '守': 'shou2', '听': 'ting', '呼': 'hu4',
    '叫': 'jiao', '紧': 'jin5', '急': 'ji4', '救': 'jiu3', '援': 'yuan3',
    '消': 'xiao2', '防': 'fang', '医': 'yi3', '疗': 'liao2',
    '保': 'bao', '维': 'wei5', '修': 'xiu', '检': 'jian', '查': 'cha',
    '测': 'ce', '试': 'shi3', '报': 'bao2', '警': 'jing2', '号': 'hao2',
    '连': 'lian', '接': 'jie', '断': 'duan2', '开': 'kai', '射': 'she',
    '频': 'pin', '道': 'dao2', '波': 'bo', '段': 'duan3', '功': 'gong',
    '率': 'lv2', '音': 'yin2', '量': 'liang2', '关': 'guan2',
    '启': 'qi', '停': 'ting2', '暂': 'zan', '继': 'ji5', '返': 'fan',
    '确': 'que', '认': 'ren3', '取': 'qu',
    # 数字量词
    '二': 'er', '四': 'si', '五': 'wu4', '六': 'liu3', '七': 'qi2',
    '八': 'ba', '九': 'jiu4', '十': 'shi4', '百': 'bai', '千': 'qian3',
    '万': 'wan2', '半': 'ban', '点': 'dian2', '零': 'ling', '两': 'liang3',
    '几': 'ji6', '第': 'di2', '每': 'mei', '各': 'ge2',
    '号': 'hao3', '区': 'qu2', '栋': 'dong4', '层': 'ceng', '排': 'pai',
    '座': 'zuo2', '间': 'jian2', '组': 'zu', '队': 'dui2',
    # 天气地形
    '天': 'tian', '水': 'shui', '火': 'huo', '风': 'feng2',
    '山': 'shan2', '河': 'he4', '桥': 'qiao', '港': 'gang',
    '村': 'cun', '站': 'zhan', '门': 'men2', '口': 'kou', '车': 'che',
    '船': 'chuan2', '街': 'jie2', '巷': 'xiang2', '坪': 'ping',
    '坝': 'ba2', '湾': 'wan3', '坡': 'po', '岭': 'ling2',
    '峰': 'feng3', '谷': 'gu', '洞': 'dong5', '溪': 'xi3',
    '池': 'chi', '湖': 'hu4', '江': 'jiang3',
    '洲': 'zhou3', '岛': 'dao3',
    # 颜色状态
    '红': 'hong', '黄': 'huang2', '蓝': 'lan2', '绿': 'lv4',
    '白': 'bai2', '黑': 'hei2', '紫': 'zi2', '金': 'jin5',
    '铜': 'tong2', '铁': 'tie', '明': 'ming', '亮': 'liang4',
    '暗': 'an2', '清': 'qing2', '强': 'qiang', '弱': 'ruo',
    '轻': 'qing3', '重': 'zhong2', '热': 're', '冷': 'leng',
    '凉': 'liang5', '干': 'gan3', '湿': 'shi5', '稳': 'wen2',
    '满': 'man2', '空': 'kong', '真': 'zhen', '假': 'jia3',
    '坏': 'huai',
    # 动词形容词
    '进': 'jin6', '退': 'tui', '升': 'sheng2', '降': 'jiang4',
    '增': 'zeng2', '减': 'jian3', '加': 'jia4', '更': 'geng',
    '换': 'huan', '移': 'yi4', '扩': 'kuo', '缩': 'suo',
    '显': 'xian', '输': 'shu', '读': 'du3', '写': 'xie3',
    '播': 'bo2', '通': 'tong3', '阻': 'zu2', '逆': 'ni',
    '常': 'chang2', '异': 'yi5',
    # 补充
    '工': 'gong2', '房': 'fang2', '屋': 'wu5', '舍': 'she2',
    '院': 'yuan4', '场': 'chang3', '园': 'yuan5', '馆': 'guan3',
    '室': 'shi6', '厅': 'ting3', '墙': 'qiang2', '壁': 'bi2',
    '柱': 'zhu4', '顶': 'ding2', '底': 'di3', '板': 'ban2',
    '梯': 'ti', '走': 'zou', '廊': 'lang',
    '器': 'qi2', '油': 'you3', '汽': 'qi3', '煤': 'mei2',
    '光': 'guang2', '声': 'sheng3', '色': 'se', '形': 'xing2',
    '宽': 'kuan', '窄': 'zhai', '浅': 'qian4', '厚': 'hou2',
    '薄': 'bo3', '粗': 'cu', '细': 'xi4', '疏': 'shu2', '密': 'mi',
    '电': 'dian3',
    # 扩展
    '级': 'ji7', '种': 'zhong3', '类': 'lei', '样': 'yang3',
    '飞': 'fei', '潜': 'qian5', '爬': 'pa',
    '跳': 'tiao', '跑': 'pao', '立': 'li2', '坐': 'zuo3',
    '卧': 'wo', '倒': 'dao4', '拿': 'na', '放': 'fang2',
    '送': 'song2', '迎': 'ying', '还': 'huan2', '买': 'mai',
    '卖': 'mai2', '借': 'jie3', '看': 'kan', '想': 'xiang3',
    '思': 'si2', '愿': 'yuan6', '意': 'yi6', '欲': 'yu4',
    '需': 'xu2', '必': 'bi3', '须': 'xu3', '该': 'gai',
    '应': 'ying2', '被': 'bei2', '把': 'ba3', '给': 'gei',
    '让': 'rang', '向': 'xiang4', '从': 'cong', '过': 'guo3',
    '无': 'wu6', '没': 'mei2',
    '花': 'hua', '草': 'cao', '树': 'shu', '木': 'mu',
    '竹': 'zhu5', '米': 'mi2', '麦': 'mai3', '豆': 'dou',
    '瓜': 'gua', '果': 'guo4', '菜': 'cai2', '茶': 'cha2',
    '药': 'yao2', '鱼': 'yu5', '鸟': 'niao', '兽': 'shou3',
    '虫': 'chong', '龙': 'long', '凤': 'feng4', '虎': 'hu5',
    '狮': 'shi7', '熊': 'xiong', '狼': 'lang2', '猫': 'mao',
    '狗': 'gou', '鸡': 'ji8', '鸭': 'ya2', '鹅': 'e2',
    '铝': 'lv5', '钢': 'gang2', '玉': 'yu6', '石': 'shi8',
    '玻': 'bo4', '璃': 'li3', '陶': 'tao', '瓷': 'ci',
    '漆': 'qi4', '布': 'bu2', '纸': 'zhi', '皮': 'pi',
    '毛': 'mao2', '棉': 'mian3', '麻': 'ma3', '丝': 'si3',
    '绸': 'chou', '缎': 'duan4', '呢': 'ni2', '绒': 'rong',
    '春': 'chun', '夏': 'xia3', '秋': 'qiu', '冬': 'dong6',
    '晨': 'chen2', '昏': 'hun', '晚': 'wan4', '日': 'ri',
    '夜': 'ye4', '朝': 'zhao2', '夕': 'xi4', '阴': 'yin',
    '晴': 'qing4', '霜': 'shuang', '露': 'lu', '雾': 'wu7',
    '云': 'yun2', '霞': 'xia4', '虹': 'hong2',
    '赤': 'chi2', '橙': 'cheng3', '灰': 'hui', '褐': 'he2',
    '粉': 'fen', '彩': 'cai3', '浓': 'nong', '淡': 'dan',
    # 用户补充
    '巅': 'dian', '昌': 'chang4', '平': 'ping2', '协': 'xie2',
    '战': 'zhan', '斗': 'dou2', '班': 'ban3', '陵': 'ling3',
    '蔚': 'wei7,yu', '县': 'xian', '苑': 'yuan', '儋': 'dan',
    '范': 'fan', '围': 'wei2', '盒': 'he2',
    '才': 'cai', '啊': 'a', '闫': 'yan', '党': 'dang', '隘': 'ai', '服': 'fu', '曙': 'shu',
    '炉': 'lu2', '索': 'suo', '蛟': 'jiao', '载': 'zai',
    '登': 'deng', '鬼': 'gui', '朋': 'peng', '鼎': 'ding',
    '誉': 'yu', '倌': 'guan', '猪': 'zhu', '佩': 'pei',
    '荒': 'huang', '柏': 'bai', '庭': 'ting',
    '局': 'ju', '乘': 'cheng',
    # UI 补充字 PINYIN（与 SPI 字表同批）
    '业': 'ye3', '临': 'lin2', '于': 'yu7', '仪': 'yi8', '作': 'zuo4',
    '侧': 'ce2', '候': 'hou3', '储': 'chu2', '全': 'quan2', '典': 'dian3',
    '准': 'zhun', '分': 'fen2', '切': 'qie', '列': 'lie', '制': 'zhi2',
    '力': 'li4', '包': 'bao3', '占': 'zhan2', '即': 'ji9', '厂': 'chang5',
    '压': 'ya3', '反': 'fan2', '叮': 'ding2', '名': 'ming2', '命': 'ming3',
    '咚': 'dong7', '响': 'xiang5', '噪': 'zao', '回': 'hui2', '型': 'xing3',
    '复': 'fu4', '它': 'ta2', '完': 'wan5', '定': 'ding3', '导': 'dao3',
    '射': 'she2', '尾': 'wei8', '差': 'cha2', '带': 'dai', '度': 'du4',
    '式': 'shi9', '待': 'dai2', '忙': 'mang', '念': 'nian2', '恢': 'hui3',
    '息': 'xi5', '户': 'hu6', '手': 'shou3', '打': 'da2', '扫': 'sao',
    '描': 'miao2', '持': 'chi2', '按': 'an3', '控': 'kong2', '提': 'ti2',
    '文': 'wen2', '最': 'zui', '条': 'tiao2', '校': 'xiao2', '模': 'mo2',
    '止': 'zhi3', '步': 'bu2', '比': 'bi4', '版': 'ban4', '现': 'xian2',
    '画': 'hua2', '益': 'yi9', '监': 'jian2', '盘': 'pan2', '直': 'zhi4',
    '码': 'ma3', '禁': 'jin6', '称': 'cheng3', '端': 'duan2', '等': 'deng2',
    '简': 'jian3', '精': 'jing3', '系': 'xi6', '繁': 'fan3', '纪': 'ji10',
    '经': 'jing2', '统': 'tong2', '继': 'ji11', '置': 'zhi5', '联': 'lian2',
    '自': 'zi3', '航': 'hang3', '节': 'jie4', '英': 'ying3', '表': 'biao',
    '视': 'shi10', '觉': 'jue', '解': 'jie5', '言': 'yan3', '设': 'she3',
    '试': 'shi11', '请': 'qing5', '跨': 'kua', '部': 'bu3', '铃': 'ling4',
    '锁': 'suo2', '键': 'jian4', '限': 'xian3', '除': 'chu3', '静': 'jing4',
    '页': 'ye5', '魅': 'mei4', '默': 'mo3',
    '弹': 'tan',

    '克': 'ke', '拟': 'ni', '背': 'bei3', '语': 'yu', '调': 'tiao3',
    '字': 'zi', '数': 'shu', '方': 'fang', '用': 'yong', '省': 'sheng',
    '迪': 'di',
    '儿': 'er',
    '韵': 'yun',
    '体': 'ti',
    '情': 'qing',
    # 追加常用（信道命名 / UI）
    '引': 'yin',
    '物': 'wu',
    '玩': 'wan',
    '具': 'ju',
    '伟': 'wei',
    '炜': 'wei',
    '护': 'hu',
    '择': 'ze',
    '选': 'xuan',
    '闭': 'bi',
    '刷': 'shua',
    '址': 'zhi',
    '共': 'gong',
    '窑': 'yao',
    '装': 'zhuang',
    '旅': 'lv',
}

# Remove duplicate keys (keep first occurrence), support comma-separated multi-pinyin
_seen = {}
PINYIN_MAP_CLEAN = {}
for ch, py in PINYIN_MAP.items():
    if ch not in _seen:
        _seen[ch] = True
        # Strip numeric suffix for grouping, support comma-separated pinyin
        base_pys = []
        for part in py.split(','):
            base_py = ''.join(c for c in part.strip() if not c.isdigit())
            if base_py:
                base_pys.append(base_py)
        PINYIN_MAP_CLEAN[ch] = base_pys

def get_pinyin(ch):
    """Get pinyin for a character using pypinyin library"""
    try:
        py_list = pinyin(ch, style=Style.NORMAL)
        if py_list and py_list[0]:
            return py_list[0][0]
    except:
        pass
    return None

def build_pinyin_table(char_list):
    """Build pinyin syllable → character indices mapping"""
    syllable_to_indices = {}
    for i, ch in enumerate(char_list):
        # First try PINYIN_MAP_CLEAN (list of pinyin), then try pypinyin
        pys = PINYIN_MAP_CLEAN.get(ch)
        if pys is None:
            py = get_pinyin(ch)
            pys = [py] if py else ['zz']
        for py in pys:
            if py not in syllable_to_indices:
                syllable_to_indices[py] = []
            syllable_to_indices[py].append(i)
    return syllable_to_indices

def generate_header(char_list, bdf_chars, output_file):
    """Generate C header with full font data arrays for embedded SPI Flash init"""

    # Filter characters that exist in BDF
    valid_chars = []
    missing = []
    for ch in char_list:
        code = ord(ch)
        if code in bdf_chars:
            valid_chars.append(ch)
        else:
            missing.append(ch)

    if missing:
        print(f"Warning: {len(missing)} characters not found in BDF font:")
        print(''.join(missing))

    print(f"Valid characters: {len(valid_chars)}")

    # Generate font bitmaps
    font_data = []
    char_map = []  # [(unicode, index), ...]

    for ch in valid_chars:
        code = ord(ch)
        bitmap = bdf_chars[code]
        char_map.append((code, len(font_data)))
        rows = bitmap_to_uint16(bitmap)
        font_data.extend(rows)

    # Build pinyin table
    pinyin_table = build_pinyin_table(valid_chars)
    pinyin_sorted = sorted(pinyin_table.items())

    # Calculate sizes
    font_size = len(font_data) * 2
    index_size = len(char_map) * 4
    pinyin_offset = font_size + index_size

    # Calculate pinyin total size
    total_py_bytes = 0
    for py, indices in pinyin_sorted:
        total_py_bytes += 1 + len(py) + 1 + len(indices) * 2

    # ── Generate full header with arrays ──
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("/* Auto-generated Chinese font + pinyin data for CN channel names */\n")
        f.write("/* Font: WenQuanYi Bitmap Song 9pt (12x12) */\n")
        f.write(f"/* Characters: {len(valid_chars)}, Pinyin syllables: {len(pinyin_sorted)} */\n\n")
        f.write("#ifndef CN_FONT_DATA_H\n")
        f.write("#define CN_FONT_DATA_H\n\n")
        f.write("#include <stdint.h>\n\n")

        # ── SPI Flash address layout ──
        f.write("/* SPI Flash layout for CN font data (base: 0x010200) */\n")
        f.write(f"#define CN_FONT_FLASH_BASE      0x010200u\n")
        f.write(f"#define CN_FONT_CHAR_COUNT      {len(valid_chars)}u\n")
        f.write(f"#define CN_FONT_BITMAP_SIZE     {font_size}u   /* {len(font_data)} uint16_t */\n")
        f.write(f"#define CN_FONT_INDEX_SIZE      {index_size}u   /* {len(char_map)} entries x 4 bytes */\n")
        f.write(f"#define CN_FONT_PY_OFFSET       {pinyin_offset}u  /* pinyin table offset from base */\n")
        f.write(f"#define CN_FONT_PY_COUNT        {len(pinyin_sorted)}u\n")
        f.write(f"#define CN_FONT_VERSION         2u\n")
        f.write(f"#define CN_FONT_VERSION_OFFSET  {font_size + index_size + total_py_bytes}u  /* version byte offset from base */\n\n")

        # ── Font bitmap data ──
        f.write(f"/* Font bitmaps: {len(valid_chars)} chars x 12 rows x uint16_t */\n")
        f.write(f"/* Write to SPI Flash at CN_FONT_FLASH_BASE */\n")
        f.write(f"static const uint16_t cn_font_bitmaps[{len(font_data)}] = {{\n")
        for i, data in enumerate(font_data):
            if i % 12 == 0:
                ch_idx = i // 12
                if ch_idx < len(valid_chars):
                    f.write(f"    /* [{ch_idx:3d}] '{valid_chars[ch_idx]}' U+{ord(valid_chars[ch_idx]):04X} */\n")
            if i % 8 == 0:
                f.write("    ")
            f.write(f"0x{data:04X}")
            if i < len(font_data) - 1:
                f.write(", ")
            if i % 8 == 7:
                f.write("\n")
        f.write("\n};\n\n")

        # ── Unicode index table ──
        f.write(f"/* Unicode → font index mapping */\n")
        f.write(f"/* Write to SPI Flash at CN_FONT_FLASH_BASE + CN_FONT_BITMAP_SIZE */\n")
        f.write(f"static const uint32_t cn_font_index[{len(char_map)}] = {{\n")
        for i, (unicode_val, idx) in enumerate(char_map):
            if i % 8 == 0:
                f.write("    ")
            f.write(f"0x{unicode_val:04X}{idx:04X}")
            if i < len(char_map) - 1:
                f.write(", ")
            if i % 8 == 7:
                f.write("\n")
        f.write("\n};\n\n")

        # ── Pinyin table ──
        f.write(f"/* Pinyin lookup table */\n")
        f.write(f"/* Write to SPI Flash at CN_FONT_FLASH_BASE + CN_FONT_PY_OFFSET */\n")
        f.write(f"/* Format per entry: [str_len:1][ascii:str_len][char_count:1][indices:char_count*2] */\n")
        f.write(f"#define CN_FONT_PY_TOTAL_SIZE  {total_py_bytes}u\n\n")

        f.write(f"static const uint8_t cn_pinyin_data[{total_py_bytes}] = {{\n")
        offset = 0
        for py, indices in pinyin_sorted:
            line = f"    /* {py:6s} */ "
            data_bytes = []
            data_bytes.append(len(py))
            for c in py:
                data_bytes.append(ord(c))
            data_bytes.append(len(indices))
            for idx in indices:
                data_bytes.append(idx >> 8)
                data_bytes.append(idx & 0xFF)
            line += ', '.join(f'0x{b:02X}' for b in data_bytes)
            if offset + len(data_bytes) < total_py_bytes:
                line += ','
            line += f'  /* {py} → {len(indices)} chars */\n'
            f.write(line)
            offset += len(data_bytes)
        f.write("};\n\n")

        f.write(f"/* Character list: {''.join(valid_chars)} */\n\n")
        f.write("#endif /* CN_FONT_DATA_H */\n")

    print(f"Output: {output_file}")
    print(f"Font data: {font_size} bytes ({len(font_data)} uint16_t)")
    print(f"Index table: {index_size} bytes ({len(char_map)} entries)")
    print(f"Pinyin table: {total_py_bytes} bytes ({len(pinyin_sorted)} syllables)")
    print(f"Total: {font_size + index_size + total_py_bytes} bytes")

def generate_bin(char_list, bdf_chars, output_file):
    """Generate binary file for SPI Flash: [bitmaps][index][pinyin][version]"""
    import struct

    valid_chars = [ch for ch in char_list if ord(ch) in bdf_chars]

    font_data = []
    char_map = []
    for ch in valid_chars:
        code = ord(ch)
        bitmap = bdf_chars[code]
        char_map.append((code, len(font_data)))
        font_data.extend(bitmap_to_uint16(bitmap))

    pinyin_table = build_pinyin_table(valid_chars)
    pinyin_sorted = sorted(pinyin_table.items())

    with open(output_file, 'wb') as f:
        # Font bitmaps (uint16_t, little-endian)
        for val in font_data:
            f.write(struct.pack('<H', val))

        # Unicode index (uint32_t, little-endian: unicode<<16 | index)
        for unicode_val, idx in char_map:
            f.write(struct.pack('<I', (unicode_val << 16) | idx))

        # Pinyin table
        for py, indices in pinyin_sorted:
            f.write(struct.pack('B', len(py)))
            f.write(py.encode('ascii'))
            f.write(struct.pack('B', len(indices)))
            for idx in indices:
                f.write(struct.pack('>H', idx))

        # Version marker at offset 20825
        # Pad to version offset
        font_size = len(font_data) * 2
        index_size = len(char_map) * 4
        pinyin_offset = font_size + index_size
        # Calculate pinyin total size
        total_py_bytes = 0
        for py, indices in pinyin_sorted:
            total_py_bytes += 1 + len(py) + 1 + len(indices) * 2
        version_offset = font_size + index_size + total_py_bytes
        # Pad if needed
        current = f.tell()
        if current < version_offset:
            f.write(b'\xff' * (version_offset - current))
        f.write(struct.pack('B', 2))  # CN_FONT_VERSION = 2

    total = os.path.getsize(output_file)
    print(f"Binary: {output_file} ({total} bytes)")

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    bdf_path = os.path.join(script_dir, "..", "bdf", "wenquanyi_9pt.bdf")
    output_path = os.path.join(script_dir, "..", "cn_font_data.h")

    if not os.path.exists(bdf_path):
        print(f"Error: BDF file not found: {bdf_path}")
        sys.exit(1)

    # 若字表中再次出现重复汉字，在此列出（正常情况应为 0）
    source_concat = ''.join(CN_CHARS_500)
    han_in_source = [ch for ch in source_concat if ord(ch) >= 0x4E00]
    han_occurrences = Counter(han_in_source)
    duplicate_pairs = []
    for single_char, occurrence_count in han_occurrences.items():
        if occurrence_count > 1:
            duplicate_pairs.append((single_char, occurrence_count))
    duplicate_pairs.sort(key=lambda item: (-item[1], item[0]))
    redundant_total = 0
    for single_char, occurrence_count in duplicate_pairs:
        redundant_total += occurrence_count - 1
    if duplicate_pairs:
        unique_han_count = len(han_occurrences)
        print(
            "字表源串: 汉字总出现次数 %d，唯一汉字 %d；%d 个字重复出现，冗余 %d 处（输出字序仅保留首次出现）"
            % (len(han_in_source), unique_han_count, len(duplicate_pairs), redundant_total)
        )
        preview_limit = 30
        shown = 0
        for single_char, occurrence_count in duplicate_pairs:
            if shown >= preview_limit:
                rest_count = len(duplicate_pairs) - preview_limit
                print("  ... 另有 %d 个重复字未列出" % rest_count)
                break
            print("  x%d %s (U+%04X)" % (occurrence_count, single_char, ord(single_char)))
            shown += 1
    else:
        print("字表源串: %d 个汉字，无重复" % len(han_in_source))

    # 按 Unicode 顺序遍历字表（tuple 内可为多段字符串拼接成一条）+ 追加字表文件
    append_segment = load_cn_chars_append(script_dir)
    if append_segment:
        print("追加字表 cn_chars_append.txt: %d 个字符（拼接在去重前）" % len(append_segment))
    combined_source_text = "".join(CN_CHARS_500) + append_segment
    seen = set()
    unique_chars = []
    for single_character in combined_source_text:
        if single_character not in seen and ord(single_character) >= 0x4E00:
            seen.add(single_character)
            unique_chars.append(single_character)

    print(f"Target characters: {len(unique_chars)}")

    print(f"Parsing BDF file: {bdf_path}")
    bdf_chars = parse_bdf(bdf_path)
    print(f"Total characters in BDF: {len(bdf_chars)}")

    print("Generating font data...")
    generate_header(unique_chars, bdf_chars, output_path)

    # Generate binary for SPI Flash
    bin_path = os.path.join(script_dir, "..", "..", "docs", "font", "cn_font.bin")
    os.makedirs(os.path.dirname(bin_path), exist_ok=True)
    print("Generating font binary...")
    generate_bin(unique_chars, bdf_chars, bin_path)

    bin_path_fonts = os.path.join(script_dir, "..", "..", "docs", "fonts", "cn_font.bin")
    os.makedirs(os.path.dirname(bin_path_fonts), exist_ok=True)
    shutil.copy2(bin_path, bin_path_fonts)
    print(f"Copied binary to: {bin_path_fonts}")

if __name__ == '__main__':
    main()
