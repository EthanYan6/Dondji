/* Copyright 2023 fagci
 * https://github.com/fagci
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
#include "app/spectrum.h"
#include "am_fix.h"
#include "audio.h"
#include "misc.h"

#ifdef ENABLE_SCAN_RANGES
#include "chFrScanner.h"
#endif

#include "board.h"
#include "driver/backlight.h"
#include "frequencies.h"
#include "helper/battery.h"
#include "ui/helper.h"
#include "ui/main.h"
#include "ui/status.h"

#ifdef ENABLE_FEAT_F4HWN
#include "ui/dualvfo_u8g2_freq.h"
#endif

#ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
#include "screenshot.h"
#endif

#ifdef ENABLE_FEAT_F4HWN_SPECTRUM
#include "driver/py25q16.h"
#endif

struct FrequencyBandInfo
{
    uint32_t lower;
    uint32_t upper;
    uint32_t middle;
};

#define F_MIN frequencyBandTable[0].lower
#define F_MAX frequencyBandTable[BAND_N_ELEM - 1].upper

const uint16_t RSSI_MAX_VALUE = 65535;

static uint32_t initialFreq;
static char String[32];

static bool isInitialized = false;
bool isListening = true;
bool monitorMode = false;
bool redrawStatus = true;
bool redrawScreen = false;
bool newScanStart = true;
bool preventKeypress = true;
bool audioState = true;
bool lockAGC = false;

State currentState = SPECTRUM, previousState = SPECTRUM;

PeakInfo peak;
ScanInfo scanInfo;
static KeyboardState kbd = {KEY_INVALID, KEY_INVALID, 0};

#ifdef ENABLE_SCAN_RANGES
static uint16_t blacklistFreqs[15];
static uint8_t blacklistFreqsIdx;
#endif

const char *bwOptions[] = {"25", "12.5", "6.25"};
const uint8_t modulationTypeTuneSteps[] = {100, 50, 10};
const uint8_t modTypeReg47Values[] = {1, 7, 5};

SpectrumSettings settings = {.stepsCount = STEPS_64,
                             .scanStepIndex = S_STEP_25_0kHz,
                             .frequencyChangeStep = 80000,
                             .scanDelay = 3200,
                             .rssiTriggerLevel = 150,
                             .backlightState = true,
                             .bw = BK4819_FILTER_BW_WIDE,
                             .listenBw = BK4819_FILTER_BW_WIDE,
                             .modulationType = false,
                             .dbMin = -130,
                             .dbMax = -50};

uint32_t fMeasure = 0;
uint32_t currentFreq, tempFreq;
uint16_t rssiHistory[128];

#define WATERFALL_HISTORY_DEPTH  16U
#define WATERFALL_COLOR_LEVELS   16U
#define WF_FLOOR_MIN_LEVEL       2U
static uint8_t waterfallHistory[128][WATERFALL_HISTORY_DEPTH / 2];
static uint8_t waterfallIndex = 0;
static uint16_t scanReg30 = 0;
static uint8_t renderTimer = 0;
static uint8_t renderPage = 0;
#define RENDER_PERIOD_TICKS 20
#define FRAME_LINES 8

int vfo;
uint8_t freqInputIndex = 0;
uint8_t freqInputDotIndex = 0;
KEY_Code_t freqInputArr[10];
char freqInputString[11];

uint8_t menuState = 0;
uint16_t listenT = 0;

RegisterSpec registerSpecs[] = {
    {},
    {"LNAs", BK4819_REG_13, 8, 0b11, 1},
    {"LNA", BK4819_REG_13, 5, 0b111, 1},
    {"PGA", BK4819_REG_13, 0, 0b111, 1},
    //{"BPF", BK4819_REG_3D, 0, 0xFFFF, 0x2aaa},
    // {"MIX", 0x13, 3, 0b11, 1}, // TODO: hidden
};

#ifdef ENABLE_FEAT_F4HWN_SPECTRUM
const int8_t LNAsOptions[] = {-19, -16, -11, 0};
const int8_t LNAOptions[] = {-24, -19, -14, -9, -6, -4, -2, 0};
const int8_t VGAOptions[] = {-33, -27, -21, -15, -9, -6, -3, 0};
//const char *BPFOptions[] = {"8.46", "7.25", "6.35", "5.64", "5.08", "4.62", "4.23"};

/* 顶栏信道名绘制区，清除宽度勿覆盖右侧与 MAIN ONLY 一致的电池与百分比 */
#define SPECTRUM_STATUS_CH_NAME_X0    46u
#define SPECTRUM_STATUS_CH_NAME_CLR_W 48u
/* 右上角电池+百分比区域保留。100% 时文本会更靠左，需保留到 x=94 避免把“1”擦掉 */
#define SPECTRUM_STATUS_RIGHT_RESERVED_X 94u

static void LoadSettings()
{
    uint8_t Data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    PY25Q16_ReadBuffer(0x00A148, Data, sizeof(Data));

    // Data[0]: scanStepIndex (7:4), stepsCount (3:2), listenBw (1:0)
    settings.scanStepIndex = (Data[0] >> 4) & 0x0F;

    if (settings.scanStepIndex > 14)
    {
        settings.scanStepIndex = S_STEP_25_0kHz;
    }

    settings.stepsCount = (Data[0] >> 2) & 0x03;

    if (settings.stepsCount > 3)
    {
        settings.stepsCount = STEPS_64;
    }

    settings.listenBw = Data[0] & 0x03;

    if (settings.listenBw > 2)
    {
        settings.listenBw = BK4819_FILTER_BW_WIDE;
    }

    // Data[1]: dbMax encoded as (dbMax + 130) / 5
    if (Data[1] <= 28)
        settings.dbMax = (int)Data[1] * 5 - 130;

    // Data[2]: rssiTriggerLevel as uint8_t (0xFF = auto)
    settings.rssiTriggerLevel = (Data[2] == 0xFF) ? RSSI_MAX_VALUE : Data[2];

    // Data[3] ~ Data[7] are free (for the moment...)
}

static void SaveSettings()
{
    uint8_t Data[8] = {0};
    PY25Q16_ReadBuffer(0x00A148, Data, sizeof(Data));

    // Data[0]: scanStepIndex (7:4), stepsCount (3:2), listenBw (1:0)
    Data[0] = (settings.scanStepIndex << 4) | (settings.stepsCount << 2) | settings.listenBw;

    // Data[1]: dbMax encoded as (dbMax + 130) / 5
    Data[1] = (uint8_t)((settings.dbMax + 130) / 5);

    // Data[2]: rssiTriggerLevel as uint8_t (0xFF = auto)
    Data[2] = (settings.rssiTriggerLevel == RSSI_MAX_VALUE) ? 0xFF : (uint8_t)settings.rssiTriggerLevel;

    PY25Q16_WriteBuffer(0x00A148, Data, sizeof(Data));
}
#endif

static uint8_t DBm2S(int dbm)
{
    uint8_t i = 0;
    dbm *= -1;
    for (i = 0; i < ARRAY_SIZE(U8RssiMap); i++)
    {
        if (dbm >= U8RssiMap[i])
        {
            return i;
        }
    }
    return i;
}

static int Rssi2DBm(uint16_t rssi)
{
    const uint8_t band = (gRxVfo->Band < BAND_N_ELEM) ? gRxVfo->Band : BAND6_400MHz;
    return (rssi / 2) - 160 + dBmCorrTable[band];
}

static uint16_t GetRegMenuValue(uint8_t st)
{
    RegisterSpec s = registerSpecs[st];
    return (BK4819_ReadRegister(s.num) >> s.offset) & s.mask;
}

void LockAGC()
{
    //RADIO_SetupAGC(settings.modulationType == MODULATION_AM, lockAGC);
    RADIO_SetupAGC(false, lockAGC);
    //lockAGC = true;
    lockAGC = false;
}

static void SetRegMenuValue(uint8_t st, bool add)
{
    uint16_t v = GetRegMenuValue(st);
    RegisterSpec s = registerSpecs[st];

    if (s.num == BK4819_REG_13)
        LockAGC();

    uint16_t reg = BK4819_ReadRegister(s.num);
    if (add && v <= s.mask - s.inc)
    {
        v += s.inc;
    }
    else if (!add && v >= 0 + s.inc)
    {
        v -= s.inc;
    }
    // TODO: use max value for bits count in max value, or reset by additional
    // mask in spec
    reg &= ~(s.mask << s.offset);
    BK4819_WriteRegister(s.num, reg | (v << s.offset));
    redrawScreen = true;
}

// GUI functions

#ifndef ENABLE_FEAT_F4HWN
static void PutPixel(uint8_t x, uint8_t y, bool fill)
{
    UI_DrawPixelBuffer(gFrameBuffer, x, y, fill);
}
static void PutPixelStatus(uint8_t x, uint8_t y, bool fill)
{
    UI_DrawPixelBuffer(&gStatusLine, x, y, fill);
}
#endif

