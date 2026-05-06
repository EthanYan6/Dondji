<div align="center">

# 🚀 Dondji

> Firmware enhancement for UV-K1 / UV-K5 V3 (PY32F071)

<p>
  <a href="./README.md">🏠 Home</a> |
  <a href="./README.zh.md">🇨🇳 中文</a>
</p>

</div>

---

## ✨ Features

* Based on F4HWN firmware v5.3.1
* Motorola R7-style UI (`motorola_r7` branch)
* Dual VFO UI redesign
* Classic icon menu
* Lock screen redesign
* Chinese localization
* Reworked channel rename flow; phone-style nine-grid (T9) text input

---

## 📸 Main UI

<img width="643" src="https://github.com/user-attachments/assets/2097c20d-58fc-4577-ba84-dbfc83876e03" />

---

## 🧭 MAIN ONLY Mode

1. Press Menu
2. Go to Display (No.3)
3. Go to RxMode (No.2)
4. Select MAIN ONLY
5. Exit

---

## ⏱ Timer

Enable:

```bash id="en1"
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
| Flash Font | Write 1259 Chinese characters to SPI Flash for channel naming |
| Dump Calibration | Export device calibration data |
| Restore Calibration | Restore calibration from backup |

**Steps:**
1. Open [flash page](https://ethanyan6.github.io/Dondji/) in Chrome / Edge
2. **Flash firmware**: Hold PTT while powering on → USB connect → Click "Flash"
3. **Flash font**: After firmware boots (no PTT needed) → USB connect → Click "Flash Font"

> Font flashing requires this custom firmware to be installed first. Font data is written to SPI Flash via USB; the firmware auto-skips re-writing on subsequent boots.

**Font Technical Details:**

| Parameter | Value |
|-----------|-------|
| Font file | `docs/font/cn_font.bin` |
| SPI Flash start address | `0x010200` |
| Character count | 1259 |
| Font size | 39,418 bytes (about 38.5 KB) |
| SPI Flash usage | 1.88% (total capacity 2MB) |

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
