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
#include "ui/dualvfo_u8g2_freq.h"

/** Logo 下方一整页（gFrameBuffer[6]）：「电压 + 三空格 + Dondji + 版本」整串居中（VERSION_STRING_2，无呼号）；不改变底部开机提示 */
static void UI_Welcome_DrawLogoBelowU8g2InfoLine(void)
{
    const uint8_t logo_below_row_index = BOOT_LOGO_FRAME_LINES;
    const uint8_t info_line_y_top      = 44u;

    char combined_info_line_buf[64];
    uint32_t voltage_whole_volts;
    uint32_t voltage_frac_two_digits;
    uint8_t combined_line_width_px;
    uint16_t lcd_width_u16;
    uint16_t side_margin_u16;
    uint8_t centered_text_left_x;

    memset(gFrameBuffer[logo_below_row_index], 0, LCD_WIDTH);

    voltage_whole_volts = (uint32_t)gBatteryVoltageAverage / 100u;
    voltage_frac_two_digits = (uint32_t)gBatteryVoltageAverage % 100u;

    sprintf(combined_info_line_buf, "%u.%02uV   Dondji %s",
            (unsigned)voltage_whole_volts,
            (unsigned)voltage_frac_two_digits,
            VERSION_STRING_2);

    combined_line_width_px = DualVfoU8g2_GetSmallTextWidth(combined_info_line_buf);
    if (combined_line_width_px == 0u)
    {
        return;
    }

    lcd_width_u16 = (uint16_t)LCD_WIDTH;
    if ((uint16_t)combined_line_width_px >= lcd_width_u16)
    {
        centered_text_left_x = 0u;
    }
    else
    {
        side_margin_u16 = (uint16_t)(lcd_width_u16 - (uint16_t)combined_line_width_px);
        centered_text_left_x = (uint8_t)(side_margin_u16 / 2u);
    }

    DualVfoU8g2_DrawSmallText(combined_info_line_buf, centered_text_left_x, info_line_y_top, true);
}

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
    /* 中文开机提示带下移 1px（57→58）；英文与中文同像素带，避免 VOffset 跨两行拆字 */
    const uint8_t boot_hint_bottom_y_start = 58u;
    const uint8_t boot_hint_bottom_y_end = 63u;

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
#ifdef ENABLE_CHINESE
        if (gSetting_boot_hint == 0)
        {
            UI_PrintStringSmallAtPixel(
                "Dondji", 0, 127, boot_hint_bottom_y_start, boot_hint_bottom_y_end, 0u);
        }
        else if (gSetting_boot_hint == 1)
        {
            UI_PrintStringSmallAtPixel(
                "Beautiful BJ", 0, 127, boot_hint_bottom_y_start, boot_hint_bottom_y_end, 0u);
        }
        else
        {
            UI_PrintStringSmallAtPixel(
                "happy 55th", 0, 127, boot_hint_bottom_y_start, boot_hint_bottom_y_end, 0u);
        }
#else
        /* 无 ENABLE_CHINESE：单行底对齐于最后一页 */
        const uint8_t boot_hint_english_single_row = 7u;

        if (gSetting_boot_hint == 0)
        {
            UI_PrintStringSmallNormalBottomInRow(
                "Dondji", 0, 127, boot_hint_english_single_row);
        }
        else if (gSetting_boot_hint == 1)
        {
            UI_PrintStringSmallNormalBottomInRow(
                "Beautiful BJ", 0, 127, boot_hint_english_single_row);
        }
        else
        {
            UI_PrintStringSmallNormalBottomInRow(
                "happy 55th", 0, 127, boot_hint_english_single_row);
        }
#endif
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
        line_one_text = "松开按键";
        line_two_text = "解锁全部菜单";
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

void UI_DisplaySideKeyError(void)
{
    const char *line_one_text = "Wrong key!";
    const char *line_two_text = "Use the top one.";

    memset(gStatusLine,  0, sizeof(gStatusLine));
#if defined(ENABLE_FEAT_F4HWN_CTR) || defined(ENABLE_FEAT_F4HWN_INV)
        ST7565_ContrastAndInv();
#endif
    UI_DisplayClear();

#ifdef ENABLE_CHINESE
    if (gUiLanguage == UI_LANGUAGE_CN) {
        line_one_text = "按错了吧";
        line_two_text = "是上面那个键";
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

    ST7565_BlitStatusLine();
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

    if (gEeprom.POWER_ON_DISPLAY_MODE == POWER_ON_DISPLAY_MODE_LOGO) {
#define LOGO_FLASH_ADDR     0x1FF000
#define LOGO_HEADER_SIZE    8
#define LOGO_BITMAP_ADDR    (LOGO_FLASH_ADDR + LOGO_HEADER_SIZE)
        uint8_t logo_header[LOGO_HEADER_SIZE];
        PY25Q16_ReadBuffer(LOGO_FLASH_ADDR, logo_header, sizeof(logo_header));
        if (logo_header[0] == 'D' && logo_header[1] == 'O' && logo_header[2] == 'N' && logo_header[3] == 'D') {
            PY25Q16_ReadBuffer(LOGO_BITMAP_ADDR, gStatusLine, sizeof(gStatusLine));
            PY25Q16_ReadBuffer(LOGO_BITMAP_ADDR + sizeof(gStatusLine), gFrameBuffer, sizeof(gFrameBuffer));
            ST7565_BlitStatusLine();
            ST7565_BlitFullScreen();
#ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
            SCREENSHOT_Update(true);
#endif
            return;
        }
    }

#ifdef ENABLE_FEAT_F4HWN
    ST7565_BlitFullScreenDualVfoTightTop();

    if (gEeprom.POWER_ON_DISPLAY_MODE == POWER_ON_DISPLAY_MODE_NONE) {
        ST7565_FillScreen(0x00);
#else
    if (gEeprom.POWER_ON_DISPLAY_MODE == POWER_ON_DISPLAY_MODE_NONE) {
        ST7565_FillScreen(0xFF);
#endif
    } else {
#ifndef ENABLE_FEAT_F4HWN
        memset(WelcomeString0, 0, sizeof(WelcomeString0));
        memset(WelcomeString1, 0, sizeof(WelcomeString1));

        PY25Q16_ReadBuffer(0x00A0C8, WelcomeString0, 16);
        PY25Q16_ReadBuffer(0x00A0D8, WelcomeString1, 16);

        sprintf(WelcomeString2, "%u.%02uV %u%%",
                gBatteryVoltageAverage / 100,
                gBatteryVoltageAverage % 100,
                BATTERY_VoltsToPercent(gBatteryVoltageAverage));

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
        /* F4HWN：顶部 Mini Kong Logo，Logo 下一行 u8g2 电压/版本，最下行仍为菜单「开机提示」 */
        UI_Welcome_CopyLogoToFrameBuffer();
        UI_Welcome_DrawLogoBelowU8g2InfoLine();
        UI_Welcome_DrawBootHintBottom();
#endif

        ST7565_BlitFullScreenDualVfoTightTop();

        #ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
            SCREENSHOT_Update(true);
        #endif
    }
}