#ifndef ENABLE_FEAT_F4HWN
static void GUI_DisplaySmallest(const char *pString, uint8_t x, uint8_t y,
                                bool statusbar, bool fill)
{
    uint8_t c;
    uint8_t pixels;
    const uint8_t *p = (const uint8_t *)pString;

    while ((c = *p++) && c != '\0')
    {
        c -= 0x20;
        for (int i = 0; i < 3; ++i)
        {
            pixels = gFont3x5[c][i];
            for (int j = 0; j < 6; ++j)
            {
                if (pixels & 1)
                {
                    if (statusbar)
                        PutPixelStatus(x + i, y + j, fill);
                    else
                        PutPixel(x + i, y + j, fill);
                }
                pixels >>= 1;
            }
        }
        x += 4;
    }
}
#endif

// Utility functions

static int clamp(int v, int min, int max)
{
    return v <= min ? min : (v >= max ? max : v);
}

static uint8_t my_abs(signed v) { return v > 0 ? v : -v; }

void SetState(State state)
{
    previousState = currentState;
    currentState = state;
    redrawScreen = true;
    redrawStatus = true;
}

// Radio functions

static void ToggleAFBit(bool on)
{
    uint16_t reg = BK4819_ReadRegister(BK4819_REG_47);
    reg &= ~(1 << 8);
    if (on)
        reg |= on << 8;
    BK4819_WriteRegister(BK4819_REG_47, reg);
}

static const BK4819_REGISTER_t registers_to_save[] = {
    BK4819_REG_30,
    BK4819_REG_37,
    BK4819_REG_3D,
    BK4819_REG_43,
    BK4819_REG_47,
    BK4819_REG_48,
    BK4819_REG_7E,
};

static uint16_t registers_stack[sizeof(registers_to_save)];

static void BackupRegisters()
{
    for (uint32_t i = 0; i < ARRAY_SIZE(registers_to_save); i++)
    {
        registers_stack[i] = BK4819_ReadRegister(registers_to_save[i]);
    }
}

static void RestoreRegisters()
{

    for (uint32_t i = 0; i < ARRAY_SIZE(registers_to_save); i++)
    {
        BK4819_WriteRegister(registers_to_save[i], registers_stack[i]);
    }

#ifdef ENABLE_FEAT_F4HWN
    gVfoConfigureMode = VFO_CONFIGURE;
#endif
}

static void ToggleAFDAC(bool on)
{
    uint32_t Reg = BK4819_ReadRegister(BK4819_REG_30);
    Reg &= ~(1 << 9);
    if (on)
        Reg |= (1 << 9);
    BK4819_WriteRegister(BK4819_REG_30, Reg);
}

static void SetF(uint32_t f)
{
    fMeasure = f;

    BK4819_SetFrequency(fMeasure);
    BK4819_PickRXFilterPathBasedOnFrequency(fMeasure);
    uint16_t reg = BK4819_ReadRegister(BK4819_REG_30);
    BK4819_WriteRegister(BK4819_REG_30, 0);
    BK4819_WriteRegister(BK4819_REG_30, reg);
}

static void SetFScan(uint32_t f)
{
    if ((f < 28000000) != (fMeasure < 28000000))
        BK4819_PickRXFilterPathBasedOnFrequency(f);
    fMeasure = f;
    BK4819_SetFrequency(f);
    BK4819_WriteRegister(BK4819_REG_30, 0);
    BK4819_WriteRegister(BK4819_REG_30, scanReg30);
}

// Spectrum related

bool IsPeakOverLevel() { return peak.rssi >= settings.rssiTriggerLevel; }

static void ResetPeak()
{
    peak.t = 0;
    peak.rssi = 0;
}

#ifdef ENABLE_FEAT_F4HWN_SPECTRUM
    static void setTailFoundInterrupt()
    {
        BK4819_WriteRegister(BK4819_REG_3F, BK4819_REG_02_CxCSS_TAIL | BK4819_REG_02_SQUELCH_FOUND);
    }

    static bool checkIfTailFound()
    {
      uint16_t interrupt_status_bits;
      // if interrupt waiting to be handled
      if(BK4819_ReadRegister(BK4819_REG_0C) & 1u) {
        // reset the interrupt
        BK4819_WriteRegister(BK4819_REG_02, 0);
        // fetch the interrupt status bits
        interrupt_status_bits = BK4819_ReadRegister(BK4819_REG_02);
        // if tail found interrupt
        if (interrupt_status_bits & BK4819_REG_02_CxCSS_TAIL)
        {
            listenT = 0;
            // disable interrupts
            BK4819_WriteRegister(BK4819_REG_3F, 0);
            // reset the interrupt
            BK4819_WriteRegister(BK4819_REG_02, 0);
            return true;
        }
      }
      return false;
    }
#endif

bool IsCenterMode() { return settings.scanStepIndex < S_STEP_2_5kHz; }
// scan step in 0.01khz
uint16_t GetScanStep() { return scanStepValues[settings.scanStepIndex]; }

uint16_t GetStepsCount()
{
#ifdef ENABLE_SCAN_RANGES
    if (gScanRangeStart)
    {
        uint32_t range = gScanRangeStop - gScanRangeStart;
        uint16_t step = GetScanStep();
        return (range / step) + 1;  // +1 to include up limit
    }
#endif
    return 128 >> settings.stepsCount;
}

#ifdef ENABLE_SCAN_RANGES
static uint16_t GetStepsCountDisplay()
{
    if (gScanRangeStart)
    {
        return (gScanRangeStop - gScanRangeStart) / GetScanStep();
    }
    return GetStepsCount();
}
#endif

uint32_t GetBW() { return GetStepsCount() * GetScanStep(); }
uint32_t GetFStart()
{
    return IsCenterMode() ? currentFreq - (GetBW() >> 1) : currentFreq;
}

uint32_t GetFEnd()
{
#ifdef ENABLE_SCAN_RANGES
    if (gScanRangeStart)
    {
        return gScanRangeStop;
    }
#endif
    return currentFreq + GetBW();
}

static void TuneToPeak()
{
    scanInfo.f = peak.f;
    scanInfo.rssi = peak.rssi;
    scanInfo.i = peak.i;
    SetF(scanInfo.f);
}

static void DeInitSpectrum()
{
    SetF(initialFreq);
    RestoreRegisters();
    isInitialized = false;
}

uint8_t GetBWRegValueForScan()
{
    return scanStepBWRegValues[settings.scanStepIndex];
}

uint16_t GetRssi()
{
    uint8_t guard = 50;
    while (guard-- && (BK4819_ReadRegister(0x63) & 0xFF) >= 200)
    {
        SYSTICK_DelayUs(1);
    }
    BK4819_GetRSSI(); // discard first read for AGC settling
    uint16_t rssi = BK4819_GetRSSI();
#ifdef ENABLE_AM_FIX
    if (settings.modulationType == MODULATION_AM && gSetting_AM_fix)
        rssi += AM_fix_get_gain_diff() * 2;
#endif
    return rssi;
}

static void ToggleAudio(bool on)
{
    if (on == audioState)
    {
        return;
    }
    audioState = on;
    if (on)
    {
        AUDIO_AudioPathOn();
    }
    else
    {
        AUDIO_AudioPathOff();
    }
}

static void ToggleRX(bool on)
{
    #ifdef ENABLE_FEAT_F4HWN_SPECTRUM
    if (isListening == on) {
        return;
    }
    #endif
    isListening = on;

    //RADIO_SetupAGC(settings.modulationType == MODULATION_AM, lockAGC);
    RADIO_SetupAGC(false, lockAGC);

    BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2_GREEN, on);

    ToggleAudio(on);
    ToggleAFDAC(on);
    ToggleAFBit(on);

    if (on)
    {
    #ifdef ENABLE_FEAT_F4HWN_SPECTRUM
        listenT = 100;
        BK4819_WriteRegister(0x43, listenBWRegValues[settings.listenBw]);
        setTailFoundInterrupt();
    #else
        listenT = 1000;
        BK4819_WriteRegister(0x43, listenBWRegValues[settings.listenBw]);
    #endif
    }
    else
    {
        BK4819_WriteRegister(0x43, GetBWRegValueForScan());
    }
}

// Scan info

static void ResetScanStats()
{
    scanInfo.rssi = 0;
    scanInfo.rssiMax = 0;
    scanInfo.iPeak = 0;
    scanInfo.fPeak = 0;
}

static void InitScan()
{
    ResetScanStats();
    scanInfo.i = 0;
    scanInfo.f = GetFStart();

    scanInfo.scanStep = GetScanStep();
    scanInfo.measurementsCount = GetStepsCount();
    scanReg30 = BK4819_ReadRegister(BK4819_REG_30) & ~(1u << 9);
}

static void ResetBlacklist()
{
    for (int i = 0; i < 128; ++i)
    {
        if (rssiHistory[i] == RSSI_MAX_VALUE)
            rssiHistory[i] = 0;
    }
#ifdef ENABLE_SCAN_RANGES
    memset(blacklistFreqs, 0, sizeof(blacklistFreqs));
    blacklistFreqsIdx = 0;
#endif
}

