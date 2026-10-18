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

#include "app/app.h"
#include "app/dtmf.h"
#include "app/generic.h"
#include "app/menu.h"
#include "app/scanner.h"
#include "audio.h"
#include "driver/bk4819.h"
#include "frequencies.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "ui/inputbox.h"
#include "ui/ui.h"

DCS_CodeType_t    gScanCssResultType;
uint8_t           gScanCssResultCode;
bool              gScanSingleFrequency; // scan CTCSS/DCS codes for current frequency
SCAN_SaveState_t  gScannerSaveState;
uint16_t          gScanChannel;
uint32_t          gScanFrequency;
SCAN_CssState_t   gScanCssState;
uint8_t           gScanProgressIndicator;
bool              gScanUseCssResult;

STEP_Setting_t    stepSetting;
uint8_t           scanHitCount;

/* 进扫前 ScreenChannel；Stop 写回后再全量重载，避免频率 VFO 被冲成副信道 */
static uint16_t s_scr0, s_scr1;

/* F+4 一键测频：扫频页面按 3 手动切换 VHF/UHF 前端（固定所选段扫频，不再轮扫） */
bool gScanBandIsUhf;

/*
 * F+4 谐波验证：BK4819 频率计数器对强信号（中继/近台，通常带亚音）会因限幅
 * 失真锁到 2/3 次谐波（读数偏高，如 147→295/441），也可能把 UHF 信号欠采样
 * 为 f/3（读数偏低）。锁定后对 f/2、f/3、3f 候选逐个实测 RSSI，仅当某
 * 候选 RSSI 明显强于读数时才改判，否则保留原读数。RSSI 刻度 2 单位/dB。
 */
#define SCAN_VERIFY_RSSI_MARGIN     12u  /* 约 6dB */

static uint8_t  scanVerifyTotal;    /* 候选总数，0 = 未在验证 */
static uint8_t  scanVerifyIdx;
static uint32_t scanVerifyFreq[3];  /* [0] 为原锁频基准；候选至多 3 个 */
static uint16_t scanVerifyRssi[3];

static void SCANNER_StartCssScanAtFrequency(const uint32_t frequency)
{
    gScanFrequency         = frequency;
    gScanCssResultCode     = 0xFF;
    gScanCssResultType     = 0xFF;
    scanHitCount           = 0;
    gScanUseCssResult      = false;
    gScanProgressIndicator = 0;
    gScanCssState          = SCAN_CSS_STATE_SCANNING;
    gScanDelay_10ms        = scan_delay_10ms;

    BK4819_SetScanFrequency(gScanFrequency);

    if (!gCssBackgroundScan)
        GUI_SelectNextDisplay(DISPLAY_SCANNER);

    gUpdateStatus = true;
}

/* 为频率验证调谐到指定频率（含前端选择），并留 20ms 稳定 */
static void SCANNER_TuneForVerification(const uint32_t frequency)
{
    BK4819_SetFrequency(frequency);
    BK4819_PickRXFilterPathBasedOnFrequency(frequency);
    BK4819_RX_TurnOn();
    gScanDelay_10ms = 2;
}

/* 锁定后开始谐波验证；无可验证候选时返回 false */
static bool SCANNER_StartFrequencyVerification(const uint32_t frequency)
{
    static const uint8_t mult[3][2] = {{1, 2}, {1, 3}, {3, 1}};  /* f/2 f/3 3f */

    scanVerifyFreq[0] = frequency;
    scanVerifyTotal   = 1;
    for (uint8_t i = 0; i < 3; i++) {
        const uint32_t f = frequency * mult[i][0] / mult[i][1];
        if (RX_freq_check(f) == 0)
            scanVerifyFreq[scanVerifyTotal++] = f;
    }

    if (scanVerifyTotal < 2)
        return false;

    scanVerifyIdx = 0;
    SCANNER_TuneForVerification(frequency);
    return true;
}

