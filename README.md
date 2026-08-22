<div align="center">

# 🚀 Dondji（叮咚鸡）

> Firmware for UV-K1 / UV-K5·K6 V3 (PY32F071 MCU)  
> 开源对讲机固件项目

<p>
  <a href="./README.zh.md">🇨🇳 中文文档</a> |
  <a href="./README.en.md">🇺🇸 English Docs</a>
</p>

<p>
  <a href="https://github.com/EthanYan6/Dondji/stargazers">
    <img src="https://img.shields.io/github/stars/EthanYan6/Dondji?style=flat-square" />
  </a>
  <a href="https://github.com/EthanYan6/Dondji/network">
    <img src="https://img.shields.io/github/forks/EthanYan6/Dondji?style=flat-square" />
  </a>
  <a href="https://img.shields.io/github/downloads/EthanYan6/Dondji/total">
    <img src="https://img.shields.io/github/downloads/EthanYan6/Dondji/total?style=flat-square" />
  </a>
  <a href="https://github.com/EthanYan6/Dondji/releases">
    <img src="https://img.shields.io/github/v/release/EthanYan6/Dondji?style=flat-square" />
  </a>
  <a href="https://github.com/EthanYan6/Dondji/issues">
    <img src="https://img.shields.io/github/issues/EthanYan6/Dondji?style=flat-square" />
  </a>
  <img src="https://komarev.com/ghpvc/?username=EthanYan6&repo=Dondji&style=flat-square" />
</p>

</div>

---

## Maintainer

**BD1AHN**

## Official Website

https://ethanyan6.github.io/Dondji/

---

## 商业合作声明

叮咚鸡（Dondji）源码采用 **Apache License 2.0** 开源协议。

源码开放使用，包括在遵守 Apache License 2.0 条款的前提下用于商业产品。

但是，以下内容属于 Dondji 项目的品牌及官方身份资产：

* 叮咚鸡（Dondji）名称；
* Dondji Logo；
* 官方网站；
* 官方刷机入口；
* 官方身份；
* 官方认证；
* 官方合作关系。

如商业主体希望获得以下品牌或官方合作权益：

* Dondji 品牌授权；
* Dondji 联名合作；
* 官方认证；
* 官方合作关系；
* 官方品牌宣传或支持；

请联系：

**BD1AHN**

> 商业使用 Dondji 源代码与获得 Dondji 品牌或官方合作授权属于不同事项。

详细说明：

