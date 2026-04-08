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
#include "../bitmaps.h"
#include "../board.h"
#include "../dcs.h"
#include "../driver/backlight.h"
#include "../driver/bk4819.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../external/printf/printf.h"
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
    {"F1Shrt",      MENU_F1SHRT        },
    {"F1Long",      MENU_F1LONG        },
    {"F2Shrt",      MENU_F2SHRT        },
    {"F2Long",      MENU_F2LONG        },
    {"M Long",      MENU_MLONG         },

    {"KeyLck",      MENU_AUTOLK        }, // was "AUTOLk"
    {"TxTOut",      MENU_TOT           }, // was "TOT"
    {"BatSav",      MENU_SAVE          }, // was "SAVE"
    {"BatTxt",      MENU_BAT_TXT       },
    {"Mic",         MENU_MIC           },
    {"ChDisp",      MENU_MDF           }, // was "MDF"
    {"POnMsg",      MENU_PONMSG        },
    {"BLTime",      MENU_ABR           }, // was "ABR"
    {"BLMin",       MENU_ABR_MIN       },
    {"BLMax",       MENU_ABR_MAX       },
    {"BLTxRx",      MENU_ABR_ON_TX_RX  },
    {"Beep",        MENU_BEEP          },
#ifdef ENABLE_VOICE
    {"Voice",       MENU_VOICE         },
#endif
    {"Roger",       MENU_ROGER         },
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
    {"RxMode",      MENU_TDR           },
    {"Sql",         MENU_SQL           },
#ifdef ENABLE_FEAT_F4HWN
    {"SetPwr",      MENU_SET_PWR       },
    {"SetPTT",      MENU_SET_PTT       },
    {"SetTOT",      MENU_SET_TOT       },
    {"SetEOT",      MENU_SET_EOT       },
    {"SetCtr",      MENU_SET_CTR       },
    {"SetInv",      MENU_SET_INV       },
    {"SetLck",      MENU_SET_LCK       },
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

const char gSubMenu_BATTYP[][12] =
{
    "1600mAh K5",
    "2200mAh K5",
    "3500mAh K5",
    "1400mAh K1",
    "2500mAh K1"
};

const char gSubMenu_SET_NAV[][17] =
{
    "LEFT\nRIGHT\nUV-K1",
    "UP\nDOWN\nUV-K5(8)",
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
            if (!gMenuUseMainOnlyStatus || gMenuMainPageActive)
                return i;

            for (visible = 0; visible < menu_count; visible++)
                if (MENU_GetActualMenuIndexFromCursor(visible) == i)
                    return visible;
            return 0;
        }
    return 0;
}

int32_t gSubMenuSelection;

// edit box
char    edit_original[17]; // a copy of the text before editing so that we can easily test for changes/difference
char    edit[17];
int     edit_index;

static bool UI_MENU_GetBufferPixel(uint8_t x, uint8_t y)
{
    return (gFrameBuffer[y >> 3][x] >> (y & 7u)) & 1u;
}

static void UI_MENU_ShiftLeftMenuAreaDown(uint8_t width, uint8_t shift_px)
{
    if (shift_px == 0 || width == 0)
        return;

    for (int16_t y = 55; y >= 0; y--)
    {
        for (uint8_t x = 0; x < width; x++)
        {
            const bool on = (y >= shift_px) ? UI_MENU_GetBufferPixel(x, (uint8_t)(y - shift_px)) : false;
            PutPixel(x, (uint8_t)y, on);
        }
    }
}

static void UI_MENU_ShiftRightMenuAreaDown(uint8_t x_start, uint8_t shift_px)
{
    if (shift_px == 0 || x_start >= LCD_WIDTH)
        return;

    for (int16_t y = 55; y >= 0; y--)
    {
        for (uint8_t x = x_start; x < LCD_WIDTH; x++)
        {
            const bool on = (y >= shift_px) ? UI_MENU_GetBufferPixel(x, (uint8_t)(y - shift_px)) : false;
            PutPixel(x, (uint8_t)y, on);
        }
    }
}

