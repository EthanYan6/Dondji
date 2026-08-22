# Add Yan ID alongside MDC (mangosteen-compatible)

**Date:** 2026-08-22  
**Status:** Approved  
**Approach:** Minimal GGM2 FSK Yan ID module ported from mangosteen; keep MDC fully intact  
**Supersedes:** `2026-07-23-yan-id-replace-mdc-design.md` (replace-MDC approach abandoned)

## Goal

Add **Yan ID** to Dondji so it interoperates with mangosteen (GGM2 FSK packet type 5), while **keeping** existing MDC1200 TX/RX/UI. Prior receive/parse attempts that failed to decode are discarded; reimplement from mangosteen’s working packet + AirCopy FSK path.

Naming is unified as **Yan ID** in menus, Roger submenu, popup title, and docs (not `YAN ID` / `YanID`).

## Decisions

| Topic | Choice |
|-------|--------|
| Interop with mangosteen | Yes — same GGM2 type-5 air format |
| MDC | Keep fully (sources, menus, Roger=`MDC`, RX/TX) |
| Yan TX | Roger=`Yan ID` and non-empty `yan_id` |
| Yan RX enable | Separate menu **接收 Yan ID** (default OFF) |
| Menu visibility | Show **接收 Yan ID** only when Roger=`Yan ID`; hide when Roger=`MDC` (and other modes); when hidden, treat receive as OFF and use MDC-only logic if Roger=`MDC` |
| Self-echo | Ignore next self-RX after local Yan TX; never show own ID locally |
| Messenger SMS stack | Not ported |
| Implementation source | Surgical port from mangosteen `messenger_packet` / `messenger_rf` Yan ID paths — not a from-scratch AirCopy guess |

## Menu & settings

Order near existing Roger block:

`Roger` → `MDC ID` → `Yan ID` → `接收 Yan ID` (conditional) → `STE` → …

| Item | Behavior |
|------|----------|
| **Roger** | `OFF` / `ROGER` / `MDC` / `Yan ID` |
| **MDC ID** | Existing 4-digit hex; unchanged |
| **Yan ID** | Text edit, A–Z / 0–9, max 6. Lowercase uppercased; invalid chars stop fill. Always browsable. |
| **接收 Yan ID** | Menu title EN: `Yan ID Rx` (or equivalent short label); CN: `接收 Yan ID`. ON/OFF, **default OFF**. Visible in menu browse **only when** `gEeprom.ROGER == ROGER_MODE_YAN_ID`. When Roger≠`Yan ID`, item hidden and Yan receive forced OFF at runtime (EEPROM value kept so switching back restores preference). When Roger=`MDC`, MDC path only. |

### TX / RX matrix

| Roger | Transmit | 「接收 Yan ID」 menu | FSK RX |
|-------|----------|---------------------|--------|
| OFF / ROGER | None / beep | Hidden | Neither MDC nor Yan FSK |
| MDC | MDC1200 (existing) | Hidden; receive treated OFF | MDC only |
| Yan ID | GGM2 type-5 if `yan_id` non-empty; else no FSK | Shown; user ON/OFF (default OFF) | Yan FSK only if 接收 Yan ID=ON |

Self-TX echo: after successful Yan send, set ignore-self so local popup does not show own callsign.

### Storage

- `ROGER_MODE_YAN_ID = 3` — fourth Roger mode (mangosteen-compatible). Load accepts `0..3`.
- `gEeprom.yan_id[YAN_ID_LEN + 1]` with `YAN_ID_LEN = 6`, persisted at `0x00A0A8 + 0x20` (same 8-byte gap as mangosteen; first 6 chars).
- `gEeprom.yan_id_rx` (bool) at byte index 7 of that same 8-byte gap (`0x00A0A8 + 0x20 + 7`). Default `0`. Does not collide with mangosteen (it zeros unused bytes).
- MDC ID storage at `0x00A172` unchanged.

## Air protocol

Compatible with mangosteen Messenger GGM2:

