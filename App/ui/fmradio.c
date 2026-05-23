/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
#include "ui/dualvfo_u8g2_freq.h"

#ifdef ENABLE_CHINESE
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
    UI_DrawLineBuffer(gFrameBuffer, 0, 0, (int16_t)(LCD_WIDTH - 1u), 0, true);
#endif

    const uint16_t freqMin = BK1080_GetFreqLoLimit(gEeprom.FM_Band);
    const uint16_t freqMax = BK1080_GetFreqHiLimit(gEeprom.FM_Band);

    sprintf(String, "%d%s-%dMHz", 
        freqMin / 10,
        gEeprom.FM_Band == 0 ? ".5" : "",
        freqMax / 10
    );
    
#ifdef ENABLE_FEAT_F4HWN
    {
        const uint8_t text_width = DualVfoU8g2_GetSmallTextWidth(String);
        const uint8_t x = (uint8_t)(LCD_WIDTH - text_width - 2);
        DualVfoU8g2_DrawSmallText(String, x, 2, true);
    }
#else
    UI_PrintStringSmallNormal(String, 1, 0, 0);
#endif

    if (gEeprom.FM_IsMrMode) {
        int currentIdx = (int)gEeprom.FM_SelectedChannel;
        int prev2Idx = -1, prev1Idx = -1, next1Idx = -1, next2Idx = -1;
        
        for (int i = currentIdx - 1; i >= 0; i--) {
            if (FM_CheckValidChannel((uint8_t)i)) {
                if (prev1Idx == -1) prev1Idx = i;
                else if (prev2Idx == -1) {
                    prev2Idx = i;
                    break;
                }
            }
        }
        
        for (int i = currentIdx + 1; i < FM_CHANNELS_MAX; i++) {
            if (FM_CheckValidChannel((uint8_t)i)) {
                if (next1Idx == -1) next1Idx = i;
                else if (next2Idx == -1) {
                    next2Idx = i;
                    break;
                }
            }
        }
        
        if (prev2Idx >= 0) {
            sprintf(String, "%3d.%d", gFM_Channels[prev2Idx] / 10, gFM_Channels[prev2Idx] % 10);
#ifdef ENABLE_FEAT_F4HWN
            DualVfoU8g2_DrawSmallText(String, 2, 10, true);
#else
            UI_PrintStringSmallNormal(String, 1, 0, 1);
#endif
        }
        
        if (prev1Idx >= 0) {
            sprintf(String, "%3d.%d", gFM_Channels[prev1Idx] / 10, gFM_Channels[prev1Idx] % 10);
#ifdef ENABLE_FEAT_F4HWN
            DualVfoU8g2_DrawSmallText(String, 7, 18, true);
#else
            UI_PrintStringSmallNormal(String, 1, 0, 2);
#endif
        }
        
        if (next1Idx >= 0) {
            sprintf(String, "%3d.%d", gFM_Channels[next1Idx] / 10, gFM_Channels[next1Idx] % 10);
#ifdef ENABLE_FEAT_F4HWN
            DualVfoU8g2_DrawSmallText(String, 7, 42, true);
#else
            UI_PrintStringSmallNormal(String, 1, 0, 5);
#endif
        }
        
        if (next2Idx >= 0) {
            sprintf(String, "%3d.%d", gFM_Channels[next2Idx] / 10, gFM_Channels[next2Idx] % 10);
#ifdef ENABLE_FEAT_F4HWN
            DualVfoU8g2_DrawSmallText(String, 2, 50, true);
#else
            UI_PrintStringSmallNormal(String, 1, 0, 6);
#endif
        }
    } else {
        uint16_t freq_m2 = (gEeprom.FM_FrequencyPlaying >= freqMin + 2) ? gEeprom.FM_FrequencyPlaying - 2 : freqMin;
        uint16_t freq_m1 = (gEeprom.FM_FrequencyPlaying >= freqMin + 1) ? gEeprom.FM_FrequencyPlaying - 1 : freqMin;
        uint16_t freq_p1 = (gEeprom.FM_FrequencyPlaying <= freqMax - 1) ? gEeprom.FM_FrequencyPlaying + 1 : freqMax;
        uint16_t freq_p2 = (gEeprom.FM_FrequencyPlaying <= freqMax - 2) ? gEeprom.FM_FrequencyPlaying + 2 : freqMax;
        
        sprintf(String, "%3d.%d", freq_m2 / 10, freq_m2 % 10);
#ifdef ENABLE_FEAT_F4HWN
        DualVfoU8g2_DrawSmallText(String, 2, 10, true);
#else
        UI_PrintStringSmallNormal(String, 1, 0, 1);
#endif
        
        sprintf(String, "%3d.%d", freq_m1 / 10, freq_m1 % 10);
#ifdef ENABLE_FEAT_F4HWN
        DualVfoU8g2_DrawSmallText(String, 7, 18, true);
#else
        UI_PrintStringSmallNormal(String, 1, 0, 2);
#endif
        
        sprintf(String, "%3d.%d", freq_p1 / 10, freq_p1 % 10);
#ifdef ENABLE_FEAT_F4HWN
        DualVfoU8g2_DrawSmallText(String, 7, 42, true);
#else
        UI_PrintStringSmallNormal(String, 1, 0, 5);
#endif
        
        sprintf(String, "%3d.%d", freq_p2 / 10, freq_p2 % 10);
#ifdef ENABLE_FEAT_F4HWN
        DualVfoU8g2_DrawSmallText(String, 2, 50, true);
#else
        UI_PrintStringSmallNormal(String, 1, 0, 6);
#endif
    }

    memset(String, 0, sizeof(String));
    if (gInputBoxIndex == 0) {
        sprintf(String, "%3d.%d", gEeprom.FM_FrequencyPlaying / 10, gEeprom.FM_FrequencyPlaying % 10);
    } else {
        const char * ascii = INPUTBOX_GetAscii();
        sprintf(String, "%.3s.%.1s", ascii, ascii + 3);
    }
    
    UI_DisplayFrequency(String, 2, 3, false);

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
        UI_PrintStringSmallAtPixel(pPrintStr, 61, LCD_WIDTH - 1, 34, 46, 0);
    else
#endif
        UI_PrintStringSmallNormalAt(pPrintStr, 70, 28);

#ifdef ENABLE_FEAT_F4HWN
    if (gEeprom.KEY_LOCK && gKeypadLocked > 0) {
        UI_DisplayUnlockKeyboard(-10);
    }
#endif

    ST7565_BlitFullScreen();
}

#endif
