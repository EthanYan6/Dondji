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
#include <stdlib.h>

#include "../app/dtmf.h"
#include "../app/menu.h"
#include "../app/mdc1200.h"
#include "../bitmaps.h"
#include "../board.h"
#include "../dcs.h"
#include "../driver/backlight.h"
#include "../driver/bk4819.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../external/printf/printf.h"
#include "../font.h"
#include "../frequencies.h"
#include "../helper/battery.h"
#include "../misc.h"
#include "../settings.h"

#ifdef ENABLE_FEAT_F4HWN
    #include "../version.h"
#endif

#include "helper.h"
#include "inputbox.h"
#include "menu.h"
#include "status.h"
#include "ui.h"

#ifdef ENABLE_CHINESE
#include "menu_sub_values_cn.h"
#define SUBV(en, cn) ((gUiLanguage == UI_LANGUAGE_CN) ? (const char *)(cn) : (const char *)(en))
#else
#define SUBV(en, cn) (en)
#endif

#ifdef ENABLE_FEAT_F4HWN
#include "../driver/py25q16.h"
#endif

#ifdef ENABLE_FEAT_F4HWN_QRCODE
static const uint8_t BITMAP_QR_Dondji_Compressed[165] = {
    0x00, 0x00, 0xFC, 0x04, 0x74, 0x74, 0x74, 0x04, 0xFC, 0x00, 0x5C, 0x50, 0x30, 0x0C, 0xEC, 0xDC,
    0xE8, 0xC8, 0x90, 0x5C, 0x74, 0x8C, 0x50, 0x00, 0xFC, 0x04, 0x74, 0x74, 0x74, 0x04, 0xFC, 0x00,
    0x00, 0x00, 0x00, 0xF5, 0x29, 0xD9, 0xB5, 0x15, 0x9D, 0x55, 0x0C, 0x15, 0x14, 0x43, 0xD8, 0x2B,
    0x44, 0x7F, 0x6E, 0xCF, 0x20, 0xE1, 0x6C, 0x75, 0xC4, 0x49, 0xD1, 0xE5, 0x79, 0xCD, 0x25, 0x4D,
    0x00, 0x00, 0x00, 0x00, 0x7A, 0x72, 0x5A, 0x40, 0x2B, 0x24, 0x55, 0x60, 0xA0, 0x8E, 0x69, 0x89,
    0x0B, 0xF2, 0x13, 0xEF, 0x31, 0x2B, 0x6B, 0x5A, 0xEE, 0x60, 0x75, 0x73, 0xEE, 0x28, 0xE7, 0x59,
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x41, 0x5D, 0x5D, 0x5D, 0x41, 0x7F, 0x00, 0x4F, 0x35, 0x01,
    0x69, 0x4D, 0x2F, 0x58, 0x56, 0x5C, 0x30, 0x32, 0x2B, 0x67, 0x64, 0x7D, 0x5C, 0x47, 0x7D, 0x1A,
    0x3C, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00
};

static void QR_SetPixel(uint8_t x, uint8_t y)
{
    if (x >= 128 || y >= 64) return;
    if (y < 8) {
        gStatusLine[x] |= (uint8_t)(1u << y);
    } else {
        const uint8_t fb_y = (uint8_t)(y - 8u);
        gFrameBuffer[fb_y >> 3][x] |= (uint8_t)(1u << (fb_y & 7u));
    }
}

static void UI_DrawQRCode(uint8_t origin_x, uint8_t origin_y)
{
    const uint8_t *bitmap = (const uint8_t *)BITMAP_QR_Dondji_Compressed;
    const uint8_t size = 33;

    for (uint8_t qy = 0; qy < size; qy++) {
        for (uint8_t qx = 0; qx < size; qx++) {
            if (qy < 32 ?
                ((bitmap[(uint16_t)(qy >> 3) * (uint16_t)size + (uint16_t)qx] >> (qy & 7u)) & 1u) :
                ((bitmap[132 + (qx >> 3)] >> (qx & 7u)) & 1u)) {
                QR_SetPixel((uint8_t)(origin_x + qx),
                            (uint8_t)(origin_y + qy));
            }
        }
    }
}
#endif

/* Level-2 browse: +1 — right pane is x≥50, clears left title; +4 stacked everything at bottom. L3 edit: +2. */
#define MENU_VALUE_ROW_EXTRA() ((uint8_t)(gIsInSubMenu ? 2u : 1u))
#define MENU_VALUE_ROW(BASE) ((uint8_t)((uint8_t)(BASE) + MENU_VALUE_ROW_EXTRA()))

#ifdef ENABLE_CHINESE
/* Channel MEM_* 3-line layout: 7px ASCII band, CH_GAP px, 12px Han name — avoids row-grid overlap */
enum { CH_SM_H = 7, CH_GAP = 3, CH_CN_H = 12 };
/* 3-line channel block height: Ch + gap + Freq + gap + Name(Han) */
#define CH_MEM_BLOCK_H ((unsigned)CH_SM_H + (unsigned)CH_GAP + (unsigned)CH_SM_H + (unsigned)CH_GAP + (unsigned)CH_CN_H)
static uint8_t CH_after(uint8_t y, unsigned h) { return (uint8_t)((unsigned)y + h + (unsigned)CH_GAP); }
/* 存信道 / 删信道 / 命名信道右栏：与「命名信道」编辑带（UI_PrintStringSmallChannelNameBand）一致，混排拉丁不额外下移 */
static void UI_MENU_PrintMemChPaneMaybeCjk(const char *text,
                                           unsigned int x1,
                                           unsigned int x2,
                                           uint8_t y0)
{
    const char *p = text;
    if (p == NULL)
        p = "";
    UI_PrintStringSmallChannelNameBand(p, (uint8_t)x1, (uint8_t)x2, y0);
}
/* F1/F2/M 侧键两项两行值：行间额外 2px（与 CH_GAP 区分） */
static uint8_t SF_after_2px(uint8_t y, unsigned h) { return (uint8_t)((unsigned)y + h + 2u); }
/* L2: center block in right pane (~y8..55); L3: start below title (y11..18), avoid overlap with menu title */
static uint8_t CH_mem_blk_y0(void)
{
    const unsigned block_h = CH_MEM_BLOCK_H;
    uint8_t block_y_start = 0u;
    if (gIsInSubMenu)
    {
        const unsigned top = 20u;
        const unsigned bot = 63u;
        const unsigned avail = (bot >= top) ? (bot - top + 1u) : 0u;
        const unsigned pad = (avail > block_h) ? ((avail - block_h) / 2u) : 0u;
        block_y_start = (uint8_t)(top + pad);
    }
    else
    {
        const unsigned top = 8u;
        const unsigned bot = 55u;
        const unsigned avail = (bot >= top) ? (bot - top + 1u) : 0u;
        const unsigned pad = (avail > block_h) ? ((avail - block_h) / 2u) : 0u;
        block_y_start = (uint8_t)(top + pad);
        block_y_start = (uint8_t)(block_y_start + 7u);
    }

    return block_y_start;
}
/* SysInf (MENU_VOL): line height for stacking — Han uses 12px band, ASCII small 7px */
static uint8_t VOL_line_band_height(const char *line)
{
    const char *p = line;
    while (*p != 0) {
        const uint8_t c0 = (uint8_t)p[0];
        if (c0 >= 0xE4u && c0 <= 0xEFu && p[1] != 0 && p[2] != 0)
            return CH_CN_H;
        if (c0 >= 0x80u)
            p += 3;
        else
            p += 1;
    }
    return CH_SM_H;
}
#endif

#ifdef ENABLE_FEAT_F4HWN
extern uint32_t _eflash_used;
extern uint32_t _sdata;
extern uint32_t _ebss;

static void UI_MENU_GetMemPercents(uint16_t *flash_percent_x10, uint16_t *sram_percent_x10)
{
    const uintptr_t flash_origin = 0x08002800u;
    const uint32_t flash_size = 118u * 1024u;
    const uintptr_t sram_origin = 0x20000000u;
    const uint32_t sram_size = 16u * 1024u;

    uintptr_t flash_used_addr = (uintptr_t)&_eflash_used;
    uintptr_t sram_data_start = (uintptr_t)&_sdata;
    uintptr_t sram_data_end = (uintptr_t)&_ebss;

    uint32_t flash_used_bytes = 0u;
    uint32_t sram_used_bytes = 0u;
    uint16_t flash_result = 0u;
    uint16_t sram_result = 0u;

    if (flash_used_addr > flash_origin)
    {
        flash_used_bytes = (uint32_t)(flash_used_addr - flash_origin);
    }
    if (flash_used_bytes > flash_size)
    {
        flash_used_bytes = flash_size;
    }
    flash_result = (uint16_t)((flash_used_bytes * 1000u) / flash_size);

    if (sram_data_end > sram_data_start && sram_data_start >= sram_origin)
    {
        sram_used_bytes = (uint32_t)(sram_data_end - sram_data_start);
    }
    if (sram_used_bytes > sram_size)
    {
        sram_used_bytes = sram_size;
    }
    sram_result = (uint16_t)((sram_used_bytes * 1000u) / sram_size);

    *flash_percent_x10 = flash_result;
    *sram_percent_x10 = sram_result;
}

static uint8_t UI_MENU_GetRowPixelStart(const uint8_t row)
{
    uint16_t row_pixel_start = 0u;
    uint8_t pixel_start = 0u;

    row_pixel_start = (uint16_t)row * 8u;
    if (row_pixel_start > 63u)
    {
        row_pixel_start = 63u;
    }
    pixel_start = (uint8_t)row_pixel_start;
    return pixel_start;
}

static void UI_MENU_FormatBuildTimeBeijing(char *time_output, const size_t output_size)
{
    int utc_hour = 0;
    int utc_minute = 0;
    int utc_second = 0;
    int beijing_hour = 0;
    int beijing_minute = 0;
    int beijing_second = 0;
    int beijing_total_seconds = 0;

    if (time_output == NULL || output_size < 16u)
    {
        return;
    }

    if (BuildTime[0] < '0' || BuildTime[0] > '9' ||
        BuildTime[1] < '0' || BuildTime[1] > '9' ||
        BuildTime[2] != ':' ||
        BuildTime[3] < '0' || BuildTime[3] > '9' ||
        BuildTime[4] < '0' || BuildTime[4] > '9' ||
        BuildTime[5] != ':' ||
        BuildTime[6] < '0' || BuildTime[6] > '9' ||
        BuildTime[7] < '0' || BuildTime[7] > '9')
    {
        strcpy(time_output, BuildTime);
        return;
    }

    utc_hour = (BuildTime[0] - '0') * 10 + (BuildTime[1] - '0');
    utc_minute = (BuildTime[3] - '0') * 10 + (BuildTime[4] - '0');
    utc_second = (BuildTime[6] - '0') * 10 + (BuildTime[7] - '0');

    beijing_total_seconds = (utc_hour * 3600) + (utc_minute * 60) + utc_second + (8 * 3600);
    while (beijing_total_seconds >= 86400)
    {
        beijing_total_seconds = beijing_total_seconds - 86400;
    }

    beijing_hour = beijing_total_seconds / 3600;
    beijing_minute = (beijing_total_seconds % 3600) / 60;
    beijing_second = beijing_total_seconds % 60;

    time_output[0] = 'B';
    time_output[1] = 'J';
    time_output[2] = ' ';
    time_output[3] = (char)('0' + (beijing_hour / 10));
    time_output[4] = (char)('0' + (beijing_hour % 10));
    time_output[5] = ':';
    time_output[6] = (char)('0' + (beijing_minute / 10));
    time_output[7] = (char)('0' + (beijing_minute % 10));
    time_output[8] = ':';
    time_output[9] = (char)('0' + (beijing_second / 10));
    time_output[10] = (char)('0' + (beijing_second % 10));
    time_output[11] = '\0';
}

static void UI_MENU_FormatBuildDateSlash(char *date_output, const size_t output_size)
{
    int month_number = 0;
    int day_tens = 0;
    int day_units = 0;
    int year_thousands = 0;
    int year_hundreds = 0;
    int year_tens = 0;
    int year_units = 0;
    int day_number = 0;

    if (date_output == NULL || output_size < 11u)
    {
        return;
    }

    if (strncmp(BuildDate + 0, "Jan", 3u) == 0)
    {
        month_number = 1;
    }
    else if (strncmp(BuildDate + 0, "Feb", 3u) == 0)
    {
        month_number = 2;
    }
    else if (strncmp(BuildDate + 0, "Mar", 3u) == 0)
    {
        month_number = 3;
    }
    else if (strncmp(BuildDate + 0, "Apr", 3u) == 0)
    {
        month_number = 4;
    }
    else if (strncmp(BuildDate + 0, "May", 3u) == 0)
    {
        month_number = 5;
    }
    else if (strncmp(BuildDate + 0, "Jun", 3u) == 0)
    {
        month_number = 6;
    }
    else if (strncmp(BuildDate + 0, "Jul", 3u) == 0)
    {
        month_number = 7;
    }
    else if (strncmp(BuildDate + 0, "Aug", 3u) == 0)
    {
        month_number = 8;
    }
    else if (strncmp(BuildDate + 0, "Sep", 3u) == 0)
    {
        month_number = 9;
    }
    else if (strncmp(BuildDate + 0, "Oct", 3u) == 0)
    {
        month_number = 10;
    }
    else if (strncmp(BuildDate + 0, "Nov", 3u) == 0)
    {
        month_number = 11;
    }
    else if (strncmp(BuildDate + 0, "Dec", 3u) == 0)
    {
        month_number = 12;
    }
    else
    {
        strcpy(date_output, BuildDate);
        return;
    }

    if (BuildDate[4] == ' ')
    {
        day_tens = 0;
    }
    else
    {
        if (BuildDate[4] < '0' || BuildDate[4] > '9')
        {
            strcpy(date_output, BuildDate);
            return;
        }
        day_tens = BuildDate[4] - '0';
    }

    if (BuildDate[5] < '0' || BuildDate[5] > '9' ||
        BuildDate[7] < '0' || BuildDate[7] > '9' ||
        BuildDate[8] < '0' || BuildDate[8] > '9' ||
        BuildDate[9] < '0' || BuildDate[9] > '9' ||
        BuildDate[10] < '0' || BuildDate[10] > '9')
    {
        strcpy(date_output, BuildDate);
        return;
    }

    day_units = BuildDate[5] - '0';
    year_thousands = BuildDate[7] - '0';
    year_hundreds = BuildDate[8] - '0';
    year_tens = BuildDate[9] - '0';
    year_units = BuildDate[10] - '0';
    day_number = day_tens * 10 + day_units;

    date_output[0] = (char)('0' + year_thousands);
    date_output[1] = (char)('0' + year_hundreds);
    date_output[2] = (char)('0' + year_tens);
    date_output[3] = (char)('0' + year_units);
    date_output[4] = '/';
    date_output[5] = (char)('0' + (month_number / 10));
    date_output[6] = (char)('0' + (month_number % 10));
    date_output[7] = '/';
    date_output[8] = (char)('0' + (day_number / 10));
    date_output[9] = (char)('0' + (day_number % 10));
    date_output[10] = '\0';
}
#endif

const t_menu_item MenuList[] =
{
//   text,          menu ID
    {"Step",        MENU_STEP          },
    {"Power",       MENU_TXP           }, // was "TXP"
    {"RxDCS",       MENU_R_DCS         }, // was "R_DCS"
    {"RxCTCS",      MENU_R_CTCS        }, // was "R_CTCS"
    {"TxDCS",       MENU_T_DCS         }, // was "T_DCS"
    {"TxCTCS",      MENU_T_CTCS        }, // was "T_CTCS"
    {"TxODir",      MENU_SFT_D         }, // was "SFT_D"
    {"TxOffs",      MENU_OFFSET        }, // was "OFFSET"
    {"W/N",         MENU_W_N           },
#ifndef ENABLE_FEAT_F4HWN
    {"Scramb",      MENU_SCR           }, // was "SCR"
#endif
    {"BusyCL",      MENU_BCL           }, // was "BCL"
    {"Compnd",      MENU_COMPAND       },
    {"Mode",        MENU_AM            }, // was "AM"
#ifdef ENABLE_FEAT_F4HWN
    {"TXLock",      MENU_TX_LOCK       }, 
#endif
    {"ChList",      MENU_LIST_CH       },
    {"ChSave",      MENU_MEM_CH        }, // was "MEM-CH"
    {"ChDele",      MENU_DEL_CH        }, // was "DEL-CH"
    {"ChName",      MENU_MEM_NAME      },
    {"ChDisp",      MENU_MDF           }, // was "MDF"

    {"ScList",       MENU_S_LIST       },
    {"ScPri",        MENU_S_PRI        },
    {"PriCh1",       MENU_S_PRI_CH_1   },
    {"PriCh2",       MENU_S_PRI_CH_2   },
    {"ScnRev",      MENU_SC_REV        },
#ifndef ENABLE_FEAT_F4HWN
    #ifdef ENABLE_NOAA
        {"NOAA-S",      MENU_NOAA_S    },
    #endif
#endif
    {"Sql",         MENU_SQL           },
    {"F1Shrt",      MENU_F1SHRT        },
    {"F1Long",      MENU_F1LONG        },
    {"F2Shrt",      MENU_F2SHRT        },
    {"F2Long",      MENU_F2LONG        },
    {"M Long",      MENU_MLONG         },

    {"KeyLck",      MENU_AUTOLK        }, // was "AUTOLk"
#ifdef ENABLE_FEAT_F4HWN
    {"SetLck",      MENU_SET_LCK       },
#endif
    {"Lang",        MENU_LANGUAGE      },
    {"RxMode",      MENU_TDR           },
    {"TxTOut",      MENU_TOT           }, // was "TOT"
    {"BatSav",      MENU_SAVE          }, // was "SAVE"
    {"BatTxt",      MENU_BAT_TXT       },
    {"Mic",         MENU_MIC           },
    {"POnMsg",      MENU_PONMSG        },
    {"BootHnt",     MENU_BOOT_HINT     },
    {"BLTime",      MENU_ABR           }, // was "ABR"
    {"BLMin",       MENU_ABR_MIN       },
    {"BLMax",       MENU_ABR_MAX       },
    {"BLTxRx",      MENU_ABR_ON_TX_RX  },
    {"Beep",        MENU_BEEP          },
#ifdef ENABLE_VOICE
    {"Voice",       MENU_VOICE         },
#endif
    {"Roger",       MENU_ROGER         },
    {"MDC ID",      MENU_MDC_ID        },
    {"STE",         MENU_STE           },
    {"RP STE",      MENU_RP_STE        },
    {"1 Call",      MENU_1_CALL        },
#ifdef ENABLE_ALARM
    {"AlarmT",      MENU_AL_MOD        },
#endif
#ifdef ENABLE_DTMF_CALLING
    {"ANI ID",      MENU_ANI_ID        },
#endif
    {"UPCode",      MENU_UPCODE        },
    {"DWCode",      MENU_DWCODE        },
    {"PTT ID",      MENU_PTT_ID        },
    {"D ST",        MENU_D_ST          },
#ifdef ENABLE_DTMF_CALLING
    {"D Resp",      MENU_D_RSP         },
    {"D Hold",      MENU_D_HOLD        },
#endif
    {"D Prel",      MENU_D_PRE         },
#ifdef ENABLE_DTMF_CALLING
    {"D Decd",      MENU_D_DCD         },
    {"D List",      MENU_D_LIST        },
#endif
    {"D Live",      MENU_D_LIVE_DEC    }, // live DTMF decoder
#ifndef ENABLE_FEAT_F4HWN
    #ifdef ENABLE_AM_FIX
        {"AM Fix",      MENU_AM_FIX        },
    #endif
#endif
    {"VOX",         MENU_VOX           },
#ifdef ENABLE_FEAT_F4HWN
    {"SysInf",      MENU_VOL           }, // was "VOL"
#else
    {"BatVol",      MENU_VOL           }, // was "VOL"
#endif
#ifndef ENABLE_FEAT_F4HWN
#ifdef ENABLE_AUDIO_BAR
    {"MicBar",      MENU_MIC_BAR       },
#endif
#endif
#ifdef ENABLE_FEAT_F4HWN
    {"SetPwr",      MENU_SET_PWR       },
    {"SetPTT",      MENU_SET_PTT       },
    {"SetTOT",      MENU_SET_TOT       },
    {"SetEOT",      MENU_SET_EOT       },
#ifdef ENABLE_FEAT_F4HWN_CTR
    {"SetCtr",      MENU_SET_CTR       },
#endif
    {"SetInv",      MENU_SET_INV       },
#ifdef ENABLE_FEAT_F4HWN_AUDIO    
    {"SetRxA",      MENU_SET_AUD       },
#endif
    {"SetTmr",      MENU_SET_TMR       },
#ifdef ENABLE_FEAT_F4HWN_SLEEP
    {"SetOff",       MENU_SET_OFF      },
#endif
#ifdef ENABLE_FEAT_F4HWN_NARROWER
    {"SetNFM",      MENU_SET_NFM       },
#endif
#ifdef ENABLE_FEAT_F4HWN_VOL
    {"SetVol",      MENU_SET_VOL       },
#endif
#ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
    {"SetKey",      MENU_SET_KEY       },
#endif
#ifdef ENABLE_NOAA
    {"SetNWR",      MENU_NOAA_S    },
#endif
#ifdef ENABLE_AUDIO_BAR
    {"MicBar",      MENU_MIC_BAR       },
#endif
#endif
    // hidden menu items from here on
    // enabled if pressing both the PTT and upper side button at power-on
    {"F Lock",      MENU_F_LOCK        },
#ifndef ENABLE_FEAT_F4HWN
    {"Tx 200",      MENU_200TX         }, // was "200TX"
    {"Tx 350",      MENU_350TX         }, // was "350TX"
    {"Tx 500",      MENU_500TX         }, // was "500TX"
#endif
    {"350 En",      MENU_350EN         }, // was "350EN"
#ifndef ENABLE_FEAT_F4HWN
    {"ScraEn",      MENU_SCREN         }, // was "SCREN"
#endif
#ifdef ENABLE_F_CAL_MENU
    {"FrCali",      MENU_F_CALI        }, // reference xtal calibration
#endif
    {"BatCal",      MENU_BATCAL        }, // battery voltage calibration
    {"BatTyp",      MENU_BATTYP        }, // battery type 1600/2200mAh
    {"SetNav",      MENU_SET_NAV       }, // set navigation (LEFT / RIGHT or UP / DOWN)
    {"Reset",       MENU_RESET         }, // might be better to move this to the hidden menu items ?

    {"",                              0xff               }  // end of list - DO NOT delete or move this this
};

