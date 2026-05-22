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

#include <assert.h>

#include "battery.h"
#include "driver/backlight.h"
#include "driver/st7565.h"
#include "functions.h"
#include "misc.h"
#include "settings.h"
#include "ui/battery.h"
#include "ui/menu.h"
#include "ui/ui.h"
//#include "debugging.h"

uint16_t          gBatteryCalibration[6];
uint16_t          gBatteryCurrentVoltage;
uint16_t          gBatteryCurrent;
uint16_t          gBatteryVoltages[4];
uint16_t          gBatteryVoltageAverage;
uint8_t           gBatteryDisplayLevel;
uint8_t           gBatteryIconFillPercent;
bool              gChargingWithTypeC;
bool              gLowBatteryBlink;
bool              gLowBattery;
bool              gLowBatteryConfirmed;
uint16_t          gBatteryCheckCounter;

bool              gBatteryUpdatePaused = false;

uint16_t          gBatteryUpdateDelayCountdown = 0;

typedef enum {
    BATTERY_LOW_INACTIVE,
    BATTERY_LOW_ACTIVE,
    BATTERY_LOW_CONFIRMED
} BatteryLow_t;

uint16_t          lowBatteryCountdown;
const uint16_t    lowBatteryPeriod = 30;

volatile uint16_t gPowerSave_10ms;

const uint16_t Voltage2PercentageTable[][7][2] = {
    [BATTERY_TYPE_1600_MAH] = {
        {828, 100},
        {814, 97 },
        {760, 25 },
        {729, 6  },
        {630, 0  },
        {0,   0  },
        {0,   0  },
    },

    [BATTERY_TYPE_2200_MAH] = {
        {832, 100},
        {813, 95 },
        {740, 60 },
        {707, 21 },
        {682, 5  },
        {630, 0  },
        {0,   0  },
    },

    [BATTERY_TYPE_3500_MAH] = {
        {837, 100},
        {826, 95 },
        {750, 50 },
        {700, 25 },
        {620, 5  },
        {600, 0  },
        {0,   0  },
    },

    // Estimated discharge curve for 1500 mAh K1 battery (improve this)
    [BATTERY_TYPE_1500_MAH] = {
        {828, 100},  // Fully charged (measured ~8.28V)
        {813, 97 },  // Top end
        {758, 25 },  // Mid level
        {726, 6  },  // Almost empty
        {630, 0  },  // Fully discharged (conservative)
        {0,   0  },
        {0,   0  },
    },

    // Estimated discharge curve for 2500 mAh K1 battery (improve this)
    [BATTERY_TYPE_2500_MAH] = {
        {839, 100},  // Fully charged (measured ~8.39V)
        {818, 95 },  // Top end (slightly raised vs 816)
        {745, 55 },  // Mid range
        {703, 25 },  // Low level
        {668, 5  },  // Almost empty
        {623, 0  },  // Fully discharged (between 630 and 600)
        {0,   0  },
    },
};

/* Useless (for compilator only)
static_assert(
    (ARRAY_SIZE(Voltage2PercentageTable[BATTERY_TYPE_1600_MAH]) ==
    ARRAY_SIZE(Voltage2PercentageTable[BATTERY_TYPE_2200_MAH])) &&
    (ARRAY_SIZE(Voltage2PercentageTable[BATTERY_TYPE_2200_MAH]) ==
    ARRAY_SIZE(Voltage2PercentageTable[BATTERY_TYPE_3500_MAH]))
    );
*/

unsigned int BATTERY_VoltsToPercent(const unsigned int voltage_10mV)
{
    const uint16_t (*crv)[2] = Voltage2PercentageTable[gEeprom.BATTERY_TYPE];
    const int mulipl = 1000;
    for (unsigned int i = 1; i < ARRAY_SIZE(Voltage2PercentageTable[BATTERY_TYPE_2200_MAH]); i++) {
        if (voltage_10mV > crv[i][0]) {
            const int a = (crv[i - 1][1] - crv[i][1]) * mulipl / (crv[i - 1][0] - crv[i][0]);
            const int b = crv[i][1] - a * crv[i][0] / mulipl;
            const int p = a * voltage_10mV / mulipl + b;
            return MIN(MAX(p, 0), 100);
        }
    }

    return 0;
}

