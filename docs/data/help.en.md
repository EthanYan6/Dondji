# Dondji User Manual

This manual is divided into three chapters:

1. **Chapter 1 — System User Guide**: Day-to-day operation after flashing Dondji firmware (radio, scanning, saving channels, naming, spectrum, toolbox, etc.)
2. **Chapter 2 — Firmware & Flashing**: Web-based flashing, calibration, configuration, storage addresses, and technical notes
3. **Chapter 3 — FAQ & Other**: Frequently asked questions and contact information

> Compatible only with Quansheng **UV-K1**, and **UV-K5 / UV-K6 V3**. For K5/K6, confirm the body label shows **V3** (K1 has no V3 variant).

---

# Chapter 1 — System User Guide

Welcome to **Dondji (叮咚鸡)**. This chapter covers basic operation after power-on. For flashing, calibration, and config backup, see Chapter 2.

## 1.1 Basic Operation

### Power, Volume, and Keys

- Turn the knob to power on / off; rotate to adjust volume.
- The **`#` key on the radio is the F key** (short press for F-combo keys; long press to lock the keypad).

### Opening the Menu

1. Short press **MENU** to enter the menu.
2. Use **↑/↓** or number keys to jump to menu items quickly; press **MENU** again to enter a sub-item.
3. **EXIT** returns to the previous level or exits the menu.

### Frequency Mode (VFO) vs Channel Mode (MR)

| Mode | Description |
|------|------|
| **Frequency mode (VFO)** | Tune frequency directly; you can change step size, modulation, etc. |
| **Channel mode (MR)** | Recall saved memory channels |

