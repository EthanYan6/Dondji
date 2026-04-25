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

#ifdef ENABLE_FMRADIO

#include <string.h>

#include "app/fm.h"
#include "driver/bk1080.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "font.h"
#include "misc.h"
#include "settings.h"
#include "ui/fmradio.h"
#include "ui/helper.h"
#include "ui/inputbox.h"
#include "ui/ui.h"

/* 与 UI_PrintString(..., Line=0, Width=8) 相同布局，整字下移若干像素 */
#define FM_UI_BIG_FM_LABEL_DOWN_SHIFT_PX 3u

static void FM_UI_DrawBigFmMark(void)
{
    const char      label[]     = "FM";
    const uint8_t   start_x     = 2u;
    const uint8_t   char_pitch  = 8u;
    const uint8_t   down_shift  = FM_UI_BIG_FM_LABEL_DOWN_SHIFT_PX;
    const size_t    label_len   = sizeof(label) - 1u;
    size_t          char_index;

    for (char_index = 0u; char_index < label_len; char_index++) {
        const char ch = label[char_index];
        if (ch <= ' ' || ch >= 127)
            continue;

        const unsigned int glyph_index = (unsigned int)(ch - ' ' - 1u);
        const unsigned int ofs         = (unsigned int)start_x + (unsigned int)char_index * (unsigned int)char_pitch;
        uint8_t            col;

        for (col = 0u; col < 7u; col++) {
            const uint8_t x = (uint8_t)(ofs + (unsigned int)col);
            uint8_t        row_in_glyph;

            for (row_in_glyph = 0u; row_in_glyph < 8u; row_in_glyph++) {
                const uint8_t mask = (uint8_t)(1u << row_in_glyph);
                if ((gFontBig[glyph_index][col] & mask) == 0u)
                    continue;
                {
                    const uint8_t y_pixel = (uint8_t)(row_in_glyph + down_shift);
                    UI_DrawPixelBuffer(gFrameBuffer, x, y_pixel, true);
                }
            }
            for (row_in_glyph = 0u; row_in_glyph < 8u; row_in_glyph++) {
                const uint8_t mask = (uint8_t)(1u << row_in_glyph);
                if ((gFontBig[glyph_index][(uint8_t)(col + 7u)] & mask) == 0u)
                    continue;
                {
                    const uint8_t y_pixel = (uint8_t)(row_in_glyph + 8u + down_shift);
                    UI_DrawPixelBuffer(gFrameBuffer, x, y_pixel, true);
                }
            }
        }
    }
}

#ifdef ENABLE_CHINESE
/* 仅中文 UI：与记忆频率差 1 步(0.1MHz) 时也显示 频率(CHnn)，不改变英文与按键逻辑 */
static uint8_t FM_UI_CnMemorySlotForPlaying(void)
{
    const uint16_t play = gEeprom.FM_FrequencyPlaying;

    for (unsigned i = 0; i < FM_CHANNELS_MAX; i++) {
        if (!FM_CheckValidChannel((uint8_t)i))
            continue;
        const uint16_t mem = gFM_Channels[i];
        if (mem == play)
            return (uint8_t)(i + 1u);
        if (mem > play) {
            if (mem - play <= 1u)
                return (uint8_t)(i + 1u);
        } else if (play - mem <= 1u)
            return (uint8_t)(i + 1u);
    }
    return 0;
}
#endif