- Wire length: 94 bytes  
- Magic: `'G','G','M','2'`, version `2`  
- Type: `5` (`YAN_ID`)  
- `from` = Yan ID (callsign field), `to` = `"ALL"`, empty payload  
- CRC16 identical to mangosteen `MSG_PACKET_Crc16` (init `0xFFFF`, poly `0x1021`)  
- AirCopy framing: `0xABCD` + payload words + outer CRC + `0xDCBA`, long preamble TX  

Non-type-5 / bad CRC: ignore. No Inbox / HEARD / ACK / PING / PONG.

## New modules

| File | Responsibility |
|------|----------------|
| `App/app/yan_id_packet.c/.h` | Build/parse type-5 frame + CRC (from mangosteen packet helpers) |
| `App/app/yan_id_rf.c/.h` | FSK TX (`YAN_RF_Send`), RX assemble/parse, enable/disable, self-RX ignore |

Reuse BK4819 AirCopy helpers (`BK4819_SetupAircopy`, FSK FIFO, reset). Port only the minimal long-preamble send + RX buffer parse needed for Yan ID — do **not** port messenger UI/store/SMS.

## TX path

```
PTT release → RADIO_SendEndOfTransmission
           → BK4819_PlayRoger() / BK4829 path equivalent
           → ROGER_MODE_ROGER     → existing beep
           → ROGER_MODE_MDC       → existing MDC1200
           → ROGER_MODE_YAN_ID && yan_id[0] → YAN_RF_Send()
           → DTMF end-of-TX / CSS STE (unchanged)
```

On Yan send success: set ignore-next-self-RX. FSK busy / failure: skip silently.

## RX path

In `RADIO_SetupRegisters` / interrupt path (parallel to MDC gating):

1. If `ROGER == ROGER_MODE_MDC` → enable MDC RX only (existing); Yan FSK off.  
2. Else if `ROGER == ROGER_MODE_YAN_ID` && `yan_id_rx` → enable Yan FSK RX; on complete frame parse type 5; if not self-ignore, copy `from` into `gYanId_RX` (max 6) and set timeout (~same order as MDC popup).  
3. Else → Yan FSK off; MDC off unless step 1.

Timeout tick clears buffer and refreshes UI.

## UI popup

- Title: `Yan ID`  
- Body: received callsign  
- Geometry: reuse MDC-style centered box (or equivalent)  
- Show on `DISPLAY_MAIN` while timeout > 0 and Yan receive path is active  

CN Roger submenu fourth value and related strings: **Yan ID**; receive item: **接收 Yan ID**.

## MDC

- Keep `app/mdc1200.c` in the build.  
- Keep `MENU_MDC_ID`, Roger `MDC`, popup, enable/disable RX.  
- No change to MDC wire format or EEPROM ID address.

## Error handling

| Case | Behavior |
|------|----------|
| Empty Yan ID + Roger Yan ID | No Yan FSK TX |
| 接收 Yan ID OFF | No Yan parse / popup |
| Roger MDC | Hide 接收 Yan ID; Yan RX forced off; MDC only |
| FSK busy / TX fail | Silent skip |
| Bad CRC / wrong type | Ignore |
| Self-echo after Yan TX | Ignore (no local popup) |
| Non-Yan peer | Brief digital burst only; no UI |

## Non-goals

- Full Messenger / SMS / MsgRx menu  
- Removing or replacing MDC as a Roger option  
- DTMF encoding of alphanumeric IDs  
- Changing STE / 尾音消除 behavior  

## Acceptance

1. Build succeeds; MDC TX/RX still work.  
2. Menus: Roger includes `Yan ID`; `Yan ID` editable; `接收 Yan ID` visible only when Roger=`Yan ID`, default OFF.  
3. Roger=`MDC`: no `接收 Yan ID` in browse; MDC-only FSK.  
4. Dondji TX (Roger=`Yan ID`, non-empty ID) → mangosteen shows callsign when peer MsgRx/Yan path allows.  
5. mangosteen TX → Dondji shows Yan ID popup when 接收 Yan ID=ON.  
6. Local self-echo does not popup.  
7. 接收 Yan ID=OFF → no Yan popup even if Roger=`Yan ID`.