void BATTERY_GetReadings(const bool bDisplayBatteryLevel)
{
    if (gBatteryUpdatePaused) {
        return;
    }

    const uint8_t  PreviousBatteryLevel = gBatteryDisplayLevel;
    const uint16_t Voltage              = (gBatteryVoltages[0] + gBatteryVoltages[1] + gBatteryVoltages[2] + gBatteryVoltages[3]) / 4;

    gBatteryVoltageAverage = (Voltage * 760) / gBatteryCalibration[3];

    if(gBatteryVoltageAverage > 890)
        gBatteryDisplayLevel = 7; // battery overvoltage
    else if(gBatteryVoltageAverage < 630 && (gEeprom.BATTERY_TYPE == BATTERY_TYPE_1600_MAH || gEeprom.BATTERY_TYPE == BATTERY_TYPE_2200_MAH))
        gBatteryDisplayLevel = 0; // battery critical
    else if(gBatteryVoltageAverage < 600 && (gEeprom.BATTERY_TYPE == BATTERY_TYPE_3500_MAH))
        gBatteryDisplayLevel = 0; // battery critical
    else {
        gBatteryDisplayLevel = 1;
        const uint8_t levels[] = {5,17,41,65,88};
        uint8_t perc = BATTERY_VoltsToPercent(gBatteryVoltageAverage);
        //char str[64];
        //LogUart("----------\n");
        //sprintf(str, "%d %d %d %d %d %d %d\n", gBatteryVoltages[0], gBatteryVoltages[1], gBatteryVoltages[2], gBatteryVoltages[3], Voltage, gBatteryVoltageAverage, perc);
        //LogUart(str);

        for(uint8_t i = 6; i >= 2; i--){
            //sprintf(str, "%d %d %d\n", perc, levels[i-2], i);
            //LogUart(str);
            if (perc > levels[i-2]) {
                gBatteryDisplayLevel = i;
                break;
            }
        }
    }

    /*
     * 电池图标填充（gBatteryIconFillPercent）：仍按电量曲线换算为 0～100。
     * - BatTxt=百分比：整数百分比变化 1% 时更新并触发刷新。
     * - BatTxt=无/电压：平均电压每变化 0.1V（10×10mV）时更新并触发刷新。
     */
    {
        static uint8_t  s_last_battery_percent_snapshot = 255u;
        static uint16_t s_last_voltage_bucket_for_icon = 0xFFFFu;
        static uint8_t  s_last_battery_text_mode        = 255u;

        const uint8_t bat_txt_mode = gSetting_battery_text;

        if (bat_txt_mode != s_last_battery_text_mode) {
            s_last_battery_text_mode        = bat_txt_mode;
            s_last_battery_percent_snapshot = 255u;
            s_last_voltage_bucket_for_icon  = 0xFFFFu;
        }

        const unsigned int voltage_for_percent = gBatteryVoltageAverage;
        unsigned int       percent_snapshot_u  = BATTERY_VoltsToPercent(voltage_for_percent);
        if (percent_snapshot_u > 100u) {
            percent_snapshot_u = 100u;
        }
        const uint8_t percent_snapshot = (uint8_t)percent_snapshot_u;

        bool icon_fill_changed = false;

        if (bat_txt_mode == 2u) {
            if (percent_snapshot != s_last_battery_percent_snapshot) {
                s_last_battery_percent_snapshot = percent_snapshot;
                icon_fill_changed               = true;
            }
        } else {
            const uint16_t voltage_centivolts = gBatteryVoltageAverage;
            const uint16_t voltage_bucket     = (uint16_t)(voltage_centivolts / 10u);

            if (voltage_bucket != s_last_voltage_bucket_for_icon) {
                s_last_voltage_bucket_for_icon = voltage_bucket;
                icon_fill_changed              = true;
            }
        }

        if (icon_fill_changed) {
            gBatteryIconFillPercent = percent_snapshot;
            gUpdateStatus           = true;
#ifdef ENABLE_FMRADIO
            const bool is_fm_screen = (gScreenToDisplay == DISPLAY_FM);
#else
            const bool is_fm_screen = false;
#endif
#ifdef ENABLE_FEAT_F4HWN
            const bool is_scanner_screen = (gScreenToDisplay == DISPLAY_SCANNER);
#else
            const bool is_scanner_screen = false;
#endif
            if (!is_fm_screen && !is_scanner_screen) {
                gUpdateDisplay = true;
            }
        }
    }

    if ((gScreenToDisplay == DISPLAY_MENU) && UI_MENU_GetCurrentMenuId() == MENU_VOL)
        gUpdateDisplay = true;

    if (gBatteryCurrent < 501)
    {
        if (gChargingWithTypeC)
        {
            gUpdateStatus  = true;
            gUpdateDisplay = true;
        }

        gChargingWithTypeC = false;
    }
    else
    {
        if (!gChargingWithTypeC)
        {
            gUpdateStatus  = true;
            gUpdateDisplay = true;
            BACKLIGHT_TurnOn();
        }

        gChargingWithTypeC = true;
    }

    if (PreviousBatteryLevel != gBatteryDisplayLevel)
    {
        if(gBatteryDisplayLevel > 2)
            gLowBatteryConfirmed = false;
        else if (gBatteryDisplayLevel < 2)
        {
            gLowBattery = true;
        }
        else
        {
            gLowBattery = false;

            if (bDisplayBatteryLevel)
                UI_DisplayBattery(gBatteryDisplayLevel, gLowBatteryBlink);
        }

        if(!gLowBatteryConfirmed)
            gUpdateDisplay = true;

        lowBatteryCountdown = 0;
    }
}