void UI_DisplayFM(void)
{
    char String[40] = {0};
    char *pPrintStr = String;
    UI_DisplayClear();

#ifdef ENABLE_FEAT_F4HWN
    /* 与菜单一致：定制顶栏（status.c）下方内容区顶边横线 */
    UI_DrawLineBuffer(gFrameBuffer, 0, 0, (int16_t)(LCD_WIDTH - 1u), 0, true);
#endif

    FM_UI_DrawBigFmMark();

    sprintf(String, "%d%s-%dM", 
        BK1080_GetFreqLoLimit(gEeprom.FM_Band)/10,
        gEeprom.FM_Band == 0 ? ".5" : "",
        BK1080_GetFreqHiLimit(gEeprom.FM_Band)/10
        );
    
    UI_PrintStringSmallNormal(String, 1, 0, 6);

    //uint8_t spacings[] = {20,10,5};
    //sprintf(String, "%d0k", spacings[gEeprom.FM_Space % 3]);
    //UI_PrintStringSmallNormal(String, 127 - 4*7, 0, 6);

    if (gAskToSave) {
#ifdef ENABLE_CHINESE
        if (gUiLanguage == UI_LANGUAGE_CN)
            pPrintStr = "\xe4\xbf\x9d\xe5\xad\x98?";
        else
#endif
            pPrintStr = "SAVE?";
    } else if (gAskToDelete) {
#ifdef ENABLE_CHINESE
        if (gUiLanguage == UI_LANGUAGE_CN)
            pPrintStr = "\xe5\x88\xa0\xe9\x99\xa4?";
        else
#endif
            pPrintStr = "DEL?";
    } else if (gFM_ScanState == FM_SCAN_OFF) {
        if (gEeprom.FM_IsMrMode) {
#ifdef ENABLE_CHINESE
            if (gUiLanguage == UI_LANGUAGE_CN)
                sprintf(String, "\xe4\xbf\xa1\xe9\x81\x93(CH%02u)", (unsigned)gEeprom.FM_SelectedChannel + 1u);
            else
#endif
                sprintf(String, "MR(CH%02u)", gEeprom.FM_SelectedChannel + 1);
            pPrintStr = String;
        } else {
#ifdef ENABLE_CHINESE
            if (gUiLanguage == UI_LANGUAGE_CN) {
                const uint8_t slot = FM_UI_CnMemorySlotForPlaying();
                if (slot != 0u) {
                    sprintf(String, "\xe9\xa2\x91\xe7\x8e\x87(CH%02u)", (unsigned)slot);
                    pPrintStr = String;
                } else
                    pPrintStr = "\xe9\xa2\x91\xe7\x8e\x87";
            } else
#endif
            {
                pPrintStr = "VFO";
                for (unsigned int i = 0; i < FM_CHANNELS_MAX; i++) {
                    if (gEeprom.FM_FrequencyPlaying == gFM_Channels[i]) {
                        sprintf(String, "VFO(CH%02u)", i + 1);
                        pPrintStr = String;
                        break;
                    }
                }
            }
        }
    } else if (gFM_AutoScan) {
#ifdef ENABLE_CHINESE
        if (gUiLanguage == UI_LANGUAGE_CN)
            sprintf(String, "\xe9\xa2\x91\xe7\x8e\x87\xe6\x89\xab\xe6\x8f\x8f(%u)", (unsigned)gFM_ChannelPosition);
        else
#endif
            sprintf(String, "A-SCAN(%u)", gFM_ChannelPosition);
        pPrintStr = String;
    } else {
#ifdef ENABLE_CHINESE
        if (gUiLanguage == UI_LANGUAGE_CN)
            pPrintStr = "\xe4\xbf\xa1\xe9\x81\x93\xe6\x89\xab\xe6\x8f\x8f";
        else
#endif
            pPrintStr = "M-SCAN";
    }

#ifdef ENABLE_CHINESE
    if (gUiLanguage == UI_LANGUAGE_CN)
        UI_PrintStringSmallAtPixel(pPrintStr, 0u, LCD_WIDTH - 1u, 36u, 52u, 3u);
    else
#endif
        UI_PrintString(pPrintStr, 0, 127, 3, 10); /* memory, vfo, scan */

    memset(String, 0, sizeof(String));
    if (gAskToSave || (gEeprom.FM_IsMrMode && gInputBoxIndex > 0)) {
        UI_GenerateChannelString(String, gFM_ChannelPosition);
    } else if (gAskToDelete) {
        sprintf(String, "CH-%02u", gEeprom.FM_SelectedChannel + 1);
    } else {
        if (gInputBoxIndex == 0) {
            sprintf(String, "%3d.%d", gEeprom.FM_FrequencyPlaying / 10, gEeprom.FM_FrequencyPlaying % 10);
        } else {
            const char * ascii = INPUTBOX_GetAscii();
            sprintf(String, "%.3s.%.1s",ascii, ascii + 3);
        }

        UI_DisplayFrequency(String, 36, 1, gInputBoxIndex == 0);  // frequency
        
#ifdef ENABLE_FEAT_F4HWN
        // 键盘锁定时显示解锁提示框，使用与 main only 相同的偏移
        if (gEeprom.KEY_LOCK && gKeypadLocked > 0) {
            UI_DisplayUnlockKeyboard(-10);
        }
#endif
        
        ST7565_BlitFullScreen();
        return;
    }

    UI_PrintString(String, 0, 127, 1, 10);

#ifdef ENABLE_FEAT_F4HWN
    // 键盘锁定时显示解锁提示框，使用与 main only 相同的偏移
    if (gEeprom.KEY_LOCK && gKeypadLocked > 0) {
        UI_DisplayUnlockKeyboard(-10);
    }
#endif

    ST7565_BlitFullScreen();
}

#endif
