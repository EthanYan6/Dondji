# 添加中文字库字符流程

缺字时按下面顺序做；**不要跳过「同步固件常量」与「改网页常量」**，否则现象与「字库/固件已最新」仍可能对不上。

---

## 为何必须同步 `App/settings.h` 与 `docs/js/flash.js`

固件里**不**从 `cn_font_data.h` 大数组读布局，而是用 **`App/settings.h`** 里的 `CN_FONT_*` 去 SPI Flash 里取位图、Unicode 索引、**拼音表**。

若你重新跑了 `gen_cn_font.py`、字库多了一两个字，**位图 + 索引表变长，拼音表起始字节 `CN_FONT_PY_OFFSET` 会跟着变**。若只刷了新 `cn_font.bin` 却忘了改固件里的旧常量，固件仍用**旧偏移**去扫拼音区，等于从 Flash 里错误位置解析拼音——会出现：

- 任意拼音候选错乱（例如输入 `zhong` 却出现毫不相干的字），或大量音节匹配失败；
- **不是个别音节 bug**，而是整张拼音表整体错位。

网页工具里的 **`docs/js/flash.js`** 若与头文件不一致，校验版本字节、读取索引时会偏离正确地址，同样会造成「字库对了、工具以为错了」或写入错位。

**结论**：每次重生成字库后，必须把 **`cn_font_data.h` 顶部的布局宏**同步到 **`settings.h`** 与 **`flash.js`**，再**重新编译并刷固件**；仅刷字库 bin、不刷含新常量的固件，问题仍在。

---

## 快速添加字符（推荐方式）

使用 `App/tools/cn_chars_append.txt` 文件追加字符，无需修改 `gen_cn_font.py` 中的 `CN_CHARS_500`。

### 步骤 1：添加字符到追加文件

编辑 `App/tools/cn_chars_append.txt`，在末尾添加新字符（每行一个或连续写）：

```
铭缺君餐足浴娱留橘桔
```

以 `#` 开头的行会被忽略。

### 步骤 2：添加拼音映射

编辑 `App/tools/gen_cn_font.py`，在 `PINYIN_MAP` 字典末尾添加：

```python
'铭': 'ming',
'缺': 'que',
'君': 'jun',
'餐': 'can',
'足': 'zu',
'浴': 'yu',
'娱': 'yu',
'留': 'liu',
'橘': 'ju',
'桔': 'jie,ju',  # 多音字用逗号分隔
```

**多音字格式**：`'字': 'pinyin1,pinyin2'`，如 `'桔': 'jie,ju'` 表示桔字同时出现在 jie 和 ju 两个拼音候选中。

### 步骤 3：运行生成脚本

```bash
python App/tools/gen_cn_font.py
```

### 步骤 4：同步常量到 settings.h 和 flash.js

从 `App/cn_font_data.h` 顶部复制以下常量：

| 常量 | 说明 |
|------|------|
| `CN_FONT_CHAR_COUNT` | 字符总数 |
| `CN_FONT_BITMAP_SIZE` | 位图数据大小 |
| `CN_FONT_INDEX_SIZE` | 索引表大小 |
| `CN_FONT_PY_OFFSET` | 拼音表偏移（最易漏！） |
| `CN_FONT_PY_COUNT` | 拼音音节数 |
| `CN_FONT_VERSION` | 版本号 |
| `CN_FONT_VERSION_OFFSET` | 版本字节偏移 |
| `CN_FONT_PY_TOTAL_SIZE` | 拼音表总大小 |

更新到：
- `App/settings.h` 中的 `#if defined(ENABLE_CHINESE)` 块
- `docs/js/flash.js` 顶部的 `const CN_FONT_*`

### 步骤 5：更新文档中的字数说明

更新以下文件中的字符数和字库大小：
- `README.md`、`README.zh.md`、`README.en.md`
- `docs/index.html`
- `docs/fonts/README.md`

### 步骤 6：验证

```bash
# 检查新字符是否在字库中
python -c "
chars = '铭缺君餐足浴娱留橘桔'
with open('App/cn_font_data.h', 'r', encoding='utf-8') as f:
    content = f.read()
for c in chars:
    print(f'{c}: {\"OK\" if c in content else \"MISSING\"}')"
```

---

## 完整检查清单

- [ ] 修改 `App/tools/cn_chars_append.txt`：追加新字符
- [ ] 修改 `App/tools/gen_cn_font.py`：在 `PINYIN_MAP` 添加拼音映射
- [ ] 运行 `python App/tools/gen_cn_font.py`，无报错
- [ ] 确认生成：`App/cn_font_data.h`、`docs/font/cn_font.bin`、`docs/fonts/cn_font.bin`
- [ ] **`App/settings.h`**：同步 `CN_FONT_*` 常量
- [ ] **`docs/js/flash.js`**：同步 `CN_FONT_*` 常量
- [ ] 文档：`README.md`、`README.zh.md`、`README.en.md`、`docs/index.html`、`docs/fonts/README.md`
- [ ] **重新编译固件并烧录**
- [ ] 设备测试：命名信道 → 拼音模式 → 输入新字符拼音，候选应正常

---

## 一键提取常量脚本

生成完成后，在仓库根目录执行：

