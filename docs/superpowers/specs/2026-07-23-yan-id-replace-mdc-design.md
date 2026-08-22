# Replace MDC with Yan ID (mangosteen-compatible)

**Date:** 2026-07-23  
**Status:** Superseded by `2026-08-22-yan-id-alongside-mdc-design.md` (keep MDC; add Yan ID + 接收 Yan ID)  
**Approach:** Minimal GGM2 FSK Yan ID module; functionally replace MDC TX/RX/UI; keep `mdc1200` sources unbuilt

## Goal

Remove MDC1200 transmit/receive wiring from Dondji and replace it with **Yan ID** that interoperates with mangosteen (GGM2 FSK packet type 5). Peers show a centered popup with the received callsign.

## Decisions

| Topic | Choice |
|-------|--------|
| Interop with mangosteen | Yes — same GGM2 type-5 air format |
| MDC source files | Keep on disk; remove from build / stop calling (option B) |
| RX enable | When Roger = `YAN ID`, auto-enable FSK RX (like current MDC) |
| RX UI | Centered popup (clone of MDC popup), not DTMF line |
| Messenger SMS stack | Not ported |

## Menu & settings

Settings order stays: … → 发射尾音 (Roger) → **Yan ID** (was MDC ID) → STE → …

| Item | Behavior |
|------|----------|
| **Yan ID** | Text edit, A–Z / 0–9, max 6 chars. Lowercase uppercased; spaces stripped. |
| **Roger / 发射尾音** | `OFF` / `ROGER` / `YAN ID` (replaces `MDC`) |

Transmit Yan ID only when:

1. `gEeprom.ROGER == ROGER_MODE_YAN_ID`, and  
2. `yan_id` is non-empty after trim.

If Roger is `YAN ID` but Yan ID is empty → behave like `OFF`.

### Storage

- `ROGER_MODE_YAN_ID = 2` — **reuses former `ROGER_MODE_MDC` enum value** so existing EEPROM value `2` maps to Yan mode instead of an invalid index.
- `gEeprom.yan_id[YAN_ID_LEN + 1]` with `YAN_ID_LEN = 6`, persisted at `0x00A0A8 + 0x20` (same gap as mangosteen).
- Stop reading/writing `MDC1200_ID_EEPROM_ADDR` (`0x00A172`).

Roger load check: accept `0..2` (was `0..2` with MDC; still three modes).

## Air protocol

Compatible with mangosteen Messenger GGM2:

- Wire length: 94 bytes  
- Magic: `'G','G','M','2'`, version `2`  
- Type: `5` (`YAN_ID`)  
- `from` = Yan ID (callsign field), `to` = `"ALL"`, empty payload  
- CRC16 identical to mangosteen `MSG_PACKET_Crc16`  
- AirCopy framing: `0xABCD` + payload words + outer CRC + `0xDCBA`, long preamble TX  

Non-type-5 / bad CRC: ignore. No Inbox / HEARD / ACK / PING / PONG.

## New modules

| File | Responsibility |
|------|----------------|
| `App/app/yan_id_packet.c/.h` | Build/parse type-5 frame + CRC |
| `App/app/yan_id_rf.c/.h` | FSK TX (`YAN_RF_Send`), RX assemble/parse, enable/disable sidecar, self-RX ignore |

Reuse existing BK4819 AirCopy helpers (`BK4819_SetupAircopy`, FSK FIFO, reset). Extract only the minimal long-preamble send + RX buffer parse logic from mangosteen `messenger_rf` — do **not** port messenger UI/store/SMS.

## TX path

```
PTT release → RADIO_SendEndOfTransmission
           → BK4819_PlayRoger()
           → if ROGER_MODE_YAN_ID && yan_id[0] → YAN_RF_Send()
           → DTMF end-of-TX / CSS STE (unchanged)
```

On send success: set ignore-next-self-RX so the transmitter does not show its own popup. FSK busy / failure: skip silently.

## RX path

When `ROGER == ROGER_MODE_YAN_ID`:

1. `RADIO_SetupRegisters` enables Yan FSK RX (replaces `BK4819_EnableMDC1200Rx`).
2. `CheckRadioInterrupts` in `app.c` fills FSK buffer; on complete frame, parse type 5.
3. Copy `from` into `gYanId_RX` (max 6), set `gYanId_RX_timeout` (~5 s, same order as MDC tick).
4. Timeout tick clears buffer and refreshes UI.

When Roger is not `YAN ID`, disable Yan FSK RX (same pattern as today’s MDC gate).

## UI popup

Replace MDC popup wiring with Yan:

- Title: `Yan ID`  
- Body: received callsign string  
- Geometry: reuse MDC centered 60×30 box  
- Show on `DISPLAY_MAIN` while timeout > 0 and Roger = `YAN ID`

Menu strings: `MENU_MDC_ID` → `MENU_YAN_ID` / `"Yan ID"`; Roger submenu `"MDC"` → `"YAN ID"`; CN 发射尾音 values updated accordingly.

## MDC deactivation (keep sources)

- Remove `app/mdc1200.c` from `App/CMakeLists.txt`.
- Stop all live calls to MDC encode/RX/popup/init/save.
- Leave `mdc1200.c/.h` and unused MDC driver helpers on disk for possible restore.
- Update user-facing docs that mention MDC ID / Roger=MDC to Yan ID.

## Error handling

| Case | Behavior |
|------|----------|
| Empty Yan ID + Roger YAN ID | No FSK (like OFF) |
| FSK busy / TX fail | Silent skip |
| Bad CRC / wrong type | Ignore |
| Non-Yan peer | Brief digital burst only; no UI |

## Non-goals

- Full Messenger / SMS / MsgRx menu  
- Keeping MDC as a Roger option  
- DTMF encoding of alphanumeric IDs  
- Changing STE / 尾音消除 behavior  

## Acceptance

1. Build succeeds; `mdc1200.c` not linked.  
2. Menus show Yan ID + Roger `YAN ID`; no MDC.  
3. Dondji TX → mangosteen shows callsign (MsgRx + Yan Roger as required on peer).  
4. mangosteen TX → Dondji shows Yan ID popup.  
5. Empty Yan ID does not transmit.  
6. Self-echo after TX does not popup locally.
