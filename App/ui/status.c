/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include <string.h>

#include "app/chFrScanner.h"
#include "app/menu.h"
#ifdef ENABLE_FMRADIO
    #include "app/fm.h"
#endif
#include "app/scanner.h"
#include "bitmaps.h"
#include "driver/keyboard.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "functions.h"
#include "helper/battery.h"
#include "misc.h"
#include "settings.h"
#include "ui/battery.h"
#include "ui/helper.h"
#include "ui/menu.h"
#include "ui/ui.h"
#include "ui/status.h"
#include "ui/dualvfo_u8g2_freq.h"
#include "radio.h"

/* 顶栏：电压/百分比小字与电池图标左缘水平间隔（像素） */
#define STATUS_BAT_TEXT_TO_ICON_GAP_PX 3u

#ifdef ENABLE_FEAT_F4HWN_RX_TX_TIMER
#ifndef ENABLE_FEAT_F4HWN_DEBUG
static void convertTime(uint8_t *line, uint8_t type)
{
    uint16_t t = (type == 0) ? (gTxTimerCountdown_500ms / 2) : (3600 - gRxTimerCountdown_500ms / 2);
    uint16_t m = t / 60;
    uint8_t s = (uint8_t)(t % 60);

    gStatusLine[0] = gStatusLine[7] = gStatusLine[14] = 0x00;

    char str[10];
    sprintf(str, "%02u:%02u", (unsigned)m, s);
    UI_PrintStringSmallBufferNormal(str, line);

    gUpdateStatus = true;
}
#endif
#endif

#ifdef ENABLE_FEAT_F4HWN
bool UI_IsDualVfoMainScreen(void)
{
    return gScreenToDisplay == DISPLAY_MAIN && !gAirCopyBootMode &&
           (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF || gEeprom.CROSS_BAND_RX_TX != CROSS_BAND_OFF);
}
#endif