static void RelaunchScan()
{
    InitScan();
    ResetPeak();
    ToggleRX(false);
#ifdef SPECTRUM_AUTOMATIC_SQUELCH
    settings.rssiTriggerLevel = RSSI_MAX_VALUE;
#endif
    preventKeypress = true;
    scanInfo.rssiMin = RSSI_MAX_VALUE;
    memset(waterfallHistory, 0, sizeof(waterfallHistory));
    waterfallIndex = 0;
#ifdef ENABLE_FEAT_F4HWN_SPECTRUM
    SaveSettings();
#endif
}

static void UpdateScanInfo()
{
    if (scanInfo.rssi > scanInfo.rssiMax)
    {
        scanInfo.rssiMax = scanInfo.rssi;
        scanInfo.fPeak = scanInfo.f;
        scanInfo.iPeak = scanInfo.i;
    }

    if (scanInfo.rssi < scanInfo.rssiMin)
    {
        scanInfo.rssiMin = scanInfo.rssi;
        settings.dbMin = Rssi2DBm(scanInfo.rssiMin);
        if (settings.dbMin > settings.dbMax - 10)
            settings.dbMin = settings.dbMax - 10;
        redrawStatus = true;
    }
}

static void AutoTriggerLevel()
{
    if (settings.rssiTriggerLevel == RSSI_MAX_VALUE)
    {
        settings.rssiTriggerLevel = clamp(scanInfo.rssiMax + 8, 0, RSSI_MAX_VALUE);
    }
}

static void UpdatePeakInfoForce()
{
    peak.t = 0;
    peak.rssi = scanInfo.rssiMax;
    peak.f = scanInfo.fPeak;
    peak.i = scanInfo.iPeak;
    AutoTriggerLevel();
}

static void UpdatePeakInfo()
{
    if (peak.f == 0 || peak.t >= 1024 || peak.rssi < scanInfo.rssiMax)
        UpdatePeakInfoForce();
}

static uint8_t GetHistorySlot(uint16_t idx)
{
#ifdef ENABLE_SCAN_RANGES
    if (scanInfo.measurementsCount > ARRAY_SIZE(rssiHistory))
    {
        uint32_t slot = (uint32_t)idx * ARRAY_SIZE(rssiHistory) / scanInfo.measurementsCount;
        if (slot >= ARRAY_SIZE(rssiHistory))
            slot = ARRAY_SIZE(rssiHistory) - 1;
        return (uint8_t)slot;
    }
#endif
    return (uint8_t)idx;
}

static void SetRssiHistory(uint16_t idx, uint16_t rssi)
{
    uint8_t slot = GetHistorySlot(idx);

    if (rssi == RSSI_MAX_VALUE)
    {
        rssiHistory[slot] = RSSI_MAX_VALUE;
        return;
    }

#ifdef ENABLE_SCAN_RANGES
    if (scanInfo.measurementsCount > ARRAY_SIZE(rssiHistory))
    {
        uint16_t previous_rssi = rssiHistory[slot];
        if (previous_rssi == RSSI_MAX_VALUE)
            return;

        if (rssi >= previous_rssi)
            rssiHistory[slot] = rssi;
        else
            rssiHistory[slot] = (uint16_t)((3u * previous_rssi + rssi) >> 2);

        return;
    }
#endif

    rssiHistory[slot] = rssi;
}

static void Measure()
{
    uint16_t rssi = scanInfo.rssi = GetRssi();
    SetRssiHistory(scanInfo.i, rssi);
}

// Update things by keypress

static uint16_t dbm2rssi(int dBm)
{
    const uint8_t band = (gRxVfo->Band < BAND_N_ELEM) ? gRxVfo->Band : BAND6_400MHz;
    return (dBm + 160 - dBmCorrTable[band]) * 2;
}

static void ClampRssiTriggerLevel()
{
    settings.rssiTriggerLevel =
        clamp(settings.rssiTriggerLevel, dbm2rssi(settings.dbMin),
              dbm2rssi(settings.dbMax));
}

static void UpdateRssiTriggerLevel(bool inc)
{
    if (inc)
        settings.rssiTriggerLevel += 2;
    else
        settings.rssiTriggerLevel -= 2;

    ClampRssiTriggerLevel();

    redrawScreen = true;
    redrawStatus = true;
}

static void UpdateDBMax(bool inc)
{
    if (inc && settings.dbMax < 10)
    {
        settings.dbMax += 1;
    }
    else if (!inc && settings.dbMax > settings.dbMin)
    {
        settings.dbMax -= 1;
    }
    else
    {
        return;
    }

    ClampRssiTriggerLevel();
    redrawStatus = true;
    redrawScreen = true;
    SYSTEM_DelayMs(20);
}

static void UpdateScanStep(bool inc)
{
    if (inc)
    {
        settings.scanStepIndex = settings.scanStepIndex != S_STEP_100_0kHz ? settings.scanStepIndex + 1 : 0;
    }
    else
    {
        settings.scanStepIndex = settings.scanStepIndex != 0 ? settings.scanStepIndex - 1 : S_STEP_100_0kHz;
    }

    settings.frequencyChangeStep = GetBW() >> 1;
    RelaunchScan();
    ResetBlacklist();
    redrawScreen = true;
}

static void UpdateCurrentFreq(bool inc)
{
    if (inc && currentFreq < F_MAX)
    {
        currentFreq += settings.frequencyChangeStep;
    }
    else if (!inc && currentFreq > F_MIN)
    {
        currentFreq -= settings.frequencyChangeStep;
    }
    else
    {
        return;
    }
    RelaunchScan();
    ResetBlacklist();
    redrawScreen = true;
}

static void UpdateCurrentFreqStill(bool inc)
{
    uint8_t offset = modulationTypeTuneSteps[settings.modulationType];
    uint32_t f = fMeasure;
    if (inc && f < F_MAX)
    {
        f += offset;
    }
    else if (!inc && f > F_MIN)
    {
        f -= offset;
    }
    SetF(f);
    redrawScreen = true;
}

static void UpdateFreqChangeStep(bool inc)
{
    uint16_t diff = GetScanStep() * 4;
    if (inc && settings.frequencyChangeStep < 200000)
    {
        settings.frequencyChangeStep += diff;
    }
    else if (!inc && settings.frequencyChangeStep > 10000)
    {
        settings.frequencyChangeStep -= diff;
    }
    SYSTEM_DelayMs(100);
    redrawScreen = true;
}

static void ToggleModulation()
{
    if (settings.modulationType < MODULATION_UKNOWN - 1)
    {
        settings.modulationType++;
    }
    else
    {
        settings.modulationType = MODULATION_FM;
    }
    RADIO_SetModulation(settings.modulationType);

    RelaunchScan();
    redrawScreen = true;
}

static void ToggleListeningBW()
{
    if (settings.listenBw == BK4819_FILTER_BW_NARROWER)
    {
        settings.listenBw = BK4819_FILTER_BW_WIDE;
    }
    else
    {
        settings.listenBw++;
    }
    redrawScreen = true;
}

static void ToggleBacklight()
{
    settings.backlightState = !settings.backlightState;
    if (settings.backlightState)
    {
        // BACKLIGHT_TurnOn();
        BACKLIGHT_SetBrightness(gEeprom.BACKLIGHT_MAX);
    }
    else
    {
        // BACKLIGHT_TurnOff();
        BACKLIGHT_SetBrightness(gEeprom.BACKLIGHT_MIN);
    }
}

static void ToggleStepsCount()
{
    if (settings.stepsCount == STEPS_128)
    {
        settings.stepsCount = STEPS_16;
    }
    else
    {
        settings.stepsCount--;
    }
    settings.frequencyChangeStep = GetBW() >> 1;
    RelaunchScan();
    ResetBlacklist();
    redrawScreen = true;
}

static void ResetFreqInput()
{
    tempFreq = 0;
    for (int i = 0; i < 10; ++i)
    {
        freqInputString[i] = '-';
    }
}

static void FreqInput()
{
    freqInputIndex = 0;
    freqInputDotIndex = 0;
    ResetFreqInput();
    SetState(FREQ_INPUT);
}