void BATTERY_TimeSlice500ms(void)
{
    if (!gLowBattery) {
        return;
    }

    gLowBatteryBlink = ++lowBatteryCountdown & 1;

    if (!gLowBatteryConfirmed) {
        UI_DisplayBattery(0, gLowBatteryBlink);
    } else {
        gUpdateStatus = true;
    }

    if (gCurrentFunction == FUNCTION_TRANSMIT) {
        return;
    }

    // not transmitting

    if (lowBatteryCountdown < lowBatteryPeriod) {
        if (lowBatteryCountdown == lowBatteryPeriod-1 && !gChargingWithTypeC && !gLowBatteryConfirmed) {
            AUDIO_PlayBeep(BEEP_500HZ_60MS_DOUBLE_BEEP);
        }
        return;
    }

    lowBatteryCountdown = 0;

    if (gChargingWithTypeC) {
        return;
    }

    // not on charge
    if (!gLowBatteryConfirmed) {
        AUDIO_PlayBeep(BEEP_500HZ_60MS_DOUBLE_BEEP);
#ifdef ENABLE_VOICE
        AUDIO_SetVoiceID(0, VOICE_ID_LOW_VOLTAGE);
#endif
    }

    if (gBatteryDisplayLevel != 0) {
#ifdef ENABLE_VOICE
        AUDIO_PlaySingleVoice(false);
#endif
        return;
    }

#ifdef ENABLE_VOICE
    AUDIO_PlaySingleVoice(true);
#endif

    gReducedService = true;

    FUNCTION_Select(FUNCTION_POWER_SAVE);

    ST7565_HardwareReset();

    if (gEeprom.BACKLIGHT_TIME < 61) {
        BACKLIGHT_TurnOff();
    }
}