static bool UI_MENU_NameContainsCh(const char *name)
{
    if (name == NULL)
        return false;
    for (uint8_t i = 0; name[i] != 0 && name[i + 1] != 0; i++)
    {
        const char a = name[i];
        const char b = name[i + 1];
        if ((a == 'c' || a == 'C') && (b == 'h' || b == 'H'))
            return true;
    }
    return false;
}

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
    static const char * const names[] = {"Channel", "Settings", "Other", "About"};
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
        UI_MENU_DrawLauncherOther40(icon_left, icon_top);
    }
    else if (idx == 3)
    {
        UI_MENU_DrawLauncherAbout40(icon_left, icon_top);
    }
    else
    {
        UI_DrawRectangleBuffer(gFrameBuffer, icon_left + 5, icon_top + 5, icon_left + 34, icon_top + 34, true);
    }

    UI_MENU_DrawChevron(false, 15, 23, 12, 3);
    UI_MENU_DrawChevron(true, 112, 23, 12, 3);

    if (icon_count > 0 && names[idx][0] != 0)
    {
        UI_PrintString(names[idx], 0, LCD_WIDTH - 1, 5, 8);
        UI_PrintString(names[idx], 1, LCD_WIDTH - 1, 5, 8);
    }
    
    sprintf(index_str, "%u/%u", (unsigned int)(idx + 1), (unsigned int)icon_count);
    UI_PrintStringSmallNormal(index_str, 104, 0, 6);

    ST7565_BlitFullScreen();
}