const uint8_t FIRST_HIDDEN_MENU_ITEM = MENU_F_LOCK;

const char gSubMenu_TXP[][6] =
{
    "USER",
    "LOW 1",
    "LOW 2",
    "LOW 3",
    "LOW 4",
    "LOW 5",
    "MID",
    "HIGH"
};

const char gSubMenu_SFT_D[][4] =
{
    "OFF",
    "+",
    "-"
};

const char gSubMenu_W_N[][7] =
{
    "WIDE",
    "NARROW"
};

const char gSubMenu_OFF_ON[][4] =
{
    "OFF",
    "ON"
};

#ifdef ENABLE_AUDIO_BAR
const char gSubMenu_MIC_BAR_STYLE[][10] =
{
    "OFF",
    "Bar",
    "Popup",
};
#endif

const char gSubMenu_NA[4] =
{
    "N/A"
};

const char* const gSubMenu_RXMode[] =
{
    "MAIN\nONLY",       // TX and RX on main only
    "DUAL RX\nRESPOND", // Watch both and respond
    "CROSS\nBAND",      // TX on main, RX on secondary
    "MAIN TX\nDUAL RX"  // always TX on main, but RX on both
};

#ifdef ENABLE_VOICE
    const char gSubMenu_VOICE[][4] =
    {
        "OFF",
        "CHI",
        "ENG"
    };
#endif

const char* const gSubMenu_MDF[] =
{
    "FREQ",
    "CHANNEL\nNUMBER",
    "NAME",
    "NAME\n+\nFREQ"
};

#ifdef ENABLE_ALARM
    const char gSubMenu_AL_MOD[][5] =
    {
        "SITE",
        "TONE"
    };
#endif

#ifdef ENABLE_DTMF_CALLING
const char gSubMenu_D_RSP[][11] =
{
    "DO\nNOTHING",
    "RING",
    "REPLY",
    "BOTH"
};
#endif

const char* const gSubMenu_PTT_ID[] =
{
    "OFF",
    "UP CODE",
    "DOWN CODE",
    "UP+DOWN\nCODE",
    "APOLLO\nQUINDAR"
};

const char gSubMenu_PONMSG[][8] =
{
#ifdef ENABLE_FEAT_F4HWN
    "ALL",
    "SOUND",
#else
    "FULL",
#endif
    "MESSAGE",
    "VOLTAGE",
    "NONE"
};

const char gSubMenu_ROGER[][6] =
{
    "OFF",
    "ROGER",
    "MDC"
};

const char gSubMenu_RESET[][4] =
{
    "VFO",
    "ALL"
};

const char * const gSubMenu_F_LOCK[] =
{
    "DEFAULT+\n137-174\n400-470",
    "FCC HAM\n144-148\n420-450",
#ifdef ENABLE_FEAT_F4HWN_CA
    "CA HAM\n144-148\n430-450",
#endif
    "CE HAM\n144-146\n430-440",
    "GB HAM\n144-148\n430-440",
    "137-174\n400-430",
    "137-174\n400-438",
#ifdef ENABLE_FEAT_F4HWN_PMR
    "PMR 446",
#endif
#ifdef ENABLE_FEAT_F4HWN_GMRS_FRS_MURS
    "GMRS\nFRS\nMURS",
#endif
    "DISABLE\nALL",
    "UNLOCK\nALL",
};

const char gSubMenu_RX_TX[][6] =
{
    "OFF",
    "TX",
    "RX",
    "TX/RX"
};

const char gSubMenu_BAT_TXT[][8] =
{
    "NONE",
    "VOLTAGE",
    "PERCENT"
};

const char gSubMenu_LANGUAGE[][8] =
{
    "English",
    "\xE4\xB8\xAD\xE6\x96\x87"
};

const char gSubMenu_BOOT_HINT[][14] =
{
    "Dondji",
    "Beautiful BJ",
    "happy 55th"
};

const char gSubMenu_BATTYP[][12] =
{
    "1600mAh K5",
    "2200mAh K5",
    "3500mAh K5",
    "1400mAh K1",
    "2500mAh K1"
};

const char gSubMenu_SET_NAV[][20] =
{
    "LEFT\nRIGHT\nUV-K1(8)",
    "UP\nDOWN\nUV-K5(6)",
};

#ifndef ENABLE_FEAT_F4HWN
const char gSubMenu_SCRAMBLER[][7] =
{
    "OFF",
    "2600Hz",
    "2700Hz",
    "2800Hz",
    "2900Hz",
    "3000Hz",
    "3100Hz",
    "3200Hz",
    "3300Hz",
    "3400Hz",
    "3500Hz"
};
#endif

#ifdef ENABLE_FEAT_F4HWN
    const char gSubMenu_SET_PWR[][6] =
    {
        "< 20m",
        "125m",
        "250m",
        "500m",
        "1",
        "2",
        "5"
    };

    const char gSubMenu_SET_PTT[][8] =
    {
        "CLASSIC",
        "ONEPUSH"
    };

    const char gSubMenu_SET_TOT[][7] =  // Use by SET_EOT too
    {
        "OFF",
        "SOUND",
        "VISUAL",
        "ALL"
    };

    const char gSubMenu_SET_LCK[][9] =
    {
        "KEYS",
        "KEYS+PTT"
    };

    const char gSubMenu_SET_MET[][8] =
    {
        "TINY",
        "CLASSIC"
    };

    #ifdef ENABLE_FEAT_F4HWN_AUDIO
        const char gSubMenu_SET_AUD_FM[][6] =
        {
            "FLAT",
            "CLEAN",
            "MID",
            "BOOST",
            "MAX"
        };

        const char gSubMenu_SET_AUD_AM[][6] =
        {
            "SHARP",
            "STOCK",
            "OPEN"
        };
    #endif

    #ifdef ENABLE_FEAT_F4HWN_NARROWER
        const char gSubMenu_SET_NFM[][9] =
        {
            "NARROW",
            "NARROWER"
        };
    #endif

    #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
        const char gSubMenu_SET_KEY[][9] =
        {
            "KEY_MENU",
            "KEY_UP",
            "KEY_DOWN",
            "KEY_EXIT",
            "KEY_STAR"
        };
    #endif
#endif

const t_sidefunction gSubMenu_SIDEFUNCTIONS[] =
{
    {"NONE",            ACTION_OPT_NONE},
#ifdef ENABLE_FLASHLIGHT
    {"FLASH\nLIGHT",    ACTION_OPT_FLASHLIGHT},
#endif
    {"POWER",           ACTION_OPT_POWER},
    {"MONITOR",         ACTION_OPT_MONITOR},
    {"SCAN",            ACTION_OPT_SCAN},
#ifdef ENABLE_VOX
    {"VOX",             ACTION_OPT_VOX},
#endif
#ifdef ENABLE_ALARM
    {"ALARM",           ACTION_OPT_ALARM},
#endif
#ifdef ENABLE_FMRADIO
    {"FM RADIO",        ACTION_OPT_FM},
#endif
#ifdef ENABLE_TX1750
    {"1750Hz",          ACTION_OPT_1750},
#endif
#ifdef ENABLE_REGA
    {"REGA\nALARM",     ACTION_OPT_REGA_ALARM},
    {"REGA\nTEST",      ACTION_OPT_REGA_TEST},
#endif
    {"LOCK\nKEYPAD",    ACTION_OPT_KEYLOCK},
    {"VFO A\nVFO B",    ACTION_OPT_A_B},
    {"VFO\nMEM",        ACTION_OPT_VFO_MR},
    {"MODE",            ACTION_OPT_SWITCH_DEMODUL},
#ifdef ENABLE_BLMIN_TMP_OFF
    {"BLMIN\nTMP OFF",  ACTION_OPT_BLMIN_TMP_OFF},      //BackLight Minimum Temporay OFF
#endif
#ifdef ENABLE_FEAT_F4HWN
    {"RX MODE",         ACTION_OPT_RXMODE},
    {"MAIN ONLY",       ACTION_OPT_MAINONLY},
    {"PTT",             ACTION_OPT_PTT},
    {"WIDE\nNARROW",    ACTION_OPT_WN},
    {"MUTE",            ACTION_OPT_MUTE},
    #ifdef ENABLE_FEAT_F4HWN_AUDIO
        {"RxA",            ACTION_OPT_RXA},
    #endif
    #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
        {"POWER\nHIGH",    ACTION_OPT_POWER_HIGH},
        {"REMOVE\nOFFSET",  ACTION_OPT_REMOVE_OFFSET},
    #endif
#endif
};

const uint8_t gSubMenu_SIDEFUNCTIONS_size = ARRAY_SIZE(gSubMenu_SIDEFUNCTIONS);

bool    gIsInSubMenu;
uint8_t gMenuCursor;
static uint8_t UI_MENU_GetActualIndexFromCursor(uint8_t cursor)
{
    return MENU_GetActualMenuIndexFromCursor(cursor);
}

int UI_MENU_GetCurrentMenuId() {
    const uint8_t idx = UI_MENU_GetActualIndexFromCursor(gMenuCursor);
    if(idx < ARRAY_SIZE(MenuList))
        return MenuList[idx].menu_id;

    return MenuList[ARRAY_SIZE(MenuList)-1].menu_id;
}

uint8_t UI_MENU_GetMenuIdx(uint8_t id)
{
    uint8_t visible = 0;
    const uint8_t menu_count = MENU_GetActiveMenuCount();
    for(uint8_t i = 0; i < ARRAY_SIZE(MenuList); i++)
        if(MenuList[i].menu_id == id)
        {
            if (MENU_IsMenuIdExcludedFromBrowse(MenuList[i].menu_id))
            {
                if (i > 0u)
                {
                    for (int j = (int)i - 1; j >= 0; j--)
                        if (!MENU_IsMenuIdExcludedFromBrowse(MenuList[(uint8_t)j].menu_id))
                            return MENU_GetVisibleCursorForActualIndex((uint8_t)j);
                }
                return 0;
            }
            if (gMenuMainPageActive)
                return i;
            if (!gMenuUseMainOnlyStatus)
                return MENU_GetVisibleCursorForActualIndex(i);

            for (visible = 0; visible < menu_count; visible++)
                if (MENU_GetActualMenuIndexFromCursor(visible) == i)
                    return visible;
            return 0;
        }
    return 0;
}

int32_t gSubMenuSelection;

// edit box
char    edit_original[24];
char    edit[24];
int     edit_index;

static const char *UI_MENU_MemNameModeLabel(void)
{
    switch (gMemNameInputMode)
    {
        case MEM_NAME_INPUT_UPPER: return "A";
        case MEM_NAME_INPUT_DIGIT: return "1";
        case MEM_NAME_INPUT_SYMBOL: return ",";
        case MEM_NAME_INPUT_PINYIN:  return "PY";
        case MEM_NAME_INPUT_LOWER:
        default: return "a";
    }
}

static void UI_MENU_DrawMemNameCandidates(unsigned int x1, unsigned int x2)
{
    const uint8_t count = gMemNameCandidateCount;
    if (count == 0u || x2 <= x1)
        return;

    const unsigned int avail = (x2 - x1) + 1u;
    for (uint8_t i = 0; i < count; i++)
    {
        char num[2];
        char sym[2];
        const unsigned int slot_l = x1 + (avail * i) / count;
        const unsigned int slot_r = x1 + (avail * (i + 1u)) / count - 1u;
        const unsigned int slot_w = (slot_r >= slot_l) ? (slot_r - slot_l + 1u) : 0u;
        const unsigned int w_num = 6u;
        const unsigned int w_sym = 6u;
        const unsigned int token_w = w_num + 4u + w_sym; // "数字序号 + 4px + 符号"
        unsigned int token_x = slot_l;

        if (slot_w > token_w)
            token_x = slot_l + (slot_w - token_w) / 2u;

        num[0] = (char)('1' + i);
        num[1] = 0;
        sym[0] = gMemNameCandidates[i];
        sym[1] = 0;

        UI_PrintStringSmallAtPixel(num, token_x, token_x, 50u, 57u, 0u);
        UI_PrintStringSmallAtPixel(sym, token_x + w_num + 4u, token_x + w_num + 4u, 50u, 57u, 0u);
    }
}

/* 符号模式：固定 6 格均匀分布（与候选条相同纵坐标），数字 1–6 + 对应符号；未满格符号为空 */
static void UI_MENU_DrawMemNameSymbolSixPack(unsigned int x1, unsigned int x2)
{
    static const uint8_t slot_total = 6u;
    const uint8_t n = gMemNameCandidateCount;
    const unsigned int y_strip_top = 50u;
    const unsigned int y_strip_bot = 57u;
    uint8_t slot_index;

    if (x2 <= x1)
    {
        return;
    }

    {
        const unsigned int avail = (unsigned int)((x2 - x1) + 1u);
        const unsigned int w_num = 6u;
        const unsigned int w_sym = 6u;
        const unsigned int gap_num_sym = 4u;

        for (slot_index = 0; slot_index < slot_total; slot_index++)
        {
            char num_str[2];
            char sym_str[2];
            unsigned int slot_l;
            unsigned int slot_r;
            unsigned int slot_w;
            unsigned int token_w;
            unsigned int token_x;
            unsigned int sym_x;

            slot_l = x1 + (avail * (unsigned int)slot_index) / (unsigned int)slot_total;
            slot_r = x1 + (avail * ((unsigned int)slot_index + 1u)) / (unsigned int)slot_total - 1u;

            if (slot_r >= slot_l)
            {
                slot_w = slot_r - slot_l + 1u;
            }
            else
            {
                slot_w = 0u;
            }

            token_w = w_num + gap_num_sym + w_sym;
            token_x = slot_l;
            if (slot_w > token_w)
            {
                token_x = slot_l + (slot_w - token_w) / 2u;
            }

            num_str[0] = (char)('1' + slot_index);
            num_str[1] = 0;
            UI_PrintStringSmallAtPixel(num_str, token_x, token_x, y_strip_top, y_strip_bot, 0u);

            sym_x = token_x + w_num + gap_num_sym;

            if (slot_index < n)
            {
                sym_str[0] = gMemNameCandidates[slot_index];
                sym_str[1] = 0;
            }
            else
            {
                sym_str[0] = ' ';
                sym_str[1] = 0;
            }
            UI_PrintStringSmallAtPixel(sym_str, sym_x, sym_x, y_strip_top, y_strip_bot, 0u);
        }
    }
}