```bash
python -c "
import re
from pathlib import Path
text = Path('App/cn_font_data.h').read_text(encoding='utf-8')
names = [
    'CN_FONT_FLASH_BASE',
    'CN_FONT_CHAR_COUNT',
    'CN_FONT_BITMAP_SIZE',
    'CN_FONT_INDEX_SIZE',
    'CN_FONT_PY_OFFSET',
    'CN_FONT_PY_COUNT',
    'CN_FONT_VERSION',
    'CN_FONT_VERSION_OFFSET',
]
for n in names:
    m = re.search(r'^#define\s+' + re.escape(n) + r'\s+(\S+)', text, re.M)
    print(('#define %-26s %s' % (n, m.group(1))) if m else '# missing ' + n)
m2 = re.search(r'^#define\s+CN_FONT_PY_TOTAL_SIZE\s+(\S+)', text, re.M)
print(('#define %-26s %s' % ('CN_FONT_PY_TOTAL_SIZE', m2.group(1))) if m2 else '# missing CN_FONT_PY_TOTAL_SIZE')
"
```

**关系校验：**

- `CN_FONT_INDEX_SIZE` = `CN_FONT_CHAR_COUNT * 4`
- `CN_FONT_PY_OFFSET` = `CN_FONT_BITMAP_SIZE + CN_FONT_INDEX_SIZE`
- `CN_FONT_VERSION_OFFSET` = `CN_FONT_PY_OFFSET + CN_FONT_PY_TOTAL_SIZE`
- `cn_font.bin` 文件字节数 = `CN_FONT_VERSION_OFFSET + 1`

---

## 确认字符是否在 BDF 字体中

```bash
python -c "
bdf_path = 'App/bdf/wenquanyi_9pt.bdf'
chars = '铭缺君餐足浴娱留橘桔'
targets = {ord(c): c for c in chars}
with open(bdf_path, 'r', encoding='utf-8') as f:
    for line in f:
        if line.startswith('ENCODING'):
            enc = int(line.split()[1])
            if enc in targets:
                print(f'U+{enc:04X} ({targets[enc]}): found in BDF')
                del targets[enc]
for k, v in targets.items():
    print(f'U+{k:04X} ({v}): NOT in BDF')
"
```

如果 BDF 中没有该字符，需要更换字体或手动制作点阵。

---

## 示例：本次添加字符记录

### 添加的字符

| 字符 | Unicode | 拼音 | 说明 |
|------|---------|------|------|
| 铭 | U+94ED | ming | 信道命名 |
| 缺 | U+7F3A | que | 信道命名 |
| 君 | U+541B | jun | 信道命名 |
| 餐 | U+9910 | can | 信道命名 |
| 足 | U+8DB3 | zu | 信道命名 |
| 浴 | U+6D74 | yu | 信道命名 |
| 娱 | U+5A31 | yu | 信道命名 |
| 留 | U+7559 | liu | 信道命名 |
| 橘 | U+6A58 | ju | 信道命名 |
| 桔 | U+6854 | jie, ju | 多音字 |

### 更新前后对比

| 参数 | 更新前 | 更新后 |
|------|--------|--------|
| 字符数 | 1324 | 1359 |
| 拼音音节 | 331 | 334 |
| 字库大小 | 41,461 bytes | 42,527 bytes |
| Flash 占用 | 1.98% | 2.03% |

### 修改的文件

1. `App/tools/cn_chars_append.txt` - 追加字符
2. `App/tools/gen_cn_font.py` - 添加拼音映射
3. `App/cn_font_data.h` - 自动生成
4. `docs/font/cn_font.bin` - 自动生成
5. `docs/fonts/cn_font.bin` - 自动复制
6. `App/settings.h` - 同步常量
7. `docs/js/flash.js` - 同步常量
8. `README.md` - 更新字数说明
9. `README.zh.md` - 更新字数说明
10. `README.en.md` - 更新字数说明
11. `docs/index.html` - 更新字数说明
12. `docs/fonts/README.md` - 更新字数说明和补充字符表

---

## 多音字处理

多音字需要在 `PINYIN_MAP` 中用逗号分隔多个拼音：

```python
'桔': 'jie,ju',      # 桔子(jié)、桔梗(jú)
'蔚': 'wei,yu',      # 蔚蓝(wèi)、蔚县(yù)
'行': 'xing,hang',   # 行走(xíng)、银行(háng)
```

该字会同时出现在多个拼音的候选列表中。生成后可在 `cn_font_data.h` 的拼音表中验证：

```c
/* jie    */ 0x03, 0x6A, 0x69, 0x65, 0x09, ... 0x05, 0x26,  /* jie → 9 chars */
/* ju     */ 0x02, 0x6A, 0x75, 0x0C, ... 0x05, 0x26,        /* ju → 12 chars */
```

`0x0526` = 1318 是桔的索引，同时出现在 jie 和 ju 两个音节中。

---

## 将来可做：脚本自动同步常量

在 `gen_cn_font.py` 末尾根据本次生成结果自动替换 `settings.h` / `flash.js` 中的 `CN_FONT_*` 数值，可彻底避免漏同步。若你在本仓库实现该逻辑，请在本节补充调用方式。