static void UpdateFreqInput(KEY_Code_t key)
{
    if (key != KEY_EXIT && freqInputIndex >= 10)
    {
        return;
    }
    if (key == KEY_STAR)
    {
        if (freqInputIndex == 0 || freqInputDotIndex)
        {
            return;
        }
        freqInputDotIndex = freqInputIndex;
    }
    if (key == KEY_EXIT)
    {
        freqInputIndex--;
        if (freqInputDotIndex == freqInputIndex)
            freqInputDotIndex = 0;
    }
    else
    {
        freqInputArr[freqInputIndex++] = key;
    }

    ResetFreqInput();

    uint8_t dotIndex =
        freqInputDotIndex == 0 ? freqInputIndex : freqInputDotIndex;

    KEY_Code_t digitKey;
    for (int i = 0; i < 10; ++i)
    {
        if (i < freqInputIndex)
        {
            digitKey = freqInputArr[i];
            freqInputString[i] = digitKey <= KEY_9 ? '0' + digitKey - KEY_0 : '.';
        }
        else
        {
            freqInputString[i] = '-';
        }
    }

    uint32_t base = 100000; // 1MHz in BK units
    for (int i = dotIndex - 1; i >= 0; --i)
    {
        tempFreq += (freqInputArr[i] - KEY_0) * base;
        base *= 10;
    }

    base = 10000; // 0.1MHz in BK units
    if (dotIndex < freqInputIndex)
    {
        for (int i = dotIndex + 1; i < freqInputIndex; ++i)
        {
            tempFreq += (freqInputArr[i] - KEY_0) * base;
            base /= 10;
        }
    }
    redrawScreen = true;
}

static void Blacklist()
{
#ifdef ENABLE_SCAN_RANGES
    blacklistFreqs[blacklistFreqsIdx++ % ARRAY_SIZE(blacklistFreqs)] = peak.i;
#endif

    SetRssiHistory(peak.i, RSSI_MAX_VALUE);
    ResetPeak();
    ToggleRX(false);
    ResetScanStats();
}

#ifdef ENABLE_SCAN_RANGES
static bool IsBlacklisted(uint16_t idx)
{
    if (blacklistFreqsIdx)
        for (uint8_t i = 0; i < ARRAY_SIZE(blacklistFreqs); i++)
            if (blacklistFreqs[i] == idx)
                return true;
    return false;
}
#endif

// Draw things

static void SetWaterfallLevel(uint8_t x, uint8_t y, uint8_t level)
{
    if (x >= 128 || y >= WATERFALL_HISTORY_DEPTH) return;
    uint8_t row = y >> 1;
    if (!(y & 1))
        waterfallHistory[x][row] = (waterfallHistory[x][row] & 0xF0) | (level & 0x0F);
    else
        waterfallHistory[x][row] = (waterfallHistory[x][row] & 0x0F) | (level << 4);
}

static uint8_t GetWaterfallLevel(uint8_t x, uint8_t y)
{
    if (x >= 128 || y >= WATERFALL_HISTORY_DEPTH) return 0;
    uint8_t row = y >> 1;
    if (!(y & 1)) return waterfallHistory[x][row] & 0x0F;
    return (waterfallHistory[x][row] >> 4) & 0x0F;
}

static void UpdateWaterfall(void)
{
    waterfallIndex = (waterfallIndex + 1) % WATERFALL_HISTORY_DEPTH;

    uint16_t stepsCount = GetStepsCount();
    uint8_t bars = (stepsCount > 128) ? 128 : stepsCount;

    uint16_t minRssi = 0xFFFF, maxRssi = 0;
    uint16_t validSamples = 0;
    for (uint8_t x = 0; x < bars; x++)
    {
        uint16_t rssi = rssiHistory[x];
        if (rssi != RSSI_MAX_VALUE && rssi != 0)
        {
            if (rssi < minRssi) minRssi = rssi;
            if (rssi > maxRssi) maxRssi = rssi;
            validSamples++;
        }
    }

    uint16_t range = (maxRssi > minRssi) ? (maxRssi - minRssi) : 1;

    for (uint8_t x = 0; x < bars; x++)
    {
        uint16_t rssi = rssiHistory[x];
        uint8_t level = 0;
        if (rssi != RSSI_MAX_VALUE && rssi != 0 && validSamples > 0)
        {
            uint8_t dither = (x ^ waterfallIndex) & 0x01;
            level = (uint8_t)((((uint32_t)(rssi - minRssi) * 15 + dither) / range));
            if (level == 0 && (rssi & 0x01)) level = 1;
            if (level == 1) level = WF_FLOOR_MIN_LEVEL;
        }
        SetWaterfallLevel(x, waterfallIndex, level);
    }

    for (uint8_t x = bars; x < 128; x++)
    {
        SetWaterfallLevel(x, waterfallIndex, 0);
    }
}

static void DrawWaterfall(void)
{
    static const uint8_t bayerMatrix[4][4] = {
        { 0, 8, 2, 10 }, { 12, 4, 14, 6 },
        { 3, 11, 1, 9 },  { 15, 7, 13, 5 }
    };

    const uint8_t WATERFALL_START_Y = 40;
    uint16_t stepsCount = GetStepsCount();
    uint8_t bars = (stepsCount > 128) ? 128 : stepsCount;

    for (uint8_t y_offset = 0; y_offset < WATERFALL_HISTORY_DEPTH; y_offset++)
    {
        uint8_t y_pos = WATERFALL_START_Y + y_offset;
        if (y_pos > 63) break;

        int16_t historyRow = (int16_t)waterfallIndex - y_offset;
        while (historyRow < 0) historyRow += WATERFALL_HISTORY_DEPTH;

        uint8_t currentFade = 16;
        if (y_offset > 10)
        {
            uint8_t drop = (y_offset - 10) * 2;
            currentFade = (drop >= 16) ? 0 : 16 - drop;
        }

        for (uint8_t x = 0; x < 128; x++)
        {
            uint16_t specIdx;
            if (bars <= 1)
                specIdx = 0;
            else
                specIdx = ((uint32_t)x * bars) / 128;
            if (specIdx >= bars) specIdx = bars - 1;

            uint8_t level = GetWaterfallLevel((uint8_t)specIdx, (uint8_t)historyRow);

            if (currentFade < 16)
            {
                level = (level * currentFade) >> 4;
            }

            if (level > bayerMatrix[y_pos & 3][x & 3])
            {
                gFrameBuffer[y_pos >> 3][x] &= ~(1 << (y_pos & 7));
            }
            else
            {
                gFrameBuffer[y_pos >> 3][x] |= (1 << (y_pos & 7));
            }
        }
    }
}

static uint8_t iSqrt(uint16_t value)
{
    if (value == 0)
    {
        return 0;
    }

    uint16_t current = value;
    uint16_t next = (current + 1) >> 1;

    while (next < current)
    {
        current = next;
        next = (current + value / current) >> 1;
    }

    return (uint8_t)current;
}

// applied x2 to prevent initial rounding
uint8_t Rssi2PX(uint16_t rssi, uint8_t pxMin, uint8_t pxMax)
{
    const int DB_MIN = settings.dbMin << 1;
    const int DB_MAX = settings.dbMax << 1;
    const int DB_RANGE = DB_MAX - DB_MIN;

    const uint8_t PX_RANGE = pxMax - pxMin;

    int dbm = clamp(Rssi2DBm(rssi) << 1, DB_MIN, DB_MAX);

    uint8_t linear = (uint8_t)(((dbm - DB_MIN) * PX_RANGE + DB_RANGE / 2) / DB_RANGE);
    uint8_t compressed = iSqrt((uint16_t)linear * PX_RANGE);
    uint8_t blended = (uint8_t)(((uint16_t)linear + compressed) / 2);
    uint8_t result = blended + pxMin;

    return result;
}

uint8_t Rssi2Y(uint16_t rssi)
{
    return DrawingEndY - Rssi2PX(rssi, 0, DrawingEndY - DrawingTopY);
}

static void DrawLine(int x0, int y0, int x1, int y1, bool fill);
static uint8_t GetSpectrumBaseY(void);
static uint8_t SpecIdxToX(uint16_t idx);

static void DrawSpectrumEnhanced(void)
{
    const uint16_t bars = GetStepsCount();
    const uint8_t SHADE_MAX_Y = GetSpectrumBaseY();

    uint8_t prevX = SpecIdxToX(0);
    uint8_t prevY = Rssi2Y(rssiHistory[0]);

    for (uint16_t i = 1; i < bars; i++) {
        uint8_t currX = SpecIdxToX(i);
        uint8_t currY = Rssi2Y(rssiHistory[i]);

        DrawLine(prevX, prevY, currX, currY, 1);

        if (currX >= prevX) {
            for (uint8_t x = prevX; x <= currX; x++) {
                uint8_t dx = currX - prevX + 1;
                uint8_t yStart = prevY + ((currY - prevY) * (x - prevX) / dx);
                
                if (yStart <= SHADE_MAX_Y) {
                    for (uint8_t y = yStart; y <= SHADE_MAX_Y; y++) {
                        if ((x ^ y) & 0x01) {
                            gFrameBuffer[y >> 3][x] |= (1 << (y & 7));
                        }
                    }
                }
            }
        }
        prevX = currX;
        prevY = currY;
    }
}