#ifdef ENABLE_CHINESE
static void UI_MENU_DrawMemNamePinyinEdit(unsigned int sub_val_x1, unsigned int sub_val_x2)
{
    const uint8_t y_mode = 20u;
    const uint8_t y_name = 28u;
    const uint8_t y_pinyin = (uint8_t)(y_name + 4u);
    const uint8_t y_strip = (uint8_t)(y_name + 22u);
    const size_t max_b = (size_t)CHANNEL_NAME_MAX_BYTES;

    switch (gMemNameInputMode)
    {
        case MEM_NAME_INPUT_DIGIT:
            UI_PrintStringSmallAtPixel("1", (uint8_t)(sub_val_x2 - 6), (uint8_t)sub_val_x2, y_mode, (uint8_t)(y_mode + 7u), 0u);
            break;
        case MEM_NAME_INPUT_LOWER:
            UI_PrintStringSmallAtPixel("a", (uint8_t)(sub_val_x2 - 6), (uint8_t)sub_val_x2, y_mode, (uint8_t)(y_mode + 7u), 0u);
            break;
        case MEM_NAME_INPUT_UPPER:
            UI_PrintStringSmallAtPixel("A", (uint8_t)(sub_val_x2 - 6), (uint8_t)sub_val_x2, y_mode, (uint8_t)(y_mode + 7u), 0u);
            break;
        case MEM_NAME_INPUT_SYMBOL:
            UI_PrintStringSmallAtPixel(",", (uint8_t)(sub_val_x2 - 6), (uint8_t)sub_val_x2, y_mode, (uint8_t)(y_mode + 7u), 0u);
            break;
        default:
            if (gPinyinLen > 0 && gCNCandidateCount == 0)
            {
                if (gUiLanguage == UI_LANGUAGE_CN)
                    UI_PrintStringSmallAtPixel("\xe9\x80\x89\xe6\x8b\xa9", (uint8_t)(sub_val_x2 - 24), (uint8_t)sub_val_x2, y_mode, (uint8_t)(y_mode + 11u), 0u);
                else
                    UI_PrintStringSmallAtPixel("Sel.", (uint8_t)(sub_val_x2 - 18), (uint8_t)sub_val_x2, y_mode, (uint8_t)(y_mode + 7u), 0u);
            }
            else
                UI_PrintStringSmallAtPixel("PY", (uint8_t)(sub_val_x2 - 12), (uint8_t)sub_val_x2, y_mode, (uint8_t)(y_mode + 7u), 0u);
            break;
    }

    {
        const uint8_t eng_cw = 6;
        const uint8_t chn_cw = 12;
        const uint8_t ul_spacing = 1;
        uint8_t x = (uint8_t)sub_val_x1;
        size_t bi = 0;
        uint8_t slot_x[16];
        uint8_t slot_w[16];
        uint8_t slot_count = 0;
        int8_t cursor_slot = -1;

        while (bi < max_b && x < sub_val_x2 && slot_count < 15)
        {
            slot_x[slot_count] = x;
            if ((uint8_t)edit[bi] >= 0xE4 && (uint8_t)edit[bi] <= 0xEF)
            {
                char ch[4] = { edit[bi], edit[bi + 1], edit[bi + 2], 0 };
                UI_PrintStringSmallAtPixel(ch, x, (uint8_t)(x + chn_cw), y_name, (uint8_t)(y_name + 11u), 0u);
                slot_w[slot_count] = chn_cw;
                if (edit_index >= 0 && (size_t)edit_index == bi)
                    cursor_slot = (int8_t)slot_count;
                x += chn_cw + ul_spacing;
                bi += 3;
            }
            else if (edit[bi] == '_' || edit[bi] == 0)
            {
                if (edit[bi] == '_' && bi + 2 < max_b && edit[bi + 1] == '_' && edit[bi + 2] == '_')
                {
                    slot_w[slot_count] = chn_cw;
                    if (edit_index >= 0 &&
                        ((size_t)edit_index == bi || (size_t)edit_index == bi + 1 || (size_t)edit_index == bi + 2))
                        cursor_slot = (int8_t)slot_count;
                    x += chn_cw + ul_spacing;
                    bi += 3;
                }
                else
                {
                    slot_w[slot_count] = eng_cw;
                    if (edit_index >= 0 && (size_t)edit_index == bi)
                        cursor_slot = (int8_t)slot_count;
                    x += eng_cw + ul_spacing;
                    bi++;
                }
            }
            else
            {
                char ch[2] = { edit[bi], 0 };
                UI_PrintStringSmallAtPixel(ch, x, (uint8_t)(x + eng_cw), y_name, (uint8_t)(y_name + 11u), 0u);
                slot_w[slot_count] = eng_cw;
                if (edit_index >= 0 && (size_t)edit_index == bi)
                    cursor_slot = (int8_t)slot_count;
                x += eng_cw + ul_spacing;
                bi++;
            }
            slot_count++;
        }

        while (bi < max_b && slot_count < 15 && x + eng_cw <= sub_val_x2)
        {
            slot_x[slot_count] = x;
            if (edit[bi] == '_' && bi + 2 < max_b && edit[bi + 1] == '_' && edit[bi + 2] == '_')
            {
                slot_w[slot_count] = chn_cw;
                if (edit_index >= 0 &&
                    ((size_t)edit_index == bi || (size_t)edit_index == bi + 1 || (size_t)edit_index == bi + 2))
                    cursor_slot = (int8_t)slot_count;
                bi += 3;
                x += chn_cw + ul_spacing;
            }
            else
            {
                slot_w[slot_count] = eng_cw;
                if (edit_index >= 0 && (size_t)edit_index == bi)
                    cursor_slot = (int8_t)slot_count;
                bi++;
                x += eng_cw + ul_spacing;
            }
            slot_count++;
        }

        if (edit_index >= 0 && (size_t)edit_index == CHANNEL_NAME_MAX_BYTES && slot_count < 15 && x + eng_cw <= sub_val_x2)
        {
            slot_x[slot_count] = x;
            slot_w[slot_count] = eng_cw;
            cursor_slot = (int8_t)slot_count;
            slot_count++;
        }

        if (slot_count > 0)
        {
            const uint8_t ul_y = (uint8_t)(y_name + 8u);
            const uint8_t ul_fb_row = (uint8_t)(ul_y / 8u);
            const uint8_t ul_fb_bit = (uint8_t)(1u << (ul_y % 8u));
            uint8_t s;

            if (ul_fb_row < FRAME_LINES)
            {
                for (s = 0; s < slot_count; s++)
                {
                    uint8_t c;
                    if (cursor_slot >= 0 && (int8_t)s == cursor_slot)
                        UI_PrintStringSmallAtPixel("^", slot_x[s], (uint8_t)(slot_x[s] + slot_w[s]), (uint8_t)(ul_y + 5u),
                                                 (uint8_t)(ul_y + 12u), 0u);
                    else
                    {
                        for (c = 0; c < slot_w[s]; c++)
                            gFrameBuffer[ul_fb_row][slot_x[s] + c] |= ul_fb_bit;
                    }
                }
            }
        }
    }

    if (gPinyinLen > 0)
    {
        char pinyin_display[PINYIN_MAX_LEN + 2];
        memcpy(pinyin_display, gPinyinBuffer, gPinyinLen);
        pinyin_display[gPinyinLen] = '_';
        pinyin_display[gPinyinLen + 1] = 0;
        UI_PrintStringSmallAtPixel(pinyin_display, (uint8_t)(sub_val_x2 - (gPinyinLen + 1) * 6u), (uint8_t)sub_val_x2, y_pinyin,
                                     (uint8_t)(y_pinyin + 7u), 0u);
    }

    if (gCNCandidateCount > 0)
    {
        const unsigned strip_w = (unsigned)(sub_val_x2 - sub_val_x1);
        const unsigned slot_w = strip_w / 6u;
        uint8_t i;

        for (i = 0; i < gCNCandidateCount; i++)
        {
            char num[2];
            char utf8[4];
            uint16_t unicode = gCNCandidates[i];
            const uint8_t cx = (uint8_t)(sub_val_x1 + (unsigned)i * slot_w);

            num[0] = (char)('1' + i);
            num[1] = 0;
            UI_PrintStringSmallAtPixel(num, cx, (uint8_t)(cx + 6u), y_strip, (uint8_t)(y_strip + 7u), 0u);

            utf8[0] = (char)(0xE0 | (unicode >> 12));
            utf8[1] = (char)(0x80 | ((unicode >> 6) & 0x3F));
            utf8[2] = (char)(0x80 | (unicode & 0x3F));
            utf8[3] = 0;
            UI_PrintStringSmallAtPixel(utf8, (uint8_t)(cx + 8u), (uint8_t)(cx + 20u), y_strip, (uint8_t)(y_strip + 11u), 0u);
        }
    }
    else if (gMemNameInputMode == MEM_NAME_INPUT_SYMBOL)
    {
        UI_MENU_DrawMemNameSymbolSixPack(sub_val_x1, sub_val_x2);
    }
    else if (gMemNameCandidateCount > 0)
    {
        const unsigned strip_w = (unsigned)(sub_val_x2 - sub_val_x1);
        const unsigned slot_w = strip_w / 4u;
        uint8_t i;

        for (i = 0; i < gMemNameCandidateCount; i++)
        {
            char num[2];
            char ch[2];
            const uint8_t cx = (uint8_t)(sub_val_x1 + (unsigned)i * slot_w);

            num[0] = (char)('1' + i);
            num[1] = 0;
            UI_PrintStringSmallAtPixel(num, cx, (uint8_t)(cx + 6u), y_strip, (uint8_t)(y_strip + 7u), 0u);

            ch[0] = gMemNameCandidates[i];
            ch[1] = 0;
            UI_PrintStringSmallAtPixel(ch, (uint8_t)(cx + 8u), (uint8_t)(cx + 20u), y_strip, (uint8_t)(y_strip + 7u), 0u);
        }
    }

    if (edit_index < (int)CHANNEL_NAME_MAX_BYTES)
    {
        if (gMemNameInputMode == MEM_NAME_INPUT_PINYIN)
        {
            const bool cn = (gUiLanguage == UI_LANGUAGE_CN);
            if (gCNCandidateCount == 0 && gMemNameCandidateCount == 0)
            {
                const char *hint_py;
                if (gPinyinLen > 0)
                {
                    if (gPinyinLookupNoMatch != 0)
                        hint_py = cn ? "\xe6\x9c\xaa\xe6\x9f\xa5\xe5\x88\xb0 0\xe5\x88\xa0" : "No match 0 del";
                    else
                        hint_py = cn ? "MENU\xe7\xa1\xae\xe8\xae\xa4 0\xe5\x88\xa0" : "MENU OK 0 del";
                }
                else
                    hint_py = cn ? "2-9\xe8\xbe\x93\xe5\x85\xa5 #\xe5\x88\x87\xe6\x8d\xa2\xe6\xa8\xa1\xe5\xbc\x8f" : "2-9 input # mode";
                UI_PrintStringSmallAtPixel(hint_py, (uint8_t)sub_val_x1, (uint8_t)sub_val_x2, y_strip, (uint8_t)(y_strip + 7u), 0u);
            }
        }
        else if (gPinyinLen == 0 && gCNCandidateCount == 0 && gMemNameCandidateCount == 0 &&
                 gMemNameInputMode != MEM_NAME_INPUT_SYMBOL)
        {
            UI_PrintStringSmallAtPixel((gUiLanguage == UI_LANGUAGE_CN) ? "#\xe5\x88\x87\xe6\x8d\xa2 EXIT\xe5\x9b\x9e\xe9\x80\x80"
                                                                       : "#switch EXIT back",
                                       (uint8_t)sub_val_x1, (uint8_t)sub_val_x2, y_strip, (uint8_t)(y_strip + 7u), 0u);
        }
    }
}
#endif /* ENABLE_CHINESE */

static void UI_MENU_DrawLauncherGear40(uint8_t x, uint8_t y)
{
    static const uint64_t GEAR40[40] = {
        0x0000000000ULL,
        0x00007E0000ULL,
        0x00007E0000ULL,
        0x00007E0000ULL,
        0x00007E0000ULL,
        0x01C3FFC380ULL,
        0x03EFFFF7C0ULL,
        0x07FFFFFFE0ULL,
        0x07FFFFFFE0ULL,
        0x07FFFFFFE0ULL,
        0x03FF00FFC0ULL,
        0x01FC003F80ULL,
        0x03F8001FC0ULL,
        0x03F0000FC0ULL,
        0x07E00007E0ULL,
        0x07E00007E0ULL,
        0x07C00003E0ULL,
        0x7FC00003FEULL,
        0x7FC00003FEULL,
        0x7FC00003FEULL,
        0x7FC00003FEULL,
        0x7FC00003FEULL,
        0x7FC00003FEULL,
        0x07C00003E0ULL,
        0x07E00007E0ULL,
        0x07E00007E0ULL,
        0x03F0000FC0ULL,
        0x03F8001FC0ULL,
        0x01FC003F80ULL,
        0x03FF00FFC0ULL,
        0x07FFFFFFE0ULL,
        0x07FFFFFFE0ULL,
        0x07FFFFFFE0ULL,
        0x03EFFFF7C0ULL,
        0x01C3FFC380ULL,
        0x00007E0000ULL,
        0x00007E0000ULL,
        0x00007E0000ULL,
        0x00007E0000ULL,
        0x0000000000ULL
    };

    // Slightly reduce height to 36, keep width stretched to 48.
    for (uint8_t yy = 0; yy < 36; yy++)
    {
        const uint8_t sy = (uint8_t)((yy * 40u) / 36u);
        const uint64_t row = GEAR40[sy];
        for (uint8_t xx = 0; xx < 48; xx++)
        {
            const uint8_t sx = (uint8_t)((xx * 40u) / 48u);
            const uint64_t mask = 1ULL << (39 - sx);
            PutPixel((uint8_t)(x + xx), (uint8_t)(y + yy), (row & mask) != 0);
        }
    }
}

#ifdef ENABLE_CHINESE
static bool UI_MENU_IsToneMenu(const uint8_t menu_id)
{
    bool is_tone_menu = false;

    if (menu_id == MENU_R_CTCS ||
        menu_id == MENU_T_CTCS ||
        menu_id == MENU_R_DCS  ||
        menu_id == MENU_T_DCS)
    {
        is_tone_menu = true;
    }

    return is_tone_menu;
}

static uint8_t UI_MENU_GetToneValueYOffsetPx(const bool is_in_submenu)
{
    uint8_t value_y_offset_px = 10u;

    if (is_in_submenu)
    {
        value_y_offset_px = 10u;
    }

    return value_y_offset_px;
}

static void UI_MENU_PrintSubmenuValueLine(const char *line,
                                          unsigned int x1,
                                          unsigned int x2,
                                          unsigned int y_row,
                                          bool small_font,
                                          uint8_t y_offset_px)
{
    /* Pixel renderer for CN and EN UI — same ccc value appearance (ASCII uses embedded small font) */
    const uint8_t y0 = (uint8_t)(y_row * 8u + y_offset_px);
    const uint8_t y1 = small_font ? (uint8_t)(y0 + 7u) : (uint8_t)(y0 + 11u);
    UI_PrintStringSmallAtPixel(line, x1, x2, y0, y1, 0u);
}
#endif

static void UI_MENU_DrawChevron(bool left, uint8_t center_x, uint8_t center_y, uint8_t half_h, uint8_t thickness)
{
    for (uint8_t dy = 0; dy <= half_h; dy++)
    {
        const int16_t y_up = (int16_t)center_y - dy;
        const int16_t y_dn = (int16_t)center_y + dy;
        const int16_t x_edge = left ? ((int16_t)center_x - dy) : ((int16_t)center_x + dy);

        for (uint8_t t = 0; t < thickness; t++)
        {
            const int16_t x = left ? (x_edge + t) : (x_edge - t);
            PutPixel((uint8_t)x, (uint8_t)y_up, true);
            PutPixel((uint8_t)x, (uint8_t)y_dn, true);
        }
    }
}

/* Level 2 (browse): [ left: menu name centered in pane, index n/m below ] | vertical line only | [ right: values ] */
static void UI_MENU_DrawLevel2SplitLayout(uint8_t menu_count, char *String)
{
    const uint8_t left_end = (uint8_t)(8u * 6u - 1u);
    /* 两行标题：左栏水平居中由 PrintStringSmallAtPixel 完成；汉字高 12px，行间空 2px */
    const uint8_t l2_y1_lo = 10u;
    const uint8_t l2_y1_hi = 21u;
    const uint8_t l2_y2_lo = 24u;
    const uint8_t l2_y2_hi = 35u;
    const uint8_t actual_idx = UI_MENU_GetActualIndexFromCursor((uint8_t)gMenuCursor);
    const char *t = UI_MENU_GetMenuTitle(&MenuList[actual_idx]);

#ifdef ENABLE_CHINESE
    if (gUiLanguage == UI_LANGUAGE_CN)
    {
        #if defined(ENABLE_DTMF_CALLING)
        if (UI_MENU_GetCurrentMenuId() == MENU_D_DCD)
        {
            /* 左栏约 48px 宽，单行「DTMF解码」会裁字 */
            UI_PrintStringSmallAtPixel("DTMF", 0, left_end, l2_y1_lo, l2_y1_hi, 0u);
            UI_PrintStringSmallAtPixel("\xe8\xa7\xa3\xe7\xa0\x81", 0, left_end, l2_y2_lo, l2_y2_hi, 3u);
        }
        else
        #endif
#ifdef ENABLE_FEAT_F4HWN
#ifdef ENABLE_FEAT_F4HWN_CTR
        if (UI_MENU_GetCurrentMenuId() == MENU_SET_CTR)
        {
            /* 设置对比度：设置 / 对比度 */
            UI_PrintStringSmallAtPixel("\xe8\xae\xbe\xe7\xbd\xae", 0, left_end, l2_y1_lo, l2_y1_hi, 3u);
            UI_PrintStringSmallAtPixel("\xe5\xaf\xb9\xe6\xaf\x94\xe5\xba\xa6", 0, left_end, l2_y2_lo, l2_y2_hi, 3u);
        }
        else
#endif
        if (UI_MENU_GetCurrentMenuId() == MENU_TX_LOCK)
        {
            UI_PrintStringSmallAtPixel("\xe6\xae\xb5\xe5\xa4\x96", 0, left_end, l2_y1_lo, l2_y1_hi, 3u);
            UI_PrintStringSmallAtPixel("\xe5\x8f\x91\xe5\xb0\x84\xe9\x94\x81", 0, left_end, l2_y2_lo, l2_y2_hi, 3u);
        }
        else
#endif
        if (UI_MENU_GetCurrentMenuId() == MENU_R_DCS)
        {
            UI_PrintStringSmallAtPixel("接收", 0, left_end, l2_y1_lo, l2_y1_hi, 3u);
            UI_PrintStringSmallAtPixel("数字亚音", 0, left_end, l2_y2_lo, l2_y2_hi, 3u);
        }
        else if (UI_MENU_GetCurrentMenuId() == MENU_R_CTCS)
        {
            UI_PrintStringSmallAtPixel("接收", 0, left_end, l2_y1_lo, l2_y1_hi, 3u);
            UI_PrintStringSmallAtPixel("模拟亚音", 0, left_end, l2_y2_lo, l2_y2_hi, 3u);
        }
        else if (UI_MENU_GetCurrentMenuId() == MENU_T_DCS)
        {
            UI_PrintStringSmallAtPixel("发射", 0, left_end, l2_y1_lo, l2_y1_hi, 3u);
            UI_PrintStringSmallAtPixel("数字亚音", 0, left_end, l2_y2_lo, l2_y2_hi, 3u);
        }
        else if (UI_MENU_GetCurrentMenuId() == MENU_T_CTCS)
        {
            UI_PrintStringSmallAtPixel("发射", 0, left_end, l2_y1_lo, l2_y1_hi, 3u);
            UI_PrintStringSmallAtPixel("模拟亚音", 0, left_end, l2_y2_lo, l2_y2_hi, 3u);
        }
        else if (UI_MENU_GetCurrentMenuId() == MENU_S_PRI_CH_1)
        {
            UI_PrintStringSmallAtPixel("\xe4\xbc\x98\xe5\x85\x88\xe4\xbf\xa1\xe9\x81\x93", 0, left_end, l2_y1_lo, l2_y1_hi, 3u);
            UI_PrintStringSmallAtPixel("1", 0, left_end, l2_y2_lo, l2_y2_hi, 0u);
        }
        else if (UI_MENU_GetCurrentMenuId() == MENU_S_PRI_CH_2)
        {
            UI_PrintStringSmallAtPixel("\xe4\xbc\x98\xe5\x85\x88\xe4\xbf\xa1\xe9\x81\x93", 0, left_end, l2_y1_lo, l2_y1_hi, 3u);
            UI_PrintStringSmallAtPixel("2", 0, left_end, l2_y2_lo, l2_y2_hi, 0u);
        }
        else if (UI_MENU_GetCurrentMenuId() == MENU_MIC)
        {   /* 设置列表左栏：两行「麦克风」「增益」（中文显示用 SPI 字库 cn_font.bin） */
            UI_PrintStringSmallAtPixel("\xe9\xba\xa6\xe5\x85\x8b\xe9\xa3\x8e", 0, left_end, l2_y1_lo, l2_y1_hi, 3u);
            UI_PrintStringSmallAtPixel("\xe5\xa2\x9e\xe7\x9b\x8a", 0, left_end, l2_y2_lo, l2_y2_hi, 3u);
        }
        else if (UI_MENU_GetCurrentMenuId() == MENU_F1SHRT)
        {
            UI_PrintStringSmallAtPixel("\xe4\xbe\xa7\xe9\x94\xae""1", 0, left_end, l2_y1_lo, l2_y1_hi, 0u);
            UI_PrintStringSmallAtPixel("\xe7\x9f\xad\xe6\x8c\x89", 0, left_end, l2_y2_lo, l2_y2_hi, 3u);
        }
        else if (UI_MENU_GetCurrentMenuId() == MENU_F1LONG)
        {
            UI_PrintStringSmallAtPixel("\xe4\xbe\xa7\xe9\x94\xae""1", 0, left_end, l2_y1_lo, l2_y1_hi, 0u);
            UI_PrintStringSmallAtPixel("\xe9\x95\xbf\xe6\x8c\x89", 0, left_end, l2_y2_lo, l2_y2_hi, 3u);
        }
        else if (UI_MENU_GetCurrentMenuId() == MENU_F2SHRT)
        {
            UI_PrintStringSmallAtPixel("\xe4\xbe\xa7\xe9\x94\xae""2", 0, left_end, l2_y1_lo, l2_y1_hi, 0u);
            UI_PrintStringSmallAtPixel("\xe7\x9f\xad\xe6\x8c\x89", 0, left_end, l2_y2_lo, l2_y2_hi, 3u);
        }
        else if (UI_MENU_GetCurrentMenuId() == MENU_F2LONG)
        {
            UI_PrintStringSmallAtPixel("\xe4\xbe\xa7\xe9\x94\xae""2", 0, left_end, l2_y1_lo, l2_y1_hi, 0u);
            UI_PrintStringSmallAtPixel("\xe9\x95\xbf\xe6\x8c\x89", 0, left_end, l2_y2_lo, l2_y2_hi, 3u);
        }
        else if (UI_MENU_GetCurrentMenuId() == MENU_MLONG)
        {
            UI_PrintStringSmallAtPixel("MENU", 0, left_end, l2_y1_lo, l2_y1_hi, 0u);
            UI_PrintStringSmallAtPixel("\xe9\x95\xbf\xe6\x8c\x89", 0, left_end, l2_y2_lo, l2_y2_hi, 3u);
        }
        else if (UI_MENU_GetCurrentMenuId() == MENU_RP_STE)
        {
            UI_PrintStringSmallAtPixel("过中继", 0, left_end, l2_y1_lo, l2_y1_hi, 3u);
            UI_PrintStringSmallAtPixel("尾音消除", 0, left_end, l2_y2_lo, l2_y2_hi, 3u);
        }
        else if (UI_MENU_GetCurrentMenuId() == MENU_TDR)
        {
            UI_PrintStringSmallAtPixel("\xe6\x8e\xa5\xe6\x94\xb6\xe6\xa8\xa1\xe5\xbc\x8f", 0, left_end, l2_y1_lo, l2_y1_hi, 3u);
            UI_PrintStringSmallAtPixel("<\xe8\xae\xbe\xe4\xb8\xbb\xe9\xa1\xb5>", 0, left_end, l2_y2_lo, l2_y2_hi, 0u);
        }
        else if (UI_MENU_GetCurrentMenuId() == MENU_VOL)
        {
            UI_PrintStringSmallAtPixel("\xe7\xb3\xbb\xe7\xbb\x9f\xe4\xbf\xa1\xe6\x81\xaf", 0, left_end, 10u, 36u, 3u);
        }
        else
        {
            /* Upper-middle of left pane; keep y_end < ~40 so rows 5+ stay for values on the right */
            UI_PrintStringSmallAtPixel(t, 0, left_end, 10, 36, 0u);
        }
    }
    else
    {
        /* English: one line only; never use CN two-line splits */
        UI_PrintStringSmallAtPixel(t, 0, left_end, 10, 36, 0u);
    }
#else
    /* Big font two rows high — roughly centered above bottom index */
    UI_PrintString(t, 0, left_end, 2, 8);
#endif

    sprintf(String, "%u/%u", (unsigned)(1u + (unsigned)gMenuCursor), (unsigned)menu_count);
    UI_PrintStringSmallNormal(String, 0, left_end, 6);

    for (unsigned int i = 0; i < 7u; i++)
        gFrameBuffer[i][(8u * 6u) + 1u] = 0xAAu;
}

