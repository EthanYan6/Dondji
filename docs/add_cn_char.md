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

## 检查清单（添加缺字后逐项打勾）

复制到 PR / 记事本里自用：

- [ ] 修改 `App/tools/gen_cn_font.py`：`CN_CHARS_500`（必要时 `PINYIN_MAP`）
- [ ] 运行 `python App/tools/gen_cn_font.py`，无报错
- [ ] 确认生成：`App/cn_font_data.h`、`docs/font/cn_font.bin`，且已复制到 `docs/fonts/cn_font.bin`（脚本会自动 copy）
- [ ] **`App/settings.h`**：将 `cn_font_data.h` 中下列宏与之一致（见下文「一键对照」）
  - `CN_FONT_CHAR_COUNT`、`CN_FONT_BITMAP_SIZE`、`CN_FONT_INDEX_SIZE`
  - **`CN_FONT_PY_OFFSET`**（最易漏，漏则拼音全错）
  - `CN_FONT_PY_COUNT`、`CN_FONT_VERSION`、`CN_FONT_VERSION_OFFSET`、`CN_FONT_PY_TOTAL_SIZE`
- [ ] **`docs/js/flash.js`**：同上命名的一组 `CN_FONT_*` 常量（勿手写展示用字数，应用 `CN_FONT_CHAR_COUNT`）
- [ ] 文档与门户页（字数、体积与当前 bin 一致即可）：`README.md`、`README.zh.md`、`README.en.md`、`docs/index.html`、`docs/fonts/README.md`
- [ ] **重新编译固件并烧录**；再按需 USB 刷字库（或一并重新刷 `cn_font.bin`）
- [ ] 设备上试：命名信道 → 拼音模式 → 输入此前出过问题的音节，候选应正常

---

## 从 `cn_font_data.h` 抄宏到 `settings.h` / `flash.js`（建议）

生成完成后，在仓库根目录执行（需 Python 3）：

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

把输出逐行覆盖到：

- `App/settings.h` 里 `#if defined(ENABLE_CHINESE)` 块中的 `CN_FONT_*`（C 用 `1234u` 形式）
- `docs/js/flash.js` 顶部对应 `const`（JS 无 `u` 后缀，数值相同）

**关系校验（应用 `ENABLE_CHINESE` 那组定义）：**

- `CN_FONT_INDEX_SIZE` = `CN_FONT_CHAR_COUNT * 4`
- `CN_FONT_BITMAP_SIZE` = 字符数 × 24（12 行 × 每行 2 字节 × 列宽以生成脚本为准；以头文件为准）
- **`CN_FONT_PY_OFFSET`** = `CN_FONT_BITMAP_SIZE + CN_FONT_INDEX_SIZE`
- **`CN_FONT_VERSION_OFFSET`** = `CN_FONT_PY_OFFSET + CN_FONT_PY_TOTAL_SIZE`
- **`cn_font.bin` 文件字节数** = `CN_FONT_VERSION_OFFSET + 1`（末尾 1 字节为版本号）

---

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

`CN_CHARS_500` 为**已去重**的连续字表（多行字符串隐式拼接）。在**字序末尾**追加新字（在最后一个 `"..."` 行内或另起一行继续接字符串），**不要**重复已出现的字。跑 `python App/tools/gen_cn_font.py` 时若源串仍有重复会打印警告。

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

这会生成 / 更新：

- `App/cn_font_data.h` — C 头文件（含布局常量与数组）
- `docs/font/cn_font.bin` — SPI Flash 二进制字库
- 同步复制：`docs/fonts/cn_font.bin`

## 5. 更新 settings.h 常量

文件：`App/settings.h`

用 **`cn_font_data.h` 顶部** 的值更新 `ENABLE_CHINESE` 区块中的以下常量（**禁止**凭记忆手填数字，务必以生成头文件为准）：

```c
#define CN_FONT_CHAR_COUNT      XXXXu
#define CN_FONT_BITMAP_SIZE     XXXXXu
#define CN_FONT_INDEX_SIZE      XXXXu
#define CN_FONT_PY_OFFSET       XXXXXu   /* 最易漏；错则拼音检索全局错位 */
#define CN_FONT_PY_COUNT        XXXu
#define CN_FONT_VERSION         XXXXu
#define CN_FONT_VERSION_OFFSET  XXXXXu
#define CN_FONT_PY_TOTAL_SIZE   XXXXu
```

`CN_FONT_FLASH_BASE`（一般为 `0x010200u`）通常不变，除非硬件分区改动。

## 6. 更新 docs/js/flash.js

将 **`cn_font_data.h` 中同名布局常量** 写到文件顶部 `const CN_FONT_*`，与 `settings.h` 数值一致。  
展示「当前字库多少字」时使用 **`${CN_FONT_CHAR_COUNT}`**，避免写死「1254」等历史数字。

## 7. 更新说明文档（可选但建议）

字符数、bin 大小、Flash 占用比例变化时，同步：

- `README.md`、`README.zh.md`、`README.en.md`
- `docs/index.html`（刷字库说明中的字数）
- `docs/fonts/README.md`（简述当前 bin 大小与字数）

## 8. 验证

```bash
# 新字是否出现在生成头文件中
python -c "
with open('App/cn_font_data.h', 'r', encoding='utf-8') as f:
    content = f.read()
print('新字符' in content)
"

# bin 大小应等于 VERSION_OFFSET + 1（Windows 可用 PowerShell：(Get-Item docs/font/cn_font.bin).Length）
wc -c docs/font/cn_font.bin
```

---

## 将来可做：由脚本自动写回常量（未实现）

在 `gen_cn_font.py` 末尾根据本次生成结果自动替换 `settings.h` / `flash.js` 中的 `CN_FONT_*` 数值，可彻底避免漏同步。若你在本仓库实现该逻辑，请在本节补充调用方式。
