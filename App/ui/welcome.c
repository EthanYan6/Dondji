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
#include <stdint.h>

#include "driver/py25q16.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "helper/battery.h"
#include "settings.h"
#include "misc.h"
#include "ui/helper.h"
#include "ui/welcome.h"
#include "ui/status.h"
#include "version.h"

#ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
    #include "screenshot.h"
#endif

#ifdef ENABLE_FEAT_F4HWN
#include "ui/boot_logo_bitmap.h"

static void UI_Welcome_CopyLogoToFrameBuffer(void)
{
    uint8_t logo_row_index = 0;

    while (logo_row_index < BOOT_LOGO_FRAME_LINES)
    {
        memcpy(gFrameBuffer[logo_row_index], gBootLogoBitmap[logo_row_index], LCD_WIDTH);
        logo_row_index = logo_row_index + 1;
    }
}

static void UI_Welcome_DrawBootHintBottom(void)
{
    /* 开机提示再下移 7 像素：英文用语义明确的像素下移（小号字 Line6 + vOffset7） */
    const uint8_t boot_hint_bottom_y_start = 57u;
    const uint8_t boot_hint_bottom_y_end = 63u;
    const uint8_t boot_hint_english_line = 6u;
    const uint8_t boot_hint_english_down_px = 7u;

#ifdef ENABLE_CHINESE
    if (gUiLanguage == UI_LANGUAGE_CN)
    {
        if (gSetting_boot_hint == 0)
        {
            UI_PrintStringSmallAtPixel("\xe5\x8f\xae\xe5\x92\x9a\xe9\xb8\xa1", 0, 127, boot_hint_bottom_y_start, boot_hint_bottom_y_end, 3u);
        }
        else if (gSetting_boot_hint == 1)
        {
            UI_PrintStringSmallAtPixel("\xe9\xad\x85\xe5\x8a\x9b\xe5\x8c\x97\xe4\xba\xac", 0, 127, boot_hint_bottom_y_start, boot_hint_bottom_y_end, 3u);
        }
        else
        {
            UI_PrintStringSmallAtPixel("\xe4\xba\x94\xe4\xba\x94\xe8\x8a\x82\xe7\xba\xaa\xe5\xbf\xb5\xe7\x89\x88", 0, 127, boot_hint_bottom_y_start, boot_hint_bottom_y_end, 3u);
        }
    }
    else
#endif
    {
        if (gSetting_boot_hint == 0)
        {
            UI_PrintStringSmallNormalVOffset(
                "Dondji", 0, 127, boot_hint_english_line, boot_hint_english_down_px);
        }
        else if (gSetting_boot_hint == 1)
        {
            UI_PrintStringSmallNormalVOffset(
                "Beautiful BJ", 0, 127, boot_hint_english_line, boot_hint_english_down_px);
        }
        else
        {
            UI_PrintStringSmallNormalVOffset(
                "happy 55th", 0, 127, boot_hint_english_line, boot_hint_english_down_px);
        }
    }
}
#endif

void UI_DisplayReleaseKeys(void)
{
    const char *line_one_text = "RELEASE";
    const char *line_two_text = "ALL KEYS";

    memset(gStatusLine,  0, sizeof(gStatusLine));
#if defined(ENABLE_FEAT_F4HWN_CTR) || defined(ENABLE_FEAT_F4HWN_INV)
        ST7565_ContrastAndInv();
#endif
    UI_DisplayClear();

#ifdef ENABLE_CHINESE
    if (gUiLanguage == UI_LANGUAGE_CN) {
        line_one_text = "解锁";
        line_two_text = "全部按键";
    }
#endif

#ifdef ENABLE_CHINESE
    if (gUiLanguage == UI_LANGUAGE_CN) {
        UI_PrintStringSmallAtPixel(line_one_text, 0, 127, 10u, 24u, 3u);
        UI_PrintStringSmallAtPixel(line_two_text, 0, 127, 34u, 48u, 3u);
    } else
#endif
    {
        UI_PrintString(line_one_text, 0, 127, 1, 10);
        UI_PrintString(line_two_text, 0, 127, 3, 10);
    }

    ST7565_BlitStatusLine();  // blank status line
    ST7565_BlitFullScreen();
}