/* Level 3 (edit): menu title top-left; main body uses full width from menu_value_x1 (values below title). */
static void UI_MENU_DrawCccMenuChrome(const t_menu_item *item)
{
    const char *title = UI_MENU_GetMenuTitle(item);

#ifdef ENABLE_CHINESE
    if (item != NULL && item->menu_id == MENU_VOL && gUiLanguage == UI_LANGUAGE_CN)
    {
        title = "\xe7\xb3\xbb\xe7\xbb\x9f\xe4\xbf\xa1\xe6\x81\xaf<\xe5\x8f\xae\xe5\x92\x9a\xe9\xb8\xa1>";
    }
    /* Title band +3px down vs previous 8..15 */
    UI_PrintStringSmallAtPixel(title, 2, 2, 11, 18, 0u);
#else
    /* Left align (End==Start); +3px down when F4HWN VOffset available */
    #ifdef ENABLE_FEAT_F4HWN
        UI_PrintStringSmallNormalVOffset(title, 2, 2, 1, 3);
    #else
        UI_PrintStringSmallNormal(title, 2, 2, 1);
    #endif
#endif
}

static void UI_MENU_DrawLauncherChannel40(uint8_t x, uint8_t y)
{
    // Document icon with folded corner + 3 lines, matching reference style.
    const uint8_t l = x + 7, t = y + 3, r = x + 41, b = y + 33;

    // Main outline (open top-right for folded corner)
    UI_DrawLineBuffer(gFrameBuffer, l + 3, t, r - 9, t, true);
    UI_DrawLineBuffer(gFrameBuffer, l, t + 3, l, b - 3, true);
    UI_DrawLineBuffer(gFrameBuffer, l + 3, b, r - 3, b, true);
    UI_DrawLineBuffer(gFrameBuffer, r, t + 12, r, b - 3, true);

    // Rounded corners
    UI_DrawLineBuffer(gFrameBuffer, l, t + 3, l + 3, t, true);
    UI_DrawLineBuffer(gFrameBuffer, l, b - 3, l + 3, b, true);
    UI_DrawLineBuffer(gFrameBuffer, r - 3, b, r, b - 3, true);

    // Folded corner
    UI_DrawLineBuffer(gFrameBuffer, r - 15, t + 11, r - 1, t - 1, true);

    // Inner horizontal lines
    UI_DrawLineBuffer(gFrameBuffer, l + 6,  t + 8,  l + 22, t + 8,  true);
    UI_DrawLineBuffer(gFrameBuffer, l + 6,  t + 14, l + 18, t + 14, true);
    UI_DrawLineBuffer(gFrameBuffer, l + 6,  t + 21, l + 14, t + 21, true);
}

static void UI_MENU_DrawLauncherOther40(uint8_t x, uint8_t y)
{
    // Folder icon in 48x36 footprint
    UI_DrawLineBuffer(gFrameBuffer, x + 6,  y + 8,  x + 21, y + 8,  true);
    UI_DrawLineBuffer(gFrameBuffer, x + 21, y + 8,  x + 26, y + 13, true);
    UI_DrawLineBuffer(gFrameBuffer, x + 26, y + 13, x + 39, y + 13, true);
    UI_DrawLineBuffer(gFrameBuffer, x + 39, y + 13, x + 41, y + 15, true);
    UI_DrawRectangleBuffer(gFrameBuffer, x + 5, y + 15, x + 42, y + 33, true);
    UI_DrawLineBuffer(gFrameBuffer, x + 6, y + 8, x + 6, y + 15, true);
    // thicken main strokes
    UI_DrawLineBuffer(gFrameBuffer, x + 5, y + 16, x + 42, y + 16, true);
    UI_DrawLineBuffer(gFrameBuffer, x + 5, y + 33, x + 42, y + 33, true);
}

static void UI_MENU_DrawLauncherDisplay40(uint8_t x, uint8_t y)
{
    uint8_t body_left = (uint8_t)(x + 6u);
    uint8_t body_top = (uint8_t)(y + 10u);
    uint8_t body_right = (uint8_t)(x + 42u);
    uint8_t body_bottom = (uint8_t)(y + 33u);
    uint8_t screen_left = (uint8_t)(x + 12u);
    uint8_t screen_top = (uint8_t)(y + 14u);
    uint8_t screen_right = (uint8_t)(x + 30u);
    uint8_t screen_bottom = (uint8_t)(y + 25u);
    uint8_t antenna_top_left_x = (uint8_t)(x + 21u);
    uint8_t antenna_top_right_x = (uint8_t)(x + 30u);
    uint8_t antenna_mid_x = (uint8_t)(x + 24u);
    uint8_t antenna_top_y = (uint8_t)(y + 2u);
    uint8_t antenna_bottom_y = body_top;

    UI_DrawRectangleBuffer(gFrameBuffer, body_left, body_top, body_right, body_bottom, true);
    UI_DrawRectangleBuffer(gFrameBuffer, screen_left, screen_top, screen_right, screen_bottom, true);

    {
        uint8_t left_line_height = (uint8_t)(antenna_bottom_y - antenna_top_y);
        uint8_t right_line_height = (uint8_t)(antenna_bottom_y - antenna_top_y);
        uint8_t left_line_dx = (uint8_t)(antenna_mid_x - antenna_top_left_x);
        uint8_t right_line_dx = (uint8_t)(antenna_top_right_x - antenna_mid_x);
        uint8_t line_step = 0u;

        for (line_step = 0u; line_step <= left_line_height; line_step++)
        {
            uint8_t line_y = (uint8_t)(antenna_top_y + line_step);
            uint8_t left_line_x = (uint8_t)(antenna_top_left_x + (line_step * left_line_dx) / left_line_height);
            uint8_t right_line_x = (uint8_t)(antenna_top_right_x - (line_step * right_line_dx) / right_line_height);

            PutPixel(left_line_x, line_y, true);
            PutPixel(right_line_x, line_y, true);
        }
    }

    UI_DrawRectangleBuffer(gFrameBuffer, x + 34u, y + 16u, x + 36u, y + 19u, true);
    UI_DrawRectangleBuffer(gFrameBuffer, x + 34u, y + 22u, x + 36u, y + 25u, true);
}

static void UI_MENU_DrawLauncherAbout40(uint8_t x, uint8_t y)
{
    const int16_t cx = (int16_t)x + 24;
    const int16_t cy = (int16_t)y + 18;

    // Outer circle ring
    for (int16_t yy = -16; yy <= 16; yy++)
        for (int16_t xx = -16; xx <= 16; xx++)
        {
            const int32_t r2 = xx * xx + yy * yy;
            if (r2 <= (16 * 16) && r2 >= (13 * 13))
                PutPixel((uint8_t)(cx + xx), (uint8_t)(cy + yy), true);
        }

    // Exclamation mark dot (upper), separated from stem.
    for (int16_t yy = -2; yy <= 2; yy++)
        for (int16_t xx = -2; xx <= 2; xx++)
            if ((xx * xx + yy * yy) <= 4)
                PutPixel((uint8_t)(cx + xx), (uint8_t)(y + 10 + yy), true);

    // Stem (lower) with rounded ends and explicit gap from dot.
    for (uint8_t yy = 18; yy <= 28; yy++)
        for (uint8_t xx = 22; xx <= 26; xx++)
            PutPixel((uint8_t)(x + xx), (uint8_t)(y + yy), true);
    for (int16_t yy = -2; yy <= 2; yy++)
        for (int16_t xx = -2; xx <= 2; xx++)
            if ((xx * xx + yy * yy) <= 4)
            {
                PutPixel((uint8_t)(x + 24 + xx), (uint8_t)(y + 18 + yy), true);
                PutPixel((uint8_t)(x + 24 + xx), (uint8_t)(y + 28 + yy), true);
            }
}

static void UI_MENU_DrawLauncherPage(void)
{
    static const char * const names_en[] = {"Channel", "Settings", "Display", "Other", "About"};
    static const char * const names_cn[] = {
        "\xe4\xbf\xa1\xe9\x81\x93",
        "\xe8\xae\xbe\xe7\xbd\xae",
        "\xe6\x98\xbe\xe7\xa4\xba",
        "\xe5\x85\xb6\xe5\xae\x83",
        "\xe5\x85\xb3\xe4\xba\x8e"
    };
    const uint8_t icon_count = MENU_MainPageIconCount();
    const uint8_t idx = icon_count > 0 ? ((uint8_t)gSubMenuSelection % icon_count) : 0;
    const uint8_t icon_left = 40;
    const uint8_t icon_top = 3;
    char index_str[12];

    UI_DisplayClear();
    UI_DrawLineBuffer(gFrameBuffer, 0, 0, LCD_WIDTH - 1, 0, true);

    if (idx == 0)
    {
        UI_MENU_DrawLauncherChannel40(icon_left, icon_top);
    }
    else if (idx == 1)
    {
        UI_MENU_DrawLauncherGear40(icon_left, icon_top);
    }
    else if (idx == 2)
    {
        UI_MENU_DrawLauncherDisplay40(icon_left, icon_top);
    }
    else if (idx == 3)
    {
        UI_MENU_DrawLauncherOther40(icon_left, icon_top);
    }
    else if (idx == 4)
    {
        UI_MENU_DrawLauncherAbout40(icon_left, icon_top);
    }
    else
    {
        UI_DrawRectangleBuffer(gFrameBuffer, icon_left + 5, icon_top + 5, icon_left + 34, icon_top + 34, true);
    }

    UI_MENU_DrawChevron(false, 15, 23, 12, 3);
    UI_MENU_DrawChevron(true, 112, 23, 12, 3);

    if (icon_count > 0)
    {
        const char *label = (gUiLanguage == UI_LANGUAGE_CN) ? names_cn[idx] : names_en[idx];
        if (label[0] != 0)
        {
#ifdef ENABLE_CHINESE
            if (gUiLanguage == UI_LANGUAGE_CN)
                UI_PrintStringSmallAtPixel(label, 0, LCD_WIDTH - 1, 47, 62, 0u);
            else
#endif
            {
                UI_PrintString(label, 0, LCD_WIDTH - 1, 5, 8);
                UI_PrintString(label, 1, LCD_WIDTH - 1, 5, 8);
            }
        }
    }
    
    sprintf(index_str, "%u/%u", (unsigned int)(idx + 1), (unsigned int)icon_count);
    UI_PrintStringSmallNormal(index_str, 104, 0, 6);

    ST7565_BlitFullScreen();
}

static void UI_MENU_DrawTopRightBadgePlain(const char *text,
                                           const uint8_t value_row,
                                           const uint8_t value_y_offset_px,
                                           const bool center_in_area,
                                           const uint8_t area_x1,
                                           const uint8_t area_x2)
{
    const size_t text_length = strlen(text);
    const size_t char_pitch = ARRAY_SIZE(gFontSmall[0]) + 1u;
    const size_t text_width = text_length * char_pitch;
    const size_t text_span = text_width + 1u;
    const uint8_t text_height = 8u;
    const uint8_t value_y_start = (uint8_t)(value_row * 8u + value_y_offset_px);
    const uint8_t min_gap_px = 3u;
    uint8_t text_x;
    uint8_t text_y_start;
    uint8_t text_y_end;

    if (text_length == 0u || value_row >= FRAME_LINES)
    {
        return;
    }

    if (center_in_area && area_x2 > area_x1 + 2u)
    {
        const uint8_t min_x = area_x1 + 1u;
        const uint8_t area_width = area_x2 - area_x1 + 1u;
        uint8_t max_x;

        if (text_span >= area_width)
        {
            text_x = min_x;
        }
        else
        {
            text_x = (uint8_t)(area_x1 + ((area_width - text_span) / 2u));
        }

        if (area_x2 > text_span)
        {
            max_x = (uint8_t)(area_x2 - text_span);
        }
        else
        {
            max_x = min_x;
        }

        if (max_x < min_x)
        {
            max_x = min_x;
        }
        if (text_x < min_x)
        {
            text_x = min_x;
        }
        else if (text_x > max_x)
        {
            text_x = max_x;
        }
    }
    else
    {
        if (text_span >= (LCD_WIDTH - 3u))
        {
            text_x = 1u;
        }
        else
        {
            const uint8_t global_shift_right = 1u;
            const uint8_t base_text_x = (uint8_t)(LCD_WIDTH - text_span - 3u);
            const uint8_t max_text_x  = (uint8_t)(LCD_WIDTH - text_span - 1u);
            const uint16_t shifted_x = (uint16_t)base_text_x + global_shift_right;

            if (shifted_x > max_text_x)
            {
                text_x = max_text_x;
            }
            else
            {
                text_x = (uint8_t)shifted_x;
            }
        }
    }

    if (value_y_start > (text_height + min_gap_px))
    {
        text_y_start = (uint8_t)(value_y_start - text_height - min_gap_px);
    }
    else
    {
        text_y_start = 0u;
    }

    text_y_end = (uint8_t)(text_y_start + text_height - 1u);
    UI_PrintStringSmallAtPixel(text, text_x, text_x, text_y_start, text_y_end, 0u);
}

void UI_DisplayMenu(void)
{
    const uint8_t menu_count = MENU_GetActiveMenuCount();
    const bool         icon_layout     = (gMenuUseMainOnlyStatus && !gMenuMainPageActive);
    const uint8_t      menu_list_width = 6u;
    unsigned int       menu_item_x1;
    unsigned int       menu_value_x1;
    if (gIsInSubMenu)
    {   /* Level 3: title top-left; values use full width from menu_value_x1 */
        menu_item_x1  = 0u;
        menu_value_x1 = 2u;
    }
    else
    {   /* Level 2: always left pane | vertical separator | right pane (also when icon_layout — was full-width + y=17 line) */
        menu_item_x1  = (8u * menu_list_width) + 2u;
        menu_value_x1 = menu_item_x1;
    }
    const unsigned int menu_item_x2    = LCD_WIDTH - 1;
    unsigned int       i;
    char               String[64];  // bigger cuz we can now do multi-line in one string (use '\n' char)
    char               top_right_badge[16];
    uint8_t            top_right_badge_value_row;
    uint8_t            top_right_badge_value_y_offset_px;

#ifdef ENABLE_DTMF_CALLING
    char               Contact[16];
#endif

    if (gMenuMainPageActive && !gIsInSubMenu)
    {
        UI_MENU_DrawLauncherPage();
        UI_DisplayMainOnlyStatusBar();
        return;
    }

    UI_DisplayClear();
    UI_DrawLineBuffer(gFrameBuffer, 0, 0, LCD_WIDTH - 1, 0, true);

    if (!gIsInSubMenu && menu_count >= 1u)
        UI_MENU_DrawLevel2SplitLayout(menu_count, String);
    else if (gIsInSubMenu && menu_count >= 1u)
    {
        const uint8_t actual_idx = UI_MENU_GetActualIndexFromCursor((uint8_t)gMenuCursor);
        UI_MENU_DrawCccMenuChrome(&MenuList[actual_idx]);
    }

    // **************

    memset(String, 0, sizeof(String));
    memset(top_right_badge, 0, sizeof(top_right_badge));
    top_right_badge_value_row = 0xFFu;
    top_right_badge_value_y_offset_px = 0u;

    bool already_printed = false;

    /* Brightness is set to max in some entries of this menu. Return it to the configured brightness
       level the "next" time we enter here.I.e., when we move from one menu to another.
       It also has to be set back to max when pressing the Exit key. */

    BACKLIGHT_TurnOn();

    //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
        uint8_t gaugeLine = 0;
        uint8_t gaugeMin = 0;
        uint8_t gaugeMax = 0;
    //#endif

    switch (UI_MENU_GetCurrentMenuId())
    {
        case MENU_SQL:
            sprintf(String, "%d", gSubMenuSelection);
            break;

        case MENU_MIC:
            {   // display the mic gain in actual dB rather than just an index number
                const uint8_t mic = gMicGain_dB2[gSubMenuSelection];
                sprintf(String, "+%u.%udB", mic / 2, (mic % 2) * 5);

                gaugeLine = 4;
                gaugeMin = 0;
                gaugeMax = 8;
            }
            break;

        case MENU_MIC_BAR:
            #ifdef ENABLE_AUDIO_BAR
                strcpy(String,
                       SUBV(gSubMenu_MIC_BAR_STYLE[gSubMenuSelection],
                            gSubMenu_MIC_BAR_STYLE_CN[gSubMenuSelection]));
            #else
                strcpy(String, gSubMenu_NA);
            #endif
            break;

        case MENU_STEP: {
            uint16_t step = gStepFrequencyTable[FREQUENCY_GetStepIdxFromSortedIdx(gSubMenuSelection)];
            sprintf(String, "%d.%02ukHz", step / 100, step % 100);
            break;
        }

        case MENU_TXP:
            if(gSubMenuSelection == 0)
            {
#ifdef ENABLE_FEAT_F4HWN
                {
                    const unsigned int n = ARRAY_SIZE(gSubMenu_SET_PWR);
                    const uint8_t      sp =
                        (gSetting_set_pwr < n) ? gSetting_set_pwr : (uint8_t)(n - 1u);
                    const char * const tier = SUBV(gSubMenu_TXP[sp + 1], gSubMenu_TXP_CN[sp + 1]);
                    char               pwrbuf[20];
                    sprintf(pwrbuf, "%sW", SUBV(gSubMenu_SET_PWR[sp], gSubMenu_SET_PWR_CN[sp]));
#ifdef ENABLE_CHINESE
                    /* 左「用」竖排居中；右为完整两档：档名（LOW1…）+ 功率，行间 2px */
                    strcpy(String, "");
                    already_printed                        = true;
                    {
                        unsigned int sub_val_x1 = menu_value_x1;
                        unsigned int sub_val_x2 = menu_item_x2;
                        if (gUiLanguage == UI_LANGUAGE_CN && icon_layout && gIsInSubMenu &&
                            menu_item_x1 == 0u)
                            sub_val_x1 = menu_value_x1 + 8u;
                        if (gUiLanguage == UI_LANGUAGE_CN && !icon_layout && gIsInSubMenu)
                            sub_val_x1 = sub_val_x1 + 2u;
                        if (gUiLanguage == UI_LANGUAGE_CN && !gIsInSubMenu && sub_val_x1 >= 50u)
                            sub_val_x1 = sub_val_x1 + 2u;

                        unsigned int y_line = 2u - 1u;
                        if (!gIsInSubMenu && UI_MENU_GetCurrentMenuId() != MENU_VOL)
                            y_line += 1u;
                        else if (gIsInSubMenu && UI_MENU_GetCurrentMenuId() != MENU_VOL)
                            y_line += 2u;
                        uint8_t yp_tier = (uint8_t)(y_line * 8u);
                        if (!gIsInSubMenu)
                        {
                            yp_tier = (uint8_t)(yp_tier + 7u);
                        }
                        const uint8_t h_tier = VOL_line_band_height(tier);
                        const uint8_t yp_pwr = SF_after_2px(yp_tier, h_tier);
                        const uint8_t h_pwr  = VOL_line_band_height(pwrbuf);
                        const uint8_t blk_h  = (uint8_t)((unsigned)h_tier + 2u + (unsigned)h_pwr);
                        /* 「用/Use」+ 4px + 档名/功率 视为一块，在右栏内整体居中；值两行共用左缘，紧贴间隙 */
                        {
                            const unsigned gap_px  = 4u;
                            const size_t   w_tier  = UI_SmallStringPixelWidth(tier);
                            const size_t   w_pwr   = UI_SmallStringPixelWidth(pwrbuf);
                            const unsigned value_w = (unsigned)((w_tier > w_pwr) ? w_tier : w_pwr);
                            const unsigned avail   = (unsigned)(sub_val_x2 - sub_val_x1);
                            unsigned       block_w;
                            unsigned       block_x0 = (unsigned)sub_val_x1;

                            if (gUiLanguage == UI_LANGUAGE_CN)
                            {
                                const unsigned label_w = 12u;
                                block_w = label_w + gap_px + value_w;
                                if (avail > block_w)
                                    block_x0 = (unsigned)sub_val_x1 + (avail - block_w) / 2u;
                                {
                                    const uint8_t x_label = (uint8_t)block_x0;
                                    const uint8_t x_val = (uint8_t)(block_x0 + label_w + gap_px);
                                    UI_PrintStringSmallAtPixel("\xe7\x94\xa8",
                                                               x_label,
                                                               (uint8_t)(x_label + 11u),
                                                               yp_tier,
                                                               (uint8_t)((unsigned)yp_tier + (unsigned)blk_h - 1u),
                                                               3u);
                                    UI_PrintStringSmallAtPixel(tier,
                                                               x_val,
                                                               x_val,
                                                               yp_tier,
                                                               (uint8_t)((unsigned)yp_tier + (unsigned)h_tier + 1u),
                                                               0u);
                                    UI_PrintStringSmallAtPixel(pwrbuf,
                                                               x_val,
                                                               x_val,
                                                               yp_pwr,
                                                               (uint8_t)((unsigned)yp_pwr + (unsigned)h_pwr + 1u),
                                                               0u);
                                }
                            }
                            else
                            {
                                const unsigned label_w = 18u;
                                block_w = label_w + gap_px + value_w;
                                if (avail > block_w)
                                    block_x0 = (unsigned)sub_val_x1 + (avail - block_w) / 2u;
                                {
                                    const uint8_t x_label = (uint8_t)block_x0;
                                    const uint8_t x_val = (uint8_t)(block_x0 + label_w + gap_px);
                                    UI_PrintStringSmallAtPixel("Use",
                                                               x_label,
                                                               (uint8_t)(x_label + 17u),
                                                               yp_tier,
                                                               (uint8_t)((unsigned)yp_tier + (unsigned)blk_h - 1u),
                                                               0u);
                                    UI_PrintStringSmallAtPixel(tier,
                                                               x_val,
                                                               x_val,
                                                               yp_tier,
                                                               (uint8_t)((unsigned)yp_tier + (unsigned)h_tier + 1u),
                                                               0u);
                                    UI_PrintStringSmallAtPixel(pwrbuf,
                                                               x_val,
                                                               x_val,
                                                               yp_pwr,
                                                               (uint8_t)((unsigned)yp_pwr + (unsigned)h_pwr + 1u),
                                                               0u);
                                }
                            }
                        }
                    }
#else
                    sprintf(String, "%s\n%sW", tier, pwrbuf);
#endif
                }
#else
                strcpy(String, SUBV(gSubMenu_TXP[gSubMenuSelection], gSubMenu_TXP_CN[gSubMenuSelection]));
#endif
            }
            else
            {
#ifdef ENABLE_FEAT_F4HWN
                sprintf(String, "%s\n%sW",
                        SUBV(gSubMenu_TXP[gSubMenuSelection], gSubMenu_TXP_CN[gSubMenuSelection]),
                        SUBV(gSubMenu_SET_PWR[gSubMenuSelection - 1], gSubMenu_SET_PWR_CN[gSubMenuSelection - 1]));
#else
                strcpy(String, SUBV(gSubMenu_TXP[gSubMenuSelection], gSubMenu_TXP_CN[gSubMenuSelection]));
#endif
            }
            break;

        case MENU_R_DCS:
        case MENU_T_DCS:
            if (gSubMenuSelection == 0)
                strcpy(String, SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]));
            else if (gSubMenuSelection < 105)
                sprintf(String, "D%03oN", DCS_Options[gSubMenuSelection -   1]);
            else
                sprintf(String, "D%03oI", DCS_Options[gSubMenuSelection - 105]);
            break;

        case MENU_R_CTCS:
        case MENU_T_CTCS:
        {
            if (gSubMenuSelection == 0)
                strcpy(String, SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]));
            else
                sprintf(String, "%u.%uHz", CTCSS_Options[gSubMenuSelection - 1] / 10, CTCSS_Options[gSubMenuSelection - 1] % 10);
            break;
        }

        case MENU_SFT_D:
            strcpy(String, SUBV(gSubMenu_SFT_D[gSubMenuSelection], gSubMenu_SFT_D_CN[gSubMenuSelection]));
            break;

        case MENU_OFFSET:
            if (!gIsInSubMenu || gInputBoxIndex == 0)
            {
                sprintf(String, "%3d.%05u", gSubMenuSelection / 100000, abs(gSubMenuSelection) % 100000);
            }
            else
            {
                const char * ascii = INPUTBOX_GetAscii();
                sprintf(String, "%.3s.%.3s  ",ascii, ascii + 3);
            }

            UI_PrintStringSmallNormal(String, menu_value_x1, menu_item_x2, MENU_VALUE_ROW(1));
            UI_PrintStringSmallNormal("MHz",  menu_value_x1, menu_item_x2, MENU_VALUE_ROW(3));

            already_printed = true;
            break;

        case MENU_W_N:
            strcpy(String, SUBV(gSubMenu_W_N[gSubMenuSelection], gSubMenu_W_N_CN[gSubMenuSelection]));
            break;

