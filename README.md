<div align="center">

# 🚀 Dondji（叮咚鸡）

> Firmware for UV-K1 / UV-K5 V3 (PY32F071 MCU)

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

## ⭐ Star History

<p align="center">
  <a href="https://star-history.com/#EthanYan6/Dondji&Date">
    <img src="https://api.star-history.com/svg?repos=EthanYan6/Dondji&type=Date&cache=0" />
  </a>
</p>

---

## ✨ Features

- Based on **F4HWN firmware 5.3.1**
- 🎨 Motorola R7-style UI (branch: `motorola_r7`)
- 📡 Dual VFO UI redesign (based on UV-KX firmware ideas)
- 📱 Classic mobile-style icon menu
- 🔒 Lock screen UI redesign
- 🌏 Chinese / English localization
- 🧭 Menu restructuring & UI refactor
- 📝 Reworked channel rename flow; phone-style nine-grid (T9) text input

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
| 刷字库 | 将 1350 个中文字符字库写入 SPI Flash，支持信道中文命名 |
| 备份校准 | 导出设备校准数据备份 |
| 恢复校准 | 从备份文件恢复校准数据 |

### 使用步骤

1. 用 Chrome / Edge / Opera 打开 [刷机页面](https://ethanyan6.github.io/Dondji/)
2. **刷固件**：按住 PTT 键开机进入 BOOT 模式 → USB 连接电脑 → 点击「刷固件」
3. **刷字库**：固件启动后（无需按 PTT）→ USB 连接电脑 → 点击「刷字库」

> ⚠️ 刷字库需要先刷入本固件，字库通过 USB 写入 SPI Flash，首次写入后固件启动时自动跳过重复写入。

### 字库技术参数

| 参数 | 值 |
|------|------|
| 字库文件 | `docs/font/cn_font.bin` |
| SPI Flash 起始地址 | `0x010200` |
| 字符数量 | 1350 个 |
| 字库大小 | 42,252 字节 (约 41.3 KB) |
| SPI Flash 占用 | 1.99% (总容量 2MB) |

### 技术说明

- 基于 Web Serial API，通过 USB 串口与设备通信
- 固件刷入使用 PY32 bootloader 协议（256 字节分页）
- 字库刷入使用自定义 SPI Flash 写入命令（0x0521），120 字节分块
- 字库格式：12×12 点阵位图 + Unicode 索引表 + 拼音查找表

---

## 🧭 How to Build

```bash
git checkout motorola_r7
python update_cn_font.py --append "新字若干"
python App/tools/check_pinyin_map.py
./compile-with-docker.sh Fusion -DAUTHOR_STRING_2=BD1AHN
```