void UI_DisplayMenu(void)
{
    const uint8_t menu_count = MENU_GetActiveMenuCount();
    const unsigned int menu_list_width = 6; // max no. of characters on the menu list (left side)
    const unsigned int menu_item_x1    = (8 * menu_list_width) + 2;
    const unsigned int menu_item_x2    = LCD_WIDTH - 1;
    unsigned int       i;
    char               String[64];  // bigger cuz we can now do multi-line in one string (use '\n' char)

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

#ifdef ENABLE_FEAT_F4HWN
    UI_DrawLineBuffer(gFrameBuffer, 48, 0, 48, 55, 1); // Be ware, status zone = 8 lines, the rest = 56 ->total 64
    //UI_DrawLineDottedBuffer(gFrameBuffer, 0, 46, 50, 46, 1);

    if (!gIsInSubMenu)
        for (uint8_t i = 0; i < 48; i += 2)
        {
            gFrameBuffer[5][i] = 0x40;
        }
#endif

#ifndef ENABLE_CUSTOM_MENU_LAYOUT
    if (menu_count <= 1)
    {
        const uint8_t actual_idx = UI_MENU_GetActualIndexFromCursor(0);
        UI_PrintString(MenuList[actual_idx].name, 0, 0, 2, 8);
    }
    else
    {
        // original menu layout
    for (i = 0; i < 3; i++)
        if (gMenuCursor > 0 || i > 0)
            if ((menu_count - 1) != gMenuCursor || i != 2)
            {
                const uint8_t actual_idx = UI_MENU_GetActualIndexFromCursor((uint8_t)(gMenuCursor + i - 1));
                UI_PrintString(MenuList[actual_idx].name, 0, 0, i * 2, 8);
            }
    }

    // invert the current menu list item pixels
    for (i = 0; i < (8 * menu_list_width); i++)
    {
        gFrameBuffer[2][i] ^= 0xFF;
        gFrameBuffer[3][i] ^= 0xFF;
    }

    // draw vertical separating dotted line
    for (i = 0; i < 7; i++)
        gFrameBuffer[i][(8 * menu_list_width) + 1] = 0xAA;

    // draw the little sub-menu triangle marker
    if (gIsInSubMenu)
        memcpy(gFrameBuffer[0] + (8 * menu_list_width) + 1, BITMAP_CurrentIndicator, sizeof(BITMAP_CurrentIndicator));

    // draw the menu index number/count
    if (!gIsInSubMenu)
    {
        sprintf(String, "%2u.%u", 1 + gMenuCursor, menu_count);
        UI_PrintStringSmallNormal(String, 2, 0, 6);
    }

#else
    {   // new menu layout .. experimental & unfinished
        const int menu_index = gMenuCursor;  // current selected menu item
        i = 1;

        if (menu_count <= 1)
        {
            const uint8_t actual_idx = UI_MENU_GetActualIndexFromCursor(0);
            UI_PrintString(MenuList[actual_idx].name, 0, 0, 2, 8);
        }
        else if (!gIsInSubMenu) {
            while (i < 2)
            {   // leading menu items - small text
                const int k = menu_index + i - 2;
                if (k < 0)
                {
                    const uint8_t actual_idx = UI_MENU_GetActualIndexFromCursor((uint8_t)(menu_count + k));
                    UI_PrintStringSmallNormal(MenuList[actual_idx].name, 0, 0, i);  // wrap-a-round
                }
                else if (k >= 0 && k < (int)menu_count)
                {
                    const uint8_t actual_idx = UI_MENU_GetActualIndexFromCursor((uint8_t)k);
                    UI_PrintStringSmallNormal(MenuList[actual_idx].name, 0, 0, i);
                }
                i++;
            }

            // current menu item - keep big n fat
            if (menu_index >= 0 && menu_index < (int)menu_count)
            {
                const uint8_t actual_idx = UI_MENU_GetActualIndexFromCursor((uint8_t)menu_index);
                UI_PrintString(MenuList[actual_idx].name, 0, 0, 2, 8);
            }
            i++;

            while (i < 4)
            {   // trailing menu item - small text
                const int k = menu_index + i - 2;
                if (k >= 0 && k < (int)menu_count)
                {
                    const uint8_t actual_idx = UI_MENU_GetActualIndexFromCursor((uint8_t)k);
                    UI_PrintStringSmallNormal(MenuList[actual_idx].name, 0, 0, 1 + i);
                }
                else if (k >= (int)menu_count)
                {
                    const uint8_t actual_idx = UI_MENU_GetActualIndexFromCursor((uint8_t)(menu_count - k));  // wrap-a-round
                    UI_PrintStringSmallNormal(MenuList[actual_idx].name, 0, 0, 1 + i);
                }
                i++;
            }

            // draw the menu index number/count
#ifndef ENABLE_FEAT_F4HWN
            sprintf(String, "%2u.%u", 1 + gMenuCursor, menu_count);
            UI_PrintStringSmallNormal(String, 2, 0, 6);
#endif
        }
        else if (menu_index >= 0 && menu_index < (int)menu_count)
        {   // current menu item
//          strcat(String, ":");
            const uint8_t actual_idx = UI_MENU_GetActualIndexFromCursor((uint8_t)menu_index);
            UI_PrintString(MenuList[actual_idx].name, 0, 0, 0, 8);
//          UI_PrintStringSmallNormal(String, 0, 0, 0);
        }

#ifdef ENABLE_FEAT_F4HWN
        sprintf(String, "%02u/%u", 1 + gMenuCursor, menu_count);
        if (gIsInSubMenu)
            GUI_DisplaySmallest(String, 6, 45, false, true);   // move up 3px from y=48
        else
            UI_PrintStringSmallNormal(String, 6, 0, 6);
#endif
    }
#endif

    if (gIsInSubMenu)
    {
        const uint8_t left_width = (uint8_t)(8 * menu_list_width);
        UI_MENU_ShiftLeftMenuAreaDown(left_width, 3);

        // Clear shift artifacts in the top-left band.
        for (uint8_t y = 0; y < 3; y++)
            for (uint8_t x = 0; x < left_width; x++)
                PutPixel(x, y, false);

        // Clear tiny dotted/short-line artifacts above the moved left title.
        for (uint8_t y = 1; y <= 4; y++)
            for (uint8_t x = 0; x < left_width; x++)
                PutPixel(x, y, false);

        // Keep the separator line continuous on both sides.
        UI_DrawLineBuffer(gFrameBuffer, 0, 0, LCD_WIDTH - 1, 0, true);
    }

    // **************

    memset(String, 0, sizeof(String));

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
                strcpy(String, gSubMenu_OFF_ON[gSubMenuSelection]);
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
                strcpy(String, gSubMenu_TXP[gSubMenuSelection]);
            }
            else
            {
                sprintf(String, "%s\n%sW", gSubMenu_TXP[gSubMenuSelection], gSubMenu_SET_PWR[gSubMenuSelection - 1]);
            }
            break;

        case MENU_R_DCS:
        case MENU_T_DCS:
            if (gSubMenuSelection == 0)
                strcpy(String, gSubMenu_OFF_ON[0]);
            else if (gSubMenuSelection < 105)
                sprintf(String, "D%03oN", DCS_Options[gSubMenuSelection -   1]);
            else
                sprintf(String, "D%03oI", DCS_Options[gSubMenuSelection - 105]);
            break;

        case MENU_R_CTCS:
        case MENU_T_CTCS:
        {
            if (gSubMenuSelection == 0)
                strcpy(String, gSubMenu_OFF_ON[0]);
            else
                sprintf(String, "%u.%uHz", CTCSS_Options[gSubMenuSelection - 1] / 10, CTCSS_Options[gSubMenuSelection - 1] % 10);
            break;
        }

        case MENU_SFT_D:
            strcpy(String, gSubMenu_SFT_D[gSubMenuSelection]);
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

            UI_PrintString(String, menu_item_x1, menu_item_x2, 1, 8);
            UI_PrintString("MHz",  menu_item_x1, menu_item_x2, 3, 8);

            already_printed = true;
            break;

        case MENU_W_N:
            strcpy(String, gSubMenu_W_N[gSubMenuSelection]);
            break;