#ifndef ENABLE_FEAT_F4HWN
        case MENU_SCR:
            strcpy(String, SUBV(gSubMenu_SCRAMBLER[gSubMenuSelection], gSubMenu_SCRAMBLER_CN[gSubMenuSelection]));
            #if 1
                if (gSubMenuSelection > 0 && gSetting_ScrambleEnable)
                    BK4819_EnableScramble(gSubMenuSelection - 1);
                else
                    BK4819_DisableScramble();
            #endif
            break;
#endif

        case MENU_VOX:
            #ifdef ENABLE_VOX
                sprintf(String, gSubMenuSelection == 0 ? SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]) : "%u", gSubMenuSelection);
            #else
                strcpy(String, gSubMenu_NA);
            #endif
            break;

        case MENU_ABR:
            if(gSubMenuSelection == 0)
            {
                strcpy(String, SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]));
            }
            else if(gSubMenuSelection < 61)
            {
                sprintf(String, "%02dm:%02ds", (((gSubMenuSelection) * 5) / 60), (((gSubMenuSelection) * 5) % 60));
                //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
                //ST7565_Gauge(4, 1, 60, gSubMenuSelection);
                gaugeLine = 4;
                gaugeMin = 1;
                gaugeMax = 60;
                //#endif
            }
            else
            {
                strcpy(String, SUBV("ON", gSubMenu_ABR_ON_CN));
            }

            // Obsolete ???
            //if(BACKLIGHT_GetBrightness() < 4)
            //    BACKLIGHT_SetBrightness(4);
            break;

        case MENU_ABR_MIN:
        case MENU_ABR_MAX:
            sprintf(String, "%d", gSubMenuSelection);
            if(gIsInSubMenu)
                BACKLIGHT_SetBrightness(gSubMenuSelection);
            // Obsolete ???
            //else if(BACKLIGHT_GetBrightness() < 4)
            //    BACKLIGHT_SetBrightness(4);
            break;

        case MENU_AM:
            strcpy(String, SUBV(gModulationStr[gSubMenuSelection], gSubMenu_MODULATION_CN[gSubMenuSelection]));
            break;

        case MENU_AUTOLK:
            if (gSubMenuSelection == 0)
                strcpy(String, SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]));
            else
            {
                sprintf(String, "%02dm:%02ds", ((gSubMenuSelection * 15) / 60), ((gSubMenuSelection * 15) % 60));
                //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
                //ST7565_Gauge(4, 1, 40, gSubMenuSelection);
                gaugeLine = 4;
                gaugeMin = 1;
                gaugeMax = 40;
                //#endif
            }
            break;

        case MENU_COMPAND:
        case MENU_ABR_ON_TX_RX:
            strcpy(String, SUBV(gSubMenu_RX_TX[gSubMenuSelection], gSubMenu_RX_TX_CN[gSubMenuSelection]));
            break;

        #ifndef ENABLE_FEAT_F4HWN
            #ifdef ENABLE_AM_FIX
                case MENU_AM_FIX:
            #endif
        #endif
        case MENU_BCL:
        case MENU_BEEP:
        case MENU_STE:
        case MENU_D_ST:
#ifdef ENABLE_DTMF_CALLING
        case MENU_D_DCD:
#endif
        case MENU_D_LIVE_DEC:
        #ifdef ENABLE_NOAA
            case MENU_NOAA_S:
        #endif
#ifndef ENABLE_FEAT_F4HWN
        case MENU_350TX:
        case MENU_200TX:
        case MENU_500TX:
#endif
        case MENU_350EN:
#ifndef ENABLE_FEAT_F4HWN
        case MENU_SCREN:
#endif
#ifdef ENABLE_FEAT_F4HWN
        case MENU_SET_TMR:
        case MENU_S_PRI:
#endif
            strcpy(String, SUBV(gSubMenu_OFF_ON[gSubMenuSelection], gSubMenu_OFF_ON_CN[gSubMenuSelection]));
            break;

        case MENU_MEM_CH:
        case MENU_1_CALL:
        case MENU_DEL_CH:
        case MENU_S_PRI_CH_1:
        case MENU_S_PRI_CH_2:
        {
            if(gSubMenuSelection == MR_CHANNELS_MAX)
            {
                {
#ifdef ENABLE_CHINESE
                    const uint8_t yn = CH_after(CH_mem_blk_y0(), CH_SM_H);
                    UI_MENU_PrintMemChPaneMaybeCjk(
                        (gUiLanguage == UI_LANGUAGE_CN) ? gSubMenu_MEM_NONE_CN : "None",
                        menu_value_x1,
                        menu_item_x2,
                        yn);
#else
                    UI_PrintStringSmallNormal("None", menu_value_x1, menu_item_x2, MENU_VALUE_ROW(1u));
#endif
                }
                already_printed = true;
                break;
            }
            else
            {
                const bool valid = RADIO_CheckValidChannel(gSubMenuSelection, false, 0);

#ifdef ENABLE_CHINESE
                {
                const uint8_t y1 = CH_mem_blk_y0();
                const uint8_t y2 = CH_after(y1, CH_SM_H);
                const uint8_t y3 = CH_after(y2, CH_SM_H);

                UI_GenerateChannelStringEx(String, valid, gSubMenuSelection);
                UI_PrintStringSmallAtPixel(String, menu_value_x1, menu_item_x2, y1, (uint8_t)(y1 + 7u), 0u);

                if (valid && !gAskForConfirmation)
                {
                    const uint32_t frequency = SETTINGS_FetchChannelFrequency(gSubMenuSelection);
                    sprintf(String, "%u.%05u", frequency / 100000, frequency % 100000);
                    UI_PrintStringSmallAtPixel(String, menu_value_x1, menu_item_x2, y2, (uint8_t)(y2 + 7u), 0u);
                }

                SETTINGS_FetchChannelName(String, gSubMenuSelection);
                {
                    const char *nameDisp = String[0] ? String : "--";
                    if (valid && !gAskForConfirmation)
                        UI_MENU_PrintMemChPaneMaybeCjk(nameDisp, menu_value_x1, menu_item_x2, y3);
                    else
                    {
                        const uint8_t yn = CH_after(y1, CH_SM_H);
                        UI_MENU_PrintMemChPaneMaybeCjk(nameDisp, menu_value_x1, menu_item_x2, yn);
                    }
                }
                }
#else
                UI_GenerateChannelStringEx(String, valid, gSubMenuSelection);
                UI_PrintStringSmallNormal(String, menu_value_x1, menu_item_x2, MENU_VALUE_ROW(0));

                if (valid && !gAskForConfirmation)
                {
                    const uint32_t frequency = SETTINGS_FetchChannelFrequency(gSubMenuSelection);
                    sprintf(String, "%u.%05u", frequency / 100000, frequency % 100000);
                    UI_PrintStringSmallNormal(String, menu_value_x1, menu_item_x2, MENU_VALUE_ROW(1));
                }

                SETTINGS_FetchChannelName(String, gSubMenuSelection);
                {
                const uint8_t name_base = (valid && !gAskForConfirmation) ? 2u : 1u;
                UI_PrintStringSmallNormal(String[0] ? String : "--", menu_value_x1, menu_item_x2, MENU_VALUE_ROW(name_base));
                }
#endif
                already_printed = true;
                break;
            }
        }

        case MENU_MEM_NAME:
        {
            const bool valid = RADIO_CheckValidChannel(gSubMenuSelection, false, 0);
#ifdef ENABLE_CHINESE
            if (gIsInSubMenu && edit_index >= 0 && gUiLanguage == UI_LANGUAGE_CN)
            {
                unsigned int sub_val_x1 = menu_value_x1;
                unsigned int sub_val_x2 = menu_item_x2;
                if (!icon_layout && gIsInSubMenu)
                    sub_val_x1 += 2u;
                UI_MENU_DrawMemNamePinyinEdit(sub_val_x1, sub_val_x2);
                already_printed = true;
                break;
            }
#endif
            if (gIsInSubMenu && edit_index >= 0)
            {
                unsigned int sub_val_x1 = menu_value_x1;
                unsigned int sub_val_x2 = menu_item_x2;
                UI_PrintStringSmallAtPixel(UI_MENU_MemNameModeLabel(), (uint8_t)(sub_val_x2 - 6u), (uint8_t)sub_val_x2, 20u, 27u, 0u);
                UI_PrintStringSmallAtPixel(edit, (uint8_t)sub_val_x1, (uint8_t)sub_val_x2, 28u, 35u, 0u);
                if (edit_index < (int)CHANNEL_NAME_MAX_BYTES)
                {
                    uint8_t char_width = 6;
                    uint8_t char_spacing = char_width + 1;
                    size_t edit_length = strlen(edit);
                    uint8_t text_start = (uint8_t)sub_val_x1;
                    if (sub_val_x2 > sub_val_x1 && edit_length > 0)
                        text_start += (uint8_t)((((sub_val_x2 - sub_val_x1) - edit_length * char_spacing) + 1u) / 2u);
                    {
                        const uint8_t underline_x = (uint8_t)(text_start + (edit_index * char_spacing) + 1u);
                        const uint8_t underline_fb_row = (uint8_t)(35u / 8u);
                        if (underline_fb_row < FRAME_LINES)
                            for (uint8_t c = 0; c < char_width; c++)
                                gFrameBuffer[underline_fb_row][underline_x + c] |= 0x01u;
                    }
                }
                if (gMemNameInputMode == MEM_NAME_INPUT_SYMBOL)
                {
                    UI_MENU_DrawMemNameSymbolSixPack((uint8_t)sub_val_x1, (uint8_t)sub_val_x2);
                }
                else if (gMemNameCandidateCount > 0u)
                {
                    UI_MENU_DrawMemNameCandidates((uint8_t)sub_val_x1, (uint8_t)sub_val_x2);
                }
                else
                {
                    /* 10 字输完按 MENU 会进入确认?/请等待!（约 y52），与底部提示同一带区，故不再叠字 */
                    const bool mem_name_still_editing_cursor =
                        (edit_index < (int)CHANNEL_NAME_MAX_BYTES);
                    const bool mem_name_not_in_confirm_dialog =
                        (gAskForConfirmation == 0);
                    const bool should_show_hash_mode_hint =
                        mem_name_still_editing_cursor &&
                        mem_name_not_in_confirm_dialog;
                    if (should_show_hash_mode_hint)
                    {
                        UI_PrintStringSmallAtPixel(
                            (gUiLanguage == UI_LANGUAGE_CN) ? "按#切换输入模式" : "Press # to switch mode",
                            (uint8_t)sub_val_x1, (uint8_t)sub_val_x2, 50u, 57u, 0u);
                    }
                }
                already_printed = true;
                break;
            }

#ifdef ENABLE_CHINESE
            {
            const uint8_t y1 = CH_mem_blk_y0();
            const uint8_t y2 = CH_after(y1, CH_SM_H);

            UI_GenerateChannelStringEx(String, valid, gSubMenuSelection);
            UI_PrintStringSmallAtPixel(String, menu_value_x1, menu_item_x2, y1, (uint8_t)(y1 + 7u), 0u);

            if (valid)
            {
                const uint32_t frequency = SETTINGS_FetchChannelFrequency(gSubMenuSelection);

                if (!gIsInSubMenu)
                {
                    edit_index = -1;
                    SETTINGS_FetchChannelName(edit, gSubMenuSelection);
                }
                if (edit_index < 0)
                {
                    SETTINGS_FetchChannelName(String, gSubMenuSelection);
                    char *pPrintStr = String[0] ? String : "--";
                    UI_MENU_PrintMemChPaneMaybeCjk(pPrintStr, menu_value_x1, menu_item_x2, y2);

                    if (!gAskForConfirmation && !(gIsInSubMenu && edit_index >= 0))
                    {
                        sprintf(String, "%u.%05u", frequency / 100000, frequency % 100000);
                        const uint8_t yf = CH_after(y2, CH_CN_H);
                        UI_PrintStringSmallAtPixel(String, menu_value_x1, menu_item_x2, yf, (uint8_t)(yf + 7u), 0u);
                    }
                }
                else
                {
                    UI_MENU_PrintMemChPaneMaybeCjk(edit, menu_value_x1, menu_item_x2, y2);
                    if (edit_index < (int)CHANNEL_NAME_MAX_BYTES)
                    {
                        uint8_t char_width = 6;
                        uint8_t char_spacing = char_width + 1;
                        size_t edit_length = strlen(edit);
                        uint8_t text_start = menu_value_x1;
                        if (menu_item_x2 > menu_value_x1 && edit_length > 0) {
                            text_start += (((menu_item_x2 - menu_value_x1) - edit_length * char_spacing) + 1) / 2;
                        }
                        uint8_t underline_x = text_start + (edit_index * char_spacing) + 1;
                        const uint8_t underline_fb_row = (uint8_t)(((unsigned)y2 + 10u) / 8u);
                        if (underline_fb_row < FRAME_LINES)
                        {
                            for (uint8_t c = 0; c < char_width; c++) {
                                gFrameBuffer[underline_fb_row][underline_x + c] |= 0x01;
                            }
                        }
                    }
                    if (!gAskForConfirmation && !(gIsInSubMenu && edit_index >= 0))
                    {
                        sprintf(String, "%u.%05u", frequency / 100000, frequency % 100000);
                        const uint8_t yf = CH_after(y2, CH_CN_H);
                        UI_PrintStringSmallAtPixel(String, menu_value_x1, menu_item_x2, yf, (uint8_t)(yf + 7u), 0u);
                    }
                }
            }
            }
#else
            UI_GenerateChannelStringEx(String, valid, gSubMenuSelection);
            UI_PrintStringSmallNormal(String, menu_value_x1, menu_item_x2, MENU_VALUE_ROW(0));

            if (valid)
            {
                const uint32_t frequency = SETTINGS_FetchChannelFrequency(gSubMenuSelection);

                if (!gIsInSubMenu)
                {
                    edit_index = -1;
                    SETTINGS_FetchChannelName(edit, gSubMenuSelection);
                }
                if (edit_index < 0)
                {
                    SETTINGS_FetchChannelName(String, gSubMenuSelection);
                    char *pPrintStr = String[0] ? String : "--";
                    UI_PrintStringSmallNormal(pPrintStr, menu_value_x1, menu_item_x2, MENU_VALUE_ROW(1));
                }
                else
                {
                    UI_PrintStringSmallNormal(edit, menu_value_x1, menu_item_x2, MENU_VALUE_ROW(1));
                    if (edit_index < (int)CHANNEL_NAME_MAX_BYTES)
                    {
                        uint8_t char_width = 6;
                        uint8_t char_spacing = char_width + 1;
                        size_t edit_length = strlen(edit);
                        uint8_t text_start = menu_value_x1;
                        if (menu_item_x2 > menu_value_x1 && edit_length > 0) {
                            text_start += (((menu_item_x2 - menu_value_x1) - edit_length * char_spacing) + 1) / 2;
                        }
                        uint8_t underline_x = text_start + (edit_index * char_spacing) + 1;
                        const uint8_t underline_fb_row = (uint8_t)(MENU_VALUE_ROW(1) + 1u);
                        for (uint8_t c = 0; c < char_width; c++) {
                            gFrameBuffer[underline_fb_row][underline_x + c] |= 0x01;
                        }
                    }
                }

                if (!gAskForConfirmation && !(gIsInSubMenu && edit_index >= 0))
                {
                    sprintf(String, "%u.%05u", frequency / 100000, frequency % 100000);
                    UI_PrintStringSmallNormal(String, menu_value_x1, menu_item_x2, MENU_VALUE_ROW(2));
                }
            }
#endif

            already_printed = true;
            break;
        }


        case MENU_SAVE:
            sprintf(String, gSubMenuSelection == 0 ? SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]) : "1:%u", gSubMenuSelection);
            break;

        case MENU_TDR:
            strcpy(String, SUBV(gSubMenu_RXMode[gSubMenuSelection], gSubMenu_RXMode_CN[gSubMenuSelection]));
            break;

        case MENU_TOT:
            sprintf(String, "%02dm:%02ds", (((gSubMenuSelection + 1) * 5) / 60), (((gSubMenuSelection + 1) * 5) % 60));
            //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
            //ST7565_Gauge(4, 5, 179, gSubMenuSelection);
            gaugeLine = 4;
            gaugeMin = 5;
            gaugeMax = 179;
            //#endif
            break;

        #ifdef ENABLE_VOICE
            case MENU_VOICE:
                strcpy(String, SUBV(gSubMenu_VOICE[gSubMenuSelection], gSubMenu_VOICE_CN[gSubMenuSelection]));
                break;
        #endif

        case MENU_SC_REV:
            if(gSubMenuSelection == 0)
            {
                strcpy(String, SUBV("STOP", gSubMenu_SC_REV_STOP_CN));
            }
            else if(gSubMenuSelection < 81)
            {
                sprintf(String, "CARRIER\n%02ds:%03dms", ((gSubMenuSelection * 250) / 1000), ((gSubMenuSelection * 250) % 1000));
                /*
                 * 条所在 gFrameBuffer 行须 <7：F4HWN 全屏刷新只输出 [0]..[6] 到硬件行 1..7，
                 * 子菜单 gauge 行 = gaugeLine+2，若仍用 5 则落到 [7] 屏外。
                 */
                gaugeLine = 4;
                gaugeMin = 1;
                gaugeMax = 80;
            }
            else
            {
                sprintf(String, "TIMEOUT\n%02dm:%02ds", (((gSubMenuSelection - 80) * 5) / 60), (((gSubMenuSelection - 80) * 5) % 60));
                gaugeLine = 4;
                gaugeMin = 80;
                gaugeMax = 104;
            }
            break;

        case MENU_MDF:
            strcpy(String, SUBV(gSubMenu_MDF[gSubMenuSelection], gSubMenu_MDF_CN[gSubMenuSelection]));
            break;

        case MENU_RP_STE:
            sprintf(String, gSubMenuSelection == 0 ? SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]) : "%u*100ms", gSubMenuSelection);
            break;

        case MENU_LIST_CH:
        case MENU_S_LIST:
            if (gSubMenuSelection == MR_CHANNELS_LIST + 1)
                strcpy(String, SUBV("ALL", gSubMenu_LIST_CN_ALL));
            else if (gSubMenuSelection == 0 && UI_MENU_GetCurrentMenuId() == MENU_LIST_CH)
                strcpy(String, SUBV("OFF", gSubMenu_LIST_CN_OFF));
            else {
                const char *name = gListName[gSubMenuSelection - 1];
                
                // If first character is empty/invalid, display "N/A"
                if (IsEmptyName(name, sizeof(gListName[0])))
                    sprintf(String, "%02u", gSubMenuSelection);
                else
                    sprintf(String, "%02u (%.3s)", gSubMenuSelection, name);
            }
            break;
            
        #ifdef ENABLE_ALARM
            case MENU_AL_MOD:
                strcpy(String, SUBV(gSubMenu_AL_MOD[gSubMenuSelection], gSubMenu_AL_MOD_CN[gSubMenuSelection]));
                break;
        #endif

