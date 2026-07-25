# 中文字库（SPI Flash）

嵌入固件与 Web 刷写使用的字形数据由脚本生成，主输出为 **`docs/font/cn_font.bin`**；生成后会**同步一份**到 **`docs/fonts/cn_font.bin`**（便于按 `docs/fonts` 路径取用）。

## 生成命令

```bash
python App/tools/gen_cn_font.py
```

会更新 `App/cn_font_data.h` 与 `docs/font/cn_font.bin`（当前约 **205367** 字节含版本字节，**6766** 个汉字字形）；运行脚本后会同步 **`docs/fonts/cn_font.bin`**。修改字表后请同步更新 `App/settings.h` 中与 `cn_font_data.h` 一致的 `CN_FONT_*` 常量（详见 `docs/add_cn_char.md`）；Web 刷字库页 `docs/js/flash.js` 中的 `CN_FONT_*`（含 `VERSION_OFFSET`）也需与头文件一致。

## 字库规格

| 参数 | 值 |
|------|-----|
| 字符数量 | 6766 个（GB2312 全部汉字） |
| 字库大小 | 205,367 字节 (约 200 KB) |
| SPI Flash 占用 | 约 9.8% (总容量 2MB) |
| 字模格式 | 12×12 点阵位图 |
| 索引格式 | Unicode → 字符位置映射 |

## 字库来源

字模来源：`App/bdf/wenquanyi_9pt.bdf`（文泉驿点阵宋体）。

字表包含：
- 原有信道命名常用字（约 1400 字）
- GB2312 一级、二级汉字（全部 6763 字）