/* 谐波验证状态机：每个 10ms tick 采一个候选，测完判定。返回 true 表示仍在验证 */
static bool SCANNER_HandleFrequencyVerification(void)
{
    if (!scanVerifyTotal)
        return false;

    scanVerifyRssi[scanVerifyIdx] = BK4819_GetRSSI();

    if (++scanVerifyIdx < scanVerifyTotal) {
        SCANNER_TuneForVerification(scanVerifyFreq[scanVerifyIdx]);
        return true;
    }

    /* 全部测完：仅当最强候选明显强于原读数时才改判，打平优先取低频基波 */
    uint8_t best = 0;
    for (uint8_t i = 1; i < scanVerifyTotal; i++)
        if (scanVerifyRssi[i] > scanVerifyRssi[best])
            best = i;

    const uint32_t verified =
        (best && scanVerifyRssi[best] >= scanVerifyRssi[0] + SCAN_VERIFY_RSSI_MARGIN)
        ? scanVerifyFreq[best] : scanVerifyFreq[0];

    scanVerifyTotal = 0;
    SCANNER_StartCssScanAtFrequency(verified);
    return true;
}

static void SCANNER_Key_DIGITS(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
    if (!bKeyHeld && bKeyPressed)
    {
        /* F+4 扫频页面按 3：切换 VHF/UHF 前端并立即重新扫频 */
        if (Key == KEY_3 && !gScanSingleFrequency && gScannerSaveState == SCAN_SAVE_NO_PROMPT) {
            gBeepToPlay            = BEEP_1KHZ_60MS_OPTIONAL;
            gScanBandIsUhf         = !gScanBandIsUhf;
            scanVerifyTotal        = 0;
            scanHitCount           = 0;
            gScanFrequency         = 0xFFFFFFFF;
            gScanCssState          = SCAN_CSS_STATE_OFF;
            gScanCssResultCode     = 0xFF;
            gScanCssResultType     = 0xFF;
            gScanUseCssResult      = false;
            gScanProgressIndicator = 0;

            BK4819_StopScan();
            BK4819_PickRXFilterPathBasedOnFrequency(gScanBandIsUhf ? 43500000u : 14500000u);
            BK4819_EnableFrequencyScan();

            gScanDelay_10ms       = scan_delay_10ms;
            gUpdateStatus         = true;
            gRequestDisplayScreen = DISPLAY_SCANNER;
            return;
        }

        if (gScannerSaveState == SCAN_SAVE_CHAN_SEL) {
            gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

            INPUTBOX_Append(Key);

            gRequestDisplayScreen = DISPLAY_SCANNER;

            if (gInputBoxIndex < 4) {
#ifdef ENABLE_VOICE
                gAnotherVoiceID = (VOICE_ID_t)Key;
#endif
                return;
            }

            gInputBoxIndex = 0;

            // uint16_t chan = ((gInputBox[0] * 100) + (gInputBox[1] * 10) + gInputBox[2]) - 1;
            uint16_t chan = ((gInputBox[0] * 1000) + (gInputBox[1] * 100) + (gInputBox[2] * 10) + gInputBox[3]) - 1;
            if (IS_MR_CHANNEL(chan)) {
#ifdef ENABLE_VOICE
                gAnotherVoiceID = (VOICE_ID_t)Key;
#endif
                gShowChPrefix = RADIO_CheckValidChannel(chan, false, 0);
                gScanChannel  = chan;
                return;
            }
        }

        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
    }
}

static void SCANNER_Key_EXIT(bool bKeyPressed, bool bKeyHeld)
{
    if (!bKeyHeld && bKeyPressed) { // short pressed
        gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

        switch (gScannerSaveState) {
            case SCAN_SAVE_NO_PROMPT:
                SCANNER_Stop();
                gRequestDisplayScreen    = DISPLAY_MAIN;
                break;

            case SCAN_SAVE_CHAN_SEL:
                if (gInputBoxIndex > 0) {
                    gInputBox[--gInputBoxIndex] = 10;
                    gRequestDisplayScreen       = DISPLAY_SCANNER;
                    break;
                }

                // Fallthrough

            case SCAN_SAVE_CHANNEL:
                gScannerSaveState     = SCAN_SAVE_NO_PROMPT;
#ifdef ENABLE_VOICE
                gAnotherVoiceID   = VOICE_ID_CANCEL;
#endif
                gRequestDisplayScreen = DISPLAY_SCANNER;
                break;
        }
    }
}