void UI_DisplayWelcome(void)
{
#ifndef ENABLE_FEAT_F4HWN
    char WelcomeString0[16];
    char WelcomeString1[16];
    char WelcomeString2[16];
#endif

    memset(gStatusLine,  0, sizeof(gStatusLine));

#if defined(ENABLE_FEAT_F4HWN_CTR) || defined(ENABLE_FEAT_F4HWN_INV)
        ST7565_ContrastAndInv();
#endif
    UI_DisplayClear();

#ifdef ENABLE_FEAT_F4HWN
    ST7565_BlitFullScreenDualVfoTightTop();

    if (gEeprom.POWER_ON_DISPLAY_MODE == POWER_ON_DISPLAY_MODE_NONE || gEeprom.POWER_ON_DISPLAY_MODE == POWER_ON_DISPLAY_MODE_SOUND) {
        ST7565_FillScreen(0x00);
#else
    if (gEeprom.POWER_ON_DISPLAY_MODE == POWER_ON_DISPLAY_MODE_NONE || gEeprom.POWER_ON_DISPLAY_MODE == POWER_ON_DISPLAY_MODE_FULL_SCREEN) {
        ST7565_FillScreen(0xFF);
#endif
    } else {
#ifndef ENABLE_FEAT_F4HWN
        memset(WelcomeString0, 0, sizeof(WelcomeString0));
        memset(WelcomeString1, 0, sizeof(WelcomeString1));

        // 0x0EB0
        PY25Q16_ReadBuffer(0x00A0C8, WelcomeString0, 16);
        // 0x0EC0
        PY25Q16_ReadBuffer(0x00A0D8, WelcomeString1, 16);

        sprintf(WelcomeString2, "%u.%02uV %u%%",
                gBatteryVoltageAverage / 100,
                gBatteryVoltageAverage % 100,
                BATTERY_VoltsToPercent(gBatteryVoltageAverage));

        if (gEeprom.POWER_ON_DISPLAY_MODE == POWER_ON_DISPLAY_MODE_VOLTAGE)
        {
            strcpy(WelcomeString0, "VOLTAGE");
            strcpy(WelcomeString1, WelcomeString2);
        }
        else if(gEeprom.POWER_ON_DISPLAY_MODE == POWER_ON_DISPLAY_MODE_ALL)
        {
            if(strlen(WelcomeString0) == 0 && strlen(WelcomeString1) == 0)
            {
                strcpy(WelcomeString0, "WELCOME");
                strcpy(WelcomeString1, WelcomeString2);
            }
            else if(strlen(WelcomeString0) == 0 || strlen(WelcomeString1) == 0)
            {
                if(strlen(WelcomeString0) == 0)
                {
                    strcpy(WelcomeString0, WelcomeString1);
                }
                strcpy(WelcomeString1, WelcomeString2);
            }
        }
        else if(gEeprom.POWER_ON_DISPLAY_MODE == POWER_ON_DISPLAY_MODE_MESSAGE)
        {
            if(strlen(WelcomeString0) == 0)
            {
                strcpy(WelcomeString0, "WELCOME");
            }

            if(strlen(WelcomeString1) == 0)
            {
                strcpy(WelcomeString1, "BIENVENUE");
            }
        }

        /* 开机首行：菜单「开机提示」可选「叮咚鸡」/「魅力北京」/「五五节纪念版」；第二行仍为电压/EEPROM（WelcomeString1） */
#ifdef ENABLE_CHINESE
        if (gUiLanguage == UI_LANGUAGE_CN) {
            if (gSetting_boot_hint == 0) {
                UI_PrintStringSmallAtPixel("\xe5\x8f\xae\xe5\x92\x9a\xe9\xb8\xa1", 0, 127, 4u, 15u, 3u);
            } else if (gSetting_boot_hint == 1) {
                UI_PrintStringSmallAtPixel("\xe9\xad\x85\xe5\x8a\x9b\xe5\x8c\x97\xe4\xba\xac", 0, 127, 4u, 15u, 3u);
            } else {
                UI_PrintStringSmallAtPixel("\xe4\xba\x94\xe4\xba\x94\xe8\x8a\x82\xe7\xba\xaa\xe5\xbf\xb5\xe7\x89\x88", 0, 127, 4u, 15u, 3u);
            }
        } else
#endif
        {
            if (gSetting_boot_hint == 0)
                UI_PrintString("Dondji", 0, 127, 0, 10);
            else if (gSetting_boot_hint == 1)
                UI_PrintString("Beautiful BJ", 0, 127, 0, 10);
            else
                UI_PrintString("happy 55th", 0, 127, 0, 10);
        }
        UI_PrintString(WelcomeString1, 0, 127, 2, 10);

        UI_PrintStringSmallNormal(Version, 0, 127, 6);

#else
        /* F4HWN：顶部 Mini Kong Logo，最下行显示菜单「开机提示」当前选项（与 menu 子项文案一致） */
        UI_Welcome_CopyLogoToFrameBuffer();
        UI_Welcome_DrawBootHintBottom();
#endif

        ST7565_BlitFullScreenDualVfoTightTop();

        #ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
            SCREENSHOT_Update(true);
        #endif
    }
}