* 商业使用说明：[COMMERCIAL_USE.md](https://github.com/EthanYan6/Dondji/blob/motorola_r7/COMMERCIAL_USE.md)
* 品牌政策：[TRADEMARK.md](https://github.com/EthanYan6/Dondji/blob/motorola_r7/TRADEMARK.md)


---
## ✨ Features

- Based on **F4HWN v5.3.1**; current Dondji release **v5.2.2** (UV-K1 / UV-K5·K6 V3, PY32F071)
- 🎨 Motorola R7-style UI (branch: `motorola_r7`) + classic phone-style icon menu
- 📡 Dual VFO / MAIN ONLY UI redesign (UV-KX-inspired)
- 🔒 Lock screen / keypad-lock UI redesign
- 🌏 Chinese / English UI with runtime language switch
- 🧭 Menu restructuring & UI refactor
- 📝 Full **GB2312** font in SPI Flash (6766 chars) + T9 pinyin channel naming
- 🌐 Browser web flash: firmware, font, calibration, config, writefreq, boot logo
- 📶 Spectrum (`F+5`): Simple / Professional + waterfall
- CW trainer (`F+7`)
- 📡 MDC1200 Roger / unit ID (TX + RX popup)
- 📖 Built-in ZH/EN help manual
- 📻 Common Fusion extras: dedicated FM radio screen, custom boot logo / boot sound, RX/TX timer, etc.

---

## 📸 UI Preview

### 🟢 Main Interface (MAIN ONLY)

<img width="643" height="407" src="https://github.com/user-attachments/assets/2097c20d-58fc-4577-ba84-dbfc83876e03" />

---

### 🔵 Dual VFO Compact UI
<img width="643" height="407" alt="image" src="https://github.com/user-attachments/assets/d606de69-2cc5-4f27-871b-d4d28c5e967e" />


---

### 🟣 System / Other Pages

<img width="635" height="406" alt="image" src="https://github.com/user-attachments/assets/ee9903a5-19d3-44a2-9ba1-4805cb241751" />

<img width="635" height="406" alt="image" src="https://github.com/user-attachments/assets/c9d5fad3-b73a-4d2a-a051-462808bd17f4" />

<img width="635" height="406" alt="image" src="https://github.com/user-attachments/assets/8857e8a0-2c29-43cd-ade6-4f68e1162648" />

<img width="635" height="406" alt="image" src="https://github.com/user-attachments/assets/94f7e1f4-9a02-41ba-a8d2-5206243cec85" />

<img width="635" height="406" alt="image" src="https://github.com/user-attachments/assets/f8c537e7-a0af-486d-b2db-537dcc4dc600" />

<img width="635" height="406" alt="image" src="https://github.com/user-attachments/assets/d51dcb0c-d584-46e0-9618-402f8c1daf1c" />

<img width="635" height="406" alt="image" src="https://github.com/user-attachments/assets/5b3dd19e-6266-42ec-ac22-f7f711947c88" />

<img width="635" height="406" alt="image" src="https://github.com/user-attachments/assets/2b3d488a-1d83-48d8-b441-9bde3312fc01" />

<img width="635" height="406" alt="image" src="https://github.com/user-attachments/assets/bf352a75-8757-4b92-b515-5f7a3b5a0ce0" />

<img width="635" height="406" alt="image" src="https://github.com/user-attachments/assets/5685327c-0b79-471b-90eb-892616e6f204" />

<img width="635" height="406" alt="image" src="https://github.com/user-attachments/assets/57fb6ae7-4a33-491a-83ad-240b1ec8383b" />


---

## 📦 Download

👉 https://github.com/EthanYan6/Dondji/releases

---

## 🌐 Web Flash Tool

在线刷机工具，无需安装任何软件，浏览器直接刷入固件和字库。

👉 **https://ethanyan6.github.io/Dondji/**

### 功能

| 功能 | 说明 |
|------|------|
| 刷固件 | 从 GitHub Releases 拉取最新固件，或选择本地 .bin 文件刷入 |
| 刷字库 | 将 6766 个中文字符字库（GB2312 全部汉字）写入 SPI Flash，支持信道中文命名 |
| 备份 / 恢复校准 | 导出或恢复设备校准数据 |
| 备份 / 恢复配置 | 导出或恢复菜单与按键等配置 |
| 写频 | 信道与相关配置编程（配合本固件） |
| 开机画面 | 上传自定义 128×64 开机图片 |

### 使用步骤

1. 用 Chrome / Edge / Opera 打开 [刷机页面](https://ethanyan6.github.io/Dondji/)
2. **刷固件**：按住 PTT 键开机进入 BOOT 模式 → USB 连接电脑 → 点击「刷固件」
3. **刷字库**：固件启动后（无需按 PTT）→ USB 连接电脑 → 点击「刷字库」

> ⚠️ 刷字库需要先刷入本固件，字库通过 USB 写入 SPI Flash，首次写入后固件启动时自动跳过重复写入。

### 字库技术参数

| 参数 | 值 |
|------|------|
| 字库文件 | `docs/font/cn_font.bin` |
| SPI Flash 起始地址 | `0x024000` |
| 字符数量 | 6766 个（GB2312 全部汉字） |
| 字库大小 | 205,367 字节 (约 200 KB) |
| SPI Flash 占用 | 约 9.8% (总容量 2MB) |

### 技术说明

- 基于 Web Serial API，通过 USB 串口与设备通信
- 固件刷入使用 PY32 bootloader 协议（256 字节分页）
- 字库刷入使用自定义 SPI Flash 写入命令（0x0521），120 字节分块
- 字库格式：12×12 点阵位图 + Unicode 索引表（字符位置映射） + 拼音查找表

---

## 🧭 How to Build

```bash
git checkout motorola_r7
python update_cn_font.py --append "新字若干"
python App/tools/check_pinyin_map.py
./compile-with-docker.sh Fusion -DAUTHOR_STRING_2=BD1AHN
```