static void SCANNER_Key_MENU(bool bKeyPressed, bool bKeyHeld)
{
    if (bKeyHeld || !bKeyPressed) // ignore long press or release button events
        return;

    /*
    if (gScanCssState == SCAN_CSS_STATE_OFF && !gScanSingleFrequency) {
        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
        return;
    }

    if (gScanCssState == SCAN_CSS_STATE_SCANNING && gScanSingleFrequency) {
        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
        return;
    }

    if (gScanCssState == SCAN_CSS_STATE_FAILED) {
        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
        return;
    }
    */

    if (gScanCssState == SCAN_CSS_STATE_FAILED ||  
        (!gScanSingleFrequency && gScanCssState == SCAN_CSS_STATE_OFF) ||  
        (gScanSingleFrequency && gScanCssState == SCAN_CSS_STATE_SCANNING))  
    {
        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
        return;
    }

    gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

    switch (gScannerSaveState) {
        case SCAN_SAVE_NO_PROMPT:
            if (!gScanSingleFrequency)
            {
                uint32_t freq250  = FREQUENCY_RoundToStep(gScanFrequency, 250);
                uint32_t freq625  = FREQUENCY_RoundToStep(gScanFrequency, 625);

                uint32_t diff250 = gScanFrequency > freq250 ? gScanFrequency - freq250 : freq250 - gScanFrequency;
                uint32_t diff625 = gScanFrequency > freq625 ? gScanFrequency - freq625 : freq625 - gScanFrequency;

                if(diff250 > diff625) {
                    stepSetting   = STEP_6_25kHz;
                    gScanFrequency = freq625;
                }
                else {
                    stepSetting   = STEP_2_5kHz;
                    gScanFrequency = freq250;
                }
            }

            if (IS_MR_CHANNEL(gTxVfo->CHANNEL_SAVE)) {
                gScannerSaveState = SCAN_SAVE_CHAN_SEL;
                gScanChannel      = gTxVfo->CHANNEL_SAVE;
                gShowChPrefix     = RADIO_CheckValidChannel(gTxVfo->CHANNEL_SAVE, false, 0);
            }
            else {
                gScannerSaveState = SCAN_SAVE_CHANNEL;
            }

            gScanCssState         = SCAN_CSS_STATE_FOUND;
#ifdef ENABLE_VOICE
            gAnotherVoiceID   = VOICE_ID_MEMORY_CHANNEL;
#endif
            gRequestDisplayScreen = DISPLAY_SCANNER;
            
            gUpdateStatus = true;
            break;

        case SCAN_SAVE_CHAN_SEL:
            if (gInputBoxIndex == 0) {
                gBeepToPlay           = BEEP_1KHZ_60MS_OPTIONAL;
                gRequestDisplayScreen = DISPLAY_SCANNER;
                gScannerSaveState     = SCAN_SAVE_CHANNEL;
            }
            break;

        case SCAN_SAVE_CHANNEL:
            if (!gScanSingleFrequency) {
                RADIO_InitInfo(gTxVfo, gTxVfo->CHANNEL_SAVE, gScanFrequency);

                if (gScanUseCssResult) {
                    gTxVfo->freq_config_RX.CodeType = gScanCssResultType;
                    gTxVfo->freq_config_RX.Code     = gScanCssResultCode;
                }

                gTxVfo->freq_config_TX     = gTxVfo->freq_config_RX;
                gTxVfo->STEP_SETTING = stepSetting;
            }
            else {
                /*
                 * 单频扫亚音保存：当前 VFO 频率已在 Start 时 InitInfo 保留，
                 * 直接写入扫到的亚音。不要 RADIO_ConfigureChannel 双端 Flash
                 * 全量重载——双信道（主频/副信道）下会把频率 VFO 冲成副信道。
                 */
                gTxVfo->freq_config_RX.CodeType = gScanCssResultType;
                gTxVfo->freq_config_RX.Code     = gScanCssResultCode;
                gTxVfo->freq_config_TX.CodeType = gScanCssResultType;
                gTxVfo->freq_config_TX.Code     = gScanCssResultCode;
            }

            uint16_t chan;
            if (IS_MR_CHANNEL(gTxVfo->CHANNEL_SAVE)) {
                chan = gScanChannel;
                gEeprom.MrChannel[gEeprom.TX_VFO] = chan;
            }
            else {
                chan = gTxVfo->Band + FREQ_CHANNEL_FIRST;
                gEeprom.FreqChannel[gEeprom.TX_VFO] = chan;
            }

            gTxVfo->CHANNEL_SAVE = chan;
            gEeprom.ScreenChannel[gEeprom.TX_VFO] = chan;
#ifdef ENABLE_VOICE
            gAnotherVoiceID = VOICE_ID_CONFIRM;
#endif
            gRequestDisplayScreen = DISPLAY_SCANNER;
            gRequestSaveChannel = 2;
            gScannerSaveState = SCAN_SAVE_NO_PROMPT;
            break;

        default:
            gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
            break;
    }
}