static void DrawStatus()
{
#ifdef SPECTRUM_EXTRA_VALUES
    sprintf(String, "%d/%d P:%d T:%d", settings.dbMin, settings.dbMax,
            Rssi2DBm(peak.rssi), Rssi2DBm(settings.rssiTriggerLevel));
#else
    sprintf(String, "%d/%d", settings.dbMin, settings.dbMax);
#endif

#ifdef ENABLE_FEAT_F4HWN
    if (!gBatteryUpdatePaused && gBatteryUpdateDelayCountdown == 0) {
        BOARD_ADC_GetBatteryInfo(&gBatteryVoltages[gBatteryVoltageIndex], &gBatteryCurrent);
        gBatteryVoltageIndex++;
        if (gBatteryVoltageIndex > 3)
            gBatteryVoltageIndex = 0;
    }
    if (gBatteryUpdateDelayCountdown > 0) {
        gBatteryUpdateDelayCountdown--;
    }
    BATTERY_GetReadings(true);

    /* 仅 dB 范围 + 电池与百分比，勿用整段 MAIN ONLY 顶栏（会与频谱右侧小字/信道名重叠） */
    UI_SpectrumDrawStatusLineDbRangeAndBattery(String);
#else
    GUI_DisplaySmallest(String, 0, 1, true, true);

    if (!gBatteryUpdatePaused && gBatteryUpdateDelayCountdown == 0) {
        BOARD_ADC_GetBatteryInfo(&gBatteryVoltages[gBatteryCheckCounter++ % 4],
                                 &gBatteryCurrent);
    }
    if (gBatteryUpdateDelayCountdown > 0) {
        gBatteryUpdateDelayCountdown--;
    }

    uint16_t voltage = (gBatteryVoltages[0] + gBatteryVoltages[1] +
                        gBatteryVoltages[2] + gBatteryVoltages[3]) /
                       4 * 760 / gBatteryCalibration[3];

    unsigned perc = BATTERY_VoltsToPercent(voltage);

    gStatusLine[116] = 0b00011100;
    gStatusLine[117] = 0b00111110;
    for (int i = 118; i <= 126; i++)
    {
        gStatusLine[i] = 0b00100010;
    }

    for (unsigned i = 127; i >= 118; i--)
    {
        if (127 - i <= (perc + 5) * 9 / 100)
        {
            gStatusLine[i] = 0b00111110;
        }
    }
#endif
}

#ifdef ENABLE_FEAT_F4HWN_SPECTRUM
static bool IsPureAscii(const char *str)
{
    while (*str) {
        if ((unsigned char)*str >= 0x80)
            return false;
        str++;
    }
    return true;
}

static void ShowChannelName(uint32_t f)
{
    static uint32_t channelF = 0;
    static char channelName[24];
    static int channelNum = -1;

    if (isListening)
    {
        if (f != channelF) {
            channelF = f;
            channelNum = -1;
            memset(channelName, 0, sizeof(channelName));
            for (unsigned int i = 0; IS_MR_CHANNEL(i); i++)
            {
                if (RADIO_CheckValidChannel(i, false, 0))
                {
                    if (SETTINGS_FetchChannelFrequency(i) == channelF)
                    {
                        SETTINGS_FetchChannelName(channelName, i);
                        channelNum = (int)i;
                        break;
                    }
                }
            }
        }
        if (channelNum >= 0 && channelName[0] != 0) {
            const char *displayText;
            char channelStr[8];

            if (IsPureAscii(channelName)) {
                displayText = channelName;
            } else {
                sprintf(channelStr, "CH-%03d", channelNum + 1);
                displayText = channelStr;
            }

            uint8_t right_limit_x = SPECTRUM_STATUS_RIGHT_RESERVED_X;
            if (right_limit_x > LCD_WIDTH)
                right_limit_x = LCD_WIDTH;

            uint8_t clear_width_max = 0;
            if (right_limit_x > SPECTRUM_STATUS_CH_NAME_X0)
                clear_width_max = (uint8_t)(right_limit_x - SPECTRUM_STATUS_CH_NAME_X0);

            uint8_t clear_width = DualVfoU8g2_GetSmallTextWidth(displayText);
            if (clear_width < SPECTRUM_STATUS_CH_NAME_CLR_W)
                clear_width = SPECTRUM_STATUS_CH_NAME_CLR_W;

            if (clear_width > clear_width_max)
                clear_width = clear_width_max;

            memset(&gStatusLine[SPECTRUM_STATUS_CH_NAME_X0], 0, clear_width);
            DualVfoU8g2_DrawSmallTextStatus(displayText, SPECTRUM_STATUS_CH_NAME_X0, 2u, true);
        }
    }
    else
    {
        uint8_t right_limit_x = SPECTRUM_STATUS_RIGHT_RESERVED_X;
        if (right_limit_x > LCD_WIDTH)
            right_limit_x = LCD_WIDTH;

        uint8_t clear_width = 0;
        if (right_limit_x > SPECTRUM_STATUS_CH_NAME_X0)
            clear_width = (uint8_t)(right_limit_x - SPECTRUM_STATUS_CH_NAME_X0);

        memset(&gStatusLine[SPECTRUM_STATUS_CH_NAME_X0], 0, clear_width);
    }
    ST7565_BlitStatusLine();
}
#endif

static void DrawF(uint32_t f)
{
    sprintf(String, "%u.%05u", f / 100000, f % 100000);
    UI_PrintStringSmallNormal(String, 8, 127, 0);

    sprintf(String, "%3s", gModulationStr[settings.modulationType]);
#ifdef ENABLE_FEAT_F4HWN
    {
        const uint8_t text_width_mod = DualVfoU8g2_GetSmallTextWidth(String);
        uint8_t       x_mod            = 0u;
        if (LCD_WIDTH > 2u + text_width_mod)
            x_mod = (uint8_t)(LCD_WIDTH - 2u - text_width_mod);
        DualVfoU8g2_DrawSmallText(String, x_mod, 0u, true);
    }
#else
    GUI_DisplaySmallest(String, 116, 0, false, true);
#endif

    sprintf(String, "%4sk", bwOptions[settings.listenBw]);
#ifdef ENABLE_FEAT_F4HWN
    {
        const uint8_t text_width_bw = DualVfoU8g2_GetSmallTextWidth(String);
        uint8_t       x_bw          = 0u;
        if (LCD_WIDTH > 2u + text_width_bw)
            x_bw = (uint8_t)(LCD_WIDTH - 2u - text_width_bw);
        DualVfoU8g2_DrawSmallText(String, x_bw, 6u, true);
    }
#else
    GUI_DisplaySmallest(String, 108, 6, false, true);
#endif

#ifdef ENABLE_FEAT_F4HWN_SPECTRUM
    ShowChannelName(f);
#endif
}

#ifdef ENABLE_FEAT_F4HWN
/* 底部步进旁：3×5 的 0x7F（上加下减），与未开 F4HWN 时 GUI_DisplaySmallest 一致；字宽约 4px */
#define SPECTRUM_BOTTOM_PM_3X5_WIDTH_PX 4u
/* 仅「±」字模 + 步进 k 数（如 800.00k）相对原位置右移；左右两侧主频不动 */
#define SPECTRUM_BOTTOM_STEP_BLOCK_SHIFT_RIGHT_PX 10u

static const char spectrum_pm_glyph_3x5[] = "\x7F";

static uint8_t Spectrum_U8g2PlusMinusColumnWidth(void)
{
    const uint8_t width_pm = SPECTRUM_BOTTOM_PM_3X5_WIDTH_PX;
    return width_pm;
}

static void Spectrum_U8g2DrawPlusMinus(uint8_t x_left, uint8_t y_top, bool set_black)
{
    (void)set_black;
    GUI_DisplaySmallest(spectrum_pm_glyph_3x5, x_left, y_top, false, true);
}
#endif

