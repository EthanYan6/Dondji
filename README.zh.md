<div align="center">

# 🚀 Dondji（叮咚鸡）

> 基于 PY32F071 的 UV-K1 / UV-K5·K6 V3 固件增强

<p>
  <a href="./README.md">🏠 首页</a> |
  <a href="./README.en.md">🇺🇸 English</a>
</p>

</div>

---

## 商业合作声明

叮咚鸡（Dondji）源码采用 Apache License 2.0 开源协议。

源码开放使用。

但是：

“叮咚鸡（Dondji）”品牌名称、Logo、官方身份及合作关系不属于 Apache License 2.0 授权范围。

任何商业产品如需：

- 官方联名；
- 固件预装；
- 品牌授权；
- 商业合作；

请联系 BD1AHN（联系方式见 [刷机网站](https://ethanyan6.github.io/Dondji/)）。

详细说明：

- [商业使用说明](COMMERCIAL_USE.md)
- [品牌使用政策](TRADEMARK.md)

---

## ✨ 功能特性

* 基于 `F4HWN` **v5.3.1** 开发；当前 Dondji 发布版本 **v5.2.2**
* 仿 Motorola R7 UI（分支：`motorola_r7`）+ 手机风格图标菜单
* 双 VFO / 仅主信道（MAIN ONLY）界面优化（参考 UV-KX-firmware）
* 锁屏 / 键盘锁界面重绘
* 中英双语界面，运行时可切换语言
* 菜单重构与 UI 整理
* 全量 GB2312 中文字库（SPI Flash，6766 字）+ 九宫格拼音（T9）信道命名
* 浏览器在线刷机：固件 / 字库 / 校准 / 配置 / 写频 / 开机画面
* 频谱（`F+5`）：简易 / 专业模式 + 瀑布图
* 工具箱（`F+7`）：电子木鱼 + CW 练习
* MDC1200 Roger / 机组 ID（收发弹窗）
* 中英文使用手册
* Fusion 版本常见能力：独立收音机页、自定义开机画面 / 提示音、收发计时等

---

## 📸 主界面

<img width="643" height="407" src="https://github.com/user-attachments/assets/2097c20d-58fc-4577-ba84-dbfc83876e03" />

---

## 🧭 如何进入 MAIN ONLY（仅主信道）

1. 按 Menu
2. 进入图标菜单第 3 项：**显示**
3. 选择第 2 项：**接收模式**
4. 选择：**仅主信道**
5. 退出菜单

---

## ⏱ 计时功能

Fusion 等默认编译预设已开启收发计时（`ENABLE_FEAT_F4HWN_RX_TX_TIMER`）。

如需在自定义编译中显式开启：

```bash
-DENABLE_FEAT_F4HWN_RX_TX_TIMER=ON
```

---

## 🔀 双 VFO 页面

<img width="643" height="407" alt="image" src="https://github.com/user-attachments/assets/d606de69-2cc5-4f27-871b-d4d28c5e967e" />

---

## 🌐 语言切换

Menu → 显示 → 显示语言 → 中文 / English

---

## 🌐 在线刷机

无需安装任何软件，浏览器直接刷入固件和字库。

👉 **https://ethanyan6.github.io/Dondji/**

| 功能 | 说明 |
|------|------|
| 刷固件 | 从 GitHub Releases 拉取最新固件，或选择本地 .bin 文件 |
| 刷字库 | 6766 个中文字符（GB2312 全部汉字）写入 SPI Flash，支持信道中文命名 |
| 备份 / 恢复校准 | 导出或恢复设备校准数据 |
| 备份 / 恢复配置 | 导出或恢复菜单与按键等配置 |
| 写频 | 信道与相关配置编程（配合本固件） |
| 开机画面 | 上传自定义 128×64 开机图片 |

**步骤：**
1. Chrome / Edge 打开 [刷机页面](https://ethanyan6.github.io/Dondji/)
2. **刷固件**：按住 PTT 开机 → USB 连电脑 → 点「刷固件」
3. **刷字库**：固件启动后 → USB 连电脑 → 点「刷字库」

> 刷字库需先刷入本固件。字库通过 USB 写入 SPI Flash，首次写入后固件启动时自动跳过重复写入。

**字库技术参数：**

| 参数 | 值 |
|------|------|
| 字库文件 | `docs/font/cn_font.bin` |
| SPI Flash 起始地址 | `0x024000` |
| 字符数量 | 6766 个（GB2312 全部汉字） |
| 字库大小 | 205,367 字节 (约 200 KB) |
| SPI Flash 占用 | 约 9.8% (总容量 2MB) |

---

## ⚠️ 注意

* 刷机存在风险
* 本项目为非官方固件

---

## ❤️ 支持

如果觉得不错，欢迎点个 ⭐

---

## 🙏 致谢

* BA4QHC
