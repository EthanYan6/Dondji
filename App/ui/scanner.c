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
    bool bCentered;
    uint8_t Start;

    UI_DisplayClear();

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
        /* x_end == x_start：不居中，左对齐（helper 仅在 x_end > x_start 时水平居中） */
        UI_PrintStringSmallAtPixel(pPrintStr, 2u, 2u, 2u, 14u, 3u);

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
        UI_PrintStringSmallAtPixel(pPrintStr, 2u, 2u, 23u, 35u, 3u);

        memset(String, 0, sizeof(String));
        if (gScannerSaveState == SCAN_SAVE_CHANNEL) {
            pPrintStr = "SAVE?";
            UI_PrintStringSmallAtPixel(pPrintStr, 0u, LCD_WIDTH - 1u, 40u, 54u, 3u);
        } else if (gScannerSaveState == SCAN_SAVE_CHAN_SEL) {
            strcpy(String, "SAVE:");
            UI_GenerateChannelStringEx(String + 5, gShowChPrefix, gScanChannel);
            UI_PrintStringSmallAtPixel(String, 0u, LCD_WIDTH - 1u, 40u, 54u, 3u);
        } else if (gScanCssState < SCAN_CSS_STATE_FOUND) {
            static const char scan_cn[] = "\xe6\x89\xab\xe6\x8f\x8f\xe4\xb8\xad";
            memcpy(String, scan_cn, sizeof(scan_cn) - 1u);
            {
                const size_t off = sizeof(scan_cn) - 1u;
                const unsigned n = (unsigned)((gScanProgressIndicator & 7) + 1u);
                memset(String + off, '.', n);
                String[off + n] = '\0';
            }
            UI_PrintStringSmallAtPixel(String, 0u, LCD_WIDTH - 1u, 40u, 54u, 3u);
        } else if (gScanCssState == SCAN_CSS_STATE_FOUND) {
            UI_PrintStringSmallAtPixel("SCAN CMP.", 0u, LCD_WIDTH - 1u, 40u, 54u, 0u);
        } else {
            UI_PrintStringSmallAtPixel("SCAN FAIL.", 0u, LCD_WIDTH - 1u, 40u, 54u, 0u);
        }

        ST7565_BlitFullScreen();
        return;
    }
#endif

    if (gScanSingleFrequency || (gScanCssState != SCAN_CSS_STATE_OFF && gScanCssState != SCAN_CSS_STATE_FAILED)) {
        sprintf(String, "FREQ:%u.%05u", gScanFrequency / 100000, gScanFrequency % 100000);
        pPrintStr = String;
    } else {
        pPrintStr = "FREQ:**.*****";
    }

    UI_PrintString(pPrintStr, 2, 0, 1, 8);

    if (gScanCssState < SCAN_CSS_STATE_FOUND || !gScanUseCssResult) {
        pPrintStr = "CTC:******";
    } else if (gScanCssResultType == CODE_TYPE_CONTINUOUS_TONE) {
        sprintf(String, "CTC:%u.%uHz", CTCSS_Options[gScanCssResultCode] / 10, CTCSS_Options[gScanCssResultCode] % 10);
        pPrintStr = String;
    } else {
        sprintf(String, "DCS:D%03oN", DCS_Options[gScanCssResultCode]);
        pPrintStr = String;
    }

    UI_PrintString(pPrintStr, 2, 0, 3, 8);
    memset(String, 0, sizeof(String));
    if (gScannerSaveState == SCAN_SAVE_CHANNEL) {
        pPrintStr = "SAVE?";
        Start     = 0;
        bCentered = 1;
    } else {
        Start     = 2;
        bCentered = 0;

        if (gScannerSaveState == SCAN_SAVE_CHAN_SEL) {
            strcpy(String, "SAVE:");
            UI_GenerateChannelStringEx(String + 5, gShowChPrefix, gScanChannel);
            pPrintStr = String;
        } else if (gScanCssState < SCAN_CSS_STATE_FOUND) {
            strcpy(String, "SCAN");
            memset(String + 4, '.', (gScanProgressIndicator & 7) + 1);
            pPrintStr = String;
        } else if (gScanCssState == SCAN_CSS_STATE_FOUND) {
            pPrintStr = "SCAN CMP.";
        } else {
            pPrintStr = "SCAN FAIL.";
        }
    }

    UI_PrintString(pPrintStr, Start, bCentered ? 127 : 0, 5, 8);

    ST7565_BlitFullScreen();
}