- Short press **`3`** or **`F+3`** / long press **`3`**: Switch between VFO and MR.
- **In channel mode, after you change tone, power, frequency offset, or other parameters, the changes are only temporary. You MUST save the channel again or they will be lost when you switch channels or power off.** See [1.4 Saving Channels](#14-saving-channels).
- In channel mode, **`F+1`**: Copy the current channel to the corresponding band’s VFO and switch to frequency mode (handy for fine-tuning before saving back).

### A / B Dual-Band and Receive Mode

- **`F+2`** / long press **`2`**: Switch A/B (may be disabled in main-channel-only mode).
- Receive mode (single watch / dual watch / cross-band, etc.) is set in the menu; see the screenshots below for single-watch and dual-watch layouts.

**Single-watch screen:**

![Single-watch screen](images/main_only_s.png)

**Dual-watch screen:**

![Dual-watch screen](images/AB_s.png)

### Common Keys (Main Screen)

| Key | Function |
|------|------|
| `Long 1` or `F+1` | VFO: switch band start frequency; MR: `F+1` copies channel to VFO |
| `Long 2` or `F+2` | Switch A/B channel (may be disabled in main-channel-only mode) |
| `Long 3` or `F+3` | Switch channel mode / frequency mode |
| `Long 4` or `F+4` | One-key frequency scan |
| `F+5` | Enter spectrum |
| `Long 5` | Dual watch + dual VFO: range scan (see 1.3) |
| `Long 6` or `F+6` | Switch power level |
| `F+7` | Open **Toolbox** (electronic wooden fish / CW) — not a game |
| `Long 8` | Reverse frequency (swap TX/RX) |
| `F+8` | Switch backlight mode |
| `Long 9` | Jump to the channel set in **Other → PTT Call** |
| `Long 0` or `F+0` | Open FM radio |
| MR `Long *` or `F+*` | Scan the current scan list |
| VFO `Long *` | Scan frequency upward |
| VFO `F+*` | Scan for tone (CTCSS/DCS) |
| `Long #` (F) | Lock keypad; long press again to unlock |
| `F+↑` / `F+↓` | Adjust squelch (if enabled in firmware) |
| `F+Side key` | Adjust step size (if enabled in firmware) |

### Menu Navigation Summary

- **Direction keys**: Move through menu items  
- **MENU**: Confirm  
- **EXIT**: Back / exit  
- **Number keys**: Quick jump  

---

## 1.2 FM Radio

Dondji uses the onboard **BK1080** for FM broadcast, on a **dedicated radio screen** (not the WFM option in the main-screen modulation menu).

### Enter / Exit

1. On the main screen, **`Long 0`** or **`F+0`** enters the radio.
2. Press **`F+0`** again, or short press **EXIT** (when no save/delete prompt is shown) to return to the main screen.

### Frequency Mode vs Channel Mode (Inside Radio)

- Inside the radio, short press **`3`** or **`F+3`**: Switch between **frequency (VFO)** and **channel (MR)**.
- **Frequency mode**: Enter frequency with number keys, or tune with **↑/↓**.
- **Channel mode**: Switch among saved FM presets.

### Switch Country Band

- On the radio screen, short press **`1`** or **`F+1`** / long press **`1`**: Switch country/region band (if the band is wrong, switch it before searching).

### Save / Delete FM Presets

**Save the current frequency to the preset list (frequency mode):**

1. Tune the frequency, then short press **MENU** — “Save?” appears.
2. Use **↑/↓** to pick a preset slot.
3. Press **MENU** again to confirm.

**Delete the current preset (channel/MR mode):**

1. Short press **MENU** — “Delete?” appears.
2. Press **MENU** again to confirm.

You can also manage some settings via the web “Programming” tools; selected FM frequency/channel/MR/band are written to the config area (see Chapter 2).

### Scan for Stations

| Action | Effect |
|------|------|
| **Long `*`** or **`F+*`** | Clear preset list, then full-band search (A-SCAN); frequencies with signal are stored in the list |
| Short press scan-related action | Scan within the existing list (M-SCAN; on-screen hint) |

Stop scanning: short press **EXIT** or **MENU** (depends on current state).

### Other Keys

| Key | Function |
|------|------|
| `F+8` | Backlight (as needed) |
| `F+9` | Backlight on/off related |
| `F+0` | Exit radio |

### Notes

- In dual watch: the radio is interrupted and exited only when an amateur signal arrives on the **main channel** (see FAQ).
- The radio uses BK1080; dual-watch polling behavior is described in Chapter 3 FAQ #12.

---

## 1.3 Scanning

### Scan Memory Channel List

1. Assign each channel to a scan list first: via the web “Programming” page, or menu **Channels → Channel List**.
2. **MENU** → **Other → Scan List**, choose the list to scan.
3. On the main screen in **channel mode**, **long press `*`** or **`F+*`** to start scanning.
4. When a signal is found, press **MENU** to stop on that channel; with no signal, **MENU** / **EXIT** both exit the scan.

### One-Key Scan and Tone Scan

1. **Long `4`** or **`F+4`**: One-key frequency scan.
2. In **frequency mode**, **`F+*`**: Scan for tone (CTCSS/DCS).

After a result, follow the on-screen **MENU** → “Save?” → (in MR mode, pick channel number) → **MENU** again to write to a memory channel. **EXIT** cancels.

### Range Scan and Spectrum Entry

1. Switch to **dual watch** (range scan is unavailable in single watch).
2. Set both A and B to **frequency mode**.
3. Enter start frequency on A, end frequency on B.
4. **Long `5`**: Start range scan (long press **EXIT** to exit).
5. **`F+5`**: Enter spectrum page (see 1.6).

### Exclude Channel During Scan

While paused on a channel during a scan, **long press MENU** can exclude that channel from the current scan (per firmware prompt).

### Related Menus

| Menu | Purpose |
|------|------|
| Scan List | Choose default list / all, etc. |
| Priority Scan, Priority Channel | Priority channel |
| Scan Resume | Stop on signal / carrier / timeout, etc. |
| Channel List | Whether a channel joins a scan list |

---

## 1.4 Saving Channels

Write the current frequency and parameters to a memory channel.

### Method 1: Menu “Save Channel”

1. On the main screen, set frequency, tone, power, modulation, offset, etc.
2. Short press **MENU** → **Save Channel (ChSave)** → **MENU** again.
3. Use **↑/↓** or enter a channel number to pick the target channel.
4. Press **MENU** → when “Confirm?” appears, press **MENU** again to finish.

To delete: menu **Delete Channel (ChDele)**, with confirmation.

You can also bulk-import channels on the web “Programming” page.

### Method 2: After Editing in Channel Mode — You MUST Save Again

In **channel mode (MR)**, when you change that channel’s tone, power, bandwidth, etc. via the menu:

- Changes may take effect **immediately** while you stay on that channel;
- **If you do NOT run Save Channel (ChSave) again, switching channels or powering off will discard the changes** — they are treated as temporary only.

**Correct workflow:** After any parameter change in MR mode, run **Save Channel (ChSave)** again to write the current settings back to that memory channel. **This step is mandatory; skipping it means your edits are not kept.**

### Method 3: Save Scan Results

After one-key scan or tone scan, follow the on-screen “Save?” flow (see 1.3).

---

## 1.5 Channel Naming and Pinyin Input

### Enter Naming

1. **MENU** → **Name Channel (ChName)**.
2. Select a valid channel → **MENU** to edit.
3. When done, short press **MENU** to move the cursor; at the end, confirm “Confirm?” to save.
4. Channel name is up to **10 bytes** (one character per English letter; about **5 Chinese characters**). Without Chinese characters, dual-watch may swap frequency and channel-name positions (see FAQ #3).

> Chinese display requires the font pack and **Display → Display Language** set to Chinese.

### Input Modes

On the naming screen, short press **`#` (F)** to cycle modes (Chinese UI often starts in Pinyin):

| Mode | Description |
|------|------|
| Pinyin | T9 Pinyin → pick Chinese character |
| Lower / Upper | Multi-tap letters |
| Numbers | Digits only |
| Symbols | Symbol page |

Tips:

- Pinyin: use **2–9** for T9 syllable digits; **↑/↓** for candidates; **\*** backspace.
- Outside Pinyin, long press `#` inserts `#`; long press a digit forces that digit.

#### Pinyin Key Map (T9)

| Key | Letters |
|------|------|
| **2** | A B C |
| **3** | D E F |
| **4** | G H I |
| **5** | J K L |
| **6** | M N O |
| **7** | P Q R S |
| **8** | T U V |
| **9** | W X Y Z |

Example: for “山” (shan), try `7426`, then pick from candidates.

---

## 1.6 Spectrum

On the main screen, **`F+5`** opens the spectrum.

### Display mode: Simple / Expert

Before entering the spectrum, choose the UI style in the menu:

1. Short press **MENU** → find **Spectrum Display (SpDisp)** → press **MENU** again.
2. Use **↑/↓** to select, then **MENU** to confirm:

| Option | Description |
|------|------|
| **Simple** | Spectrum plot sits lower; frequency cursor has guide lines and scale ticks — easier to read for everyday scanning |
| **Expert** | Denser layout with more room for parameters/markers above — better when you need more detail at once |

This setting is stored in configuration (SPI `0x00A148`) and is included in config backup/restore.

![Spectrum screen](images/频谱.jpg)

**Spectrum keys:**

| Key | Function |
|------|------|
| **Number keys** | |
| `0` | Switch modulation (FM/AM/USB) |
| `1` / `7` | Increase/decrease scan step |
| `2` / `8` | Increase/decrease frequency step |
| `3` / `9` | Increase/decrease max dB range |
| `4` | Switch scan points (128/64/32/16) |
| `5` | Frequency entry mode |
| `6` | Switch monitor bandwidth (25/12.5/6.25 kHz) |
| **Function keys** | |
| `↑` / `↓` | Tune frequency up/down |
| `F` | Lower trigger threshold |
| `*` | Raise trigger threshold |
| `MENU` | (reserved) |
| `EXIT` | Exit spectrum |
| **Side keys** | |
| `SIDE1` | Blacklist current frequency |
| `SIDE2` | Toggle backlight |
| `PTT` | Enter STILL and tune to peak |

---

## 1.7 Toolbox (F+7)

On the main screen, **`F+7`** opens the **Toolbox** (not the brick-breaker game):

| Item | Description |
|------|------|
| **Electronic wooden fish** | Fun tap sound |
| **cw** | Morse practice / related features |

**EXIT** leaves the toolbox.

---

## 1.8 MDC ID and TX Tail Tone

Current firmware still uses **MDC1200** (Yan ID replacement is in the design doc; **not yet implemented**).

1. **MENU** → **Other** → **TX Tail (Roger)**: **Off / ROGER / MDC**.
2. With **MDC** selected, edit the 4-digit hex unit ID in the **MDC ID** menu.
3. When the other station sends MDC and local Roger is MDC, the main screen can show an **MDC ID** receive alert.

In config backup, MDC ID is at SPI `0x00A172` (2 bytes); see Chapter 2 address table.

---

## 1.9 Usage Notes

- During TX/RX the voltage meter may jump with current — normal (see FAQ #13).
- Use this site or verified tools for programming and calibration; address layout is not compatible with other firmware.
- Flashing and calibration backup steps are in Chapter 2.

---

# Chapter 2 — Firmware & Flashing

Welcome to the **Dondji flashing website**. This chapter covers browser backup/restore of calibration, flashing firmware and font pack, config, programming, boot logo, and storage addresses.

## 2.1 Preparation and Recommended Order

1. Use **Chrome** or **Edge** (Web Serial required).
2. Connect the radio to the PC with the factory programming cable.
3. **Before your first third-party flash**, back up calibration on **factory firmware, normal power-on main screen** (not BOOT mode).

**Recommended order:**

1. Back up calibration (factory firmware, normal boot)  
2. Flash firmware (BOOT mode: power off, hold **PTT**, then power on)  
3. Flash font pack (normal boot; needed for Chinese menu/channel names)  
4. Verify / restore calibration (normal boot)  
5. Programming / boot image / config backup (as needed)  

Follow each step on the matching website tab.

## 2.2 Back Up Calibration

Calibration holds RSSI, battery voltage, VOX, and other hardware parameters critical for normal operation.

**Steps:**

1. Power on normally to the main operating screen (not BOOT)
2. Connect USB
3. On the web page, choose “Back Up Calibration”
4. Click “Export Calibration Data”
5. Download `calibration.dat` (512 bytes)

**Technical details:**

- Device info request auto-detects firmware version and calibration address  
- **Calibration EEPROM logical address:**
  - Firmware **v5.0.0 and above**: `0xB000`
  - Firmware **below v5.0.0**: `0x1E00`
- Read 16 bytes per transfer, 512 bytes total  
- Physically stored on SPI Flash `0x010000`–`0x0101FF` (see 2.5)

## 2.3 Restore Calibration

**Steps:**

1. Normal boot to main screen  
2. Connect USB → “Restore Calibration”  
3. Select your `calibration.dat` (must be 512 bytes)  
4. Click restore; device reboots when done  

**Important:**

- ⚠️ **Only backup files from this site and UVTools2 are supported**  
- ⚠️ **Must be normal boot before restore** — not in flash BOOT mode  
- Auto reboot after write completes  

**Technical details:** Same version detection and addresses as backup; 16 bytes per write; reboot command after completion.

## 2.4 Calibration Addresses and Data Layout

**Storage locations:**

| Medium | Address Range | Purpose | Size |
|---------|---------|------|------|
| EEPROM logical | `0x1E00` (v4+) | Calibration | 512B |
| EEPROM logical | `0xB000` (v5+) | Calibration | 512B |
| SPI Flash | `0x010000` – `0x010200` | Calibration backup area | 512B |

**Why addresses changed:** From v5.0.0, calibration moved from `0x1E00` to `0xB000`; the web tool auto-detects version. Both logical addresses map to the same SPI region.

**Calibration structure (offsets within calibration area):**

- `+0xC0` ~ `+0xD0`: RSSI  
- `+0x140` ~ `+0x14C`: Battery voltage  
- `+0x150` ~ `+0x160`: VOX sensitivity  
- `+0x168` ~ `+0x178`: VOX threshold  
- `+0x188` ~ `+0x198`: Crystal, volume gain, etc.  

**Compatibility:** Formats from different sites may not match; back up before reflashing; bad calibration can show as erratic battery, wrong signal readings, garbled display, etc.

## 2.5 Back Up and Restore Configuration

Config data covers menu settings for quick restore after factory reset. **Does not include channel frequencies/names, calibration, or font pack.**

### Use Cases

1. Export via web “Back Up Config”  
2. “Restore Config” when needed  

### Back Up Config

1. Normal boot → USB → “Back Up Config” → “Export Config Data”  
2. Get `config_backup.dat` (512 bytes)  

**Technical:** SPI physical start `0x00A000`, size 512 bytes (`0x200`); read in blocks.

### Restore Config

1. Normal boot → “Restore Config” → select backup → write → auto reboot  

### Backup Contents

| Address Range | Content |
|---------|------|
| `0x00A000` (8B) | Audio (FM/AM), squelch, TX timeout, key lock/MENU lock, VOX, mic gain |
| `0x00A008` (8B) | Backlight max/min, channel display mode, cross-band, power save, dual watch, backlight time, tail cut, current state/list |
| `0x00A010` (16B) | Current channel (A/B), MR (A/B), frequency channel (A/B), NOAA |
| `0x00A020` (4B) | FM radio: selected frequency, selected channel, MR flag, band |
| `0x00A028` (40B) | FM channel list |
| `0x00A0A8` (8B) | Beep, side-key action, scan resume, auto lock, boot display mode |
| `0x00A0B0` (8B) | Power-on password |
| `0x00A0B8` (8B) | Voice prompts, dBm correction table start |
| `0x00A0C0` (8B) | Alarm mode, Roger, tail time, TX VFO, battery type |
| `0x00A0D0` (8B) | DTMF side tone, delimiter, group code, decode response, auto reset, preload, etc. |
| `0x00A0D8` (8B) | DTMF duration/gap, allow remote Kill |
| `0x00A0F8` (48B) | DTMF ANI/KILL/REVIVE/up/down codes |
| `0x00A130` (8B) | Default scan list, enable, priority channel, CHAN_1_CALL |
| `0x00A138` (16B) | Custom AES key |
| `0x00A150` (8B) | F-LOCK, 350TX/200TX/500TX, 350EN, encryption, battery display, backlight TX/RX |
| `0x00A158` (8B) | Display settings (SET_INV, etc.), S0/S9 |
| `0x00A160` (16B) | Version string |
| `0x00A170` (2B) | UI language, boot prompt |
| `0x00A0B9` (7B) | dBm correction table |
| `0x00A0C8` (32B) | Boot Logo custom text |

**Not included:**

- **Channel data** (frequency, name, attributes) — `0x000000`–`0x008000`  
- **Calibration** — `0x010000` region  
- **Chinese font pack** — `0x024000` region  

## 2.6 SPI Flash Address Map

User data lives mainly on **PY25Q16 SPI Flash (2 MB, `0x000000`–`0x1FFFFF`)**. MCU internal Flash (`0x08002800` onward) holds program code only.

Web programming and firmware often use **SPI physical addresses** directly; legacy tools’ **EEPROM logical addresses** are mapped to SPI via `eeprom_compat`.

### Storage Overview

| Medium | Address Range | Capacity | Purpose |
|------|----------|------|------|
| SPI Flash | `0x000000` – `0x1FFFFF` | 2 MB | Channels, settings, calibration, font, voice, Logo |
| MCU internal Flash | `0x08002800` – `0x0801FFFF` | 118 KB | Firmware (Bootloader at `0x08000000`) |
| SRAM | `0x20000000` – `0x20003FFF` | 16 KB | Runtime RAM |

### Address Summary

| SPI Physical Range | Purpose | Size |
|------------------|------|------|
| `0x000000` – `0x003FFF` | MR channel frequencies | 16 KB |
| `0x004000` – `0x007FFF` | MR channel names | 16 KB |
| `0x008000` – `0x00886D` | Channel attributes + scan list names | ~2.1 KB |
| `0x009000` – `0x0090D5` | VFO frequency data | ~214 B |
| `0x00A000` – `0x00A175` | Settings / config | ~373 B (512 B backup block) |
| `0x010000` – `0x0101FF` | Calibration | 512 B |
| `0x020000` – `0x023FFF` | Legacy Chinese channel names (erasable after migration) | 16 KB |
| `0x024000` – `0x056236` | Chinese font + Pinyin index | ~200 KB |
| `0x14C000` – `0x14CFFF` | Voice index table | ~4 KB |
| `0x14D000` – ~`0x165000` | Voice audio data | ~1.6 MB |
| `0x1FF000` – `0x1FF407` | Boot Logo | 1032 B |

### Channels and VFO

| SPI Physical Address | Size | Content | Formula |
|-------------|------|------|----------|
| `0x000000` – `0x000FFF` | 4 KB | MR frequency Bank 0 | `0x000000 + channel × 16` |
| `0x001000` – `0x001FFF` | 4 KB | MR frequency Bank 1 | |
| `0x002000` – `0x002FFF` | 4 KB | MR frequency Bank 2 | |
| `0x003000` – `0x003FFF` | 4 KB | MR frequency Bank 3 | |
| `0x004000` – `0x004FFF` | 4 KB | MR name Bank 0 | `0x004000 + channel × 16` |
| `0x005000` – `0x005FFF` | 4 KB | MR name Bank 1 | |
| `0x006000` – `0x006FFF` | 4 KB | MR name Bank 2 | |
| `0x007000` – `0x007FFF` | 4 KB | MR name Bank 3 | |
| `0x008000` – `0x00880D` | ~2 KB | 1024 channel attributes (2 B each) | `0x008000 + channel × 2` |
| `0x00880E` – `0x00886D` | 96 B | Scan list names | |
| `0x009000` – `0x0090D5` | ~214 B | 14 VFO × 16 B | |

### Settings Detail

Config backup starts at `0x00A000`, 512 B total. Fields match 2.5; full list:

| SPI Physical Address | Size | Content |
|-------------|------|------|
| `0x00A000` | 8 B | Audio, squelch, timeout, lock, VOX, mic |
| `0x00A008` | 8 B | Backlight, channel display, cross-band, power save, dual watch, tail, etc. |
| `0x00A010` | 16 B | Current / MR / frequency / NOAA channels |
| `0x00A020` | 4–8 B | FM radio state |
| `0x00A028` | 40 B | FM channel list |
| `0x00A0A8` | 8 B+ | Beep, side key, scan resume, auto lock, boot display |
| `0x00A0B0` | 8 B | Power-on password |
| `0x00A0B8` | 8 B | Voice, dBm correction start |
| `0x00A0B9` | 7 B | dBm correction table |
| `0x00A0C0` | 8 B | Alarm, Roger, tail time, TX VFO, battery type |
| `0x00A0C8` | 16 B | Logo custom text line 1 |
| `0x00A0D0` | 8 B | DTMF related |
| `0x00A0D8` | 16 B | DTMF timing + Logo text line 2 |
| `0x00A0F8` | 48 B | DTMF ANI/KILL, etc. |
| `0x00A130` | 8 B | Scan list / priority channel |
| `0x00A138` | 16 B | AES key |
| `0x00A148` | 4 B | Spectrum display mode |
| `0x00A150` | 8 B | F-LOCK, extended TX, encryption, battery display, backlight TX/RX |
| `0x00A158` | 8 B | Display settings, S0/S9, build options |
| `0x00A160` | 16 B | Version string |
| `0x00A170` | 1 B | UI language |
| `0x00A172` | 2 B | **MDC1200 unit ID** |
| `0x00A174` | 1 B | Boot prompt / power-on sound |

### Calibration Data (SPI)

| SPI Physical Address | Size | Content |
|-------------|------|------|
| `0x010000` + level | 1 B × 6 | UHF squelch thresholds |
| `0x010060` + level | 1 B × 6 | VHF squelch thresholds |
| `0x0100C0` | 8 B | UHF RSSI |
| `0x0100C8` | 8 B | VHF RSSI |
| `0x0100D0` + band×16 + op×3 | 3 B | TX power calibration |
| `0x010140` | 12 B | Battery voltage |
| `0x010150` + level×2 | 2 B | VOX1 |
| `0x010168` + level×2 | 2 B | VOX0 |
| `0x010188` | 8 B | Crystal, volume/DAC, etc. |

EEPROM `0x1E00` (v4.x) and `0xB000` (v5.0.0+) both map to `0x010000`–`0x0101FF`.

### Chinese Font and Pinyin Index

| SPI Physical Address | Size | Content |
|-------------|------|------|
| `0x024000` | 162,384 B | Chinese bitmaps (6766 × 24 B) |
| `0x04BA10` | 27,064 B | Unicode → font index |
| `0x052388` | 15,918 B | Pinyin index (402 syllables) |
| `0x056236` | 1 B | Font version (current = 2) |

Pinyin record format: `[syllable length][ASCII pinyin][char count][Unicode×2…]`

Legacy Chinese channel names: `0x020000`–`0x023FFF` (non-overlapping with font).

### Voice Prompts and Boot Logo

| SPI Physical Address | Size | Content |
|-------------|------|------|
| `0x14C000` | 8 B × N | Chinese voice index |
| `0x14C800` | 8 B × N | English voice index |
| `0x14D000` + Offset | Variable | Voice data pool |
| `0x1FF000` | 8 B | Logo header (magic `"DOND"`) |
| `0x1FF008` | 1024 B | Logo bitmap |

### EEPROM Logical Address Map

| EEPROM Logical | SPI Physical | Size | Content |
|----------------|-------------|------|------|
| `0x000000` – `0x003FFF` | Same | 16 KB | MR frequencies |
| `0x004000` – `0x007FFF` | Same | 16 KB | MR names |
| `0x008000` – `0x00886D` | Same | ~2.1 KB | Attributes + list names |
| `0x009000` – `0x0090D5` | Same | ~214 B | VFO |
| `0x00A000` – `0x00A170` | Same | 368 B | Settings |
| `0x001E00` – `0x001FFF` | `0x010000` – `0x0101FF` | 512 B | Calibration (v4.x) |
| `0x00B000` – `0x00B1FF` | `0x010000` – `0x0101FF` | 512 B | Calibration (v5.0.0+) |

**Unmapped / special:**

| Address | Status | Notes |
|------|------|------|
| EEPROM `0x1C00` + index×16 | Unmapped | DTMF contacts (reads as `0xFF`, not persisted) |
| `0x0F30` – `0x0F40` | Protected | USB write blocked (reload related) |
| `0x0E98` – `0x0EA0` | Protected | USB write blocked (password related) |

## 2.7 Boot Screen and Custom Image

1. Menu **Display → Boot Screen**.  
2. Options:  
   - **Off**: No boot animation  
   - **Default**: Gorilla / voltage / sound, etc.; **boot prompt / boot sound options apply only in Default**  
   - **Custom**: Shows image uploaded on the website  

## 2.8 Flashing FAQ

(Full FAQ is in Chapter 3; flashing-related items here.)

1. **English UI right after flash?**  
   Flash the matching font pack and set **Display → Display Language** to Chinese.

2. **Bricked?**  
   Wrong firmware for your radio? Other programming/calibration tools? Address layouts differ by firmware — programming tools are not interchangeable. Follow this site’s steps.

3. **Others’ font OK but missing glyphs?**  
   Font/firmware version mismatch, or browser cached an old font — clear cache and reflash.

4. **Password when flashing back to factory?**  
   Password is often `000000`. Or: factory reset → flash F4HWN v4 → factory reset again → official tool for factory firmware. WeChat group help via the site’s floating button is recommended.

5. **Snow screen / erratic battery / wrong voltage?**  
   Bad calibration: restore factory calibration; if no backup, use factory calibration from the web page.

---

# Chapter 3 — FAQ & Other

## 3.1 Frequently Asked Questions

1. **What does “Use xxx” mean under Channels → Power?**  
   Answer: It reflects a custom power level set in **Other → Set Power**.

2. **English UI right after flashing?**  
   Answer: Flash the matching font pack and set **Display → Display Language** to Chinese.

3. **Dual channel: sometimes channel name below, sometimes frequency?**  
   Answer: By design. Without Chinese in the name, the lower line shows frequency and the upper corner shows the channel name. Names are up to ~5 Chinese characters (~15 English letters); when space is tight, display positions swap.

4. **Bricked?**  
   Answer: Wrong firmware for your radio? Other programming tools or random calibration edits? Did you follow the web flash steps exactly?  
   > Memory layouts differ by firmware; programming software is not universal. Unverified tools — risk is on you.

5. **Others’ font fine but I’m missing characters?**  
   Answer: Font/firmware version mismatch or cached old file in the browser. Clear cache and reflash the matching version.

6. **Wrong FM radio band?**  
   Answer: On the radio screen use **`1` / long `1` / `F+1`** to switch country band. See [1.2](#12-fm-radio).

7. **K1 radio but system info shows K5?**  
   Answer: User customization. Hold **PTT** and the lower adjacent side key while powering on to unlock related options; pick the battery type for your model. UI usually shows K1 or K5 only (K5/K6 share the same chip, different shells).

8. **Changed tone or other settings — gone after switching channel or reboot?**  
   Answer: **In channel (MR) mode, after changing parameters you MUST save the channel again (ChSave). Otherwise changes are temporary only and will not survive a channel change or power cycle.** See [1.4](#14-saving-channels).

9. **Long press # locks keypad and PTT too?**  
   Answer: **Other → Lock Range** can lock keypad only or include PTT.

10. **Password when flashing back to factory?**  
    Answer: Often `000000`. Or: factory reset → F4HWN gen-4 firmware → factory reset → official tool for factory firmware. WeChat group via site floating button is recommended.

11. **Snow screen / jumping battery / wrong voltage?**  
    Answer: Calibration issue — restore factory calibration; use web factory calibration if no backup.

12. **FM radio: amateur signal sometimes exits radio, sometimes not?**  
    Answer: In dual watch, only the **main channel** interrupts and exits the radio. BK4819 cannot true dual-watch; it polls. Continuous polling in radio mode hurts FM quality and uses more power.

13. **Looks very power-hungry — battery drops visibly?**  
    Answer: TX/RX current swings move the voltage meter, like an old e-bike sagging when you twist the throttle. Normal. Some other firmwares freeze the battery display during TX so it looks “steadier.”

14. **What bands work with full unlock? Hardware limits?**  
    Answer: Limited by **BK4819**: Band1 ~18–630 MHz, Band2 ~84–1300 MHz; **630–840 MHz is a hardware dead zone** — firmware cannot bypass it.

    **Full unlock (F_LOCK_NONE) approximate ranges:**

    | Mode | Receive (RX) | Transmit (TX) |
    |------|-----------|-----------|
    | Standard | 50 MHz ~ 600 MHz | 50 MHz ~ 600 MHz |
    | Wide RX mode | 18 MHz ~ 1300 MHz (dead zone excluded) | 18 MHz ~ 600 MHz + 470 MHz ~ 1300 MHz |

    **Menu “Lock Band” common options (current firmware, including compile-time items):**

    | Mode | Notes (TX related) |
    |------|------------------|
    | Default | 137–174 / 400–470 |
    | FCC amateur | 144–148 / 420–450 |
    | CA amateur | 144–148 / 430–450 (if compiled) |
    | CE amateur | 144–146 / 430–440 |
    | GB amateur | 144–148 / 430–440 |
    | 137–174 / 400–430 | Band table |
    | 137–174 / 400–438 | Band table |
    | PMR 446 | If compiled |
    | GMRS/FRS/MURS | If compiled |
    | All disabled | TX blocked |
    | Full unlock | Must select ~**10 times** in menu to activate (anti-mistouch) |

    **Other:**

    - ~280 MHz is VHF/UHF boundary — affects filtering and PA gain  
    - Wide RX can listen SW up to 1.3 GHz; TX still follows band table limits  

## 3.2 Contact

- Douyin: 小闫连不上  
- Bilibili: 小闫同学啊  
- Xiaohongshu: 小闫同学  
- WeChat Channels: 小闫连不上  

## 3.3 Other

Developed by BD1AHN. Other firmware collection: https://www.yuque.com/yanyuliang/radio/wpiyfs070wcfr55s?singleDoc#  

Mangosteen firmware site: https://ethanyan6.github.io/Mangosteen/  