static void DrawNums()
{

    if (currentState == SPECTRUM)
    {
#ifdef ENABLE_SCAN_RANGES
        if (gScanRangeStart)
        {
            sprintf(String, "%ux", GetStepsCountDisplay());
        }
        else
#endif
        {
            sprintf(String, "%ux", GetStepsCount());
        }
#ifdef ENABLE_FEAT_F4HWN
        DualVfoU8g2_DrawSmallText(String, 0u, 0u, true);
        sprintf(String, "%u.%02uk", GetScanStep() / 100, GetScanStep() % 100);
        DualVfoU8g2_DrawSmallText(String, 0u, 6u, true);
#else
        GUI_DisplaySmallest(String, 0, 0, false, true);
        sprintf(String, "%u.%02uk", GetScanStep() / 100, GetScanStep() % 100);
        GUI_DisplaySmallest(String, 0, 6, false, true);
#endif
    }

    if (IsCenterMode())
    {
#ifdef ENABLE_FEAT_F4HWN
        {
            char           part_left[16];
            char           part_khz[16];
            const unsigned step_whole = (unsigned)(settings.frequencyChangeStep / 100u);
            const unsigned step_frac  = (unsigned)(settings.frequencyChangeStep % 100u);

            sprintf(part_left, "%u.%05u", currentFreq / 100000, currentFreq % 100000);
            sprintf(part_khz, "%u.%02uk", step_whole, step_frac);

            const uint8_t width_left  = DualVfoU8g2_GetSmallTextWidth(part_left);
            const uint8_t width_col   = Spectrum_U8g2PlusMinusColumnWidth();
            const uint8_t width_khz  = DualVfoU8g2_GetSmallTextWidth(part_khz);
            const uint8_t gap_px      = 2u;
            const uint32_t total_w_u =
                (uint32_t)width_left + (uint32_t)gap_px + (uint32_t)width_col +
                (uint32_t)gap_px + (uint32_t)width_khz;
            uint8_t x0 = 0u;
            if (total_w_u <= (uint32_t)LCD_WIDTH)
                x0 = (uint8_t)(((uint32_t)LCD_WIDTH - total_w_u) / 2u);

            DualVfoU8g2_DrawSmallText(part_left, x0, 34u, true);
            {
                const uint8_t x_stack =
                    (uint8_t)(x0 + width_left + gap_px + SPECTRUM_BOTTOM_STEP_BLOCK_SHIFT_RIGHT_PX);
                Spectrum_U8g2DrawPlusMinus(x_stack, 34u, true);
            }
            {
                const uint8_t x_khz = (uint8_t)(x0 + width_left + gap_px + width_col + gap_px +
                                               SPECTRUM_BOTTOM_STEP_BLOCK_SHIFT_RIGHT_PX);
                DualVfoU8g2_DrawSmallText(part_khz, x_khz, 34u, true);
            }
        }
#else
        sprintf(String, "%u.%05u \x7F%u.%02uk", currentFreq / 100000,
                currentFreq % 100000, settings.frequencyChangeStep / 100,
                settings.frequencyChangeStep % 100);
        GUI_DisplaySmallest(String, 36, 34, false, true);
#endif
    }
    else
    {
#ifdef ENABLE_FEAT_F4HWN
        {
            char           line_start[16];
            char           line_step[16];
            char           line_end[16];
            const unsigned step_khz_whole = (unsigned)(settings.frequencyChangeStep / 100u);
            const unsigned step_khz_frac  = (unsigned)(settings.frequencyChangeStep % 100u);

            sprintf(line_start, "%u.%05u", GetFStart() / 100000, GetFStart() % 100000);
#ifdef ENABLE_SCAN_RANGES
            if (gScanRangeStart)
            {
                uint32_t bw = gScanRangeStop - gScanRangeStart;
                sprintf(line_step, "%u.%02uk", bw / 100, bw % 100);
            }
            else
#endif
            sprintf(line_step, "%u.%02uk", step_khz_whole, step_khz_frac);
            sprintf(line_end, "%u.%05u", GetFEnd() / 100000, GetFEnd() % 100000);

            DualVfoU8g2_DrawSmallText(line_start, 0u, 34u, true);

            const uint8_t width_start = DualVfoU8g2_GetSmallTextWidth(line_start);
            const uint8_t width_col   = Spectrum_U8g2PlusMinusColumnWidth();
            const uint8_t width_khz   = DualVfoU8g2_GetSmallTextWidth(line_step);
            const uint8_t width_end   = DualVfoU8g2_GetSmallTextWidth(line_end);
            const uint8_t gap_px      = 2u;
            const uint8_t width_mid   = (uint8_t)(gap_px + width_col + gap_px + width_khz);

            uint8_t x_end = 0u;
            if (LCD_WIDTH > 2u + width_end)
                x_end = (uint8_t)(LCD_WIDTH - 2u - width_end);

            uint8_t x_step = (uint8_t)(width_start + 2u);
            {
                const uint8_t shift_px = SPECTRUM_BOTTOM_STEP_BLOCK_SHIFT_RIGHT_PX;
                const uint16_t width_mid_u     = (uint16_t)width_mid;
                const uint16_t x_end_u         = (uint16_t)x_end;
                const uint16_t shift_u         = (uint16_t)shift_px;
                const uint16_t margin_u        = 2u;
                const uint16_t x_step_max_u =
                    (x_end_u > width_mid_u + margin_u + shift_u)
                        ? (uint16_t)(x_end_u - width_mid_u - margin_u - shift_u)
                        : (uint16_t)x_step;
                const uint8_t x_step_max = (uint8_t)x_step_max_u;
                if (x_step > x_step_max)
                    x_step = x_step_max;
            }

            {
                const uint8_t x_pm = (uint8_t)(x_step + SPECTRUM_BOTTOM_STEP_BLOCK_SHIFT_RIGHT_PX);
                Spectrum_U8g2DrawPlusMinus(x_pm, 34u, true);
            }
            {
                const uint8_t x_khz = (uint8_t)(x_step + gap_px + width_col + gap_px +
                                               SPECTRUM_BOTTOM_STEP_BLOCK_SHIFT_RIGHT_PX);
                DualVfoU8g2_DrawSmallText(line_step, x_khz, 34u, true);
            }
            DualVfoU8g2_DrawSmallText(line_end, x_end, 34u, true);
        }
#else
        sprintf(String, "%u.%05u", GetFStart() / 100000, GetFStart() % 100000);
        GUI_DisplaySmallest(String, 0, 34, false, true);

#ifdef ENABLE_SCAN_RANGES
        if (gScanRangeStart)
        {
            uint32_t bw = gScanRangeStop - gScanRangeStart;
            sprintf(String, "%u.%02uk", bw / 100, bw % 100);
        }
        else
#endif
        sprintf(String, "\x7F%u.%02uk", settings.frequencyChangeStep / 100,
                settings.frequencyChangeStep % 100);
        GUI_DisplaySmallest(String, 48, 34, false, true);

        sprintf(String, "%u.%05u", GetFEnd() / 100000, GetFEnd() % 100000);
        GUI_DisplaySmallest(String, 93, 34, false, true);
#endif
    }
}

static void DrawRssiTriggerLevel()
{
    if (settings.rssiTriggerLevel == RSSI_MAX_VALUE || monitorMode)
        return;
    uint8_t y = Rssi2Y(settings.rssiTriggerLevel);
    if (y < DrawingTopY) y = DrawingTopY;
    for (uint8_t x = 0; x < 128; x += 2)
    {
        PutPixel(x, y, true);
    }
}

static void DrawLine(int x0, int y0, int x1, int y1, bool fill)
{
    int dx = my_abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -my_abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true)
    {
        if (x0 >= 0 && x0 < 128 && y0 >= 0 && y0 < 64)
        {
            UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)x0, (uint8_t)y0, fill);
        }
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

static uint8_t GetSpectrumBaseY(void)
{
    return DrawingEndY + 6;
}

static uint8_t SpecIdxToX(uint16_t idx)
{
    uint16_t bars = GetStepsCount();
    if (bars <= 1)
        return 64;
    return (uint8_t)(((uint32_t)idx * 127) / (bars - 1));
}

static void DrawGridBackground(void)
{
    for (uint8_t x = 0; x < 128; x++)
    {
        bool isVerticalLinePos = (x % 16 == 0 || x == 127);
        bool isHorizontalDot = (x % 2 == 0);

        uint8_t r1 = 0;
        if (isVerticalLinePos) r1 |= 0xA0;
        if (isHorizontalDot)   r1 |= 0x10;
        gFrameBuffer[1][x] |= r1;

        for (uint8_t r = 2; r <= 3; r++) {
            uint8_t pattern = 0;
            if (isVerticalLinePos) pattern |= 0xAA;
            if (isHorizontalDot)   pattern |= 0x01;
            if (isHorizontalDot)   pattern |= 0x10;
            gFrameBuffer[r][x] |= pattern;
        }

        gFrameBuffer[4][x] |= 0x01;
    }
}

static void DrawArrow(uint8_t x)
{
    for (signed i = -2; i <= 2; ++i)
    {
        signed v = x + i;
        if (!(v & 128))
        {
            uint8_t p3 = 0;

            p3 |= 0b10000000;

            if (i >= -1 && i <= 1) p3 |= 0b01000000;

            if (i == 0) p3 |= 0b00100000;

            p3 |= 0b00010000;

            gFrameBuffer[3][v] |= p3;
        }
    }
}

static void DrawChickenIcon(uint8_t icon_left_x, uint8_t icon_top_y)
{
    static const char *chicken_rows[] = {
        "....................",
        ".......#.#..........",
        "......#####.........",
        "....##.....##.......",
        "...##........##..#..",
        "..##...........####.",
        "..#.....#........##.",
        "..#...............#.",
        "..##..............#.",
        "...##............##.",
        "....###........##...",
        "......##########....",
        "........#..#........",
        ".......##..##.......",
    };

    const uint8_t chicken_width = 20u;
    const uint8_t chicken_height = (uint8_t)ARRAY_SIZE(chicken_rows);

    for (uint8_t row_index = 0; row_index < chicken_height; ++row_index)
    {
        const char *current_row = chicken_rows[row_index];
        uint8_t pixel_y = icon_top_y + row_index;

        for (uint8_t column_index = 0; column_index < chicken_width; ++column_index)
        {
            char current_symbol = current_row[column_index];
            bool should_draw_pixel = false;

            if (current_symbol == '#')
            {
                should_draw_pixel = true;
            }

            if (should_draw_pixel)
            {
                uint8_t pixel_x = icon_left_x + column_index;
                PutPixel(pixel_x, pixel_y, true);
            }
        }
    }
}