#ifdef ENABLE_DTMF_CALLING
        case MENU_ANI_ID:
            strcpy(String, gEeprom.ANI_DTMF_ID);
            break;
#endif
        case MENU_UPCODE:
            sprintf(String, "%.8s\n%.8s", gEeprom.DTMF_UP_CODE, gEeprom.DTMF_UP_CODE + 8);
            break;

        case MENU_DWCODE:
            sprintf(String, "%.8s\n%.8s", gEeprom.DTMF_DOWN_CODE, gEeprom.DTMF_DOWN_CODE + 8);
            break;

#ifdef ENABLE_DTMF_CALLING
        case MENU_D_RSP:
            strcpy(String, SUBV(gSubMenu_D_RSP[gSubMenuSelection], gSubMenu_D_RSP_CN[gSubMenuSelection]));
            break;

        case MENU_D_HOLD:
            sprintf(String, "%ds", gSubMenuSelection);
            break;
#endif
        case MENU_D_PRE:
            sprintf(String, "%d*10ms", gSubMenuSelection);
            break;

        case MENU_PTT_ID:
            strcpy(String, SUBV(gSubMenu_PTT_ID[gSubMenuSelection], gSubMenu_PTT_ID_CN[gSubMenuSelection]));
            break;

        case MENU_BAT_TXT:
            strcpy(String, SUBV(gSubMenu_BAT_TXT[gSubMenuSelection], gSubMenu_BAT_TXT_CN[gSubMenuSelection]));
            break;

        case MENU_LANGUAGE:
            strcpy(String, gSubMenu_LANGUAGE[gSubMenuSelection]);
            break;

#ifdef ENABLE_DTMF_CALLING
        case MENU_D_LIST:
            gIsDtmfContactValid = DTMF_GetContact((int)gSubMenuSelection - 1, Contact);
            if (!gIsDtmfContactValid)
                strcpy(String, "NULL");
            else
                memcpy(String, Contact, 8);
            break;
#endif

        case MENU_PONMSG:
            strcpy(String, SUBV(gSubMenu_PONMSG[gSubMenuSelection], gSubMenu_PONMSG_CN[gSubMenuSelection]));
            break;

        case MENU_BOOT_HINT:
            if (gSubMenuSelection == 1 && !gIsInSubMenu)
            {
#ifdef ENABLE_CHINESE
                if (gUiLanguage == UI_LANGUAGE_CN)
                {
                    strcpy(String, "\xe9\xad\x85\xe5\x8a\x9b\n\xe5\x8c\x97\xe4\xba\xac");
                }
                else
#endif
                {
                    strcpy(String, "Beautiful\nBJ");
                }
            }
            else if (gSubMenuSelection == 2 && !gIsInSubMenu)
            {
#ifdef ENABLE_CHINESE
                if (gUiLanguage == UI_LANGUAGE_CN)
                {
                    strcpy(String, "\xe4\xba\x94\xe4\xba\x94\xe8\x8a\x82\n\xe7\xba\xaa\xe5\xbf\xb5\xe7\x89\x88");
                }
                else
#endif
                {
                    strcpy(String, "happy\n55th");
                }
            }
            else
            {
                strcpy(String, SUBV(gSubMenu_BOOT_HINT[gSubMenuSelection], gSubMenu_BOOT_HINT_CN[gSubMenuSelection]));
            }
            break;

        case MENU_ROGER:
            strcpy(String, SUBV(gSubMenu_ROGER[gSubMenuSelection], gSubMenu_ROGER_CN[gSubMenuSelection]));
            break;

        case MENU_VOL:
#ifdef ENABLE_FEAT_F4HWN
        {
            uint8_t page = 0u;
            uint8_t level2_value_down_offset_pixels = 0u;
            uint8_t level3_value_down_offset_pixels = 0u;
            if (gIsInSubMenu)
            {
                page = (uint8_t)gSubMenuSelection;
                level3_value_down_offset_pixels = 9u;
            }
            else
            {
                level2_value_down_offset_pixels = 12u;
            }

            if (page == 0u)
            {
                const uint8_t info_line_up_offset_pixels = 5u;
                const uint8_t info_line_gap_pixels = 3u;
                const uint8_t info_line_height_pixels = 8u;
                uint8_t info_line1_y_start = 0u;
                uint8_t info_line1_y_end = 0u;
                uint8_t info_line2_y_start = 0u;
                uint8_t info_line2_y_end = 0u;
                uint8_t info_line3_y_start = 0u;
                uint8_t info_line3_y_end = 0u;

                if (gIsInSubMenu)
                {
                    edit[0] = '\0';
                }
                else
                {
                    strcpy(edit, "<\xe5\x8f\xae\xe5\x92\x9a\xe9\xb8\xa1>");
                }
                info_line1_y_start = UI_MENU_GetRowPixelStart(MENU_VALUE_ROW(1));
                if (info_line1_y_start >= info_line_up_offset_pixels)
                {
                    info_line1_y_start = (uint8_t)(info_line1_y_start - info_line_up_offset_pixels);
                }
                else
                {
                    info_line1_y_start = 0u;
                }
                info_line1_y_end = (uint8_t)(info_line1_y_start + info_line_height_pixels - 1u);
                info_line1_y_start = (uint8_t)(info_line1_y_start + level2_value_down_offset_pixels);
                info_line1_y_end = (uint8_t)(info_line1_y_end + level2_value_down_offset_pixels);
                info_line1_y_start = (uint8_t)(info_line1_y_start + level3_value_down_offset_pixels);
                info_line1_y_end = (uint8_t)(info_line1_y_end + level3_value_down_offset_pixels);

                info_line2_y_start = (uint8_t)(info_line1_y_end + 1u + info_line_gap_pixels);
                info_line2_y_end = (uint8_t)(info_line2_y_start + info_line_height_pixels - 1u);

                info_line3_y_start = (uint8_t)(info_line2_y_end + 1u + info_line_gap_pixels);
                info_line3_y_end = (uint8_t)(info_line3_y_start + info_line_height_pixels - 1u);

                UI_PrintStringSmallAtPixel("Dondji", menu_value_x1, menu_item_x2, info_line1_y_start, info_line1_y_end, 0u);
                UI_PrintStringSmallAtPixel(VERSION_STRING_2, menu_value_x1, menu_item_x2, info_line2_y_start, info_line2_y_end, 0u);
                UI_PrintStringSmallAtPixel("BD1AHN", menu_value_x1, menu_item_x2, info_line3_y_start, info_line3_y_end, 0u);
                String[0] = '\0';
                already_printed = true;
                break;
            }

            if (page == 1u)
            {
                const uint8_t info_label_up_offset_pixels = 5u;
                const uint8_t info_label_safe_top_pixels_in_submenu = 20u;
                const uint8_t info_line_gap_pixels = 3u;
                const uint8_t info_line_height_pixels = 8u;
                const uint8_t info_label_cn_extra_up_offset_pixels = 3u;
                uint8_t info_label_y_start = 0u;
                uint8_t info_label_y_end = 0u;
                uint8_t info_line2_y_start = 0u;
                uint8_t info_line2_y_end = 0u;
                uint8_t info_line3_y_start = 0u;
                uint8_t info_line3_y_end = 0u;
                uint8_t info_label_print_y_start = 0u;
                uint8_t info_label_print_y_end = 0u;
                char beijing_build_time[16];
                char build_date_slash[16];

                UI_MENU_FormatBuildTimeBeijing(beijing_build_time, sizeof(beijing_build_time));
                UI_MENU_FormatBuildDateSlash(build_date_slash, sizeof(build_date_slash));

                info_label_y_start = UI_MENU_GetRowPixelStart(MENU_VALUE_ROW(1));
                if (info_label_y_start >= info_label_up_offset_pixels)
                {
                    info_label_y_start = (uint8_t)(info_label_y_start - info_label_up_offset_pixels);
                }
                else
                {
                    info_label_y_start = 0u;
                }
                info_label_y_end = (uint8_t)(info_label_y_start + 7u);
                info_label_y_start = (uint8_t)(info_label_y_start + level2_value_down_offset_pixels);
                info_label_y_end = (uint8_t)(info_label_y_end + level2_value_down_offset_pixels);
                info_label_y_start = (uint8_t)(info_label_y_start + level3_value_down_offset_pixels);
                info_label_y_end = (uint8_t)(info_label_y_end + level3_value_down_offset_pixels);
                if (gIsInSubMenu && info_label_y_start < info_label_safe_top_pixels_in_submenu)
                {
                    info_label_y_start = info_label_safe_top_pixels_in_submenu;
                    info_label_y_end = (uint8_t)(info_label_y_start + info_line_height_pixels - 1u);
                }
                info_line2_y_start = (uint8_t)(info_label_y_end + 1u + info_line_gap_pixels);
                info_line2_y_end = (uint8_t)(info_line2_y_start + info_line_height_pixels - 1u);
                info_line3_y_start = (uint8_t)(info_line2_y_end + 1u + info_line_gap_pixels);
                info_line3_y_end = (uint8_t)(info_line3_y_start + info_line_height_pixels - 1u);
                info_label_print_y_start = info_label_y_start;
                info_label_print_y_end = info_label_y_end;

                if (gUiLanguage == UI_LANGUAGE_CN)
                {
                    if (info_label_print_y_start >= info_label_cn_extra_up_offset_pixels)
                    {
                        info_label_print_y_start = (uint8_t)(info_label_print_y_start - info_label_cn_extra_up_offset_pixels);
                    }
                    else
                    {
                        info_label_print_y_start = 0u;
                    }
                    info_label_print_y_end = (uint8_t)(info_label_print_y_start + info_line_height_pixels - 1u);
                }

                UI_PrintStringSmallAtPixel(SUBV("BUILD", "打包时间"), menu_value_x1, menu_item_x2, info_label_print_y_start, info_label_print_y_end, 0u);
                UI_PrintStringSmallAtPixel(build_date_slash, menu_value_x1, menu_item_x2, info_line2_y_start, info_line2_y_end, 0u);
                UI_PrintStringSmallAtPixel(beijing_build_time, menu_value_x1, menu_item_x2, info_line3_y_start, info_line3_y_end, 0u);
                if (level2_value_down_offset_pixels > 0u)
                {
                    uint8_t build_commit_y_start = UI_MENU_GetRowPixelStart(MENU_VALUE_ROW(5));
                    uint8_t build_commit_y_end = (uint8_t)(build_commit_y_start + 7u);
                    build_commit_y_start = (uint8_t)(build_commit_y_start + level2_value_down_offset_pixels);
                    build_commit_y_end = (uint8_t)(build_commit_y_end + level2_value_down_offset_pixels);
                    UI_PrintStringSmallAtPixel(BuildCommit, menu_value_x1, menu_item_x2, build_commit_y_start, build_commit_y_end, 0u);
                }
                else if (level3_value_down_offset_pixels > 0u)
                {
                    uint8_t build_commit_y_start = UI_MENU_GetRowPixelStart(MENU_VALUE_ROW(5));
                    uint8_t build_commit_y_end = (uint8_t)(build_commit_y_start + 7u);
                    build_commit_y_start = (uint8_t)(build_commit_y_start + level3_value_down_offset_pixels);
                    build_commit_y_end = (uint8_t)(build_commit_y_end + level3_value_down_offset_pixels);
                    UI_PrintStringSmallAtPixel(BuildCommit, menu_value_x1, menu_item_x2, build_commit_y_start, build_commit_y_end, 0u);
                }
                else
                {
                    UI_PrintStringSmallNormal(BuildCommit, menu_value_x1, menu_item_x2, MENU_VALUE_ROW(5));
                }
                already_printed = true;
                break;
            }

            if (page == 2u)
            {
                const uint8_t info_label_up_offset_pixels = 5u;
                const uint8_t info_label_safe_top_pixels_in_submenu = 20u;
                const uint8_t info_line_gap_pixels = 3u;
                const uint8_t info_line_height_pixels = 8u;
                const uint8_t info_label_cn_extra_up_offset_pixels = 3u;
                uint8_t info_label_y_start = 0u;
                uint8_t info_label_y_end = 0u;
                uint8_t info_line2_y_start = 0u;
                uint8_t info_line2_y_end = 0u;
                uint8_t info_line3_y_start = 0u;
                uint8_t info_line3_y_end = 0u;
                uint8_t info_label_print_y_start = 0u;
                uint8_t info_label_print_y_end = 0u;
                uint16_t flash_percent_x10 = 0u;
                uint16_t sram_percent_x10 = 0u;

                UI_MENU_GetMemPercents(&flash_percent_x10, &sram_percent_x10);

                info_label_y_start = UI_MENU_GetRowPixelStart(MENU_VALUE_ROW(1));
                if (info_label_y_start >= info_label_up_offset_pixels)
                {
                    info_label_y_start = (uint8_t)(info_label_y_start - info_label_up_offset_pixels);
                }
                else
                {
                    info_label_y_start = 0u;
                }
                info_label_y_end = (uint8_t)(info_label_y_start + 7u);
                info_label_y_start = (uint8_t)(info_label_y_start + level2_value_down_offset_pixels);
                info_label_y_end = (uint8_t)(info_label_y_end + level2_value_down_offset_pixels);
                info_label_y_start = (uint8_t)(info_label_y_start + level3_value_down_offset_pixels);
                info_label_y_end = (uint8_t)(info_label_y_end + level3_value_down_offset_pixels);
                if (gIsInSubMenu && info_label_y_start < info_label_safe_top_pixels_in_submenu)
                {
                    info_label_y_start = info_label_safe_top_pixels_in_submenu;
                    info_label_y_end = (uint8_t)(info_label_y_start + info_line_height_pixels - 1u);
                }
                info_line2_y_start = (uint8_t)(info_label_y_end + 1u + info_line_gap_pixels);
                info_line2_y_end = (uint8_t)(info_line2_y_start + info_line_height_pixels - 1u);
                info_line3_y_start = (uint8_t)(info_line2_y_end + 1u + info_line_gap_pixels);
                info_line3_y_end = (uint8_t)(info_line3_y_start + info_line_height_pixels - 1u);
                info_label_print_y_start = info_label_y_start;
                info_label_print_y_end = info_label_y_end;

                if (gUiLanguage == UI_LANGUAGE_CN)
                {
                    if (info_label_print_y_start >= info_label_cn_extra_up_offset_pixels)
                    {
                        info_label_print_y_start = (uint8_t)(info_label_print_y_start - info_label_cn_extra_up_offset_pixels);
                    }
                    else
                    {
                        info_label_print_y_start = 0u;
                    }
                    info_label_print_y_end = (uint8_t)(info_label_print_y_start + info_line_height_pixels - 1u);
                }

                UI_PrintStringSmallAtPixel(SUBV("MEMORY", "存储占比"), menu_value_x1, menu_item_x2, info_label_print_y_start, info_label_print_y_end, 0u);
                sprintf(String, "FLASH %u.%u%%", flash_percent_x10 / 10u, flash_percent_x10 % 10u);
                UI_PrintStringSmallAtPixel(String, menu_value_x1, menu_item_x2, info_line2_y_start, info_line2_y_end, 0u);
                sprintf(String, "SRAM  %u.%u%%", sram_percent_x10 / 10u, sram_percent_x10 % 10u);
                UI_PrintStringSmallAtPixel(String, menu_value_x1, menu_item_x2, info_line3_y_start, info_line3_y_end, 0u);
                already_printed = true;
                break;
            }

            if (page == 3u)
            {
                uint8_t page4_line1_y0 = 0u;
                uint8_t page4_line1_y1 = 0u;
                uint8_t page4_line2_y0 = 0u;
                uint8_t page4_line2_y1 = 0u;
                uint8_t page4_line3_y0 = 0u;
                uint8_t page4_line3_y1 = 0u;
                uint32_t font_total_bytes = 0u;
                uint32_t kb_x10 = 0u;
                unsigned int kb_int_part = 0u;
                unsigned int kb_frac_part = 0u;
                uint8_t ver_byte = 0u;
                uint16_t probe_words[2];
                bool font_spi_ok = false;

                font_total_bytes = (uint32_t)CN_FONT_VERSION_OFFSET + 1u;
                kb_x10 = (font_total_bytes * 10u) / 1024u;
                kb_int_part = (unsigned int)(kb_x10 / 10u);
                kb_frac_part = (unsigned int)(kb_x10 % 10u);

                PY25Q16_ReadBuffer(CN_FONT_FLASH_BASE + CN_FONT_VERSION_OFFSET, &ver_byte, 1);
                PY25Q16_ReadBuffer(CN_FONT_FLASH_BASE, (uint8_t *)probe_words, 4);
                if (ver_byte == CN_FONT_VERSION && probe_words[0] == 0x1100 && probe_words[1] == 0x2100)
                {
                    font_spi_ok = true;
                }
                else
                {
                    font_spi_ok = false;
                }

                /* 第四页：第一行菜单名带之下留 2px，再排三等分槽位（须与左栏/顶栏标题带一致） */
                {
                    uint8_t page4_band_y_top = 0u;
                    uint8_t page4_band_y_bot = 0u;
                    uint8_t page4_menu_title_bottom_y = 0u;
                    const unsigned int page4_gap_below_menu_title_px = 2u;
                    unsigned int page4_band_avail_h = 0u;
                    unsigned int page4_slot_h0 = 0u;
                    unsigned int page4_slot_h1 = 0u;
                    unsigned int page4_slot_h2 = 0u;
                    unsigned int page4_slots_total_h = 0u;
                    unsigned int page4_rem_h = 0u;
                    unsigned int page4_y_cursor = 0u;

                    if (gIsInSubMenu)
                    {
                        /* UI_MENU_DrawCccMenuChrome：标题 PrintStringSmallAtPixel(..., 11, 18, ...) */
                        page4_menu_title_bottom_y = 18u;
                        page4_band_y_bot = 63u;
                    }
                    else
                    {
                        /* UI_MENU_DrawLevel2SplitLayout MENU_VOL：左栏「系统信息」..., 10u, 36u */
                        page4_menu_title_bottom_y = 36u;
                        page4_band_y_bot = 55u;
                    }
                    page4_band_y_top = (uint8_t)((unsigned int)page4_menu_title_bottom_y + 1u
                                                + page4_gap_below_menu_title_px);
                    page4_band_avail_h =
                        (unsigned int)page4_band_y_bot - (unsigned int)page4_band_y_top + 1u;
                    /* 用于三等分的有效高度少 1px，三行整体收紧，行间距缩小 */
                    {
                        const unsigned int page4_row_spacing_shrink_px = 1u;

                        page4_slots_total_h = page4_band_avail_h;
                        if (page4_slots_total_h > page4_row_spacing_shrink_px)
                        {
                            page4_slots_total_h =
                                page4_slots_total_h - page4_row_spacing_shrink_px;
                        }
                    }
                    page4_slot_h0 = page4_slots_total_h / 3u;
                    page4_slot_h1 = page4_slots_total_h / 3u;
                    page4_slot_h2 = page4_slots_total_h / 3u;
                    page4_rem_h =
                        page4_slots_total_h - page4_slot_h0 - page4_slot_h1 - page4_slot_h2;
                    if (page4_rem_h > 0u)
                    {
                        page4_slot_h0 = page4_slot_h0 + 1u;
                        page4_rem_h = page4_rem_h - 1u;
                    }
                    if (page4_rem_h > 0u)
                    {
                        page4_slot_h1 = page4_slot_h1 + 1u;
                        page4_rem_h = page4_rem_h - 1u;
                    }
                    if (page4_rem_h > 0u)
                    {
                        page4_slot_h2 = page4_slot_h2 + 1u;
                    }
                    page4_y_cursor = (unsigned int)page4_band_y_top;
                    page4_line1_y0 = (uint8_t)page4_y_cursor;
                    page4_y_cursor = page4_y_cursor + page4_slot_h0;
                    page4_line1_y1 = (uint8_t)(page4_y_cursor - 1u);
                    page4_line2_y0 = (uint8_t)page4_y_cursor;
                    page4_y_cursor = page4_y_cursor + page4_slot_h1;
                    page4_line2_y1 = (uint8_t)(page4_y_cursor - 1u);
                    page4_line3_y0 = (uint8_t)page4_y_cursor;
                    page4_y_cursor = page4_y_cursor + page4_slot_h2;
                    page4_line3_y1 = (uint8_t)(page4_y_cursor - 1u);

                    /* 第一行整体下移 2px，第二行整体下移 1px（相对第三行） */
                    {
                        const uint8_t page4_line1_down_px = 2u;
                        const uint8_t page4_line2_down_px = 1u;

                        page4_line1_y0 =
                            (uint8_t)((unsigned int)page4_line1_y0 + (unsigned int)page4_line1_down_px);
                        page4_line1_y1 =
                            (uint8_t)((unsigned int)page4_line1_y1 + (unsigned int)page4_line1_down_px);
                        page4_line2_y0 =
                            (uint8_t)((unsigned int)page4_line2_y0 + (unsigned int)page4_line2_down_px);
                        page4_line2_y1 =
                            (uint8_t)((unsigned int)page4_line2_y1 + (unsigned int)page4_line2_down_px);
                    }
                }

                {
                    const char *page4_label_line1 = SUBV("Font", "\xe5\xad\x97\xe5\xba\x93");
                    const char *page4_label_line2 = SUBV("Glyphs", "\xe5\xad\x97\xe6\x95\xb0");
                    const char *page4_label_line3 = SUBV("SPI", "\xe6\xa0\xa1\xe9\xaa\x8c");
                    char page4_value_line1[20];
                    char page4_value_line2[12];
                    char page4_value_line3[12];
                    size_t page4_label_w1_px = 0u;
                    size_t page4_label_w2_px = 0u;
                    size_t page4_label_w3_px = 0u;
                    size_t page4_value_w1_px = 0u;
                    size_t page4_value_w2_px = 0u;
                    size_t page4_value_w3_px = 0u;
                    size_t page4_max_label_w_px = 0u;
                    size_t page4_max_value_w_px = 0u;
                    unsigned int page4_block_w_px = 0u;
                    unsigned int page4_pane_w_px = 0u;
                    unsigned int page4_block_start_x_px = 0u;
                    unsigned int page4_value_col_left_x_px = 0u;
                    const unsigned int page4_label_value_gap_px = 8u;
                    uint8_t page4_draw_x = 0u;

#ifdef ENABLE_CHINESE
                    if (gUiLanguage == UI_LANGUAGE_CN)
                    {
                        sprintf(page4_value_line1, "%u.%uK", kb_int_part, kb_frac_part);
                    }
                    else
#endif
                    {
                        sprintf(page4_value_line1, "%u.%uKB", kb_int_part, kb_frac_part);
                    }
                    sprintf(page4_value_line2, "%u", (unsigned int)CN_FONT_CHAR_COUNT);
#ifdef ENABLE_CHINESE
                    if (gUiLanguage == UI_LANGUAGE_CN)
                    {
                        if (font_spi_ok)
                        {
                            strcpy(page4_value_line3, "\xe9\x80\x9a\xe8\xbf\x87");
                        }
                        else
                        {
                            strcpy(page4_value_line3, "\xe5\xbc\x82\xe5\xb8\xb8");
                        }
                    }
                    else
#endif
                    {
                        if (font_spi_ok)
                        {
                            strcpy(page4_value_line3, "OK");
                        }
                        else
                        {
                            strcpy(page4_value_line3, "Bad");
                        }
                    }

                    page4_label_w1_px = UI_SmallStringPixelWidth(page4_label_line1);
                    page4_label_w2_px = UI_SmallStringPixelWidth(page4_label_line2);
                    page4_label_w3_px = UI_SmallStringPixelWidth(page4_label_line3);
                    page4_value_w1_px = UI_SmallStringPixelWidth(page4_value_line1);
                    page4_value_w2_px = UI_SmallStringPixelWidth(page4_value_line2);
                    page4_value_w3_px = UI_SmallStringPixelWidth(page4_value_line3);

                    page4_max_label_w_px = page4_label_w1_px;
                    if (page4_label_w2_px > page4_max_label_w_px)
                    {
                        page4_max_label_w_px = page4_label_w2_px;
                    }
                    if (page4_label_w3_px > page4_max_label_w_px)
                    {
                        page4_max_label_w_px = page4_label_w3_px;
                    }
                    page4_max_value_w_px = page4_value_w1_px;
                    if (page4_value_w2_px > page4_max_value_w_px)
                    {
                        page4_max_value_w_px = page4_value_w2_px;
                    }
                    if (page4_value_w3_px > page4_max_value_w_px)
                    {
                        page4_max_value_w_px = page4_value_w3_px;
                    }

                    page4_block_w_px = (unsigned int)page4_max_label_w_px + page4_label_value_gap_px
                                       + (unsigned int)page4_max_value_w_px;
                    page4_pane_w_px = (unsigned int)menu_item_x2 - (unsigned int)menu_value_x1 + 1u;
                    page4_block_start_x_px = (unsigned int)menu_value_x1;
                    if (page4_block_w_px <= page4_pane_w_px)
                    {
                        page4_block_start_x_px =
                            (unsigned int)menu_value_x1 + (page4_pane_w_px - page4_block_w_px) / 2u;
                    }
                    page4_value_col_left_x_px =
                        page4_block_start_x_px + (unsigned int)page4_max_label_w_px + page4_label_value_gap_px;

                    page4_draw_x = (uint8_t)page4_block_start_x_px;
                    /* latin_down_when_mixed=0：标签与数值分两趟绘制，数值串纯英文/数字时在本行槽内垂直居中 */
                    UI_PrintStringSmallAtPixel(page4_label_line1, page4_draw_x, page4_draw_x,
                                               page4_line1_y0, page4_line1_y1, 0u);
                    {
                        const unsigned int page4_line1_value_nudge_right_px = 5u;
                        unsigned int page4_value1_draw_x = 0u;

                        page4_value1_draw_x = page4_value_col_left_x_px
                                              + (unsigned int)page4_max_value_w_px
                                              - (unsigned int)page4_value_w1_px
                                              + page4_line1_value_nudge_right_px;
                        page4_draw_x = (uint8_t)page4_value1_draw_x;
                    }
                    UI_PrintStringSmallAtPixel(page4_value_line1, page4_draw_x, page4_draw_x,
                                               page4_line1_y0, page4_line1_y1, 0u);

                    page4_draw_x = (uint8_t)page4_block_start_x_px;
                    UI_PrintStringSmallAtPixel(page4_label_line2, page4_draw_x, page4_draw_x,
                                               page4_line2_y0, page4_line2_y1, 0u);
                    page4_draw_x = (uint8_t)(page4_value_col_left_x_px
                                             + (unsigned int)page4_max_value_w_px
                                             - (unsigned int)page4_value_w2_px);
                    UI_PrintStringSmallAtPixel(page4_value_line2, page4_draw_x, page4_draw_x,
                                               page4_line2_y0, page4_line2_y1, 0u);

                    page4_draw_x = (uint8_t)page4_block_start_x_px;
                    UI_PrintStringSmallAtPixel(page4_label_line3, page4_draw_x, page4_draw_x,
                                               page4_line3_y0, page4_line3_y1, 0u);
                    page4_draw_x = (uint8_t)(page4_value_col_left_x_px
                                             + (unsigned int)page4_max_value_w_px
                                             - (unsigned int)page4_value_w3_px);
                    UI_PrintStringSmallAtPixel(page4_value_line3, page4_draw_x, page4_draw_x,
                                               page4_line3_y0, page4_line3_y1, 0u);
                }
                already_printed = true;
                break;
            }
            if (page == 4u)
            {
                const char *url_title = SUBV("web:", "\xe5\x88\xb7\xe6\x9c\xba\xe5\x86\x99\xe9\xa2\x91\xe7\xbd\x91\xe5\x9d\x80\xef\xbc\x9a");
                const char *url_line1 = "https://ethanyan6";
                const char *url_line2 = ".github.io/Dondji/";
                uint8_t url_x = (uint8_t)menu_value_x1;

                uint8_t url_band_y_top = 0u;
                uint8_t url_band_y_bot = 0u;
                if (gIsInSubMenu)
                {
                    url_band_y_top = 19u;
                    url_band_y_bot = 63u;
                }
                else
                {
                    url_band_y_top = 37u;
                    url_band_y_bot = 55u;
                }
                const unsigned int url_line_h = 8u;
                const unsigned int url_line_gap = 2u;
                const unsigned int url_total_h = 3u * url_line_h + 2u * url_line_gap;
                const unsigned int url_avail_h = (unsigned int)url_band_y_bot - (unsigned int)url_band_y_top + 1u;
                unsigned int url_start_y = url_band_y_top;
                if (url_avail_h > url_total_h)
                {
                    url_start_y = (unsigned int)url_band_y_top + (url_avail_h - url_total_h) / 2u;
                }
                uint8_t url_y1 = (uint8_t)url_start_y;
                if (url_y1 >= 2u) url_y1 = (uint8_t)(url_y1 - 2u);
                uint8_t url_y2 = (uint8_t)(url_start_y + url_line_h + url_line_gap);
                uint8_t url_y3 = (uint8_t)(url_start_y + 2u * (url_line_h + url_line_gap));

                UI_PrintStringSmallAtPixel(url_title, url_x, menu_item_x2, url_y1, (uint8_t)(url_y1 + 7u), 0u);
                UI_PrintStringSmallAtPixel(url_line1, url_x, menu_item_x2, url_y2, (uint8_t)(url_y2 + 7u), 0u);
                UI_PrintStringSmallAtPixel(url_line2, url_x, menu_item_x2, url_y3, (uint8_t)(url_y3 + 7u), 0u);

                already_printed = true;
                break;
            }
#ifdef ENABLE_FEAT_F4HWN_QRCODE
            if (page == 5u)
            {
                uint8_t qr_y = 0u;

                if (gIsInSubMenu)
                {
                    qr_y = 28u;
                }
                else
                {
                    qr_y = 28u;
                }

                UI_DrawQRCode(72, qr_y);

                already_printed = true;
                break;
            }
#endif
#ifdef ENABLE_FEAT_F4HWN
            if (page == 6u)
            {
                const uint8_t info_label_up_offset_pixels = 5u;
                const uint8_t info_label_safe_top_pixels_in_submenu = 20u;
                const uint8_t info_line_gap_pixels = 3u;
                const uint8_t info_line_height_pixels = 8u;
                const uint8_t info_label_cn_extra_up_offset_pixels = 3u;
                uint8_t info_label_y_start = 0u;
                uint8_t info_label_y_end = 0u;
                uint8_t info_line2_y_start = 0u;
                uint8_t info_line2_y_end = 0u;
                uint8_t info_line3_y_start = 0u;
                uint8_t info_line3_y_end = 0u;
                uint8_t info_label_print_y_start = 0u;
                uint8_t info_label_print_y_end = 0u;
                char val[16];

                info_label_y_start = UI_MENU_GetRowPixelStart(MENU_VALUE_ROW(1));
                if (info_label_y_start >= info_label_up_offset_pixels)
                {
                    info_label_y_start = (uint8_t)(info_label_y_start - info_label_up_offset_pixels);
                }
                else
                {
                    info_label_y_start = 0u;
                }
                info_label_y_end = (uint8_t)(info_label_y_start + 7u);
                info_label_y_start = (uint8_t)(info_label_y_start + level2_value_down_offset_pixels);
                info_label_y_end = (uint8_t)(info_label_y_end + level2_value_down_offset_pixels);
                info_label_y_start = (uint8_t)(info_label_y_start + level3_value_down_offset_pixels);
                info_label_y_end = (uint8_t)(info_label_y_end + level3_value_down_offset_pixels);
                if (gIsInSubMenu && info_label_y_start < info_label_safe_top_pixels_in_submenu)
                {
                    info_label_y_start = info_label_safe_top_pixels_in_submenu;
                    info_label_y_end = (uint8_t)(info_label_y_start + info_line_height_pixels - 1u);
                }
                info_line2_y_start = (uint8_t)(info_label_y_end + 1u + info_line_gap_pixels);
                info_line2_y_end = (uint8_t)(info_line2_y_start + info_line_height_pixels - 1u);
                info_line3_y_start = (uint8_t)(info_line2_y_end + 1u + info_line_gap_pixels);
                info_line3_y_end = (uint8_t)(info_line3_y_start + info_line_height_pixels - 1u);
                info_label_print_y_start = info_label_y_start;
                info_label_print_y_end = info_label_y_end;

                if (gUiLanguage == UI_LANGUAGE_CN)
                {
                    if (info_label_print_y_start >= info_label_cn_extra_up_offset_pixels)
                    {
                        info_label_print_y_start = (uint8_t)(info_label_print_y_start - info_label_cn_extra_up_offset_pixels);
                    }
                    else
                    {
                        info_label_print_y_start = 0u;
                    }
                    info_label_print_y_end = (uint8_t)(info_label_print_y_start + info_line_height_pixels - 1u);
                }

                UI_PrintStringSmallAtPixel(SUBV("Battery", "\xe7\x94\xb5\xe6\xb1\xa0"), menu_value_x1, menu_item_x2, info_label_print_y_start, info_label_print_y_end, 0u);
                sprintf(val, "%u.%02uV %u%%",
                    gBatteryVoltageAverage / 100,
                    gBatteryVoltageAverage % 100,
                    BATTERY_VoltsToPercent(gBatteryVoltageAverage));
                UI_PrintStringSmallAtPixel(val, menu_value_x1, menu_item_x2, info_line2_y_start, info_line2_y_end, 0u);
                UI_PrintStringSmallAtPixel(gSubMenu_BATTYP[gEeprom.BATTERY_TYPE], menu_value_x1, menu_item_x2, info_line3_y_start, info_line3_y_end, 0u);

                already_printed = true;
                break;
            }
#endif
        }
