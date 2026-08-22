<div align="center">

# 🚀 Dondji

> Open-source radio firmware project · UV-K1 / UV-K5·K6 V3 (PY32F071)

<p>
  <a href="./README.md">🏠 Home</a> |
  <a href="./README.zh.md">🇨🇳 中文</a>
</p>

</div>

---

## Maintainer

**BD1AHN**

## Official Website

https://ethanyan6.github.io/Dondji/

---

## Commercial Cooperation Statement

The source code of **Dondji** is released under the **Apache License 2.0**.

The source code is freely available for use, including use in commercial products, subject to the terms of the Apache License 2.0.

However, the following are considered brand and official identity assets of the Dondji project:

* The **Dondji** name;
* The Dondji logo;
* The official website;
* The official firmware flashing portal;
* Official project identity;
* Official certification;
* Official cooperation or partnership.

If a commercial entity wishes to obtain any of the following brand or official cooperation rights:

* Dondji brand authorization;
* Dondji co-branded products or partnerships;
* Official certification;
* Official cooperation or partnership;
* Official brand promotion or support;

please contact:

**BD1AHN**

> Commercial use of Dondji source code and authorization to use the Dondji brand or claim an official relationship are separate matters.

For more information:

* Commercial Use Policy: [COMMERCIAL_USE.md](https://github.com/EthanYan6/Dondji/blob/motorola_r7/COMMERCIAL_USE.md)
* Trademark & Brand Policy: [TRADEMARK.md](https://github.com/EthanYan6/Dondji/blob/motorola_r7/TRADEMARK.md)

---

## ✨ Features

* Based on F4HWN **v5.3.1**; current Dondji release **v5.2.2**
* Motorola R7-style UI (`motorola_r7` branch) + classic phone-style icon menu
* Dual VFO / MAIN ONLY layout redesign (UV-KX-inspired)
* Keypad lock / lock-screen UI redesign
* Chinese / English UI with runtime language switch
* Menu restructuring & UI refactor
* Full GB2312 Chinese font in SPI Flash (6766 chars) + T9 pinyin channel naming
* Browser web flash: firmware, font, calibration, config, channel programming, boot logo
* Spectrum (`F+5`): Simple / Professional modes + waterfall
* Toolbox (`F+7`): electronic wooden fish + CW practice
* MDC1200 Roger / unit ID (TX + RX popup)
* Built-in ZH/EN help manual
* Common Fusion extras: dedicated FM radio screen, custom boot logo / boot sound, RX/TX timer, etc.

---

## 📸 Main UI

<img width="643" src="https://github.com/user-attachments/assets/2097c20d-58fc-4577-ba84-dbfc83876e03" />

---

## 🧭 MAIN ONLY Mode

1. Press Menu
2. Open the 3rd icon: **Display**
3. Open item 2: **RxMode**
4. Select **MAIN ONLY**
5. Exit

---

## ⏱ Timer

RX/TX timer is enabled by default in Fusion and other default presets (`ENABLE_FEAT_F4HWN_RX_TX_TIMER`).

To enable it explicitly in a custom build:

```bash
-DENABLE_FEAT_F4HWN_RX_TX_TIMER=ON
```

---

## 🔀 Dual VFO UI

<img width="643" height="407" alt="image" src="https://github.com/user-attachments/assets/d606de69-2cc5-4f27-871b-d4d28c5e967e" />

---

## 🌐 Language

Menu → Display → Lang → 中文 / English

---

## 🌐 Web Flash Tool

Flash firmware and font data directly from your browser — no software installation needed.

👉 **https://ethanyan6.github.io/Dondji/**

| Feature | Description |
|---------|-------------|
| Flash Firmware | Pull latest from GitHub Releases, or select a local .bin file |
| Flash Font | Write 6766 Chinese characters (full GB2312) to SPI Flash for channel naming |
| Dump / Restore Calibration | Export or restore device calibration data |
| Backup / Restore Config | Export or restore menu and key settings |
| Freq Program | Channel / config programming (for this firmware) |
| Boot Logo | Upload a custom 128×64 boot image |

**Steps:**
1. Open [flash page](https://ethanyan6.github.io/Dondji/) in Chrome / Edge
2. **Flash firmware**: Hold PTT while powering on → USB connect → Click "Flash"
3. **Flash font**: After firmware boots (no PTT needed) → USB connect → Click "Flash Font"

> Font flashing requires this custom firmware to be installed first. Font data is written to SPI Flash via USB; the firmware auto-skips re-writing on subsequent boots.

**Font Technical Details:**

| Parameter | Value |
|-----------|-------|
| Font file | `docs/font/cn_font.bin` |
| SPI Flash start address | `0x024000` |
| Character count | 6766 (full GB2312) |
| Font size | 205,367 bytes (about 200 KB) |
| SPI Flash usage | ~9.8% (total capacity 2MB) |

---

## ⚠️ Disclaimer

* Flash at your own risk
* Not official firmware

---

## ❤️ Support

Give a ⭐ if you like this project!

---

## 🙏 Thanks

* BA4QHC