static void OnKeyDown(uint8_t key)
{
    bool nav = gEeprom.SET_NAV;
    bool isTrue = false;

    switch (key)
    {
    case KEY_3:
        isTrue = true;
        [[fallthrough]];
    case KEY_9:
        UpdateDBMax(isTrue);
        break;
    case KEY_1:
        isTrue = true;
        [[fallthrough]];
    case KEY_7:
        UpdateScanStep(isTrue);
        break;
    case KEY_2:
        isTrue = true;
        [[fallthrough]];
    case KEY_8:
        UpdateFreqChangeStep(isTrue);
        break;
    case KEY_UP:
        nav = !nav;
        [[fallthrough]];
    case KEY_DOWN:
#ifdef ENABLE_SCAN_RANGES
        if (!gScanRangeStart) {
#endif
        UpdateCurrentFreq(!nav);
#ifdef ENABLE_SCAN_RANGES
        }
#endif
        break;
    case KEY_SIDE1:
        Blacklist();
        break;
    case KEY_STAR:
        isTrue = true;
        [[fallthrough]];
    case KEY_F:
        UpdateRssiTriggerLevel(isTrue);
        break;
    case KEY_5:
#ifdef ENABLE_SCAN_RANGES
        if (!gScanRangeStart)
#endif
            FreqInput();
        break;
    case KEY_0:
        ToggleModulation();
        break;
    case KEY_6:
        ToggleListeningBW();
        break;
    case KEY_4:
#ifdef ENABLE_SCAN_RANGES
        if (!gScanRangeStart)
#endif
            ToggleStepsCount();
        break;
    case KEY_SIDE2:
        ToggleBacklight();
        break;
    case KEY_PTT:
        SetState(STILL);
        TuneToPeak();
        break;
    case KEY_MENU:
        break;
    case KEY_EXIT:
        if (menuState)
        {
            menuState = 0;
            break;
        }
#ifdef ENABLE_FEAT_F4HWN_SPECTRUM
        SaveSettings();
#endif
#ifdef ENABLE_FEAT_F4HWN_RESUME_STATE
        gEeprom.CURRENT_STATE = 0;
        SETTINGS_WriteCurrentState();
#endif
        DeInitSpectrum();
        break;
    default:
        break;
    }
}

static void OnKeyDownFreqInput(uint8_t key)
{
    switch (key)
    {
    case KEY_0...KEY_9:
    case KEY_STAR:
    case KEY_EXIT:
        if (freqInputIndex == 0 && key == KEY_EXIT)
        {
            SetState(previousState);
            break;
        }
        UpdateFreqInput(key);
        break;
    case KEY_MENU:
        if (tempFreq < F_MIN || tempFreq > F_MAX)
        {
            break;
        }
        SetState(previousState);
        currentFreq = tempFreq;
        if (currentState == SPECTRUM)
        {
            ResetBlacklist();
            RelaunchScan();
        }
        else
        {
            SetF(currentFreq);
        }
        break;
    default:
        break;
    }
}

void OnKeyDownStill(KEY_Code_t key)
{
    bool nav = gEeprom.SET_NAV;
    bool isTrue = false;

    switch (key)
    {
    case KEY_3:
        isTrue = true;
        [[fallthrough]];
    case KEY_9:
        UpdateDBMax(isTrue);
        break;
    case KEY_UP:
        nav = !nav;
        [[fallthrough]];
    case KEY_DOWN:
        if (menuState) {
            SetRegMenuValue(menuState, !nav);
            break;
        }
        UpdateCurrentFreqStill(!nav);
        break;
    case KEY_STAR:
        isTrue = true;
        [[fallthrough]];
    case KEY_F:
        UpdateRssiTriggerLevel(isTrue);
        break;
    case KEY_5:
        FreqInput();
        break;
    case KEY_0:
        ToggleModulation();
        break;
    case KEY_6:
        ToggleListeningBW();
        break;
    case KEY_SIDE1:
        monitorMode = !monitorMode;
        break;
    case KEY_SIDE2:
        ToggleBacklight();
        break;
    case KEY_PTT:
        // TODO: start transmit
        /* BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2_GREEN, false);
        BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, true); */
        break;
    case KEY_MENU:
        menuState = (menuState == ARRAY_SIZE(registerSpecs) - 1) ? 1 : menuState + 1;
        redrawScreen = true;
        break;
    case KEY_EXIT:
        if (!menuState)
        {
            SetState(SPECTRUM);
            lockAGC = false;
            monitorMode = false;
            RelaunchScan();
            break;
        }
        menuState = 0;
        break;
    default:
        break;
    }
}

static void RenderFreqInput() { UI_PrintString(freqInputString, 2, 127, 0, 8); }

static void RenderStatus()
{
#ifndef ENABLE_FEAT_F4HWN
    memset(gStatusLine, 0, sizeof(gStatusLine));
#endif
    DrawStatus();
    ST7565_BlitStatusLine();
}

static void RenderSpectrum()
{
    for (uint8_t r = 1; r <= 5; r++)
    {
        memset(gFrameBuffer[r], 0, sizeof(gFrameBuffer[r]));
    }

    DrawGridBackground();

    uint16_t stepsCount = GetStepsCount();
    uint8_t arrowX = 0;
    if (stepsCount > 1)
    {
        arrowX = (uint8_t)(128u * peak.i / (stepsCount - 1));
    }
    DrawSpectrumEnhanced();
    DrawArrow(arrowX);
    DrawRssiTriggerLevel();
    DrawF(peak.f);
    DrawNums();
    for (uint8_t x = 0; x < 128; x++)
        gFrameBuffer[4][x] &= 0b11111101;
    DrawWaterfall();
}

static void RenderStill()
{
    DrawF(fMeasure);

    const uint8_t METER_PAD_LEFT = 3;

    memset(&gFrameBuffer[2][METER_PAD_LEFT], 0b00010000, 121);

    for (int i = 0; i < 121; i += 5)
    {
        gFrameBuffer[2][i + METER_PAD_LEFT] = 0b00110000;
    }

    for (int i = 0; i < 121; i += 10)
    {
        gFrameBuffer[2][i + METER_PAD_LEFT] = 0b01110000;
    }

    uint8_t x = Rssi2PX(scanInfo.rssi, 0, 121);
    for (int i = 0; i < x; ++i)
    {
        if (i % 5)
        {
            gFrameBuffer[2][i + METER_PAD_LEFT] |= 0b00000111;
        }
    }

    int dbm = Rssi2DBm(scanInfo.rssi);
    uint8_t s = DBm2S(dbm);
    sprintf(String, "S: %u", s);
#ifdef ENABLE_FEAT_F4HWN
    DualVfoU8g2_DrawSmallText(String, 4u, 25u, true);
    sprintf(String, "%d dBm", dbm);
    DualVfoU8g2_DrawSmallText(String, 28u, 25u, true);
#else
    GUI_DisplaySmallest(String, 4, 25, false, true);
    sprintf(String, "%d dBm", dbm);
    GUI_DisplaySmallest(String, 28, 25, false, true);
#endif

    if (!monitorMode)
    {
        uint8_t x = Rssi2PX(settings.rssiTriggerLevel, 0, 121);
        gFrameBuffer[2][METER_PAD_LEFT + x] = 0b11111111;
    }

    const uint8_t PAD_LEFT = 4;
    const uint8_t CELL_WIDTH = 30;
    uint8_t offset = PAD_LEFT;
    uint8_t row = 4;

    for (int i = 0, idx = 1; idx <= 3; ++i, ++idx)
    {
        if (idx == 4)
        {
            row += 2;
            i = 0;
        }
        offset = PAD_LEFT + i * CELL_WIDTH;
        if (menuState == idx)
        {
            for (int j = 0; j < CELL_WIDTH; ++j)
            {
                gFrameBuffer[row][j + offset] = 0xFF;
                gFrameBuffer[row + 1][j + offset] = 0xFF;
            }
        }
        sprintf(String, "%s", registerSpecs[idx].name);
#ifdef ENABLE_FEAT_F4HWN
        DualVfoU8g2_DrawSmallText(String, (uint8_t)(offset + 2u), (uint8_t)(row * 8u + 2u),
                                   menuState != idx);
#else
        GUI_DisplaySmallest(String, offset + 2, row * 8 + 2, false,
                            menuState != idx);
#endif

#ifdef ENABLE_FEAT_F4HWN_SPECTRUM
        if(idx == 1)
        {
            sprintf(String, "%ddB", LNAsOptions[GetRegMenuValue(idx)]);
        }
        else if(idx == 2)
        {
            sprintf(String, "%ddB", LNAOptions[GetRegMenuValue(idx)]);
        }
        else if(idx == 3)
        {
            sprintf(String, "%ddB", VGAOptions[GetRegMenuValue(idx)]);
        }
        /*
        else if(idx == 4)
        {
            sprintf(String, "%skHz", BPFOptions[(GetRegMenuValue(idx) / 0x2aaa)]);
        }
        */
#else
        sprintf(String, "%u", GetRegMenuValue(idx));
#endif
#ifdef ENABLE_FEAT_F4HWN
        DualVfoU8g2_DrawSmallText(String, (uint8_t)(offset + 2u), (uint8_t)((row + 1u) * 8u + 1u),
                                   menuState != idx);
#else
        GUI_DisplaySmallest(String, offset + 2, (row + 1) * 8 + 1, false,
                            menuState != idx);
#endif
    }

    {
        const uint8_t chicken_icon_left_x = 97u;
        const uint8_t chicken_icon_top_y = 33u;
        DrawChickenIcon(chicken_icon_left_x, chicken_icon_top_y);
    }

}