#else
            sprintf(String, "%u.%02uV\n%u%%",
                gBatteryVoltageAverage / 100, gBatteryVoltageAverage % 100,
                BATTERY_VoltsToPercent(gBatteryVoltageAverage));
#endif
            break;

        case MENU_RESET:
            strcpy(String, SUBV(gSubMenu_RESET[gSubMenuSelection], gSubMenu_RESET_CN[gSubMenuSelection]));
            break;

        case MENU_F_LOCK:
#ifdef ENABLE_FEAT_F4HWN
            if(!gIsInSubMenu && gUnlockAllTxConfCnt>0 && gUnlockAllTxConfCnt<3)
#else
            if(!gIsInSubMenu && gUnlockAllTxConfCnt>0 && gUnlockAllTxConfCnt<10)
#endif
                strcpy(String, "READ\nMANUAL");
            else
                strcpy(String, SUBV(gSubMenu_F_LOCK[gSubMenuSelection], gSubMenu_F_LOCK_CN[gSubMenuSelection]));
            break;

        #ifdef ENABLE_F_CAL_MENU
            case MENU_F_CALI:
                {
                    const uint32_t value   = 22656 + gSubMenuSelection;
                    const uint32_t xtal_Hz = (0x4f0000u + value) * 5;

                    writeXtalFreqCal(gSubMenuSelection, false);

                    sprintf(String, "%d\n%u.%06u\nMHz",
                        gSubMenuSelection,
                        xtal_Hz / 1000000, xtal_Hz % 1000000);
                }
                break;
        #endif

        case MENU_BATCAL:
        {
            const uint16_t vol = (uint32_t)gBatteryVoltageAverage * gBatteryCalibration[3] / gSubMenuSelection;
            sprintf(String, "%u.%02uV\n%u", vol / 100, vol % 100, gSubMenuSelection);
            break;
        }

        case MENU_BATTYP:
            strcpy(String, gSubMenu_BATTYP[gSubMenuSelection]);
            break;

        case MENU_MDC_ID:
            if (gIsInSubMenu && edit_index >= 0)
            {
                unsigned int sub_val_x1 = menu_value_x1;
                unsigned int sub_val_x2 = menu_item_x2;
                UI_PrintStringSmallAtPixel(edit, (uint8_t)sub_val_x1, (uint8_t)sub_val_x2, 28u, 35u, 0u);
                if (edit_index < 4)
                {
                    uint8_t char_width = 6;
                    uint8_t char_spacing = char_width + 1;
                    uint8_t text_start = (uint8_t)sub_val_x1;
                    if (sub_val_x2 > sub_val_x1)
                        text_start += (uint8_t)((((sub_val_x2 - sub_val_x1) - 4 * char_spacing) + 1u) / 2u);
                    {
                        const uint8_t underline_x = (uint8_t)(text_start + (edit_index * char_spacing) + 1u);
                        const uint8_t underline_fb_row = (uint8_t)(35u / 8u);
                        if (underline_fb_row < FRAME_LINES)
                            for (uint8_t c = 0; c < char_width; c++)
                                gFrameBuffer[underline_fb_row][underline_x + c] |= 0x01u;
                    }
                }
                already_printed = true;
                break;
            }
            sprintf(String, "%04X", gMDC1200_ID);
            break;

        case MENU_SET_NAV:
            strcpy(String, SUBV(gSubMenu_SET_NAV[gSubMenuSelection], gSubMenu_SET_NAV_CN[gSubMenuSelection]));
            break;

        case MENU_F1SHRT:
        case MENU_F1LONG:
        case MENU_F2SHRT:
        case MENU_F2LONG:
        case MENU_MLONG:
            strcpy(String, SUBV(gSubMenu_SIDEFUNCTIONS[gSubMenuSelection].name, gSubMenu_SIDEFUNCTIONS_CN[gSubMenuSelection]));
            break;

#ifdef ENABLE_FEAT_F4HWN_SLEEP
        case MENU_SET_OFF:
            if(gSubMenuSelection == 0)
            {
                strcpy(String, SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]));
            }
            else if(gSubMenuSelection < 121)
            {
                sprintf(String, "%dh:%02dm", (gSubMenuSelection / 60), (gSubMenuSelection % 60));
                //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
                //ST7565_Gauge(4, 1, 120, gSubMenuSelection);
                gaugeLine = 4;
                gaugeMin = 1;
                gaugeMax = 120;
                //#endif
            }
            break;
#endif

#ifdef ENABLE_FEAT_F4HWN
        case MENU_SET_PWR:
            sprintf(String, "%s\n%sW",
                    SUBV(gSubMenu_TXP[gSubMenuSelection + 1], gSubMenu_TXP_CN[gSubMenuSelection + 1]),
                    SUBV(gSubMenu_SET_PWR[gSubMenuSelection], gSubMenu_SET_PWR_CN[gSubMenuSelection]));
            break;
    
        case MENU_SET_PTT:
            strcpy(String, SUBV(gSubMenu_SET_PTT[gSubMenuSelection], gSubMenu_SET_PTT_CN[gSubMenuSelection]));
            break;

        case MENU_SET_TOT:
        case MENU_SET_EOT:
            strcpy(String, SUBV(gSubMenu_SET_TOT[gSubMenuSelection], gSubMenu_SET_TOT_CN[gSubMenuSelection]));
            break;

#ifdef ENABLE_FEAT_F4HWN_CTR
        case MENU_SET_CTR:
            sprintf(String, "%d", gSubMenuSelection);
            gSetting_set_ctr = gSubMenuSelection;
            ST7565_ContrastAndInv();
            break;