static void SCANNER_Key_STAR(bool bKeyPressed, bool bKeyHeld)
{
    if (!bKeyHeld && bKeyPressed) {
        gBeepToPlay    = BEEP_1KHZ_60MS_OPTIONAL;
        SCANNER_Start(gScanSingleFrequency);
    }
    return;
}

static void SCANNER_Key_UP_DOWN(bool bKeyPressed, bool pKeyHeld, int8_t Direction)
{
    if (pKeyHeld) {
        if (!bKeyPressed)
            return;
    }
    else {
        if (!bKeyPressed)
            return;

        gInputBoxIndex = 0;
        gBeepToPlay    = BEEP_1KHZ_60MS_OPTIONAL;
    }

    if (!gEeprom.SET_NAV) {
        Direction = -Direction;
    }

    if (gScannerSaveState == SCAN_SAVE_CHAN_SEL) {
        gScanChannel          = NUMBER_AddWithWraparound(gScanChannel, Direction, 0, MR_CHANNEL_LAST);
        gShowChPrefix         = RADIO_CheckValidChannel(gScanChannel, false, 0);
        gRequestDisplayScreen = DISPLAY_SCANNER;
    }
    else
        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
}

void SCANNER_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
    switch (Key) {
        case KEY_0...KEY_9:
            SCANNER_Key_DIGITS(Key, bKeyPressed, bKeyHeld);
            break;
        case KEY_MENU:
            SCANNER_Key_MENU(bKeyPressed, bKeyHeld);
            break;
        case KEY_UP:
        case KEY_DOWN:
            SCANNER_Key_UP_DOWN(bKeyPressed, bKeyHeld, Key == KEY_UP ? 1 : -1);
            break;
        case KEY_EXIT:
            SCANNER_Key_EXIT(bKeyPressed, bKeyHeld);
            break;
        case KEY_STAR:
            SCANNER_Key_STAR(bKeyPressed, bKeyHeld);
            break;
        case KEY_PTT:
            GENERIC_Key_PTT(bKeyPressed);
            break;
        default:
            if (!bKeyHeld && bKeyPressed)
                gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
            break;
    }
}