void UI_DisplayMainOnlyStatusBar(void)
{
#ifdef ENABLE_FEAT_F4HWN
    char str[8] = "";
    uint8_t *line = gStatusLine;
    unsigned int x = 0;
    const uint8_t vfo = gEeprom.TX_VFO;
    const VFO_Info_t *pVfo = &gEeprom.VfoInfo[vfo];

    memset(gStatusLine, 0, sizeof(gStatusLine));

    memcpy(line + x, BITMAP_Antenna, sizeof(BITMAP_Antenna));
    x += 6;

    x -= 2;
    uint8_t bars = 0;
    if (FUNCTION_IsRx()) {
        /* RSSI 格与 gVFO_RSSI_bar_level 均按当前接收 VFO（双守时可为 B），不能用 TX_VFO */
        const uint8_t rxVfo = gEeprom.RX_VFO;
        bars = (gVFO_RSSI_bar_level[rxVfo] * 5 + 5) / 6;
        if (bars > 5) bars = 5;
    }
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t h = i + 1;
        uint8_t mask = ((1u << h) - 1u) << (7 - h);
        if (i < bars)
            line[x + (unsigned int)i * 2u] |= mask;
    }
    x += 10;

    if (pVfo->freq_config_RX.Frequency == pVfo->freq_config_TX.Frequency) {
        GUI_DisplaySmallest("|->|", x, 1, true, true);
        x += 17;
    }
    x += 1;

    {
        const char *pwr[] = {"L1","L2","L3","L4","L5","M","H"};
        uint8_t idx = pVfo->OUTPUT_POWER;
        if (idx == OUTPUT_POWER_USER)
            idx = gSetting_set_pwr + 1;
        if (idx >= 1 && idx <= 7) {
            DualVfoU8g2_DrawSmallTextStatus(pwr[idx - 1], (uint8_t)x, 2u, true);
            x += 9;
        }
    }
    x += 1;

    {
        const char *bandwidth_letter = (pVfo->CHANNEL_BANDWIDTH == BANDWIDTH_WIDE) ? "W" : "N";
        DualVfoU8g2_DrawSmallTextStatus(bandwidth_letter, (uint8_t)x, 2u, true);
        x += 6;
    }
    x += 1;

    {
        uint8_t sq = gEeprom.SQUELCH_LEVEL;
        if (sq > 9) sq = 9;
        sprintf(str, "%u", sq);
        DualVfoU8g2_DrawSmallTextStatus(str, (uint8_t)x, 2u, true);
    }
    x += 7;

    {
        const uint16_t step_frequency_hz = pVfo->StepFrequency;
        const uint16_t step_khz_whole = step_frequency_hz / 100u;
        const uint16_t step_khz_frac = step_frequency_hz % 100u;
        const unsigned int step_khz_whole_u = (unsigned int)step_khz_whole;
        const unsigned int step_khz_frac_u = (unsigned int)step_khz_frac;

        if (step_khz_frac == 0u) {
            sprintf(str, "%uK", step_khz_whole_u);
        } else {
            const uint16_t step_frac_tenths = step_khz_frac / 10u;
            const uint16_t step_frac_ones = step_khz_frac % 10u;

            if (step_frac_ones == 0u) {
                sprintf(str, "%u.%uK", step_khz_whole_u, (unsigned int)step_frac_tenths);
            } else {
                sprintf(str, "%u.%02uK", step_khz_whole_u, step_khz_frac_u);
            }
        }
    }
    DualVfoU8g2_DrawSmallTextStatus(str, (uint8_t)x, 2u, true);

    x = LCD_WIDTH - UI_BATTERY_ICON_WIDTH - 2;
    {
        uint8_t battery_bitmap[UI_BATTERY_ICON_WIDTH];
        UI_DrawBattery(battery_bitmap, gBatteryDisplayLevel, gLowBatteryBlink);
        for (uint8_t battery_pixel_x = 0u; battery_pixel_x < UI_BATTERY_ICON_WIDTH; battery_pixel_x++) {
            battery_bitmap[battery_pixel_x] <<= 1;
        }
        memcpy(line + x, battery_bitmap, UI_BATTERY_ICON_WIDTH);
    }

    /*
     * MAIN ONLY / 菜单顶栏：电池旁固定显示整数电量百分比（与 gBatteryIconFillPercent 一致）；
     * 图标填充由 UI_DrawBattery 使用该值，仅在 BATTERY_GetReadings 中百分比变化 1% 时更新。
     */
    {
        sprintf(str, "%02u%%", (unsigned)gBatteryIconFillPercent);
        const uint8_t     text_w = DualVfoU8g2_GetSmallTextWidth(str);
        const unsigned int bat_left_u = (unsigned int)x;
        const unsigned int gap_u      = (unsigned int)STATUS_BAT_TEXT_TO_ICON_GAP_PX;
        if (bat_left_u > gap_u + (unsigned int)text_w)
        {
            const uint8_t text_x = (uint8_t)(bat_left_u - gap_u - (unsigned int)text_w);
            DualVfoU8g2_DrawSmallTextStatus(str, text_x, 2u, true);
        }
    }

    ST7565_BlitStatusLine();
#endif
}