static void Render()
{
    UI_DisplayClear();

    switch (currentState)
    {
    case SPECTRUM:
        RenderSpectrum();
        break;
    case FREQ_INPUT:
        RenderFreqInput();
        break;
    case STILL:
        RenderStill();
        break;
    }
}

static bool HandleUserInput()
{
    kbd.prev = kbd.current;
    kbd.current = KEYBOARD_GetKey();

    if (kbd.current != KEY_INVALID && kbd.current == kbd.prev)
    {
        if (kbd.counter < 16)
            kbd.counter++;
        else
            kbd.counter -= 3;
        SYSTEM_DelayMs(20);
    }
    else
    {
        kbd.counter = 0;
    }

    if (kbd.counter == 3 || kbd.counter == 16)
    {
        switch (currentState)
        {
        case SPECTRUM:
            OnKeyDown(kbd.current);
            break;
        case FREQ_INPUT:
            OnKeyDownFreqInput(kbd.current);
            break;
        case STILL:
            OnKeyDownStill(kbd.current);
            break;
        }
    }

    return true;
}

static void Scan()
{
    uint8_t slot = GetHistorySlot(scanInfo.i);

    if (rssiHistory[slot] != RSSI_MAX_VALUE
#ifdef ENABLE_SCAN_RANGES
        && !IsBlacklisted(scanInfo.i)
#endif
    )
    {
        SetFScan(scanInfo.f);
        Measure();
        UpdateScanInfo();
    }
}

static void NextScanStep()
{
    ++peak.t;
    ++scanInfo.i;
    scanInfo.f += scanInfo.scanStep;
}

static void UpdateScan()
{
    Scan();

    if (scanInfo.i + 1 < scanInfo.measurementsCount)
    {
        NextScanStep();
        return;
    }

    if (! (scanInfo.measurementsCount >> 7)) // if (scanInfo.measurementsCount < 128)
        memset(&rssiHistory[scanInfo.measurementsCount], 0,
               sizeof(rssiHistory) - scanInfo.measurementsCount * sizeof(rssiHistory[0]));

    UpdateWaterfall();

    redrawScreen = true;
    preventKeypress = false;

    UpdatePeakInfo();
    if (IsPeakOverLevel())
    {
        ToggleRX(true);
        TuneToPeak();
        return;
    }

    newScanStart = true;
}

static void UpdateStill()
{
    Measure();
    redrawScreen = true;
    preventKeypress = false;

    peak.rssi = scanInfo.rssi;
    AutoTriggerLevel();

    if (IsPeakOverLevel() || monitorMode) {
        ToggleRX(true);
    }
}

static void UpdateListening()
{
    preventKeypress = false;
    #ifdef ENABLE_FEAT_F4HWN_SPECTRUM
    bool tailFound = checkIfTailFound();
    if (tailFound)
    #else
    if (currentState == STILL)
    #endif
    {
        listenT = 0;
    }
    if (listenT)
    {
        listenT--;
        SYSTEM_DelayMs(1);
        return;
    }

    if (currentState == SPECTRUM)
    {
        BK4819_WriteRegister(0x43, GetBWRegValueForScan());
        Measure();
        BK4819_WriteRegister(0x43, listenBWRegValues[settings.listenBw]);
    }
    else
    {
        Measure();
    }

    peak.rssi = scanInfo.rssi;

    if (currentState == SPECTRUM)
    {
        static uint8_t listenWfCounter = 0;
        if (++listenWfCounter >= 6)
        {
            listenWfCounter = 0;
            UpdateWaterfall();
        }
    }

    redrawScreen = true;

    #ifdef ENABLE_FEAT_F4HWN_SPECTRUM
        if ((IsPeakOverLevel() && !tailFound) || monitorMode)
        {
            listenT = 100;
            return;
        }
    #else
        if (IsPeakOverLevel() || monitorMode)
        {
            listenT = 1000;
            return;
        }
    #endif

    ToggleRX(false);
    ResetScanStats();
}

static void Tick()
{
#ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
    // Parse incoming packets on every tick so serial keys are never missed,
    // regardless of whether the screen needs redrawing.
    SCREENSHOT_ParseInput();
#endif

    if (gNextTimeslice)
    {
        gNextTimeslice = false;
#ifdef ENABLE_AM_FIX
        if (settings.modulationType == MODULATION_AM && !lockAGC)
        {
            AM_fix_10ms(vfo); // allow AM_Fix to apply its AGC action
        }
#endif
        BACKLIGHT_Update();
    }

#ifdef ENABLE_SCAN_RANGES
    if (gNextTimeslice_500ms)
    {
        gNextTimeslice_500ms = false;

        // periodically allow key input during scanning
        // so user can EXIT, change step, etc.
        if (!isListening)
        {
            UpdatePeakInfo();
            if (IsPeakOverLevel())
            {
                ToggleRX(true);
                TuneToPeak();
                return;
            }
            redrawScreen = true;
            preventKeypress = false;
        }
    }
#endif

    if (!preventKeypress)
    {
        HandleUserInput();
    }
    if (newScanStart)
    {
        InitScan();
        newScanStart = false;
    }
    if (isListening && currentState != FREQ_INPUT)
    {
        UpdateListening();
    }
    else
    {
        if (currentState == SPECTRUM)
        {
            UpdateScan();
        }
        else if (currentState == STILL)
        {
            UpdateStill();
        }
    }
    if (redrawStatus)
    {
        RenderStatus();
        redrawStatus = false;
    }
    if (redrawScreen || ++renderTimer >= RENDER_PERIOD_TICKS)
    {
        Render();
        // For screenshot
        #ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
            SCREENSHOT_Update(false);
        #endif
        redrawScreen = false;
        renderTimer = 0;
    }
    ST7565_BlitLine(renderPage);
    if (++renderPage >= FRAME_LINES)
        renderPage = 0;
}

void APP_RunSpectrum()
{
    settings.backlightState = gEeprom.BACKLIGHT_TIME == 0 ? false : true;

    // TX here coz it always? set to active VFO
    vfo = gEeprom.TX_VFO;
#ifdef ENABLE_FEAT_F4HWN_SPECTRUM
    LoadSettings();
    settings.frequencyChangeStep = GetBW() >> 1;
#endif
    // set the current frequency in the middle of the display
#ifdef ENABLE_SCAN_RANGES
    if (gScanRangeStart)
    {
        currentFreq = initialFreq = gScanRangeStart;
        for (uint8_t i = 0; i < ARRAY_SIZE(scanStepValues); i++)
        {
            if (scanStepValues[i] >= gTxVfo->StepFrequency)
            {
                settings.scanStepIndex = i;
                break;
            }
        }
        settings.stepsCount = STEPS_128;
        #ifdef ENABLE_FEAT_F4HWN_RESUME_STATE
            gEeprom.CURRENT_STATE = 5;
        #endif
    }
    else {
#endif
        currentFreq = initialFreq = gTxVfo->pRX->Frequency -
                                    ((GetStepsCount() / 2) * GetScanStep());
        #ifdef ENABLE_FEAT_F4HWN_RESUME_STATE
            gEeprom.CURRENT_STATE = 4;
        #endif
    }

    #ifdef ENABLE_FEAT_F4HWN_RESUME_STATE
        SETTINGS_WriteCurrentState();
    #endif

    BackupRegisters();

    isListening = true; // to turn off RX later
    redrawStatus = true;
    redrawScreen = true;
    newScanStart = true;

    ToggleRX(true), ToggleRX(false); // hack to prevent noise when squelch off
    RADIO_SetModulation(settings.modulationType = gTxVfo->Modulation);

#ifdef ENABLE_FEAT_F4HWN_SPECTRUM
    BK4819_SetFilterBandwidth(settings.listenBw, false);
#else
    BK4819_SetFilterBandwidth(settings.listenBw = BK4819_FILTER_BW_WIDE, false);
#endif

    RelaunchScan();

    memset(rssiHistory, 0, sizeof(rssiHistory));

    isInitialized = true;

    while (isInitialized)
    {
        Tick();
    }

    BACKLIGHT_TurnOn();
}

bool APP_IsSpectrumActive(void)
{
    bool is_spectrum_active = isInitialized;
    return is_spectrum_active;
}