void SCANNER_Start(bool singleFreq)
{
    gScanSingleFrequency = singleFreq;
    gMonitor = false;

#ifdef ENABLE_VOICE
    gAnotherVoiceID = VOICE_ID_SCANNING_BEGIN;
#endif

    BK4819_StopScan();
    RADIO_SelectVfos();

    s_scr0 = gEeprom.ScreenChannel[0];
    s_scr1 = gEeprom.ScreenChannel[1];

#ifdef ENABLE_NOAA
    if (IS_NOAA_CHANNEL(gRxVfo->CHANNEL_SAVE))
        gRxVfo->CHANNEL_SAVE = FREQ_CHANNEL_FIRST + BAND6_400MHz;
#endif

    uint8_t  backupStep      = gRxVfo->STEP_SETTING;
    uint16_t backupFrequency = gRxVfo->StepFrequency;
    /*
     * RADIO_InitInfo 会 memset 整份 VFO 并把 OUTPUT_POWER 置为 LOW1 等默认值，
     * 仅用于扫频时重配硬件；须保留用户功率与带宽，否则顶栏/ EEPROM 镜像与进测频前不一致。
     */
    const uint8_t backup_output_power    = gRxVfo->OUTPUT_POWER;
    const uint8_t backup_channel_bw      = gRxVfo->CHANNEL_BANDWIDTH;

    RADIO_InitInfo(gRxVfo, gRxVfo->CHANNEL_SAVE, gRxVfo->pRX->Frequency);

    gRxVfo->STEP_SETTING         = backupStep;
    gRxVfo->StepFrequency        = backupFrequency;
    gRxVfo->OUTPUT_POWER         = backup_output_power;
    gRxVfo->CHANNEL_BANDWIDTH    = backup_channel_bw;

    RADIO_ConfigureSquelchAndOutputPower(gRxVfo);
    RADIO_SetupRegisters(true);

#ifdef ENABLE_NOAA
    gIsNoaaMode = false;
#endif

    if (gScanSingleFrequency) {
        gScanCssState  = SCAN_CSS_STATE_SCANNING;
        gScanFrequency = gRxVfo->pRX->Frequency;
        stepSetting   = gRxVfo->STEP_SETTING;

        BK4819_PickRXFilterPathBasedOnFrequency(gScanFrequency);
        BK4819_SetScanFrequency(gScanFrequency);

        gUpdateStatus = true;
    }
    else {
        gScanCssState      = SCAN_CSS_STATE_OFF;
        gScanFrequency     = 0xFFFFFFFF;

        /* 默认跟随当前接收频率所在段（280MHz 分界）；扫频页面按 3 切换 */
        gScanBandIsUhf     = (gRxVfo->pRX->Frequency >= 28000000u);
        BK4819_PickRXFilterPathBasedOnFrequency(gScanBandIsUhf ? 43500000u : 14500000u);
        BK4819_EnableFrequencyScan();

        gUpdateStatus = true;
    }

#ifdef ENABLE_DTMF_CALLING
    DTMF_clear_RX();
#endif

    gScanDelay_10ms        = scan_delay_10ms;
    gScanCssResultCode     = 0xFF;
    gScanCssResultType     = 0xFF;
    scanHitCount           = 0;
    scanVerifyTotal        = 0;
    gScanUseCssResult      = false;
    g_CxCSS_TAIL_Found     = false;
    g_CDCSS_Lost           = false;
    gCDCSSCodeType         = 0;
    g_CTCSS_Lost           = false;
#ifdef ENABLE_VOX
    g_VOX_Lost             = false;
#endif
    g_SquelchLost          = false;
    gScannerSaveState      = SCAN_SAVE_NO_PROMPT;
    gScanProgressIndicator = 0;
}

void SCANNER_Stop(void)
{
    if(SCANNER_IsScanning()) {
        gEeprom.CROSS_BAND_RX_TX = gBackup_CROSS_BAND_RX_TX;
        gVfoConfigureMode        = VFO_CONFIGURE_RELOAD;
        gFlagResetVfos           = true;
        /* 用户已保存时 gRequestSaveChannel>0，保留保存后的 ScreenChannel */
        if (!gRequestSaveChannel) {
            gEeprom.ScreenChannel[0] = s_scr0;
            gEeprom.ScreenChannel[1] = s_scr1;
        }
        gUpdateStatus            = true;
        gCssBackgroundScan       = false;
        scanVerifyTotal          = 0;
        gScanUseCssResult        = false;
#ifdef ENABLE_VOICE
        gAnotherVoiceID          = VOICE_ID_CANCEL;
#endif
        BK4819_StopScan();
    }
}

