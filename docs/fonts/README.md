# 中文字库（SPI Flash）

嵌入固件与 Web 刷写使用的字形数据由脚本生成，主输出为 **`docs/font/cn_font.bin`**；生成后会**同步一份**到 **`docs/fonts/cn_font.bin`**（便于按 `docs/fonts` 路径取用）。

## 生成命令

```bash
python App/tools/gen_cn_font.py
```

会更新 `App/cn_font_data.h` 与 `docs/font/cn_font.bin`（当前约 **39568** 字节含版本字节，**1262** 个汉字字形）；运行脚本后会同步 **`docs/fonts/cn_font.bin`**。修改字表后请同步更新 `App/settings.h` 中与 `cn_font_data.h` 一致的 `CN_FONT_*` 常量（详见 `docs/add_cn_char.md`）；Web 刷字库页 `docs/js/flash.js` 中的 `CN_FONT_*`（含 `VERSION_OFFSET`）也需与头文件一致。

## 近期补充字符

| 汉字 | 用途简述 |
|------|----------|
| 范、围 | 「锁键范围」等界面文案 |
| 盒 | UI 或其它文案中的「盒」字 |
| 才、啊、闫、党、隘、服、曙 | 本批新补入 BDF 字模；香、中、国、务 等字原字表已含 |
| 登、鬼、朋、鼎、誉、倌、猪、佩、荒、柏、庭 | 信道名 / UI 缺字补全 |
| 执、事 | 常用词 / 信道名等缺字补全 |
| 窦、司、客 | 常用词 / 信道名等缺字补全 |

字模来源：`App/bdf/wenquanyi_9pt.bdf`（文泉驿点阵宋体）。
