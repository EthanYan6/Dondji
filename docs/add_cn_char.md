# 添加中文字库字符流程

当需要向中文字库添加新字符时，按以下步骤操作：

## 1. 确认缺失字符

```bash
python -c "
chars = '待检查的汉字'
with open('App/cn_font_data.h', 'r', encoding='utf-8') as f:
    content = f.read()
for c in chars:
    print(f'{c} (U+{ord(c):04X}): {\"found\" if c in content else \"MISSING\"}')
"
```

## 2. 确认 BDF 字体文件包含该字符

```bash
python -c "
bdf_path = 'App/bdf/wenquanyi_9pt.bdf'
targets = {0xXXXX: '字'}  # 替换为实际 Unicode 码点
with open(bdf_path, 'r', encoding='utf-8') as f:
    for line in f:
        if line.startswith('ENCODING'):
            enc = int(line.split()[1])
            if enc in targets:
                print(f'U+{enc:04X} ({targets[enc]}): found')
                del targets[enc]
for k, v in targets.items():
    print(f'U+{k:04X} ({v}): NOT in BDF')
"
```

如果 BDF 中没有该字符，需要更换字体或手动制作点阵。

## 3. 修改 gen_cn_font.py

文件：`App/tools/gen_cn_font.py`

### 3a. 将字符加入 CN_CHARS_500 列表

在 "用户补充" 区域末尾追加新字符。

### 3b. 将拼音加入 PINYIN_MAP

在 "用户补充" 区域末尾追加拼音映射，格式：`'字': 'pinyin'`。
拼音不带声调数字后缀（脚本会自动去除）。

如果一个字有多个读音，用逗号分隔：`'蔚': 'wei7,yu'`。
该字会同时出现在两个拼音的候选列表中。

如果没有显式条目，脚本会 fallback 到 pypinyin 库自动获取，但建议手动添加以确保准确。

## 4. 重新生成字库

```bash
python App/tools/gen_cn_font.py
```

这会生成两个文件：
- `App/cn_font_data.h` — C 头文件（含常量定义）
- `docs/font/cn_font.bin` — SPI Flash 二进制字库

## 5. 更新 settings.h 常量

文件：`App/settings.h`

用 `cn_font_data.h` 中的值更新以下常量：

```c
#define CN_FONT_CHAR_COUNT      XXXXu   // 字符总数
#define CN_FONT_BITMAP_SIZE     XXXXXu  // 位图大小（字符数 × 12 × 2）
#define CN_FONT_INDEX_SIZE      XXXXu   // 索引表大小（字符数 × 4）
#define CN_FONT_PY_OFFSET       XXXXXu  // 拼音表偏移 = BITMAP_SIZE + INDEX_SIZE
#define CN_FONT_PY_COUNT        XXXu    // 拼音音节数
#define CN_FONT_VERSION_OFFSET  XXXXXu  // 版本标记偏移
#define CN_FONT_PY_TOTAL_SIZE   XXXXu   // 拼音表总大小
```

验证公式：
- `PY_OFFSET = BITMAP_SIZE + INDEX_SIZE`
- `VERSION_OFFSET = PY_OFFSET + PY_TOTAL_SIZE`
- `cn_font.bin 文件大小 = VERSION_OFFSET + 1`

## 6. 更新文档（仓库根目录与 docs）

更新以下文件中的字库参数（字符数量、偏移量须与 `cn_font_data.h` 一致）：

- `README.md`、`README.zh.md`、`README.en.md` — 表格中的字符数量等
- `docs/index.html` — 「刷字库」页：支持的汉字个数说明
- `docs/js/flash.js` — `CN_FONT_VERSION_OFFSET`（与 `CN_FONT_VERSION_OFFSET` 宏相同）、从仓库加载字库时的展示文案（如「1243 字符」）
- `docs/fonts/README.md` — 可选：当前 bin 大小与字数简述

## 7. 验证

```bash
# 确认新字符在字库中
python -c "
with open('App/cn_font_data.h', 'r', encoding='utf-8') as f:
    content = f.read()
print('新字符' in content)
"

# 确认 bin 文件大小正确
wc -c docs/font/cn_font.bin
```
