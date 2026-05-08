# 中文字库（SPI Flash）

嵌入固件与 Web 刷写使用的字形数据由脚本生成，主输出为 **`docs/font/cn_font.bin`**；生成后会**同步一份**到 **`docs/fonts/cn_font.bin`**（便于按 `docs/fonts` 路径取用）。

## 生成命令

```bash
python App/tools/gen_cn_font.py
```

会更新 `App/cn_font_data.h` 与 `docs/font/cn_font.bin`（当前约 **218614** 字节含版本字节，**6766** 个汉字字形）；运行脚本后会同步 **`docs/fonts/cn_font.bin`**。修改字表后请同步更新 `App/settings.h` 中与 `cn_font_data.h` 一致的 `CN_FONT_*` 常量（详见 `docs/add_cn_char.md`）；Web 刷字库页 `docs/js/flash.js` 中的 `CN_FONT_*`（含 `VERSION_OFFSET`）也需与头文件一致。

## 字库结构

| 区域 | 起始地址 | 结束地址 | 大小 | 说明 |
|------|----------|----------|------|------|
| 字库位图数据 | 0x010200 | 0x038B18 | 162,384 字节 | 6766个字符×12行×2字节 |
| 字库索引表 | 0x038B18 | 0x042A54 | 40,596 字节 | 6766个条目×6字节 |
| 拼音索引表 | 0x042A54 | 0x0467D5 | 15,633 字节 | 401个拼音音节 |
| 版本标记 | 0x045915 | 0x045915 | 1 字节 | 版本号=3 |

## 字符集

- **GB2312 完整字符集**：6763 个汉字
- **自定义字符**：3 个（不在 GB2312 中的字符）
- **总计**：6766 个汉字

字模来源：`App/bdf/wenquanyi_9pt.bdf`（文泉驿点阵宋体）。