void UI_DisplayStatus()
{
    char str[8] = "";

    gUpdateStatus = false;
    memset(gStatusLine, 0, sizeof(gStatusLine));

#ifdef ENABLE_FEAT_F4HWN
    /* 菜单顶栏与 MAIN ONLY 主界面一致：天线、实时 RSSI 条、功率/带宽/静噪/步进、电量 */
    if (gScreenToDisplay == DISPLAY_MENU)
    {
        UI_DisplayMainOnlyStatusBar();
        return;
    }

    // 主页面 (MAIN ONLY): 定制顶部菜单栏
    if (gScreenToDisplay == DISPLAY_MAIN && !gAirCopyBootMode &&
        gEeprom.DUAL_WATCH == DUAL_WATCH_OFF && gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF) {
        UI_DisplayMainOnlyStatusBar();
        return;
    }

    /* 双 VFO 主界面：不单独刷空白状态行；顶行由 ST7565_BlitFullScreenDualVfoTightTop 与 gFrameBuffer[0] 合并输出 */
    if (UI_IsDualVfoMainScreen())
        return;
#endif

    uint8_t     *line = gStatusLine;
    unsigned int x    = 0;

#ifdef ENABLE_NOAA
    // NOAA indicator
    if (!(gScanStateDir != SCAN_OFF || SCANNER_IsScanning()) && gIsNoaaMode) { // NOASS SCAN indicator
        memcpy(line + x, BITMAP_NOAA, sizeof(BITMAP_NOAA));
    }
    // Power Save indicator
    else if (gCurrentFunction == FUNCTION_POWER_SAVE) {
        memcpy(line + x, gFontPowerSave, sizeof(gFontPowerSave));
    }
    x += 8;
#else
    // Power Save indicator
    if (gCurrentFunction == FUNCTION_POWER_SAVE) {
        memcpy(line + x, gFontPowerSave, sizeof(gFontPowerSave));
    }
    x += 8;
#endif

    unsigned int x1 = x;

#ifdef ENABLE_DTMF_CALLING
    if (gSetting_KILLED) {
        memset(line + x, 0xFF, 10);
        x1 = x + 10;
    }
    else
#endif
    { // SCAN indicator
        if (gScanStateDir != SCAN_OFF || SCANNER_IsScanning()) {
            if (IS_MR_CHANNEL(gNextMrChannel) && !SCANNER_IsScanning()) { // channel mode

                uint8_t end = 0;

                if(gEeprom.SCAN_LIST_DEFAULT == MR_CHANNELS_LIST + 1)
                {
                    sprintf(str, gEeprom.SCAN_LIST_ENABLED ? "%s+" : "%s", "ALL");
                    end = gEeprom.SCAN_LIST_ENABLED ? 18 : 14;
                }
                else
                {
                    const char *name = gListName[gEeprom.SCAN_LIST_DEFAULT - 1];

                    // Check if name is valid
                    if (!IsEmptyName(name, sizeof(gListName[0]))) {
                        sprintf(str, "%.3s%s", name, gEeprom.SCAN_LIST_ENABLED ? "+" : "");
                        end = gEeprom.SCAN_LIST_ENABLED ? 18 : 14;
                    } 
                    else {
                        sprintf(str, "%02d%s", gEeprom.SCAN_LIST_DEFAULT, gEeprom.SCAN_LIST_ENABLED ? "+" : "");
                        end = gEeprom.SCAN_LIST_ENABLED ? 14 : 10;
                    }
                }

                GUI_DisplaySmallest(str, 2, 1, true, true);

                gStatusLine[0] ^= 0x3E;
                for (uint8_t x = 1; x < end; x++)
                {
                    gStatusLine[x] ^= 0x7F;
                }
                gStatusLine[end] ^= 0x3E;
            }
            else {  // frequency mode
                memcpy(line + x + 1, gFontS, sizeof(gFontS));
                //UI_PrintStringSmallBufferNormal("S", line + x + 1);
            }
            x1 = x + 10;
        }
    }
    x += 10;  // font character width

    #ifdef ENABLE_FEAT_F4HWN_DEBUG
        // Only for debug
        // Only for debug
        // Only for debug

        sprintf(str, "%d", gDebug);
        UI_PrintStringSmallBufferNormal(str, line + x + 1);
        x += 16;
    #else
        #ifdef ENABLE_VOICE
        // VOICE indicator
        if (gEeprom.VOICE_PROMPT != VOICE_PROMPT_OFF){
            memcpy(line + x, BITMAP_VoicePrompt, sizeof(BITMAP_VoicePrompt));
            x1 = x + sizeof(BITMAP_VoicePrompt);
        }
        x += sizeof(BITMAP_VoicePrompt);
        #endif

        if(!SCANNER_IsScanning()) {
        #ifdef ENABLE_FEAT_F4HWN_RX_TX_TIMER
            if(gCurrentFunction == FUNCTION_TRANSMIT && gSetting_set_tmr == true)
            {
                convertTime(line, 0);
            }
            else if(FUNCTION_IsRx() && gSetting_set_tmr == true)
            {
                convertTime(line, 1);
            }
            else
        #endif
            {
                if(!gAirCopyBootMode) {
                    #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
                    if(gEeprom.MENU_LOCK == true) {
                        memcpy(line + x + 2, gFontRO, sizeof(gFontRO));
                    }
                    else
                    {
                    #endif
                        uint8_t dw = (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF) + (gEeprom.CROSS_BAND_RX_TX != CROSS_BAND_OFF) * 2;
                        if(dw == 1 || dw == 3) { // DWR - dual watch + respond
                            if(gDualWatchActive)
                                memcpy(line + x + (dw==1?0:2), gFontDWR, sizeof(gFontDWR) - (dw==1?0:5));
                            else
                                memcpy(line + x + 3, gFontHold, sizeof(gFontHold));
                        }
                        else if(dw == 2) { // XB - crossband
                            memcpy(line + x + 2, gFontXB, sizeof(gFontXB));
                        }
                        else
                        {
                            memcpy(line + x + 2, gFontMO, sizeof(gFontMO));
                        }
                    #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
                    }
                    #endif
                }
            }
        }
        x += sizeof(gFontDWR) + 3;
    #endif

#ifdef ENABLE_VOX
    // VOX indicator
    if (gEeprom.VOX_SWITCH) {
        memcpy(line + x, gFontVox, sizeof(gFontVox));
        x1 = x + sizeof(gFontVox) + 1;
    }
    x += sizeof(gFontVox) + 3;
#endif

#ifdef ENABLE_FEAT_F4HWN
    // PTT indicator
    if(!gAirCopyBootMode) {
        if (gSetting_set_ptt_session) {
            memcpy(line + x, gFontPttOnePush, sizeof(gFontPttOnePush));
            x1 = x + sizeof(gFontPttOnePush) + 1;
        }
        else
        {
            memcpy(line + x, gFontPttClassic, sizeof(gFontPttClassic));
            x1 = x + sizeof(gFontPttClassic) + 1;       
        }
    }
    x += sizeof(gFontPttClassic) + 3;
#endif

    x = MAX(x1, 69u);

    const void *src = NULL;   // Pointer to the font/bitmap to copy
    size_t size = 0;          // Size of the font/bitmap

    // Determine the source and size based on conditions
    if (gEeprom.KEY_LOCK) {
        src = gFontKeyLock;
        size = sizeof(gFontKeyLock);
    }
    else if (gWasFKeyPressed) {
        src = gFontF;
        size = sizeof(gFontF);
    }
    #ifdef ENABLE_FEAT_F4HWN
        else if (gMute) {
            src = gFontMute;
            size = sizeof(gFontMute);
        }
    #endif
    else if (gBackLight) {
        src = gFontLight;
        size = sizeof(gFontLight);
    }
    #ifdef ENABLE_FEAT_F4HWN_CHARGING_C
    else if (gChargingWithTypeC) {
        src = BITMAP_USB_C;
        size = sizeof(BITMAP_USB_C);
    }
    #endif

    // Perform the memcpy if a source was selected
    if (src) {
        memcpy(line + x + 1, src, size);
    }

    // Battery
    unsigned int x2 = LCD_WIDTH - UI_BATTERY_ICON_WIDTH - 0;

    {
        uint8_t battery_bitmap[UI_BATTERY_ICON_WIDTH];
        UI_DrawBattery(battery_bitmap, gBatteryDisplayLevel, gLowBatteryBlink);
        for (uint8_t battery_pixel_x = 0u; battery_pixel_x < UI_BATTERY_ICON_WIDTH; battery_pixel_x++) {
            battery_bitmap[battery_pixel_x] <<= 1;
        }
        memcpy(line + x2, battery_bitmap, UI_BATTERY_ICON_WIDTH);
    }

    bool BatTxt = false;

#ifdef ENABLE_FMRADIO
    if (gScreenToDisplay == DISPLAY_FM)
    {
        sprintf(str, "%02u%%", (unsigned)gBatteryIconFillPercent);
        BatTxt = true;
    }
    else
#endif
    {
        switch (gSetting_battery_text)
        {
        default:
        case 0:
            break;

        case 1: {
            const uint16_t voltage =
                (gBatteryVoltageAverage <= 999) ? gBatteryVoltageAverage : 999;
            sprintf(str, "%u.%02u", voltage / 100, voltage % 100);
            BatTxt = true;
            break;
        }

        case 2:
            sprintf(str, "%02u%%", (unsigned)gBatteryIconFillPercent);
            BatTxt = true;
            break;
        }
    }

    if (BatTxt) {
        const uint8_t     text_w = DualVfoU8g2_GetSmallTextWidth(str);
        const unsigned int bat_left_u = (unsigned int)x2;
        const unsigned int gap_u      = (unsigned int)STATUS_BAT_TEXT_TO_ICON_GAP_PX;
        if (bat_left_u > gap_u + (unsigned int)text_w)
        {
            const uint8_t text_x = (uint8_t)(bat_left_u - gap_u - (unsigned int)text_w);
            DualVfoU8g2_DrawSmallTextStatus(str, text_x, 2u, true);
        }
    }

    // **************

    ST7565_BlitStatusLine();
}