#endif

        case MENU_SET_INV:
            #ifdef ENABLE_FEAT_F4HWN_INV
                strcpy(String, SUBV(gSubMenu_OFF_ON[gSubMenuSelection], gSubMenu_OFF_ON_CN[gSubMenuSelection]));
                ST7565_ContrastAndInv();
            #else
                strcpy(String, gSubMenu_NA);
            #endif
            break;

        case MENU_TX_LOCK:
            if(TX_freq_check(gEeprom.VfoInfo[gEeprom.TX_VFO].pTX->Frequency) == 0)
            {
                strcpy(String, SUBV("Inside\nF Lock\nPlan", gSubMenu_TX_LOCK_INSIDE_CN));
            }
            else
            {
                strcpy(String, SUBV(gSubMenu_OFF_ON[gSubMenuSelection], gSubMenu_OFF_ON_CN[gSubMenuSelection]));
            }
            break;

        case MENU_SET_LCK:
            strcpy(String, SUBV(gSubMenu_SET_LCK[gSubMenuSelection], gSubMenu_SET_LCK_CN[gSubMenuSelection]));
            break;

        case MENU_SET_MET:
        case MENU_SET_GUI:
            strcpy(String, SUBV(gSubMenu_SET_MET[gSubMenuSelection], gSubMenu_SET_MET_CN[gSubMenuSelection]));
            break;

        #ifdef ENABLE_FEAT_F4HWN_AUDIO
            case MENU_SET_AUD:
                if(gTxVfo->Modulation == MODULATION_AM) {
                    strcpy(String, SUBV(gSubMenu_SET_AUD_AM[gSubMenuSelection], gSubMenu_SET_AUD_AM_CN[gSubMenuSelection]));
                    UI_PrintStringSmallNormal("AM", 114, 0, MENU_VALUE_ROW(0));
                }
                else if (gTxVfo->Modulation == MODULATION_USB) {
                    strcpy(String, "USB");
                    UI_PrintStringSmallNormal("USB", 108, 0, MENU_VALUE_ROW(0));
                }
                else {
                    strcpy(String, SUBV(gSubMenu_SET_AUD_FM[gSubMenuSelection], gSubMenu_SET_AUD_FM_CN[gSubMenuSelection]));
                    UI_PrintStringSmallNormal("FM", 114, 0, MENU_VALUE_ROW(0));
                }
                break;
        #endif

        #ifdef ENABLE_FEAT_F4HWN_NARROWER
            case MENU_SET_NFM:
                strcpy(String, SUBV(gSubMenu_SET_NFM[gSubMenuSelection], gSubMenu_SET_NFM_CN[gSubMenuSelection]));
                break;
        #endif

        #ifdef ENABLE_FEAT_F4HWN_VOL
            case MENU_SET_VOL:
                if(gSubMenuSelection == 0)
                {
                    strcpy(String, SUBV(gSubMenu_OFF_ON[0], gSubMenu_OFF_ON_CN[0]));
                }
                else if(gSubMenuSelection < 64)
                {
                    sprintf(String, "%02u", gSubMenuSelection);
                    //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
                    //ST7565_Gauge(4, 1, 63, gSubMenuSelection);
                    gaugeLine = 4;
                    gaugeMin = 1;
                    gaugeMax = 63;
                    //#endif
                }
                // gEeprom.VOLUME_GAIN = gSubMenuSelection;
                BK4819_WriteRegister(BK4819_REG_48,
                    (11u << 12)                |     // ??? .. 0 ~ 15, doesn't seem to make any difference
                    ( 0u << 10)                |     // AF Rx Gain-1
                    (gEeprom.VOLUME_GAIN << 4) |     // AF Rx Gain-2
                    (gEeprom.DAC_GAIN    << 0));     // AF DAC Gain (after Gain-1 and Gain-2)
                break;
        #endif

        #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
            case MENU_SET_KEY:
                strcpy(String, SUBV(gSubMenu_SET_KEY[gSubMenuSelection], gSubMenu_SET_KEY_CN[gSubMenuSelection]));
                break;                
        #endif
#endif

    }


    //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
    if(gaugeLine != 0)
    {
        const uint8_t gl = (uint8_t)(gaugeLine + MENU_VALUE_ROW_EXTRA());
        if (gl < FRAME_LINES)
            ST7565_Gauge(gl, gaugeMin, gaugeMax, gSubMenuSelection);
    }
    //#endif

    if (!already_printed)
    {   // we now do multi-line text in a single string

        unsigned int y;
        unsigned int lines = 1;
        unsigned int len   = strlen(String);
        bool         small = false;

        if (len > 0)
        {
            // count number of lines
            for (i = 0; i < len; i++)
            {
                if (String[i] == '\n' && i < (len - 1))
                {   // found new line char
                    lines++;
                    String[i] = 0;  // null terminate the line
                }
            }

            if (lines > 3)
            {   // use small text
                small = true;
                if (lines > 7)
                    lines = 7;
            }

            // center vertically'ish
            /*
            if (small)
                y = 3 - ((lines + 0) / 2);  // untested
            else
                y = 2 - ((lines + 0) / 2);
            */

            y = (small ? 3 : 2) - (lines / 2); 

            // only for non-F4HWN SysInf voltage display
#ifndef ENABLE_FEAT_F4HWN
            if(UI_MENU_GetCurrentMenuId() == MENU_VOL)
            {
                sprintf(edit, "%u.%02uV %u%%",
                    gBatteryVoltageAverage / 100, gBatteryVoltageAverage % 100,
                    BATTERY_VoltsToPercent(gBatteryVoltageAverage)
                );

#ifndef ENABLE_CHINESE
                UI_PrintStringSmallNormal(edit, 54, 127, MENU_VALUE_ROW(1));

                y = MENU_VALUE_ROW(2);
#endif
            }
#endif

            if (!gIsInSubMenu && UI_MENU_GetCurrentMenuId() != MENU_VOL)
                y += 1u;
            else if (gIsInSubMenu && UI_MENU_GetCurrentMenuId() != MENU_VOL)
                y += 2u;

            if (UI_MENU_IsToneMenu(UI_MENU_GetCurrentMenuId()))
            {
                top_right_badge_value_row = (uint8_t)y;
                top_right_badge_value_y_offset_px = UI_MENU_GetToneValueYOffsetPx(gIsInSubMenu);
            }
            {
                unsigned int sub_val_x1 = menu_value_x1;
                unsigned int sub_val_x2 = menu_item_x2;
                uint8_t submenu_value_y_offset_px = 0u;
                bool is_tone_menu_current = false;
#ifndef ENABLE_CHINESE
                uint8_t ascii_line_y_start = 0u;
                uint8_t ascii_line_y_end = 0u;
#endif

                is_tone_menu_current = UI_MENU_IsToneMenu(UI_MENU_GetCurrentMenuId());
                if (is_tone_menu_current)
                {
                    submenu_value_y_offset_px = UI_MENU_GetToneValueYOffsetPx(gIsInSubMenu);
                }
                if (UI_MENU_GetCurrentMenuId() == MENU_TXP ||
                    UI_MENU_GetCurrentMenuId() == MENU_SET_PWR)
                {
                    if (!gIsInSubMenu)
                    {
                        submenu_value_y_offset_px = (uint8_t)(submenu_value_y_offset_px + 7u);
                    }
                }
#ifdef ENABLE_CHINESE
                if (gUiLanguage == UI_LANGUAGE_CN && icon_layout && gIsInSubMenu && menu_item_x1 == 0u)
                    sub_val_x1 = menu_value_x1 + 8u;
                if (gUiLanguage == UI_LANGUAGE_CN && !icon_layout && gIsInSubMenu)
                    sub_val_x1 = sub_val_x1 + 2u;
                if (gUiLanguage == UI_LANGUAGE_CN && !gIsInSubMenu && sub_val_x1 >= 50u)
                    sub_val_x1 = sub_val_x1 + 2u;
                if (is_tone_menu_current && gIsInSubMenu)
                {
                    sub_val_x1 = menu_value_x1;
                    sub_val_x2 = menu_item_x2;
                }
#endif
            // draw the text lines
#ifdef ENABLE_CHINESE
            if (UI_MENU_GetCurrentMenuId() == MENU_VOL && len > 0)
            {
                uint8_t yp = (uint8_t)(MENU_VALUE_ROW(1u) * 8u);
                uint8_t dondji_tag_y_start = 0u;
                uint8_t dondji_tag_y_end = 0u;

                if (yp >= 2u)
                {
                    dondji_tag_y_start = (uint8_t)(yp - 2u);
                }
                else
                {
                    dondji_tag_y_start = 0u;
                }
                dondji_tag_y_end = (uint8_t)(dondji_tag_y_start + 7u);
                UI_PrintStringSmallAtPixel(edit, sub_val_x1, sub_val_x2, dondji_tag_y_start, dondji_tag_y_end, 0u);
                yp = CH_after(yp, CH_SM_H);
                i = 0;
                while (i < len && lines > 0)
                {
                    const char *pline = String + i;
                    const uint8_t band = VOL_line_band_height(pline);
                    if (band == CH_CN_H)
                        UI_PrintStringSmallAtPixel(pline, sub_val_x1, sub_val_x2, yp, (uint8_t)(yp + 11u), 0u);
                    else
                        UI_PrintStringSmallAtPixel(pline, sub_val_x1, sub_val_x2, yp, (uint8_t)(yp + 7u), 0u);
                    yp = CH_after(yp, band);
                    while (i < len && String[i] >= 32)
                        i++;
                    while (i < len && String[i] < 32)
                        i++;
                    lines--;
                }
            }
            else if (len > 0u && lines == 2u &&
                     UI_MENU_GetCurrentMenuId() == MENU_BOOT_HINT &&
                     !gIsInSubMenu &&
                     (gSubMenuSelection == 1 || gSubMenuSelection == 2))
            {
                const uint8_t line_draw_mode = (uint8_t)(gUiLanguage == UI_LANGUAGE_CN ? 3u : 0u);
                const char *first_line_text = String;
                const char *second_line_text = String;
                const uint8_t boot_hint_two_line_down_offset_px = 3u;
                uint8_t first_line_y_start = (uint8_t)(y * 8u);
                uint8_t first_line_y_end = (uint8_t)(first_line_y_start + 11u);
                uint8_t second_line_y_start = first_line_y_start;
                uint8_t second_line_y_end = first_line_y_end;
                size_t first_line_length = strlen(first_line_text);
                size_t second_line_offset = 0u;

                second_line_offset = first_line_length + 1u;
                second_line_text = String + second_line_offset;
                second_line_y_start = SF_after_2px(first_line_y_start, CH_CN_H);
                second_line_y_start = (uint8_t)(second_line_y_start + 5u);
                second_line_y_end = (uint8_t)(second_line_y_start + 11u);
                first_line_y_start = (uint8_t)(first_line_y_start + boot_hint_two_line_down_offset_px);
                first_line_y_end = (uint8_t)(first_line_y_start + 11u);
                second_line_y_start = (uint8_t)(second_line_y_start + boot_hint_two_line_down_offset_px);
                second_line_y_end = (uint8_t)(second_line_y_start + 11u);

                UI_PrintStringSmallAtPixel(first_line_text, sub_val_x1, sub_val_x2, first_line_y_start, first_line_y_end, line_draw_mode);
                UI_PrintStringSmallAtPixel(second_line_text, sub_val_x1, sub_val_x2, second_line_y_start, second_line_y_end, line_draw_mode);
            }
            else if (len > 0u && lines == 2u &&
                     (UI_MENU_GetCurrentMenuId() == MENU_F1SHRT ||
                      UI_MENU_GetCurrentMenuId() == MENU_F1LONG ||
                      UI_MENU_GetCurrentMenuId() == MENU_F2SHRT ||
                      UI_MENU_GetCurrentMenuId() == MENU_F2LONG ||
                      UI_MENU_GetCurrentMenuId() == MENU_MLONG))
            {
                const uint8_t ld_mix = 0u;
                uint8_t yp = (uint8_t)(y * 8u);
                i = 0;
                unsigned int rem = lines;
                while (i < len && rem > 0u)
                {
                    const char *pline = String + i;
                    const uint8_t band = VOL_line_band_height(pline);
                    if (band == CH_CN_H)
                        UI_PrintStringSmallAtPixel(pline, sub_val_x1, sub_val_x2, yp, (uint8_t)(yp + 11u), ld_mix);
                    else
                        UI_PrintStringSmallAtPixel(pline, sub_val_x1, sub_val_x2, yp, (uint8_t)(yp + 7u), 0u);
                    yp = SF_after_2px(yp, band);
                    while (i < len && String[i] >= 32)
                        i++;
                    while (i < len && String[i] < 32)
                        i++;
                    rem--;
                }
            }
#ifdef ENABLE_FEAT_F4HWN
            else if (len > 0u && lines == 3u &&
                     UI_MENU_GetCurrentMenuId() == MENU_TX_LOCK)
            {   /* 段外发射锁「频段内」三行值：行间 2px（与 SF_after_2px / 侧键两行一致） */
                const uint8_t ld_mix = 0u;
                uint8_t yp = (uint8_t)(y * 8u);
                i = 0;
                unsigned int rem = lines;
                while (i < len && rem > 0u)
                {
                    const char *pline = String + i;
                    const uint8_t band = VOL_line_band_height(pline);
                    if (band == CH_CN_H)
                        UI_PrintStringSmallAtPixel(pline, sub_val_x1, sub_val_x2, yp, (uint8_t)(yp + 11u), ld_mix);
                    else
                        UI_PrintStringSmallAtPixel(pline, sub_val_x1, sub_val_x2, yp, (uint8_t)(yp + 7u), 0u);
                    yp = SF_after_2px(yp, band);
                    while (i < len && String[i] >= 32)
                        i++;
                    while (i < len && String[i] < 32)
                        i++;
                    rem--;
                }
            }
#endif
            else if (len > 0u && lines == 3u &&
                     UI_MENU_GetCurrentMenuId() == MENU_SET_NAV &&
                     gUiLanguage == UI_LANGUAGE_CN)
            {   /* 导航键三行值：行间 2px（与侧键两行 / TX_LOCK 一致） */
                const uint8_t ld_mix = 0u;
                uint8_t yp = (uint8_t)(y * 8u);
                i = 0;
                unsigned int rem = lines;
                while (i < len && rem > 0u)
                {
                    const char *pline = String + i;
                    const uint8_t band = VOL_line_band_height(pline);
                    if (band == CH_CN_H)
                        UI_PrintStringSmallAtPixel(pline, sub_val_x1, sub_val_x2, yp, (uint8_t)(yp + 11u), ld_mix);
                    else
                        UI_PrintStringSmallAtPixel(pline, sub_val_x1, sub_val_x2, yp, (uint8_t)(yp + 7u), 0u);
                    yp = SF_after_2px(yp, band);
                    while (i < len && String[i] >= 32)
                        i++;
                    while (i < len && String[i] < 32)
                        i++;
                    rem--;
                }
            }
            else if (len > 0u && lines >= 2u && lines <= 3u &&
                     UI_MENU_GetCurrentMenuId() == MENU_F_LOCK &&
                     gUiLanguage == UI_LANGUAGE_CN)
            {   /* 锁定频段 2～3 行值：行间 2px */
                const uint8_t ld_mix = 0u;
                uint8_t yp = (uint8_t)(y * 8u);
                i = 0;
                unsigned int rem = lines;
                while (i < len && rem > 0u)
                {
                    const char *pline = String + i;
                    const uint8_t band = VOL_line_band_height(pline);
                    if (band == CH_CN_H)
                        UI_PrintStringSmallAtPixel(pline, sub_val_x1, sub_val_x2, yp, (uint8_t)(yp + 11u), ld_mix);
                    else
                        UI_PrintStringSmallAtPixel(pline, sub_val_x1, sub_val_x2, yp, (uint8_t)(yp + 7u), 0u);
                    yp = SF_after_2px(yp, band);
                    while (i < len && String[i] >= 32)
                        i++;
                    while (i < len && String[i] < 32)
                        i++;
                    rem--;
                }
            }
            else
#endif
            for (i = 0; i < len && lines > 0; lines--)
            {
#ifdef ENABLE_CHINESE
                UI_MENU_PrintSubmenuValueLine(String + i, sub_val_x1, sub_val_x2, y, small, submenu_value_y_offset_px);
#else
                if (submenu_value_y_offset_px == 0u)
                {
                    UI_PrintStringSmallNormal(String + i, menu_value_x1, menu_item_x2, y);
                }
                else
                {
                    ascii_line_y_start = (uint8_t)(y * 8u + submenu_value_y_offset_px);
                    ascii_line_y_end = (uint8_t)(ascii_line_y_start + 7u);
                    UI_PrintStringSmallAtPixel(String + i, menu_value_x1, menu_item_x2, ascii_line_y_start, ascii_line_y_end, 0u);
                }
#endif

                // look for start of next line
                while (i < len && String[i] >= 32)
                    i++;

                // hop over the null term char(s)
                while (i < len && String[i] < 32)
                    i++;

                y += 1;
            }
            }
        }
    }

    if (UI_MENU_GetCurrentMenuId() == MENU_S_PRI_CH_1 || UI_MENU_GetCurrentMenuId() == MENU_S_PRI_CH_2)
    {

    }

    if ((UI_MENU_GetCurrentMenuId() == MENU_R_CTCS || UI_MENU_GetCurrentMenuId() == MENU_R_DCS) && gCssBackgroundScan)
        UI_PrintStringSmallNormal("SCAN", menu_value_x1, menu_item_x2, MENU_VALUE_ROW(4));

#ifdef ENABLE_DTMF_CALLING
    if (UI_MENU_GetCurrentMenuId() == MENU_D_LIST && gIsDtmfContactValid) {
        Contact[11] = 0;
        memcpy(&gDTMF_ID, Contact + 8, 4);
        sprintf(String, "ID:%4s", gDTMF_ID);
        UI_PrintStringSmallNormal(String, menu_value_x1, menu_item_x2, MENU_VALUE_ROW(4));
    }
#endif

    if (UI_MENU_GetCurrentMenuId() == MENU_R_CTCS ||
        UI_MENU_GetCurrentMenuId() == MENU_T_CTCS ||
        UI_MENU_GetCurrentMenuId() == MENU_R_DCS  ||
        UI_MENU_GetCurrentMenuId() == MENU_T_DCS
#ifdef ENABLE_DTMF_CALLING
        || UI_MENU_GetCurrentMenuId() == MENU_D_LIST
#endif
    ) {
        if (UI_MENU_GetCurrentMenuId() == MENU_R_CTCS ||
            UI_MENU_GetCurrentMenuId() == MENU_T_CTCS)
        {
            uint8_t approved_index = 0xFF;
            if (gSubMenuSelection > 0)
            {
                approved_index = DCS_GetCtcssApprovedIndex((uint8_t)(gSubMenuSelection - 1));
            }

            if (gSubMenuSelection == 0)
            {
                sprintf(top_right_badge, "00/00");
            }
            else if (approved_index != 0xFF)
            {
                sprintf(top_right_badge, "%02d/%02d", gSubMenuSelection, approved_index + 1);
            }
            else
            {
                sprintf(top_right_badge, "%02d/--", gSubMenuSelection);
            }
        }
        else
        {
            sprintf(top_right_badge, "%03d", gSubMenuSelection);
        }
    }
    if (top_right_badge[0] != '\0' && top_right_badge_value_row != 0xFFu)
    {
        UI_MENU_DrawTopRightBadgePlain(top_right_badge,
                                       top_right_badge_value_row,
                                       top_right_badge_value_y_offset_px,
                                       true,
                                       menu_value_x1,
                                       menu_item_x2);
    }

    if ((UI_MENU_GetCurrentMenuId() == MENU_RESET      ||
         UI_MENU_GetCurrentMenuId() == MENU_MEM_CH     ||
         UI_MENU_GetCurrentMenuId() == MENU_MEM_NAME   ||
         UI_MENU_GetCurrentMenuId() == MENU_DEL_CH) && gAskForConfirmation)
    {
        const char *pPrintStr;
#ifdef ENABLE_CHINESE
        /*
         * 存信道 / 删信道 / 命名信道 / 复位：共用本段。
         * 标点与 FM 页一致：汉字 + 英文 ? / !（勿用全角 U+FF1F/U+FF01，混排绘制更稳）。
         * 根据当前语言设置动态选择提示语言。
         */
        if (gAskForConfirmation == 1) {
            pPrintStr = (gUiLanguage == UI_LANGUAGE_CN) ? "\xe7\xa1\xae\xe8\xae\xa4?" : "SURE?";
        } else {
            pPrintStr = (gUiLanguage == UI_LANGUAGE_CN) ? "\xe8\xaf\xb7\xe7\xad\x89\xe5\xbe\x85!" : "WAIT!";
        }
        /* 整行宽度居中 */
        UI_PrintStringSmallAtPixel(pPrintStr, 0u, LCD_WIDTH - 1u, 52u, 59u, 3u);
#else
        pPrintStr = (gAskForConfirmation == 1) ? "SURE?" : "WAIT!";
        UI_PrintStringSmallNormal(pPrintStr, menu_value_x1, menu_item_x2, 6);
#endif
    }

    ST7565_BlitFullScreen();
#ifdef ENABLE_FEAT_F4HWN
    /*
     * 与主页面一致：顶栏（含电池）走 gStatusLine + BlitStatusLine。
     * 双守等模式下主界面不通过 UI_DisplayStatus 维护顶栏，仅 gUpdateDisplay 刷菜单时
     * 若不在这里补画，菜单顶栏电池会长期不更新。
     */
    UI_DisplayMainOnlyStatusBar();
#endif
}