#ifndef ENABLE_FEAT_F4HWN
        case MENU_SCR:
            strcpy(String, gSubMenu_SCRAMBLER[gSubMenuSelection]);
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
                sprintf(String, gSubMenuSelection == 0 ? gSubMenu_OFF_ON[0] : "%u", gSubMenuSelection);
            #else
                strcpy(String, gSubMenu_NA);
            #endif
            break;

        case MENU_ABR:
            if(gSubMenuSelection == 0)
            {
                strcpy(String, gSubMenu_OFF_ON[0]);
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
                strcpy(String, "ON");
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
            strcpy(String, gModulationStr[gSubMenuSelection]);
            break;

        case MENU_AUTOLK:
            if (gSubMenuSelection == 0)
                strcpy(String, gSubMenu_OFF_ON[0]);
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
            strcpy(String, gSubMenu_RX_TX[gSubMenuSelection]);
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
            strcpy(String, gSubMenu_OFF_ON[gSubMenuSelection]);
            break;

        case MENU_MEM_CH:
        case MENU_1_CALL:
        case MENU_DEL_CH:
        case MENU_S_PRI_CH_1:
        case MENU_S_PRI_CH_2:
        {
            if(gSubMenuSelection == MR_CHANNELS_MAX)
            {
                UI_PrintString("None", menu_item_x1, menu_item_x2, 2, 8);
                already_printed = true;
                break;
            }
            else
            {
                const bool valid = RADIO_CheckValidChannel(gSubMenuSelection, false, 0);

                UI_GenerateChannelStringEx(String, valid, gSubMenuSelection);
                UI_PrintString(String, menu_item_x1, menu_item_x2, 0, 8);

                if (valid && !gAskForConfirmation)
                {   // show the frequency so that the user knows the channels frequency
                    const uint32_t frequency = SETTINGS_FetchChannelFrequency(gSubMenuSelection);
                    sprintf(String, "%u.%05u", frequency / 100000, frequency % 100000);
                    UI_PrintString(String, menu_item_x1, menu_item_x2, 4, 8);
                }

                SETTINGS_FetchChannelName(String, gSubMenuSelection);
                UI_PrintString(String[0] ? String : "--", menu_item_x1, menu_item_x2, 2, 8);
                already_printed = true;
                break;
            }
        }

        case MENU_MEM_NAME:
        {
            const bool valid = RADIO_CheckValidChannel(gSubMenuSelection, false, 0);

            UI_GenerateChannelStringEx(String, valid, gSubMenuSelection);
            UI_PrintString(String, menu_item_x1, menu_item_x2, 0, 8);

            if (valid)
            {
                const uint32_t frequency = SETTINGS_FetchChannelFrequency(gSubMenuSelection);

                //if (!gIsInSubMenu || edit_index < 0)
                if (!gIsInSubMenu)
                    edit_index = -1;
                if (edit_index < 0)
                {   // show the channel name
                    SETTINGS_FetchChannelName(String, gSubMenuSelection);
                    char *pPrintStr = String[0] ? String : "--";
                    UI_PrintString(pPrintStr, menu_item_x1, menu_item_x2, 2, 8);
                }
                else
                {   // show the channel name being edited
                    //UI_PrintString(edit, menu_item_x1, 0, 2, 8);
                    UI_PrintString(edit, menu_item_x1, menu_item_x2, 2, 8);
                    if (edit_index < 10)
                        //UI_PrintString("^", menu_item_x1 + (8 * edit_index), 0, 4, 8);  // show the cursor
                        UI_PrintString("^", menu_item_x1 - 1 + (8 * edit_index),0, 4, 8); // show the cursor
                }

                if (!gAskForConfirmation)
                {   // show the frequency so that the user knows the channels frequency
                    sprintf(String, "%u.%05u", frequency / 100000, frequency % 100000);
                    UI_PrintString(String, menu_item_x1, menu_item_x2, 4 + (gIsInSubMenu && edit_index >= 0), 8);
                }
            }

            already_printed = true;
            break;
        }

        case MENU_SAVE:
            sprintf(String, gSubMenuSelection == 0 ? gSubMenu_OFF_ON[0] : "1:%u", gSubMenuSelection);
            break;

        case MENU_TDR:
            strcpy(String, gSubMenu_RXMode[gSubMenuSelection]);
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
                strcpy(String, gSubMenu_VOICE[gSubMenuSelection]);
                break;
        #endif

        case MENU_SC_REV:
            if(gSubMenuSelection == 0)
            {
                strcpy(String, "STOP");
            }
            else if(gSubMenuSelection < 81)
            {
                sprintf(String, "CARRIER\n%02ds:%03dms", ((gSubMenuSelection * 250) / 1000), ((gSubMenuSelection * 250) % 1000));
                //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
                //ST7565_Gauge(5, 1, 80, gSubMenuSelection);
                gaugeLine = 5;
                gaugeMin = 1;
                gaugeMax = 80;
                //#endif
            }
            else
            {
                sprintf(String, "TIMEOUT\n%02dm:%02ds", (((gSubMenuSelection - 80) * 5) / 60), (((gSubMenuSelection - 80) * 5) % 60));
                //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
                //ST7565_Gauge(5, 80, 104, gSubMenuSelection);
                gaugeLine = 5;
                gaugeMin = 80;
                gaugeMax = 104;
                //#endif
            }
            break;

        case MENU_MDF:
            strcpy(String, gSubMenu_MDF[gSubMenuSelection]);
            break;

        case MENU_RP_STE:
            sprintf(String, gSubMenuSelection == 0 ? gSubMenu_OFF_ON[0] : "%u*100ms", gSubMenuSelection);
            break;

        case MENU_LIST_CH:
        case MENU_S_LIST:
            if (gSubMenuSelection == MR_CHANNELS_LIST + 1)
                strcpy(String, "ALL");
            else if (gSubMenuSelection == 0 && UI_MENU_GetCurrentMenuId() == MENU_LIST_CH)
                strcpy(String, "OFF");
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
                sprintf(String, gSubMenu_AL_MOD[gSubMenuSelection]);
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
            strcpy(String, gSubMenu_D_RSP[gSubMenuSelection]);
            break;

        case MENU_D_HOLD:
            sprintf(String, "%ds", gSubMenuSelection);
            break;
#endif
        case MENU_D_PRE:
            sprintf(String, "%d*10ms", gSubMenuSelection);
            break;

        case MENU_PTT_ID:
            strcpy(String, gSubMenu_PTT_ID[gSubMenuSelection]);
            break;

        case MENU_BAT_TXT:
            strcpy(String, gSubMenu_BAT_TXT[gSubMenuSelection]);
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
            strcpy(String, gSubMenu_PONMSG[gSubMenuSelection]);
            break;

        case MENU_ROGER:
            strcpy(String, gSubMenu_ROGER[gSubMenuSelection]);
            break;

        case MENU_VOL:
#ifdef ENABLE_FEAT_F4HWN
            sprintf(String, "%s\n%s",
                AUTHOR_STRING_2,
                VERSION_STRING_2
            );
#else
            sprintf(String, "%u.%02uV\n%u%%",
                gBatteryVoltageAverage / 100, gBatteryVoltageAverage % 100,
                BATTERY_VoltsToPercent(gBatteryVoltageAverage));
#endif
            break;

        case MENU_RESET:
            strcpy(String, gSubMenu_RESET[gSubMenuSelection]);
            break;

        case MENU_F_LOCK:
#ifdef ENABLE_FEAT_F4HWN
            if(!gIsInSubMenu && gUnlockAllTxConfCnt>0 && gUnlockAllTxConfCnt<3)
#else
            if(!gIsInSubMenu && gUnlockAllTxConfCnt>0 && gUnlockAllTxConfCnt<10)
#endif
                strcpy(String, "READ\nMANUAL");
            else
                strcpy(String, gSubMenu_F_LOCK[gSubMenuSelection]);
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

        case MENU_SET_NAV:
            strcpy(String, gSubMenu_SET_NAV[gSubMenuSelection]);
            break;

        case MENU_F1SHRT:
        case MENU_F1LONG:
        case MENU_F2SHRT:
        case MENU_F2LONG:
        case MENU_MLONG:
            strcpy(String, gSubMenu_SIDEFUNCTIONS[gSubMenuSelection].name);
            break;

#ifdef ENABLE_FEAT_F4HWN_SLEEP
        case MENU_SET_OFF:
            if(gSubMenuSelection == 0)
            {
                strcpy(String, gSubMenu_OFF_ON[0]);
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
            sprintf(String, "%s\n%sW", gSubMenu_TXP[gSubMenuSelection + 1], gSubMenu_SET_PWR[gSubMenuSelection]);
            break;
    
        case MENU_SET_PTT:
            strcpy(String, gSubMenu_SET_PTT[gSubMenuSelection]);
            break;

        case MENU_SET_TOT:
        case MENU_SET_EOT:
            strcpy(String, gSubMenu_SET_TOT[gSubMenuSelection]); // Same as SET_TOT
            break;

        case MENU_SET_CTR:
            #ifdef ENABLE_FEAT_F4HWN_CTR
                sprintf(String, "%d", gSubMenuSelection);
                gSetting_set_ctr = gSubMenuSelection;
                ST7565_ContrastAndInv();
            #else
                strcpy(String, gSubMenu_NA);
            #endif
            break;

        case MENU_SET_INV:
            #ifdef ENABLE_FEAT_F4HWN_INV
                strcpy(String, gSubMenu_OFF_ON[gSubMenuSelection]);
                ST7565_ContrastAndInv();
            #else
                strcpy(String, gSubMenu_NA);
            #endif
            break;

        case MENU_TX_LOCK:
            if(TX_freq_check(gEeprom.VfoInfo[gEeprom.TX_VFO].pTX->Frequency) == 0)
            {
                strcpy(String, "Inside\nF Lock\nPlan");
            }
            else
            {
                strcpy(String, gSubMenu_OFF_ON[gSubMenuSelection]);
            }
            break;

        case MENU_SET_LCK:
            strcpy(String, gSubMenu_SET_LCK[gSubMenuSelection]);
            break;

        case MENU_SET_MET:
        case MENU_SET_GUI:
            strcpy(String, gSubMenu_SET_MET[gSubMenuSelection]); // Same as SET_MET
            break;

        #ifdef ENABLE_FEAT_F4HWN_AUDIO
            case MENU_SET_AUD:
                if(gTxVfo->Modulation == MODULATION_AM) {
                    strcpy(String, gSubMenu_SET_AUD_AM[gSubMenuSelection]);
                    UI_PrintStringSmallNormal("AM", 114, 0, 0);
                }
                else if (gTxVfo->Modulation == MODULATION_USB) {
                    strcpy(String, "USB");
                    UI_PrintStringSmallNormal("USB", 108, 0, 0);
                }
                else {
                    strcpy(String, gSubMenu_SET_AUD_FM[gSubMenuSelection]);
                    UI_PrintStringSmallNormal("FM", 114, 0, 0);
                }
                break;
        #endif

        #ifdef ENABLE_FEAT_F4HWN_NARROWER
            case MENU_SET_NFM:
                strcpy(String, gSubMenu_SET_NFM[gSubMenuSelection]);
                break;
        #endif

        #ifdef ENABLE_FEAT_F4HWN_VOL
            case MENU_SET_VOL:
                if(gSubMenuSelection == 0)
                {
                    strcpy(String, gSubMenu_OFF_ON[0]);
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
                strcpy(String, gSubMenu_SET_KEY[gSubMenuSelection]);
                break;                
        #endif
#endif

    }

    //#if !defined(ENABLE_SPECTRUM) || !defined(ENABLE_FMRADIO)
    if(gaugeLine != 0)
    {
        ST7565_Gauge(gaugeLine, gaugeMin, gaugeMax, gSubMenuSelection);
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

            // only for SysInf
            if(UI_MENU_GetCurrentMenuId() == MENU_VOL)
            {
                sprintf(edit, "%u.%02uV %u%%",
                    gBatteryVoltageAverage / 100, gBatteryVoltageAverage % 100,
                    BATTERY_VoltsToPercent(gBatteryVoltageAverage)
                );

                UI_PrintStringSmallNormal(edit, 54, 127, 1);

                #ifdef ENABLE_FEAT_F4HWN
                    UI_PrintStringSmallNormal(Edition, 54, 127, 6);
                #endif

                y = 2;
            }

            // draw the text lines
            for (i = 0; i < len && lines > 0; lines--)
            {
                if (small)
                    UI_PrintStringSmallNormal(String + i, menu_item_x1, menu_item_x2, y);
                else
                    UI_PrintString(String + i, menu_item_x1, menu_item_x2, y, 8);

                // look for start of next line
                while (i < len && String[i] >= 32)
                    i++;

                // hop over the null term char(s)
                while (i < len && String[i] < 32)
                    i++;

                y += small ? 1 : 2;
            }
        }
    }

    if (UI_MENU_GetCurrentMenuId() == MENU_S_PRI_CH_1 || UI_MENU_GetCurrentMenuId() == MENU_S_PRI_CH_2)
    {

    }

    if ((UI_MENU_GetCurrentMenuId() == MENU_R_CTCS || UI_MENU_GetCurrentMenuId() == MENU_R_DCS) && gCssBackgroundScan)
        UI_PrintString("SCAN", menu_item_x1, menu_item_x2, 4, 8);

#ifdef ENABLE_DTMF_CALLING
    if (UI_MENU_GetCurrentMenuId() == MENU_D_LIST && gIsDtmfContactValid) {
        Contact[11] = 0;
        memcpy(&gDTMF_ID, Contact + 8, 4);
        sprintf(String, "ID:%4s", gDTMF_ID);
        UI_PrintString(String, menu_item_x1, menu_item_x2, 4, 8);
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
        sprintf(String, "%03d", gSubMenuSelection);
        UI_PrintStringSmallNormal(String, 107, 0, 0);
    }

    if ((UI_MENU_GetCurrentMenuId() == MENU_RESET    ||
         UI_MENU_GetCurrentMenuId() == MENU_MEM_CH   ||
         UI_MENU_GetCurrentMenuId() == MENU_MEM_NAME ||
         UI_MENU_GetCurrentMenuId() == MENU_DEL_CH) && gAskForConfirmation)
    {   // display confirmation
        char *pPrintStr = (gAskForConfirmation == 1) ? "SURE?" : "WAIT!";
        UI_PrintString(pPrintStr, menu_item_x1, menu_item_x2, 5, 8);
    }

    // Final pass: for menu names containing "Ch/ch", move right content down and
    // remove the extra top line/artifacts over the content area.
    {
        const uint8_t actual_idx = UI_MENU_GetActualIndexFromCursor(gMenuCursor);
        const bool need_shift_right = (actual_idx < ARRAY_SIZE(MenuList)) &&
                                      (UI_MENU_NameContainsCh(MenuList[actual_idx].name) ||
                                       MenuList[actual_idx].menu_id == MENU_1_CALL);
        if (need_shift_right)
        {
            const uint8_t menu_id = MenuList[actual_idx].menu_id;
            const uint8_t shift_px =
                (menu_id == MENU_MDF)    ? 2 :
                (menu_id == MENU_1_CALL) ? 5 :
                                           6; // default Ch* shift
            UI_MENU_ShiftRightMenuAreaDown((uint8_t)menu_item_x1, shift_px);
            for (uint8_t y = 0; y <= shift_px; y++)
                for (uint8_t x = (uint8_t)menu_item_x1; x < LCD_WIDTH; x++)
                    PutPixel(x, y, false);
            // Keep the top menu-bar separator line continuous.
            UI_DrawLineBuffer(gFrameBuffer, 0, 0, LCD_WIDTH - 1, 0, true);

            // Explicitly ensure right-top line for ChList / PriCh1 / PriCh2.
            if (menu_id == MENU_LIST_CH || menu_id == MENU_S_PRI_CH_1 || menu_id == MENU_S_PRI_CH_2)
                UI_DrawLineBuffer(gFrameBuffer, (uint8_t)menu_item_x1, 0, LCD_WIDTH - 1, 0, true);
        }
    }

    ST7565_BlitFullScreen();
}