void SCANNER_TimeSlice10ms(void)
{
    if (!SCANNER_IsScanning())
        return;

    if (gScanDelay_10ms > 0) {
        gScanDelay_10ms--;
        return;
    }

    if (gScannerSaveState != SCAN_SAVE_NO_PROMPT) {
        return;
    }

    /* 锁定后的谐波验证状态机优先于扫描状态 */
    if (SCANNER_HandleFrequencyVerification())
        return;

    switch (gScanCssState) {
        case SCAN_CSS_STATE_OFF: {
            // must be RF frequency scanning if we're here ?
            uint32_t result;
            if (!BK4819_GetFrequencyScanResult(&result))
                break; /* 芯片仍在扫：固定当前所选前端等结果 */

            int32_t delta = result - gScanFrequency;
            gScanFrequency = result;

            if (delta < 0)
                delta = -delta;
            if (delta < 100)
                scanHitCount++;
            else
                scanHitCount = 0;

            BK4819_DisableFrequencyScan();

            if (scanHitCount < 3) {
                BK4819_EnableFrequencyScan();
                gScanDelay_10ms = scan_delay_10ms;
            }
            else if (SCANNER_StartFrequencyVerification(gScanFrequency)) {
                /* 验证状态机接管，延迟已在其中设置 */
            }
            else {
                SCANNER_StartCssScanAtFrequency(gScanFrequency);
                gScanDelay_10ms = scan_delay_10ms;
            }
            //gScanDelay_10ms = 1;   // 10ms
            break;
        }
        case SCAN_CSS_STATE_SCANNING: {
            uint32_t cdcssFreq;
            uint16_t ctcssFreq;
            BK4819_CssScanResult_t scanResult = BK4819_GetCxCSSScanResult(&cdcssFreq, &ctcssFreq);
            if (scanResult == BK4819_CSS_RESULT_NOT_FOUND)
                break;

            BK4819_Disable();

            if (scanResult == BK4819_CSS_RESULT_CDCSS) {
                const uint8_t Code = DCS_GetCdcssCode(cdcssFreq);
                if (Code != 0xFF)
                {
                    gScanCssResultCode = Code;
                    gScanCssResultType = CODE_TYPE_DIGITAL;
                    gScanCssState      = SCAN_CSS_STATE_FOUND;
                    gScanUseCssResult  = true;
                    gUpdateStatus      = true;
                }
            }
            else if (scanResult == BK4819_CSS_RESULT_CTCSS) {
                const uint8_t Code = DCS_GetCtcssCode(ctcssFreq);
                if (Code != 0xFF) {
                    if (Code == gScanCssResultCode && gScanCssResultType == CODE_TYPE_CONTINUOUS_TONE) {
                        if (++scanHitCount >= 2) {
                            gScanCssState     = SCAN_CSS_STATE_FOUND;
                            gScanUseCssResult = true;
                            gUpdateStatus     = true;
                        }
                    }
                    else
                        scanHitCount = 0;

                    gScanCssResultType = CODE_TYPE_CONTINUOUS_TONE;
                    gScanCssResultCode = Code;
                }
            }

            if (gScanCssState < SCAN_CSS_STATE_FOUND) { // scanning or off
                BK4819_SetScanFrequency(gScanFrequency);
                gScanDelay_10ms = scan_delay_10ms;
                break;
            }

            if(gCssBackgroundScan) {
                gCssBackgroundScan = false;
                if(gScanUseCssResult)
                    MENU_CssScanFound();
            }
            else
                GUI_SelectNextDisplay(DISPLAY_SCANNER);


            break;
        }
        default:
            gCssBackgroundScan = false;
            break;
    }

}

void SCANNER_TimeSlice500ms(void)
{
    if (SCANNER_IsScanning() && gScannerSaveState == SCAN_SAVE_NO_PROMPT && gScanCssState < SCAN_CSS_STATE_FOUND) {
        gScanProgressIndicator++;
#ifndef ENABLE_NO_CODE_SCAN_TIMEOUT
        if (gScanProgressIndicator > 32) {
            if (gScanCssState == SCAN_CSS_STATE_SCANNING && !gScanSingleFrequency)
                gScanCssState = SCAN_CSS_STATE_FOUND;
            else
                gScanCssState = SCAN_CSS_STATE_FAILED;

            gUpdateStatus = true;
        }
#endif
        gUpdateDisplay = true;
    }
    else if(gCssBackgroundScan) {
        gUpdateDisplay = true;
    }
}

bool SCANNER_IsScanning(void)
{
    return gCssBackgroundScan || (gScreenToDisplay == DISPLAY_SCANNER);
}