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

#include <stdbool.h>
#include <string.h>
#include "app/scanner.h"
#include "dcs.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "misc.h"
#include "settings.h"
#include "ui/helper.h"
#include "ui/scanner.h"

void UI_DisplayScanner(void)
{
    char  String[48] = {0};
    char *pPrintStr = String;
    uint8_t Start;

    UI_DisplayClear();

#ifdef ENABLE_FEAT_F4HWN
    /* 与菜单 UI_DisplayMenu 相同：顶栏（gStatusLine）下方第一行像素画横线 */
    UI_DrawLineBuffer(gFrameBuffer, 0, 0, (int16_t)(LCD_WIDTH - 1u), 0, true);
#endif

    /* 未锁定时显示所选扫描段，锁定后按实测频率显示（CN/EN 共用） */
    const bool freq_known = gScanSingleFrequency ||
        (gScanCssState != SCAN_CSS_STATE_OFF && gScanCssState != SCAN_CSS_STATE_FAILED);
    const bool uhf = freq_known ? (gScanFrequency >= 28000000u) : gScanBandIsUhf;

#ifdef ENABLE_CHINESE
    if (gUiLanguage == UI_LANGUAGE_CN)
    {
        /* 频率 / 亚音 / 扫描中 — 小字 UTF-8；数字与 Hz 走嵌入拉丁小字 */
        if (gScanSingleFrequency || (gScanCssState != SCAN_CSS_STATE_OFF && gScanCssState != SCAN_CSS_STATE_FAILED)) {
            sprintf(String, "\xe9\xa2\x91\xe7\x8e\x87:%u.%05u",
                    (unsigned)(gScanFrequency / 100000u), (unsigned)(gScanFrequency % 100000u));
            pPrintStr = String;
        } else {
            pPrintStr = "\xe9\xa2\x91\xe7\x8e\x87:**.*****";
        }
        UI_PrintStringSmallAtPixel(pPrintStr, 2u, 2u, 9u, 21u, 0u);

        if (gScanCssState < SCAN_CSS_STATE_FOUND || !gScanUseCssResult) {
            pPrintStr = "\xe4\xba\x9a\xe9\x9f\xb3:******";
        } else if (gScanCssResultType == CODE_TYPE_CONTINUOUS_TONE) {
            sprintf(String, "\xe4\xba\x9a\xe9\x9f\xb3:%u.%uHz",
                    (unsigned)(CTCSS_Options[gScanCssResultCode] / 10u),
                    (unsigned)(CTCSS_Options[gScanCssResultCode] % 10u));
            pPrintStr = String;
        } else {
            sprintf(String, "DCS:D%03oN", DCS_Options[gScanCssResultCode]);
            pPrintStr = String;
        }
        UI_PrintStringSmallAtPixel(pPrintStr, 2u, 2u, 23u, 35u, 0u);

        /* 波段独立一行 */
        UI_PrintStringSmallAtPixel(uhf ? "\xe6\xb3\xa2\xe6\xae\xb5:UHF" : "\xe6\xb3\xa2\xe6\xae\xb5:VHF",
                                   2u, 2u, 37u, 49u, 0u);

        if (gScannerSaveState == SCAN_SAVE_CHANNEL) {
            /* 与 FM 中文「保存?」相同：汉字 + ASCII ? */
            pPrintStr = "\xe4\xbf\x9d\xe5\xad\x98?";
            UI_PrintStringSmallAtPixel(pPrintStr, 0u, LCD_WIDTH - 1u, 51u, 63u, 0u);
        } else if (gScannerSaveState == SCAN_SAVE_CHAN_SEL) {
            /* 「保存:」+ 信道号，与上方「保存?」确认语义一致；前缀 UTF-8 共 7 字节 */
            static const char save_chan_prefix_cn[] = "\xe4\xbf\x9d\xe5\xad\x98:";
            const size_t save_chan_prefix_len = sizeof(save_chan_prefix_cn) - 1u;
            memcpy(String, save_chan_prefix_cn, save_chan_prefix_len);
            UI_GenerateChannelStringEx(String + save_chan_prefix_len, gShowChPrefix, gScanChannel);
            UI_PrintStringSmallAtPixel(String, 0u, LCD_WIDTH - 1u, 51u, 63u, 0u);
        } else if (gScanCssState < SCAN_CSS_STATE_FOUND) {
            static const char scan_cn[] = "\xe6\x89\xab\xe6\x8f\x8f\xe4\xb8\xad";
            memcpy(String, scan_cn, sizeof(scan_cn) - 1u);
            {
                const size_t off = sizeof(scan_cn) - 1u;
                const unsigned n = (unsigned)((gScanProgressIndicator & 7) + 1u);
                memset(String + off, '.', n);
                String[off + n] = '\0';
            }
            UI_PrintStringSmallAtPixel(String, 0u, LCD_WIDTH - 1u, 51u, 63u, 0u);
        } else if (gScanCssState == SCAN_CSS_STATE_FOUND) {
            /* 扫描完成 */
            UI_PrintStringSmallAtPixel(
                "\xe6\x89\xab\xe6\x8f\x8f\xe5\xae\x8c\xe6\x88\x90",
                0u,
                LCD_WIDTH - 1u,
                51u,
                63u,
                0u);
        } else {
            UI_PrintStringSmallAtPixel("SCAN FAIL.", 0u, LCD_WIDTH - 1u, 51u, 63u, 0u);
        }

        ST7565_BlitFullScreen();
        return;
    }
#endif

    if (freq_known) {
        sprintf(String, "FREQ:%u.%05u", gScanFrequency / 100000, gScanFrequency % 100000);
        pPrintStr = String;
    } else {
        pPrintStr = "FREQ:**.*****";
    }

    /* vOffset 下移避开 fb y0 分隔线；12px 行距 = 10px 笔画 + 2px 间距 */
    UI_PrintStringVOffset(pPrintStr, 2, 0, 1);

    if (gScanCssState < SCAN_CSS_STATE_FOUND || !gScanUseCssResult) {
        pPrintStr = "CTC:******";
    } else if (gScanCssResultType == CODE_TYPE_CONTINUOUS_TONE) {
        sprintf(String, "CTC:%u.%uHz", CTCSS_Options[gScanCssResultCode] / 10, CTCSS_Options[gScanCssResultCode] % 10);
        pPrintStr = String;
    } else {
        sprintf(String, "DCS:D%03oN", DCS_Options[gScanCssResultCode]);
        pPrintStr = String;
    }

    UI_PrintStringVOffset(pPrintStr, 2, 1, 5);

    if (gScannerSaveState == SCAN_SAVE_CHANNEL) {
        pPrintStr = "SAVE?";
        Start     = 44;  /* 5 字×8px 居中：(128-40)/2 */
    } else {
        Start     = 2;

        if (gScannerSaveState == SCAN_SAVE_CHAN_SEL) {
            strcpy(String, "SAVE:");
            UI_GenerateChannelStringEx(String + 5, gShowChPrefix, gScanChannel);
            pPrintStr = String;
        } else if (gScanCssState < SCAN_CSS_STATE_FOUND) {
            const unsigned n = (unsigned)((gScanProgressIndicator & 7) + 1);
            memcpy(String, "SCAN....", 4 + n);
            String[4 + n] = '\0';
            pPrintStr = String;
        } else if (gScanCssState == SCAN_CSS_STATE_FOUND) {
            pPrintStr = "SCAN CMP.";
        } else {
            pPrintStr = "SCAN FAIL.";
        }
    }

    UI_PrintStringVOffset(pPrintStr, Start, 3, 1);

    /* bottom line: band on its own row (fb line 6 = last visible; fb 7 is not blitted) */
    UI_PrintStringSmallNormal(uhf ? "BAND:UHF" : "BAND:VHF", 2, 0, 6);

    ST7565_BlitFullScreen();
}
