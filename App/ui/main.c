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
#include <stdlib.h>  // abs()

#include "app/chFrScanner.h"
#include "app/dtmf.h"
#ifdef ENABLE_AM_FIX
    #include "am_fix.h"
#endif
#include "bitmaps.h"
#include "board.h"
#include "font.h"
#include "driver/bk4819.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "functions.h"
#include "helper/battery.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "ui/helper.h"
#include "ui/battery.h"
#include "ui/inputbox.h"
#include "ui/main.h"
#include "ui/ui.h"
#include "audio.h"
#include "menu.h"

#ifdef ENABLE_FEAT_F4HWN
    #include "driver/system.h"
    #include "ui/dualvfo_u8g2_freq.h"
    #include "ui/dualvfo_smeter_xbm.h"
#endif

center_line_t center_line = CENTER_LINE_NONE;

#ifdef ENABLE_FEAT_F4HWN
    // static int8_t RxBlink;
    static int8_t RxBlinkLed = 0;
    static int8_t RxBlinkLedCounter;
    static int8_t RxLine;
    static uint32_t RxOnVfofrequency;

    static bool isMainOnly()
    {
        const bool dual_watch_is_off = (gEeprom.DUAL_WATCH == DUAL_WATCH_OFF);
        const bool cross_band_is_off = (gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF);
        const bool is_main_only_mode = dual_watch_is_off && cross_band_is_off;

        if (!is_main_only_mode) {
            return false;
        }

        const bool is_main_screen = (gScreenToDisplay == DISPLAY_MAIN);
        const bool is_channel_scan_running = (gScanStateDir != SCAN_OFF);
        const bool has_cross_band_backup = (gBackup_CROSS_BAND_RX_TX != CROSS_BAND_OFF);
        const bool keep_dual_layout_on_scan =
            is_main_screen && is_channel_scan_running && has_cross_band_backup;

        if (keep_dual_layout_on_scan) {
            return false;
        }

        return true;
    }
#endif

const char *VfoStateStr[] = {
       [VFO_STATE_NORMAL]="",
       [VFO_STATE_BUSY]="BUSY",
       [VFO_STATE_BAT_LOW]="BAT LOW",
       [VFO_STATE_TX_DISABLE]="TX DISABLE",
       [VFO_STATE_TIMEOUT]="TIMEOUT",
       [VFO_STATE_ALARM]="ALARM",
       [VFO_STATE_VOLTAGE_HIGH]="VOLT HIGH"
};

#ifdef ENABLE_FEAT_F4HWN
/* Dual-VFO main layout (RxMode != MAIN ONLY): display only; no radio logic changes. */
static bool DualVfoShouldUseLegacyMain(void)
{
#ifdef ENABLE_SCAN_RANGES
    if (gScanRangeStart)
        return true;
#endif
    if (gDTMF_InputMode
#ifdef ENABLE_DTMF_CALLING
        || gDTMF_CallState != DTMF_CALL_STATE_NONE || gDTMF_IsTx
#endif
        )
        return true;
    return false;
}

/* 主界面、频率(VFO)模式、正在键入频率数字：专用全屏输入页，不走双面板/旧布局 */
static bool DualVfoMainFreqEntryScreen(void)
{
    return gScreenToDisplay == DISPLAY_MAIN && !gAirCopyBootMode && gInputBoxIndex > 0 &&
           IS_FREQ_CHANNEL(gEeprom.ScreenChannel[gEeprom.TX_VFO]);
}

static void UI_DisplayMain_FreqInputBare(void)
{
    char            fs[16];
    const char *    ascii     = INPUTBOX_GetAscii();
    const uint32_t  frequency = gEeprom.VfoInfo[gEeprom.TX_VFO].pRX->Frequency;
    const bool      isGigaF   = frequency >= _1GHz_in_KHz;

    sprintf(fs, "%.*s.%.3s", 3 + (unsigned)isGigaF, ascii, ascii + 3 + (unsigned)isGigaF);

    /* 仅大字频率，水平居中；与旧逻辑相同格式，占 framebuffer 两行 */
    UI_PrintString(fs, 0, LCD_WIDTH, 3, 8);
}

static const char *DualVfoPowerWord(uint8_t vfoIdx)
{
    const VFO_Info_t *v = &gEeprom.VfoInfo[vfoIdx];
    uint8_t           p = v->OUTPUT_POWER % 8u;
    if (p == OUTPUT_POWER_USER)
        p = (uint8_t)(gSetting_set_pwr + 1u);
    static const char *const lowNames[5] = {"LOW1", "LOW2", "LOW3", "LOW4", "LOW5"};
    if (p >= 1u && p <= 5u)
        return lowNames[p - 1u];
    if (p == 6u)
        return "MID";
    if (p == 7u)
        return "HIGH";
    return "";
}

static void DualVfoFmtChId(unsigned int vfoIdx, char *out, size_t outLen)
{
    const uint16_t ch = gEeprom.ScreenChannel[vfoIdx];
    if (IS_MR_CHANNEL(ch))
        snprintf(out, outLen, "M-%04u", (unsigned)(ch + 1u));
    else if (IS_FREQ_CHANNEL(ch))
    {
        const uint8_t f   = (uint8_t)(1u + ch - FREQ_CHANNEL_FIRST);
        const bool    gig = gEeprom.VfoInfo[vfoIdx].pRX->Frequency >= _1GHz_in_KHz;
        snprintf(out, outLen, gig ? "F%u+" : "F%u", (unsigned)f);
    }
#ifdef ENABLE_NOAA
    else if (IS_NOAA_CHANNEL(ch))
        snprintf(out, outLen, "N%u", (unsigned)(1u + ch - NOAA_CHANNEL_FIRST));
#endif
    else
        snprintf(out, outLen, "?");
}

static void DualVfoHeaderLeft(unsigned int vfoIdx, char *out, size_t outLen)
{
    const uint16_t ch = gEeprom.ScreenChannel[vfoIdx];
    if (IS_MR_CHANNEL(ch))
    {
        SETTINGS_FetchChannelName(out, ch);
        if (out[0] == 0)
            snprintf(out, outLen, "CH%04u", (unsigned)(ch + 1u));
    }
    else
        snprintf(out, outLen, "VFO");
    out[outLen - 1u] = 0;
    if (strlen(out) > (size_t)CHANNEL_NAME_MAX_BYTES)
        out[CHANNEL_NAME_MAX_BYTES] = 0;
}

static void DualVfoHeaderRight(unsigned int vfoIdx, char *out, size_t outLen)
{
    const VFO_Info_t *v = &gEeprom.VfoInfo[vfoIdx];
#ifdef ENABLE_FEAT_F4HWN_NARROWER
    bool narrower = (v->CHANNEL_BANDWIDTH == BANDWIDTH_NARROW && gSetting_set_nfm == 1);
    const char *bw =
        (v->CHANNEL_BANDWIDTH == BANDWIDTH_WIDE) ? "WIDE" : (narrower ? "NAR+" : "NAR");
#else
    const char *bw = (v->CHANNEL_BANDWIDTH == BANDWIDTH_WIDE) ? "WIDE" : "NAR";
#endif
    snprintf(out, outLen, "%s %s %s", gModulationStr[v->Modulation], bw, DualVfoPowerWord(vfoIdx));
}

/* 最小字宽约 4px；S 表左侧清空到该列，与频率区错开 */
#define DUAL_VFO_FREQ_COL 62u

/*
 * 双 VFO 像素布局；主信道 A/B 宽 7px、高 6px；下面板 A/B 再各 +2px；A/B 下 1px 再画信道号；右侧 2x 频率。
 */
#define DUAL_VFO_AB_TALL_H 6u
#define DUAL_VFO_AB_TALL_W 7u
#define DUAL_VFO_AB_BOT_H  (DUAL_VFO_AB_TALL_H + 2u)
#define DUAL_VFO_AB_BOT_W  (DUAL_VFO_AB_TALL_W + 2u)
#define DV_Y_TOP_HDR   0u
#define DV_Y_TOP_AB    10u /* 主信道 A/B 及其下方信道号下移 2px */
#define DV_Y_TOP_CH    9u  /* 主信道右侧 2x 频率基线 */
#define DV_Y_TOP_DET   22u
#define DV_Y_BOT_HDR   29u
#define DV_Y_BOT_MAIN       39u /* A/B 上移 1px；旁信道号/RX 用 DV_Y_BOT_BESIDE_AB */
#define DV_Y_BOT_BESIDE_AB  (DV_Y_BOT_MAIN + 2u) /* 框旁信道号、RX、TX 字下移 2px */
#define DV_Y_BOT_FREQ_LINE  ((DV_Y_BOT_MAIN + DUAL_VFO_AB_BOT_H + 1u) - 9u) /* 相对 -10 参考：下移 1px（整串含末两位） */
/* 底栏：S 表在左；右为电池 8px + 其下居中百分比；DV_Y_METER 对齐电池页顶 */
#define DV_Y_METER          50u /* 电池图标（及底栏）下移 2px */
#define DV_BAT_ICON_H       8u
#define DV_Y_PCT            (DV_Y_METER + DV_BAT_ICON_H - 1u) /* 百分比下移 1px */
#define DV_Y_RXMODE         (DV_Y_METER + 7u)            /* 右下角 A/B 模式上移 4px */
#define DV_BAT_FLUSH_RIGHT  0u /* batX = LCD_WIDTH - batW - 此值，0 即贴右 */
#define DV_BAT_MODE_SHIFT_R (-1) /* 右下角 A/B 模式相对原布局左移 3px（原 +2 -> 现 -1） */
#define DV_BAT_PCT_SHIFT_R  2u /* 电量百分比再右移 */
#define DV_BAT_MODE_PCT_HGAP_PX 4u /* 底栏：模式字与电量百分比水平间隔 */
/* S 表：UV-KX 位图 + 离散 3×3 块（与 UI_DrawRSSI 一致）；S/+dB 列对齐 posX+38 */
#define DV_SMETER_BAR_X0    1u
#define DV_SMETER_LABEL_Y   (DV_Y_METER + 3u) /* 与 UV-KX DisplayRSSIBar 中 UI_DrawRSSI 的 y 一致 */
#define DV_SMETER_SREAD_X   (DV_SMETER_BAR_X0 + 38u)
#define DV_SMETER_SVALUE_Y  (DV_Y_METER + 2u) /* S 读数；较 +4 再上移 2px */
#define DV_SMETER_DBB_Y     (DV_Y_METER + 8u) /* +dB 第二行；较 +10 再上移 2px */
#define DV_STATUS_BAR_LINE1_Y  (DV_Y_METER + 0u)
#define DV_STATUS_BAR_LINE2_Y  (DV_Y_METER + 7u)

/* 副信道频率：3 列宽 2+2+1px（较原 1px/列总宽约 +2）；CHAR_W 含字后空，间隔较原再减 1px */
/* 副频 u8g2 加粗字后竖向占位略增，RX/TX XOR 条与之匹配 */
#define DUAL_VFO_SUB_FREQ_H       12u
#define DUAL_VFO_SUB_FREQ_CHAR_W  7u
/* A/B 行下方预留 1 像素再显示信道号 */
#define DV_Y_TOP_CHID    (DV_Y_TOP_AB + DUAL_VFO_AB_TALL_H + 1u)
/* Tx 偏提示：仅主信道；副信道不显示 */
#define DV_TXOFS_GAP_L_MAIN 16u

static void DualVfoXorHStripColumns(uint8_t x0, uint8_t yTop, uint8_t yBottom)
{
    for (uint8_t y = yTop; y <= yBottom; y++)
    {
        const uint8_t bit = (uint8_t)(1u << (y % 8u));
        const uint8_t row = (uint8_t)(y / 8u);
        for (uint8_t x = x0; x < LCD_WIDTH; x++)
            gFrameBuffer[row][x] ^= bit;
    }
}

static void DualVfoFillRectBlack(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    for (uint8_t y = y0; y <= y1; y++)
        for (uint8_t x = x0; x <= x1; x++)
            PutPixel(x, y, true);
}

static void DualVfoClearRectPx(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    if (y0 > y1 || x0 > x1)
        return;
    for (uint8_t yy = y0; yy <= y1; yy++)
        for (uint8_t xx = x0; xx <= x1 && xx < LCD_WIDTH; xx++)
            PutPixel(xx, yy, false);
}

/* UV-KX / u8g2_DrawXBM：每字节 bit0 为最左像素，依次左→右 */
static void DualVfoDrawSmeterXbm(uint8_t x0, uint8_t y0)
{
    const unsigned stride = (DUALVFO_SMETER_XBM_WIDTH + 7u) / 8u;
    for (uint8_t py = 0; py < DUALVFO_SMETER_XBM_HEIGHT; py++)
    {
        for (uint8_t px = 0; px < DUALVFO_SMETER_XBM_WIDTH; px++)
        {
            const unsigned bi = (unsigned)py * stride + (unsigned)(px / 8u);
            const uint8_t  b  = dualvfo_smeter_xbm_bits[bi];
            if (b & (uint8_t)(1u << (px % 8u)))
            {
                const uint8_t x = (uint8_t)(x0 + px);
                const uint8_t y = (uint8_t)(y0 + py);
                if (x < LCD_WIDTH && y < LCD_HEIGHT)
                    PutPixel(x, y, true);
            }
        }
    }
}

/* shortenTopBlack：为真时去掉反色条上方 1px 黑带（下面信道名与 A/B 之间少 1px 连续黑） */
static void DualVfoDrawInvertedHeaderPx(uint8_t y, unsigned int vfoIdx, bool shortenTopBlack)
{
    char L[20];
    char R[22];
    /* 信道名行：使用 u8g2 小字；默认上沿 y-1 黑带，下扩至 y+6 */
    const uint8_t yTop = shortenTopBlack ? y : ((y > 0u) ? (uint8_t)(y - 1u) : 0u);
    const uint8_t yBot = (uint8_t)(y + 6u); /* 原 y..y+5 共 6 行字区，下扩 1px */
    DualVfoFillRectBlack(0, yTop, (uint8_t)(LCD_WIDTH - 1u), yBot);
    DualVfoHeaderLeft(vfoIdx, L, sizeof(L));
    DualVfoHeaderRight(vfoIdx, R, sizeof(R));
    /* 黑底范围不变；字下移 1 像素 */
    DualVfoU8g2_DrawSmallText(L, 2u, (uint8_t)(y + 1u), false);
    {
        const unsigned int rw = (unsigned int)DualVfoU8g2_GetSmallTextWidth(R);
        const uint8_t      x = (rw < LCD_WIDTH - 4u) ? (uint8_t)(LCD_WIDTH - 2u - rw) : 2u;
        DualVfoU8g2_DrawSmallText(R, x, (uint8_t)(y + 1u), false);
    }
}

/* 单字符 3x5 纵向映射到高度 tallH，列宽 2px（总宽约 6px） */
static void DualVfoDrawChar3x5Tall(uint8_t x0, uint8_t y0, char ch, bool setBlack, uint8_t tallH)
{
    if ((uint8_t)ch < 0x20u || (uint8_t)ch > 0x7fu)
        return;
    const uint8_t ci = (uint8_t)(ch - 0x20u);
    for (unsigned col = 0; col < 3u; col++)
    {
        uint8_t pixels = gFont3x5[ci][col];
        for (unsigned sr = 0; sr < 6u; sr++)
        {
            const unsigned dr0 = (sr * tallH) / 6u;
            const unsigned dr1 = ((sr + 1u) * tallH + 5u) / 6u;
            if (pixels & 1u)
            {
                for (unsigned dr = dr0; dr < dr1 && dr < tallH; dr++)
                    for (unsigned dx = 0; dx < 2u; dx++)
                    {
                        const uint8_t px = (uint8_t)(x0 + col * 2u + dx);
                        if (px < LCD_WIDTH)
                            PutPixel(px, (uint8_t)(y0 + (uint8_t)dr), setBlack);
                    }
            }
            pixels >>= 1;
        }
    }
}

static void DualVfoDrawTallAbInverse(uint8_t x0, uint8_t y0, char ch)
{
    for (uint8_t py = 0; py < DUAL_VFO_AB_TALL_H; py++)
        for (uint8_t px = 0; px < DUAL_VFO_AB_TALL_W; px++)
            PutPixel((uint8_t)(x0 + px), (uint8_t)(y0 + py), true);
    DualVfoDrawChar3x5Tall((uint8_t)(x0 + 1u), y0, ch, false, DUAL_VFO_AB_TALL_H);
}

static void DualVfoDrawAb1pxBlackMargins(uint8_t y, uint8_t xL, uint8_t xR)
{
    const uint8_t yB = (uint8_t)(y + DUAL_VFO_AB_TALL_H - 1u);
    if (y >= 1u)
        DualVfoFillRectBlack((uint8_t)(xL - 1u), (uint8_t)(y - 1u), (uint8_t)(xR + 1u), (uint8_t)(y - 1u));
    DualVfoFillRectBlack((uint8_t)(xL - 1u), y, (uint8_t)(xL - 1u), yB);
    DualVfoFillRectBlack((uint8_t)(xR + 1u), y, (uint8_t)(xR + 1u), yB);
}

/* A/B 闪烁：phase 每刷新递增；>>DV_DUAL_VFO_AB_BLINK_SH 为半周期（越小越快） */
#define DV_DUAL_VFO_AB_BLINK_SH 1u
static uint16_t s_DualVfoAbBlinkPhase;
static bool     s_DualVfoAbBlinkShowAb = true;
static bool     s_DualVfoAbBlinkPrevRx;
static unsigned s_DualVfoAbBlinkPrevRxVfo;

/* 内区 innerL..innerR 为 abW×abH。主：黑底+镂空 A/B（固定 DUAL_VFO_AB_TALL_*）；副：空心框+最小字 A/B 框内居中（BOT_*）。
 * 仅当本 VFO 正在接收时 A/B 随 s_DualVfoAbBlinkShowAb 间歇擦除（闪烁）；RX/TX 字每帧照画。
 * labelY：框旁 RX/TX 最小字基线 y（上面板与 y 相同；下面板为 DV_Y_BOT_BESIDE_AB）。
 * rxBesideAb：主信道在框旁画 RX；副信道 RX 由 DualVfoDrawBottomChannel 绘制（与框隔 2px）。 */
static void DualVfoDrawAbRxTxOnlyPx(unsigned int vfoIdx, uint8_t y, unsigned int activeTxVFO,
                                    bool topInverseStyle, bool rxBesideAb, uint8_t abW, uint8_t abH,
                                    uint8_t labelY)
{
    const char letter = (vfoIdx == 0) ? 'A' : 'B';

    const bool rxHere =
        (bool)(FUNCTION_IsRx() && gEeprom.RX_VFO == vfoIdx && VfoState[vfoIdx] == VFO_STATE_NORMAL);
    const bool txHere =
        (bool)(gCurrentFunction == FUNCTION_TRANSMIT && activeTxVFO == vfoIdx);

    const uint8_t innerL = 1u;
    const uint8_t innerR = (uint8_t)(innerL + abW - 1u);
    const uint8_t rxX = (uint8_t)(4u + abW + 1u + 2u);
    const uint8_t txX = (uint8_t)(innerR + 2u + 2u);

    const bool    showAb = (!rxHere) || s_DualVfoAbBlinkShowAb;
    const uint8_t yTopC  = (y >= 1u) ? (uint8_t)(y - 1u) : 0u;
    uint8_t       yEndC  = (uint8_t)(y + abH);
    if (yEndC > 63u)
        yEndC = 63u;
    const uint8_t xClr1 = (uint8_t)(innerR + 2u); /* 主/副 同一水平占位（含 1px 边距带） */

    if (rxHere && !showAb)
        DualVfoClearRectPx(0, yTopC, xClr1, yEndC);

    if (!rxHere || showAb)
    {
        if (topInverseStyle)
        {
            DualVfoDrawAb1pxBlackMargins(y, innerL, innerR);
            DualVfoDrawTallAbInverse(innerL, y, letter);
        }
        else
        {
            /* 下面板：最小字 GUI_DisplaySmallest 单字约 4×6（与 helper.c 一致），在框内居中 */
            const int16_t boxLeft = (int16_t)innerL - 1;
            const int16_t boxTop = (int16_t)y;
            const int16_t boxRight = (int16_t)innerR;
            const int16_t boxBottom = (int16_t)y + (int16_t)abH;
            UI_DrawRectangleBuffer(gFrameBuffer, boxLeft, boxTop, boxRight, boxBottom, true);
            {
                char          abStr[2] = { letter, '\0' };
                const uint8_t smCellW  = 4u;
                const uint8_t smH      = 6u;
                const uint8_t tx =
                    (uint8_t)(innerL + (abW > smCellW ? (uint8_t)((abW - smCellW) / 2u) : 0u) + 1u);
                const uint8_t ty =
                    (uint8_t)(y + (abH > smH ? (uint8_t)((abH - smH) / 2u) : 0u) + 1u);
                DualVfoU8g2_DrawSmallText(abStr, tx, ty, true);
            }
        }
    }

    if (rxHere)
    {
        if (rxBesideAb)
            DualVfoU8g2_DrawSmallText("RX", rxX, labelY, true);
    }
    else if (txHere)
        DualVfoU8g2_DrawSmallText("TX", txX, labelY, true);
}

static void DualVfoDrawChIdSmallest(unsigned int vfoIdx, uint8_t x, uint8_t y)
{
    char chId[14];
    DualVfoFmtChId(vfoIdx, chId, sizeof(chId));
    DualVfoU8g2_DrawSmallText(chId, x, y, true);
}

/* TxOffs：与菜单相同存贮，格式化为无末尾 0 的小数字符串（如 6.8、0.5） */
static void DualVfoFmtTxOffsMHzTrim(char *out, size_t cap, uint32_t o)
{
    const unsigned hi = (unsigned)(o / 100000u);
    const unsigned lo = (unsigned)(o % 100000u);
    if (lo == 0u)
    {
        snprintf(out, cap, "%u", hi);
        return;
    }
    char frac[8];
    snprintf(frac, sizeof(frac), "%05u", lo);
    size_t n = strlen(frac);
    while (n > 1u && frac[n - 1u] == '0')
        n--;
    frac[n] = '\0';
    snprintf(out, cap, "%u.%s", hi, frac);
}

/* 频率数字左侧空隙内水平居中：TxODir（+/-）与 TxOffs 之间留 1px；OFF 时不显示 */
static void DualVfoDrawTxOffsetSmallCentered(unsigned int vfoIdx, uint8_t gapL, uint8_t xFreqStart, uint8_t y)
{
    const VFO_Info_t *v = &gEeprom.VfoInfo[vfoIdx];
    unsigned          d = (unsigned)v->TX_OFFSET_FREQUENCY_DIRECTION % 3u;

    if (d == TX_OFFSET_FREQUENCY_DIRECTION_OFF)
        return;

    if (xFreqStart <= gapL + 4u)
        return;

    char             num[16];
    const uint32_t   o = v->TX_OFFSET_FREQUENCY;
    DualVfoFmtTxOffsMHzTrim(num, sizeof(num), o);

    const char *const dir   = gSubMenu_SFT_D[d];
    const unsigned int dirw = (unsigned int)DualVfoU8g2_GetSmallTextWidth(dir);
    const unsigned int numw = (unsigned int)DualVfoU8g2_GetSmallTextWidth(num);
    /* 方向 + 1px 间隔 + 数值 */
    const unsigned int totalw = dirw + 1u + numw;

    const uint8_t      gapR = (uint8_t)(xFreqStart - 2u);
    const unsigned int maxw = (unsigned int)(gapR - gapL + 1u);

    if (totalw > maxw)
        return;

    const uint8_t x = (uint8_t)((unsigned int)gapL + (maxw - totalw) / 2u);
    if ((unsigned int)x + totalw > LCD_WIDTH)
        return;

    DualVfoU8g2_DrawSmallText(dir, x, y, true);
    DualVfoU8g2_DrawSmallText(num, (uint8_t)(x + dirw + 1u), y, true);
}

static void DualVfoDrawSubFreqSmallest(uint8_t y, uint32_t frequency, bool invertTail)
{
    /* font_10_tr 基线：原 y+11，整体上移 4px → y+7；下面板反色条占至 y=35，XOR 上沿不低于 36 */
    const uint8_t baseline = (uint8_t)(y + 7u);
    const uint8_t x_sub   = DualVfoU8g2_DrawSubFreqStrip(frequency, baseline);
    if (invertTail)
    {
        uint8_t xor_y0 = (y >= 4u) ? (uint8_t)(y - 4u) : y;
        if (xor_y0 < 36u)
            xor_y0 = 36u;
        const uint8_t xor_y1 = (uint8_t)(xor_y0 + DUAL_VFO_SUB_FREQ_H - 1u);
        DualVfoXorHStripColumns(x_sub, xor_y0, xor_y1);
    }
}

static void DualVfoDrawMainFreq2x(unsigned int vfoIdx, uint32_t frequency, bool invertTail)
{
    char fs[16];
    sprintf(fs, "%3u.%05u", (unsigned)(frequency / 100000u), (unsigned)(frequency % 100000u));
    const unsigned int w  = (unsigned int)strlen(fs) * 8u;
    uint8_t            x0 = (w < LCD_WIDTH - 4u) ? (uint8_t)(LCD_WIDTH - 2u - w) : 2u;
    if (x0 < 44u)
        x0 = 44u;
    {
        const uint8_t xf = DualVfoU8g2_MainFreqComputeDrawX(frequency, x0);
        /* 仅主频 u8g2 数字（含末两位）用 xf 右移；Tx 偏置小字仍按名义左缘 x0 居中，避免牵动其它元素 */
        DualVfoDrawTxOffsetSmallCentered(vfoIdx, DV_TXOFS_GAP_L_MAIN, x0, (uint8_t)(DV_Y_TOP_CH + 3u));
        DualVfoU8g2_DrawMainFreqStrip(frequency, xf, (uint8_t)(DV_Y_TOP_CH + 11u));
        if (invertTail)
            DualVfoXorHStripColumns(xf, DV_Y_TOP_CH, (uint8_t)(DV_Y_TOP_CH + 11u));
    }
}

static void DualVfoAppendTone(char *buf, size_t cap, char tag, const FREQ_Config_t *pc)
{
    const size_t n0 = strlen(buf);
    char         tmp[22];
    if (pc->CodeType == CODE_TYPE_OFF || n0 + 8u >= cap)
        return;
    if (pc->CodeType == CODE_TYPE_CONTINUOUS_TONE)
        snprintf(tmp, sizeof(tmp), "%c%u.%u", tag, CTCSS_Options[pc->Code] / 10u,
                 CTCSS_Options[pc->Code] % 10u);
    else if (pc->CodeType == CODE_TYPE_DIGITAL)
        snprintf(tmp, sizeof(tmp), "%c%03o", tag, DCS_Options[pc->Code]);
    else if (pc->CodeType == CODE_TYPE_REVERSE_DIGITAL)
        snprintf(tmp, sizeof(tmp), "%c%03oI", tag, DCS_Options[pc->Code]);
    else
        return;
    snprintf(buf + n0, cap - n0, " %s", tmp);
}

static void DualVfoDrawTopDetailRowPx(unsigned int topVfoIdx, uint8_t y)
{
    const VFO_Info_t *v = &gEeprom.VfoInfo[topVfoIdx];
    char              buf[48];
    uint8_t           sq = gEeprom.SQUELCH_LEVEL;
    if (sq > 9u)
        sq = 9u;
    snprintf(buf, sizeof(buf), "SQ%u", sq);
    if ((v->StepFrequency / 100u) < 100u)
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " %d.%02uK", v->StepFrequency / 100,
                 v->StepFrequency % 100);
    else
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " %dK", v->StepFrequency / 100);
    DualVfoAppendTone(buf, sizeof(buf), 'R', &v->freq_config_RX);
    DualVfoAppendTone(buf, sizeof(buf), 'T', &v->freq_config_TX);
    {
        const unsigned int lw = (unsigned int)DualVfoU8g2_GetSmallTextWidth(buf);
        const uint8_t      x  = (lw < LCD_WIDTH - 4u) ? (uint8_t)(LCD_WIDTH - 2u - lw) : 2u;
        DualVfoU8g2_DrawSmallText(buf, x, y, true);
    }
}

static void DualVfoDrawTopChannel(unsigned int vfoIdx)
{
    const unsigned int activeTxVFO = gRxVfoIsActive ? gEeprom.RX_VFO : gEeprom.TX_VFO;
    enum VfoState_t    state       = VfoState[vfoIdx];

#ifdef ENABLE_ALARM
    if (gCurrentFunction == FUNCTION_TRANSMIT && gAlarmState == ALARM_STATE_SITE_ALARM && activeTxVFO == vfoIdx)
        state = VFO_STATE_ALARM;
#endif

    DualVfoDrawInvertedHeaderPx(DV_Y_TOP_HDR, vfoIdx, false);

    if (state != VFO_STATE_NORMAL)
    {
        if (state < ARRAY_SIZE(VfoStateStr))
            DualVfoU8g2_DrawSmallText(VfoStateStr[state], 2u, DV_Y_TOP_AB, true);
        return;
    }

    DualVfoDrawAbRxTxOnlyPx(vfoIdx, DV_Y_TOP_AB, activeTxVFO, true, true, DUAL_VFO_AB_TALL_W,
                            DUAL_VFO_AB_TALL_H, DV_Y_TOP_AB);
    /* A/B 下 1px 间隔后左侧显示信道号 */
    DualVfoDrawChIdSmallest(vfoIdx, 2, DV_Y_TOP_CHID);

    {
        const bool rxHere =
            (bool)(FUNCTION_IsRx() && gEeprom.RX_VFO == vfoIdx && VfoState[vfoIdx] == VFO_STATE_NORMAL);
        const bool txHere =
            (bool)(gCurrentFunction == FUNCTION_TRANSMIT && activeTxVFO == vfoIdx);
        uint32_t   frequency = gEeprom.VfoInfo[vfoIdx].pRX->Frequency;
        if (txHere)
            frequency = gEeprom.VfoInfo[vfoIdx].pTX->Frequency;
        bool cnSwap = false;
#ifdef ENABLE_CHINESE
        if (IS_MR_CHANNEL(gEeprom.ScreenChannel[vfoIdx])) {
            char cn[20];
            SETTINGS_FetchChannelName(cn, gEeprom.ScreenChannel[vfoIdx]);
            if (SETTINGS_ChannelNameHasCjkUtf8(cn)) {
                cnSwap = true;
                char fs[16];
                sprintf(fs, "%3u.%05u", (unsigned)(frequency / 100000u), (unsigned)(frequency % 100000u));
                {
                    /* 必须与 DualVfoDrawMainFreq2x 相同：用 strlen×8 估宽算 x0。
                     * 若改用 GetSmallTextWidth，x0 与非中文不一致，频差会相对「假想的 2x 频」偏一大截（约数像素）。 */
                    const unsigned int freqNominalWidthPx = (unsigned int)strlen(fs) * 8u;
                    uint8_t            x0;
                    if (freqNominalWidthPx < LCD_WIDTH - 4u)
                        x0 = (uint8_t)(LCD_WIDTH - 2u - freqNominalWidthPx);
                    else
                        x0 = 2u;
                    if (x0 < 44u)
                        x0 = 44u;
                    DualVfoDrawTxOffsetSmallCentered(vfoIdx, DV_TXOFS_GAP_L_MAIN, x0,
                                                     (uint8_t)(DV_Y_TOP_CH + 3u));
                }
                if (rxHere)
                    UI_PrintStringSmallAtPixelCnInverse(cn, DUAL_VFO_FREQ_COL, 127, 9, 20);
                else
                    UI_PrintStringSmallAtPixel(cn, DUAL_VFO_FREQ_COL, 127, 17, 28, 0);
                DualVfoFillRectBlack(2u, DV_Y_TOP_HDR, 60u, (uint8_t)(DV_Y_TOP_HDR + 6u));
                DualVfoU8g2_DrawSmallText(fs, 2u, (uint8_t)(DV_Y_TOP_HDR + 1u), false);
            }
        }
#endif
        if (!cnSwap)
            DualVfoDrawMainFreq2x(vfoIdx, frequency, rxHere || txHere);
    }

    DualVfoDrawTopDetailRowPx(vfoIdx, DV_Y_TOP_DET);
}

static void DualVfoDrawBottomChannel(unsigned int vfoIdx)
{
    const unsigned int activeTxVFO = gRxVfoIsActive ? gEeprom.RX_VFO : gEeprom.TX_VFO;
    enum VfoState_t    state       = VfoState[vfoIdx];

#ifdef ENABLE_ALARM
    if (gCurrentFunction == FUNCTION_TRANSMIT && gAlarmState == ALARM_STATE_SITE_ALARM && activeTxVFO == vfoIdx)
        state = VFO_STATE_ALARM;
#endif

    DualVfoDrawInvertedHeaderPx(DV_Y_BOT_HDR, vfoIdx, true);

    if (state != VFO_STATE_NORMAL)
    {
        if (state < ARRAY_SIZE(VfoStateStr))
            DualVfoU8g2_DrawSmallText(VfoStateStr[state], 2u, DV_Y_BOT_MAIN, true);
        return;
    }

    DualVfoDrawAbRxTxOnlyPx(vfoIdx, DV_Y_BOT_MAIN, activeTxVFO, false, false, DUAL_VFO_AB_BOT_W,
                            DUAL_VFO_AB_BOT_H, DV_Y_BOT_BESIDE_AB);

    const bool rxHere =
        (bool)(FUNCTION_IsRx() && gEeprom.RX_VFO == vfoIdx && VfoState[vfoIdx] == VFO_STATE_NORMAL);
    const bool txHere =
        (bool)(gCurrentFunction == FUNCTION_TRANSMIT && activeTxVFO == vfoIdx);

    {
        uint32_t frequency = gEeprom.VfoInfo[vfoIdx].pRX->Frequency;
        if (txHere)
            frequency = gEeprom.VfoInfo[vfoIdx].pTX->Frequency;

        /* 框右缘 innerR=1+abW-1 后留 2px 再画信道号；接收只画 RX、不画信道号 */
        {
            char            chId[14];
            const uint8_t besideX0 = (uint8_t)(1u + DUAL_VFO_AB_BOT_W + 2u); /* innerR + 1 + 2px 间隔 */
            if (rxHere)
                DualVfoU8g2_DrawSmallText("RX", besideX0, DV_Y_BOT_BESIDE_AB, true);
            else
            {
                DualVfoFmtChId(vfoIdx, chId, sizeof(chId));
                uint8_t xch = besideX0;
                if (txHere)
                    xch = (uint8_t)(xch + 9u);
                DualVfoU8g2_DrawSmallText(chId, xch, DV_Y_BOT_BESIDE_AB, true);
            }
        }

        {
            bool cnSwap = false;
#ifdef ENABLE_CHINESE
            if (IS_MR_CHANNEL(gEeprom.ScreenChannel[vfoIdx])) {
                char cn[20];
                SETTINGS_FetchChannelName(cn, gEeprom.ScreenChannel[vfoIdx]);
                if (SETTINGS_ChannelNameHasCjkUtf8(cn)) {
                    cnSwap = true;
                    /* 条左侧信道名 → 频率 */
                    {
                        char fs[16];
                        sprintf(fs, "%3u.%05u", (unsigned)(frequency / 100000u), (unsigned)(frequency % 100000u));
                        DualVfoFillRectBlack(2u, DV_Y_BOT_HDR, 60u, (uint8_t)(DV_Y_BOT_HDR + 6u));
                        DualVfoU8g2_DrawSmallText(fs, 2u, (uint8_t)(DV_Y_BOT_HDR + 1u), false);
                    }
                    /* 中文信道名 → 频率位置（接收时反色） */
                    {
                        const uint8_t cn_y = (uint8_t)(DV_Y_BOT_FREQ_LINE + 6u);
                        if (rxHere) {
                            const uint8_t cn_y_rx = (uint8_t)(cn_y - 9u);
                            UI_PrintStringSmallAtPixelCnInverse(cn, DUAL_VFO_FREQ_COL, 127,
                                                                cn_y_rx, (uint8_t)(cn_y_rx + 11u));
                        } else {
                            UI_PrintStringSmallAtPixel(cn, DUAL_VFO_FREQ_COL, 127,
                                                       cn_y, (uint8_t)(cn_y + 11u), 0);
                        }
                    }
                }
            }
#endif
            if (!cnSwap)
                DualVfoDrawSubFreqSmallest(DV_Y_BOT_FREQ_LINE, frequency, rxHere || txHere);
        }
    }
}

/* 与菜单 RxMode（gSubMenu_RXMode）四项顺序一致，底栏单行缩写 */
static const char *DualVfoRxModeShortLabel(void)
{
    static const char *const abbrev[4] = {"MAIN", "A/B", "CROSS", "A"};
    const bool channel_scan_running = (gScanStateDir != SCAN_OFF);

    if (channel_scan_running) {
        return abbrev[0];
    }

    const bool cross_band_enabled_for_label = (gEeprom.CROSS_BAND_RX_TX != CROSS_BAND_OFF);
    const bool dual_watch_enabled_for_label = (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF);

    unsigned idx =
        (dual_watch_enabled_for_label ? 1u : 0u) + (cross_band_enabled_for_label ? 2u : 0u);
    if (idx >= 4u)
        idx = 0u;
    return abbrev[idx];
}

/* S9 以上：实际超出 S9 的分贝映射为显示 +1dB～+5dB（0～40 线性，40 以上视为 5 档） */
static uint8_t DualVfoMapOverS9ToDisplayStep(unsigned overRaw)
{
    if (overRaw >= 40u)
        return 5u;
    if (overRaw == 0u)
        return 1u;
    {
        int16_t m = map((int16_t)overRaw, 0, 40, 1, 5);
        if (m < 1)
            m = 1;
        if (m > 5)
            m = 5;
        return (uint8_t)m;
    }
}

/* UV-KX convertRSSIToSLevel：0..9 为档，10 为 >S9；与底栏方块个数一致 */
static uint8_t DualVfoConvertRssiToUvSLevel(int16_t rssi_dBm)
{
    static const int16_t kUvSLevelThresholds[] = {
        -121, -115, -109, -103, -97, -91, -85, -79, -73, -67
    };
    for (uint8_t level = 0; level < (uint8_t)ARRAY_SIZE(kUvSLevelThresholds); level++)
    {
        if (rssi_dBm <= kUvSLevelThresholds[level])
            return level;
    }
    return 10u;
}

/* UV-KX UI_DrawRSSI：u8g2_DrawBox(lcd, currentX, y + 6, 3, 3)，步进 4，最多 9 块 */
static void DualVfoDrawSmeterBoxesUv(uint8_t s_level, uint8_t x, uint8_t y)
{
    uint8_t currentX = x;
    for (uint8_t i = 0; i < s_level && i < 9u; i++)
    {
        const uint8_t y0 = (uint8_t)(y + 6u);
        const uint8_t y1 = (uint8_t)(y0 + 2u);
        DualVfoFillRectBlack(currentX, y0, (uint8_t)(currentX + 2u), y1);
        currentX = (uint8_t)(currentX + 4u);
    }
}

static uint8_t s_dual_vfo_last_speaking_channel = 0u;
static bool s_dual_vfo_has_rx_channel_history = false;

static void DualVfoDrawBottomStatusBar(uint8_t left_limit_x, uint8_t right_limit_x)
{
    (void)left_limit_x;
    (void)right_limit_x;

    const bool is_receiving_signal = FUNCTION_IsRx();
    if (is_receiving_signal)
    {
        const uint8_t current_rx_channel = gEeprom.RX_VFO;
        const bool current_rx_channel_is_valid = (current_rx_channel <= 1u);
        if (current_rx_channel_is_valid)
        {
            s_dual_vfo_last_speaking_channel = current_rx_channel;
            s_dual_vfo_has_rx_channel_history = true;
        }
    }

    const bool has_active_rx_channel = s_dual_vfo_has_rx_channel_history;
    const uint8_t active_channel = s_dual_vfo_last_speaking_channel;

    char line_1_text[16];
    char line_2_text[16];
    char arrow_text[4];
    if (active_channel == 0u)
    {
        strcpy(line_1_text, "LAST A");
        strcpy(line_2_text, "B");
        strcpy(arrow_text, "<<");
    }
    else
    {
        strcpy(line_1_text, "LAST A");
        strcpy(line_2_text, "B");
        strcpy(arrow_text, "<<");
    }

    const uint8_t line_1_prefix_width = DualVfoU8g2_GetSmallTextWidth("LAST ");
    const uint8_t channel_letter_width = DualVfoU8g2_GetSmallTextWidth("A");
    const uint8_t arrow_gap_width = DualVfoU8g2_GetSmallTextWidth(" ");

    const unsigned int status_anchor_x = 54u;
    const unsigned int line_1_x_u = status_anchor_x;
    const unsigned int line_2_x_u = line_1_x_u + (unsigned int)line_1_prefix_width;
    const unsigned int arrow_x_u = line_2_x_u + (unsigned int)channel_letter_width + (unsigned int)arrow_gap_width;
    const uint8_t line_1_x = (uint8_t)line_1_x_u;
    const uint8_t line_2_x = (uint8_t)line_2_x_u;
    const uint8_t arrow_x = (uint8_t)arrow_x_u;

    DualVfoClearRectPx((uint8_t)status_anchor_x, DV_STATUS_BAR_LINE1_Y, (uint8_t)(status_anchor_x + 34u), (uint8_t)(DV_STATUS_BAR_LINE2_Y + 6u));
    DualVfoU8g2_DrawSmallText(line_1_text, line_1_x, DV_STATUS_BAR_LINE1_Y, true);
    DualVfoU8g2_DrawSmallText(line_2_text, line_2_x, DV_STATUS_BAR_LINE2_Y, true);
    if (has_active_rx_channel && active_channel == 0u)
    {
        DualVfoU8g2_DrawSmallText(arrow_text, arrow_x, DV_STATUS_BAR_LINE1_Y, true);
    }
    else if (has_active_rx_channel)
    {
        DualVfoU8g2_DrawSmallText(arrow_text, arrow_x, DV_STATUS_BAR_LINE2_Y, true);
    }
}

/* 主画布最底行：S 表 + S 值 + 电池（最后绘制，独占一行） */
static void DualVfoDrawBottomSMeterAndBattery(void)
{
    const uint8_t row   = (uint8_t)(DV_Y_METER / 8u);
    uint8_t      *rowFb = gFrameBuffer[row];
    uint8_t      *rowFbNext =
        (row + 1u < FRAME_LINES) ? gFrameBuffer[row + 1u] : rowFb;

    for (unsigned c = 0; c < DUAL_VFO_FREQ_COL; c++)
    {
        rowFb[c] = 0;
        rowFbNext[c] = 0;
    }

    DualVfoDrawSmeterXbm(DV_SMETER_BAR_X0, DV_SMETER_LABEL_Y);

    {
        const bool   rxActive = FUNCTION_IsRx();
        int16_t      rssi_dBm = 0;
        uint8_t      s_level  = 0;
        char         s9b[8]   = "";
        char         dbb[10]  = "";

        if (rxActive)
        {
            rssi_dBm = BK4819_GetRSSI_dBm() + dBmCorrTable[gRxVfo->Band];
#ifdef ENABLE_AM_FIX
            if (gSetting_AM_fix && gRxVfo->Modulation == MODULATION_AM)
                rssi_dBm = (int16_t)(rssi_dBm + AM_fix_get_gain_diff());
#endif
            s_level = DualVfoConvertRssiToUvSLevel(rssi_dBm);
            const int16_t s9_dBm = -93;

            if (s_level >= 1u && s_level <= 9u)
                sprintf(s9b, "S%u", (unsigned)s_level);
            else if (s_level == 10u)
            {
                int32_t overDb = (int32_t)rssi_dBm - (int32_t)s9_dBm;
                if (overDb < 0)
                    overDb = 0;
                strcpy(s9b, "S9");
                sprintf(dbb, "+%udB", (unsigned)DualVfoMapOverS9ToDisplayStep((unsigned)overDb));
            }
            DualVfoDrawSmeterBoxesUv(s_level, DV_SMETER_BAR_X0, DV_SMETER_LABEL_Y);
        }

        if (s9b[0] != 0)
            DualVfoU8g2_DrawSmallText(s9b, DV_SMETER_SREAD_X, DV_SMETER_SVALUE_Y, true);
        if (dbb[0] != 0)
            DualVfoU8g2_DrawSmallText(dbb, DV_SMETER_SREAD_X, DV_SMETER_DBB_Y, true);
    }

    {
        const unsigned batW = (unsigned int)UI_BATTERY_ICON_WIDTH;
        const unsigned batX = LCD_WIDTH - batW - DV_BAT_FLUSH_RIGHT;
        uint8_t        bat[UI_BATTERY_ICON_WIDTH];

        for (unsigned c = batX; c < LCD_WIDTH; c++)
        {
            rowFb[c] = 0;
            rowFbNext[c] = 0;
        }
        UI_DrawBattery(bat, gBatteryDisplayLevel, gLowBatteryBlink);
        memcpy(rowFb + batX, bat, batW);

        char pb[8];
        sprintf(pb, "%u%%", (unsigned)gBatteryIconFillPercent);
        {
            const uint8_t textW = DualVfoU8g2_GetSmallTextWidth(pb);
            const uint8_t gapRx = 2u;
            uint8_t       pctPx;
            if (batW >= textW)
                pctPx = (uint8_t)(batX + (batW - textW) / 2u);
            else
                pctPx = (uint8_t)batX;
            if ((uint32_t)pctPx + textW > LCD_WIDTH)
                pctPx = (uint8_t)(LCD_WIDTH - textW);
            if ((uint32_t)pctPx + textW + DV_BAT_PCT_SHIFT_R <= LCD_WIDTH)
                pctPx = (uint8_t)(pctPx + DV_BAT_PCT_SHIFT_R);

            const char   *rxLab = DualVfoRxModeShortLabel();
            const uint8_t rxW   = DualVfoU8g2_GetSmallTextWidth(rxLab);
            const int32_t rxX = (int32_t)batX - (int32_t)gapRx - (int32_t)rxW + (int32_t)DV_BAT_MODE_SHIFT_R;
            const bool    drawRx =
                (rxX >= (int32_t)(DUAL_VFO_FREQ_COL + 1u) && (uint32_t)rxX + (uint32_t)rxW <= (uint32_t)batX);

            /* 百分比保持原位置（已在屏右侧）；仅把模式字左移，使「模式右缘」与「% 左缘」至少间隔 gap */
            int32_t       rx_draw_x = rxX;
            const uint8_t pct_draw_x = pctPx;

            if (drawRx)
            {
                const int32_t gap_i  = (int32_t)DV_BAT_MODE_PCT_HGAP_PX;
                const int32_t rw_i   = (int32_t)rxW;
                const int32_t min_rx = (int32_t)(DUAL_VFO_FREQ_COL + 1u);
                const int32_t pct_left_edge = (int32_t)pctPx;
                const int32_t max_rx_for_gap = pct_left_edge - gap_i - rw_i;

                if (rx_draw_x > max_rx_for_gap)
                    rx_draw_x = max_rx_for_gap;
                if (rx_draw_x < min_rx)
                    rx_draw_x = min_rx;
            }

            unsigned clearFrom = (unsigned)pct_draw_x;
            if (drawRx && (unsigned)rx_draw_x < clearFrom)
                clearFrom = (unsigned)rx_draw_x;
            for (unsigned c = clearFrom; c < LCD_WIDTH; c++)
            {
                rowFb[c] = 0;
                rowFbNext[c] = 0;
            }
            memcpy(rowFb + batX, bat, batW);
            if (drawRx)
                DualVfoU8g2_DrawSmallText(rxLab, (uint8_t)rx_draw_x, DV_Y_RXMODE, true);
            DualVfoU8g2_DrawSmallText(pb, pct_draw_x, DV_Y_PCT, true);
            const uint8_t smeter_text_right_x =
                (uint8_t)(DV_SMETER_SREAD_X + DualVfoU8g2_GetSmallTextWidth("+5dB"));
            uint8_t status_left_limit_x = (uint8_t)(DUAL_VFO_FREQ_COL + 1u);
            const uint8_t smeter_safe_left_x = (uint8_t)(smeter_text_right_x + 2u);
            if (smeter_safe_left_x > status_left_limit_x)
            {
                status_left_limit_x = smeter_safe_left_x;
            }
            uint8_t status_right_limit_x = (uint8_t)(batX - 2u);
            {
                uint8_t right_reserved_left_x = pct_draw_x;
                if (drawRx && rx_draw_x >= 0)
                {
                    const uint8_t rx_draw_x_u8 = (uint8_t)rx_draw_x;
                    if (rx_draw_x_u8 < right_reserved_left_x)
                    {
                        right_reserved_left_x = rx_draw_x_u8;
                    }
                }

                if (right_reserved_left_x > 2u)
                {
                    const uint8_t right_safe_x = (uint8_t)(right_reserved_left_x - 2u);
                    if (right_safe_x < status_right_limit_x)
                    {
                        status_right_limit_x = right_safe_x;
                    }
                }
            }

            DualVfoDrawBottomStatusBar(status_left_limit_x, status_right_limit_x);
        }
    }
}

static bool UI_DisplayMain_DualVfoTwoPanel(void)
{
    const unsigned int tx  = gEeprom.TX_VFO;
    const unsigned int oth = (unsigned int)(1u - tx);

    if (FUNCTION_IsRx())
    {
        /* 刚进入接收或切换 RX 信道时复位相位，避免沿用旧计数长时间不闪 */
        if (!s_DualVfoAbBlinkPrevRx ||
            s_DualVfoAbBlinkPrevRxVfo != (unsigned)gEeprom.RX_VFO)
        {
            s_DualVfoAbBlinkPhase     = 0;
            s_DualVfoAbBlinkShowAb    = true;
            s_DualVfoAbBlinkPrevRxVfo = (unsigned)gEeprom.RX_VFO;
        }
        s_DualVfoAbBlinkPrevRx = true;
        s_DualVfoAbBlinkPhase++;
        s_DualVfoAbBlinkShowAb =
            ((s_DualVfoAbBlinkPhase >> DV_DUAL_VFO_AB_BLINK_SH) & 1u) == 0u;
    }
    else
    {
        s_DualVfoAbBlinkPrevRx = false;
        s_DualVfoAbBlinkPhase  = 0;
        s_DualVfoAbBlinkShowAb = true;
    }

    DualVfoDrawTopChannel(tx);
    DualVfoDrawBottomChannel(oth);
    DualVfoDrawBottomSMeterAndBattery();

    RxLine = -1;
    return true;
}
#endif /* ENABLE_FEAT_F4HWN */

#ifdef ENABLE_FEAT_F4HWN
static void ST7565_BlitMainPerMode(void)
{
    if (UI_IsDualVfoMainScreen())
        ST7565_BlitFullScreenDualVfoTightTop();
    else
        ST7565_BlitFullScreen();
}
#endif

// ----------------------------------------

static void DrawSmallPowerBars(uint8_t *p, unsigned int level)
{
    if(level>6)
        level = 6;

    char bar = 0b00111110;

    for(uint8_t i = 0; i <= level; i++) {
        if(gSetting_set_gui) {
            bar = (0xff << (6-i)) & 0x7F;
        }
        memset(p + 2 + i*3, bar, 2);
    }
}
#if defined ENABLE_AUDIO_BAR || defined ENABLE_RSSI_BAR

static void DrawLevelBar(uint8_t xpos, uint8_t line, uint8_t level, uint8_t bars)
{
#ifndef ENABLE_FEAT_F4HWN
    const char hollowBar[] = {
        0b01111111,
        0b01000001,
        0b01000001,
        0b01111111
    };
#endif
    
    uint8_t *p_line = gFrameBuffer[line];
    level = MIN(level, bars);

    for(uint8_t i = 0; i < level; i++) {
#ifdef ENABLE_FEAT_F4HWN
        if(gSetting_set_met)
        {
            const char hollowBar[] = {
                0b01111111,
                0b01000001,
                0b01000001,
                0b01111111
            };

            if(i < bars - 4) {
                for(uint8_t j = 0; j < 4; j++)
                    p_line[xpos + i * 5 + j] = (~(0x7F >> (i + 1))) & 0x7F;
            }
            else {
                memcpy(p_line + (xpos + i * 5), &hollowBar, ARRAY_SIZE(hollowBar));
            }
        }
        else
        {
            const char hollowBar[] = {
                0b00111110,
                0b00100010,
                0b00100010,
                0b00111110
            };

            const char simpleBar[] = {
                0b00111110,
                0b00111110,
                0b00111110,
                0b00111110
            };

            if(i < bars - 4) {
                memcpy(p_line + (xpos + i * 5), &simpleBar, ARRAY_SIZE(simpleBar));
            }
            else {
                memcpy(p_line + (xpos + i * 5), &hollowBar, ARRAY_SIZE(hollowBar));
            }
        }
#else
        if(i < bars - 4) {
            for(uint8_t j = 0; j < 4; j++)
                p_line[xpos + i * 5 + j] = (~(0x7F >> (i+1))) & 0x7F;
        }
        else {
            memcpy(p_line + (xpos + i * 5), &hollowBar, ARRAY_SIZE(hollowBar));
        }
#endif
    }
}
#endif

#ifdef ENABLE_AUDIO_BAR

#define MIC_POPUP_WIDTH     52
#define MIC_POPUP_HEIGHT    47  /* 较原 52 缩短 5px */
#define MIC_POPUP_X0        ((LCD_WIDTH - MIC_POPUP_WIDTH) / 2)
#define MIC_POPUP_Y0        ((LCD_HEIGHT - MIC_POPUP_HEIGHT) / 2)

static void MicPopup_ClearRectPixels(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    uint8_t y;
    uint8_t x;

    for (y = y1; y <= y2; y++)
    {
        for (x = x1; x <= x2; x++)
        {
            UI_DrawPixelBuffer(gFrameBuffer, x, y, false);
        }
    }
}

static void MicPopup_BlitFullScreen(void)
{
#ifdef ENABLE_FEAT_F4HWN
    if (UI_IsDualVfoMainScreen())
        ST7565_BlitFullScreenDualVfoTightTop();
    else
#endif
        ST7565_BlitFullScreen();
}

/** 线稿话筒：圆角胶囊头 + 间隙 + U 形托 + 立柱 + 底座（参考扁平描边图标） */
static void MicPopup_DrawMicIcon(uint8_t center_x, uint8_t top_y)
{
    const int16_t cx = (int16_t)center_x;
    const int16_t y0 = (int16_t)top_y;
    int16_t       ya;
    int16_t       yb;
    int16_t       x;

    /* -------- 胶囊话筒头（仅描边） -------- */
    UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)(cx - 1), (uint8_t)(y0 + 0), true);
    UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)(cx + 0), (uint8_t)(y0 + 0), true);
    UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)(cx + 1), (uint8_t)(y0 + 0), true);

    UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)(cx - 2), (uint8_t)(y0 + 1), true);
    UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)(cx + 2), (uint8_t)(y0 + 1), true);

    ya = y0 + 2;
    yb = y0 + 6;
    UI_DrawLineBuffer(gFrameBuffer, cx - 2, ya, cx - 2, yb, true);
    UI_DrawLineBuffer(gFrameBuffer, cx + 2, ya, cx + 2, yb, true);

    UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)(cx - 1), (uint8_t)(y0 + 7), true);
    UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)(cx + 0), (uint8_t)(y0 + 7), true);
    UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)(cx + 1), (uint8_t)(y0 + 7), true);

    /* -------- 与 U 托之间的间隙（无像素）y0+8 -------- */

    /* -------- U 形托（开口朝上，略宽于话筒头） -------- */
    UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)(cx - 4), (uint8_t)(y0 + 9), true);
    UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)(cx + 4), (uint8_t)(y0 + 9), true);

    UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)(cx - 4), (uint8_t)(y0 + 10), true);
    UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)(cx + 4), (uint8_t)(y0 + 10), true);

    UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)(cx - 3), (uint8_t)(y0 + 11), true);
    UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)(cx + 3), (uint8_t)(y0 + 11), true);

    for (x = cx - 2; x <= cx + 2; x++)
    {
        UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)x, (uint8_t)(y0 + 12), true);
    }

    /* -------- 立柱 -------- */
    UI_DrawLineBuffer(gFrameBuffer, cx, y0 + 13, cx, y0 + 14, true);

    /* -------- 底座 -------- */
    UI_DrawLineBuffer(gFrameBuffer, cx - 3, y0 + 15, cx + 3, y0 + 15, true);
}

static uint32_t s_mic_wave_prng_state;

/** 简易 PRNG，每帧调用多次使波形持续变化 */
static uint16_t MicPopup_WaveRandU16(void)
{
    uint32_t x;

    x = s_mic_wave_prng_state;
    if (x == 0u)
    {
        x = 0xACE1u;
    }
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_mic_wave_prng_state = x;
    return (uint16_t)(x & 0xFFFFu);
}

/**
 * 波形区：中间水平轴线 + 密排竖线，每根在上/下随机不等长；
 * 整体包络为中间高、两侧低的菱形（参考音频波形示意）。
 */
static void MicPopup_DrawWaveformBand(uint8_t inner_left, uint8_t inner_right,
                                      uint8_t wave_top, uint8_t wave_bottom)
{
    uint8_t        center_y;
    uint8_t        max_up;
    uint8_t        max_dn;
    uint8_t        n_cols;
    uint8_t        peak;
    uint16_t       env_u16;
    uint8_t        up_limit;
    uint8_t        dn_limit;
    uint8_t        h_up;
    uint8_t        h_dn;
    uint8_t        y_top;
    uint8_t        y_bot;
    uint8_t        xi;
    uint8_t        col_idx;
    uint16_t       prng_a;
    uint16_t       prng_b;

    if (inner_right < inner_left || wave_bottom < wave_top)
    {
        return;
    }

    MicPopup_ClearRectPixels(inner_left, wave_top, inner_right, wave_bottom);

    center_y = (uint8_t)(((uint16_t)wave_top + (uint16_t)wave_bottom) / 2u);
    UI_DrawLineBuffer(gFrameBuffer, inner_left, center_y, inner_right, center_y, true);

    max_up = (uint8_t)(center_y - wave_top);
    max_dn = (uint8_t)(wave_bottom - center_y);

    n_cols = (uint8_t)(inner_right - inner_left + 1u);
    if (n_cols == 0u)
    {
        return;
    }

    peak = (uint8_t)((n_cols - 1u) / 2u);
    if (peak == 0u)
    {
        peak = 1u;
    }

    col_idx = 0u;
    while (col_idx < n_cols)
    {
        if (n_cols <= 1u)
        {
            env_u16 = 255u;
        }
        else if (col_idx <= peak)
        {
            env_u16 = (uint16_t)col_idx * 255u / (uint16_t)peak;
        }
        else
        {
            env_u16 = (uint16_t)(n_cols - 1u - col_idx) * 255u / (uint16_t)peak;
        }

        up_limit = (uint8_t)(((uint16_t)max_up * env_u16) / 255u);
        dn_limit = (uint8_t)(((uint16_t)max_dn * env_u16) / 255u);

        prng_a = MicPopup_WaveRandU16();
        prng_b = MicPopup_WaveRandU16();

        if (up_limit == 0u)
        {
            h_up = 0u;
        }
        else
        {
            h_up = (uint8_t)(prng_a % (uint16_t)(up_limit + 1u));
        }

        if (dn_limit == 0u)
        {
            h_dn = 0u;
        }
        else
        {
            h_dn = (uint8_t)(prng_b % (uint16_t)(dn_limit + 1u));
        }

        y_top = (uint8_t)(center_y - h_up);
        y_bot = (uint8_t)(center_y + h_dn);

        xi = (uint8_t)(inner_left + col_idx);
        UI_DrawLineBuffer(gFrameBuffer, xi, y_top, xi, y_bot, true);

        col_idx++;
    }
}

void UI_DisplayMicBarTxPopup(bool main_screen_just_redrawn)
{
    static uint8_t  sAnimTick                 = 0u;
    static bool     s_mic_popup_main_prepared = false;

    uint8_t         inner_left;
    uint8_t         inner_right;
    uint8_t         wave_top;
    uint8_t         wave_bottom;

    if (!gSetting_mic_bar)
    {
        s_mic_popup_main_prepared = false;
        return;
    }

    if (gLowBattery && !gLowBatteryConfirmed)
    {
        s_mic_popup_main_prepared = false;
        return;
    }

    if (gCurrentFunction != FUNCTION_TRANSMIT || gScreenToDisplay != DISPLAY_MAIN)
    {
        s_mic_popup_main_prepared = false;
        return;
    }

#ifdef ENABLE_DTMF_CALLING
    if (gDTMF_CallState != DTMF_CALL_STATE_NONE)
    {
        s_mic_popup_main_prepared = false;
        return;
    }
#endif

#if defined(ENABLE_ALARM) || defined(ENABLE_TX1750)
    if (gAlarmState != ALARM_STATE_OFF)
    {
        s_mic_popup_main_prepared = false;
        return;
    }
#endif

#ifdef ENABLE_FEAT_F4HWN
    RxBlinkLed = 0;
    RxBlinkLedCounter = 0;
    BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2_GREEN, false);
#endif

    /* 本周期 app 已整屏重画则不必再调；否则本段发射内仅首帧补一次主页，供弹窗外区域正确 */
    if (main_screen_just_redrawn)
    {
        s_mic_popup_main_prepared = true;
    }
    else if (!s_mic_popup_main_prepared)
    {
        GUI_DisplayScreen();
        s_mic_popup_main_prepared = true;
    }

    inner_left  = (uint8_t)(MIC_POPUP_X0 + 4u);
    inner_right = (uint8_t)(MIC_POPUP_X0 + MIC_POPUP_WIDTH - 5u);
    wave_top    = (uint8_t)(MIC_POPUP_Y0 + 22u);
    wave_bottom = (uint8_t)(MIC_POPUP_Y0 + MIC_POPUP_HEIGHT - 3u);

    /* 每帧同时绘制：方框 + 话筒 + 声纹（声纹在 MicPopup_DrawWaveformBand 内每帧重算，持续变化） */
    MicPopup_ClearRectPixels(MIC_POPUP_X0, MIC_POPUP_Y0,
                             (uint8_t)(MIC_POPUP_X0 + MIC_POPUP_WIDTH - 1u),
                             (uint8_t)(MIC_POPUP_Y0 + MIC_POPUP_HEIGHT - 1u));
    UI_DrawRectangleBuffer(gFrameBuffer, MIC_POPUP_X0, MIC_POPUP_Y0,
                           (int16_t)(MIC_POPUP_X0 + MIC_POPUP_WIDTH - 1),
                           (int16_t)(MIC_POPUP_Y0 + MIC_POPUP_HEIGHT - 1), true);

    MicPopup_DrawMicIcon((uint8_t)(LCD_WIDTH / 2u), (uint8_t)(MIC_POPUP_Y0 + 6u));

    sAnimTick++;
    s_mic_wave_prng_state ^= (uint32_t)sAnimTick * 0x9E3779B9u;
    s_mic_wave_prng_state ^= (uint32_t)gFlashLightBlinkCounter << 8;

    MicPopup_DrawWaveformBand(inner_left, inner_right, wave_top, wave_bottom);

    MicPopup_BlitFullScreen();
}
#endif /* ENABLE_AUDIO_BAR */

#ifdef ENABLE_FEAT_F4HWN
/* 供 UI_DisplayMainOnlyStatusBar / 菜单顶栏 5 格信号条：与 DisplayRSSIBar(F4HWN) 同一映射 */
static void F4HWN_UpdateGvfoRssiBarLevelForStatusBar(void)
{
    if (!FUNCTION_IsRx()) {
        gVFO_RSSI_bar_level[gEeprom.RX_VFO] = 0;
        return;
    }

    int16_t rssi_dBm =
        BK4819_GetRSSI_dBm()
#ifdef ENABLE_AM_FIX
        + ((gSetting_AM_fix && gRxVfo->Modulation == MODULATION_AM) ? AM_fix_get_gain_diff() : 0)
#endif
        + dBmCorrTable[gRxVfo->Band];

    const int16_t s9_dBm = -93;
    const int16_t s0_dBm = -141;

    uint8_t s_level    = 1;
    uint8_t overS9dBm  = 0;
    uint8_t overS9Bars = 0;

    if (rssi_dBm <= s9_dBm) {
        int16_t sn = map(rssi_dBm, s0_dBm, s9_dBm, 1, 9);
        if (sn < 1)
            sn = 1;
        if (sn > 9)
            sn = 9;
        s_level = (uint8_t)sn;
    } else {
        s_level = 9;
        {
            int32_t od = (int32_t)rssi_dBm - (int32_t)s9_dBm;
            if (od < 0)
                od = 0;
            if (od > 255)
                od = 255;
            overS9dBm = (uint8_t)od;
        }
        overS9Bars = MIN(overS9dBm / 10u, 4u);
    }

    const uint8_t raw = s_level + overS9Bars;
    gVFO_RSSI_bar_level[gEeprom.RX_VFO] = (raw * 6u + 6u) / 13u;
    if (gVFO_RSSI_bar_level[gEeprom.RX_VFO] > 6u)
        gVFO_RSSI_bar_level[gEeprom.RX_VFO] = 6u;
}

/*
 * 与 UI_DisplayMainOnlyStatusBar 中 5 格 RSSI 一致：仅接收时按 gVFO_RSSI_bar_level 换算格数；
 * 快照变化才置 gUpdateStatus，避免菜单 / MAIN ONLY 顶栏定时强刷。
 */
static void F4HWN_RequestStatusBarIfTopRssiSnapshotChanged(void)
{
    const bool on_menu = (gScreenToDisplay == DISPLAY_MENU);
    const bool on_main_only =
        (gScreenToDisplay == DISPLAY_MAIN && !gAirCopyBootMode &&
         gEeprom.DUAL_WATCH == DUAL_WATCH_OFF && gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF);
    const bool on_scanner = (gScreenToDisplay == DISPLAY_SCANNER);
#ifdef ENABLE_FMRADIO
    const bool on_fm = (gScreenToDisplay == DISPLAY_FM);
#else
    const bool on_fm = false;
#endif
    if (!on_menu && !on_main_only && !on_scanner && !on_fm)
        return;

    const bool     rx_active = FUNCTION_IsRx();
    uint8_t        bars_0_to_5 = 0u;
    if (rx_active)
    {
        const uint8_t rx_vfo_idx = gEeprom.RX_VFO;
        const unsigned int raw_bars =
            (unsigned int)gVFO_RSSI_bar_level[rx_vfo_idx] * 5u + 5u;
        unsigned int div6 = raw_bars / 6u;
        if (div6 > 5u)
            div6 = 5u;
        bars_0_to_5 = (uint8_t)div6;
    }

    const uint16_t snapshot_u16 =
        (uint16_t)((rx_active ? 1u : 0u) << 8) | (uint16_t)bars_0_to_5;
    static uint16_t s_prev_top_rssi_snapshot = 0xFFFFu;
    if (snapshot_u16 != s_prev_top_rssi_snapshot)
    {
        s_prev_top_rssi_snapshot = snapshot_u16;
        gUpdateStatus = true;
    }
}
#endif

void DisplayRSSIBar(const bool now)
{
#if defined(ENABLE_RSSI_BAR)

    const unsigned int txt_width    = 7 * 8;                 // 8 text chars
    const unsigned int bar_x        = 2 + txt_width + 4;     // X coord of bar graph

#ifdef ENABLE_FEAT_F4HWN
    /*
    const char empty[] = {
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
    };
    */

    unsigned int line;
    if (isMainOnly())
    {
        line = 5;
    }
    else
    {
        line = 3;
    }

    //char rx[4];
    //sprintf(String, "%d", RxBlink);
    //UI_PrintStringSmallBold(String, 80, 0, RxLine);

    if(RxLine >= 0 && center_line != CENTER_LINE_IN_USE && !isMainOnly())
    {
        static bool clean = false;
        uint8_t *p_line0 = gFrameBuffer[RxLine + 0];

        clean = !clean;

        if(clean) {
            for(uint8_t i = 0; i < sizeof(BITMAP_VFO_Default); i++)
                p_line0[i] = (p_line0[i] & 0x80) | BITMAP_VFO_Default[i];
        } else {
            for(uint8_t i = 0; i < sizeof(BITMAP_VFO_Empty); i++)
                p_line0[i] = (p_line0[i] & 0x80) | BITMAP_VFO_Empty[i];
        }

        ST7565_BlitLine(RxLine);
    }
#else
    const unsigned int line = 3;
#endif
    uint8_t           *p_line        = gFrameBuffer[line];
    char               str[16];

#ifndef ENABLE_FEAT_F4HWN
    const char plus[] = {
        0b00011000,
        0b00011000,
        0b01111110,
        0b01111110,
        0b01111110,
        0b00011000,
        0b00011000,
    };
#endif

    if (gEeprom.KEY_LOCK && gKeypadLocked > 0)
        return;

#ifdef ENABLE_FEAT_F4HWN
    const bool dualStatusRssi = !isMainOnly() && !DualVfoShouldUseLegacyMain();
#else
    const bool dualStatusRssi = false;
#endif

    if (!dualStatusRssi && center_line != CENTER_LINE_RSSI)
        return;     // display is in use

    if (gCurrentFunction == FUNCTION_TRANSMIT ||
        gScreenToDisplay != DISPLAY_MAIN
#ifdef ENABLE_DTMF_CALLING
        || gDTMF_CallState != DTMF_CALL_STATE_NONE
#endif
        )
        return;     // display is in use

    if (!dualStatusRssi && now)
        memset(p_line, 0, LCD_WIDTH);

#ifdef ENABLE_FEAT_F4HWN
    int16_t rssi_dBm =
        BK4819_GetRSSI_dBm()
#ifdef ENABLE_AM_FIX
        + ((gSetting_AM_fix && gRxVfo->Modulation == MODULATION_AM) ? AM_fix_get_gain_diff() : 0)
#endif
        + dBmCorrTable[gRxVfo->Band];

    // S9 = -93 dBm, S0 = -141 dBm (IARU standard)
    const int16_t s9_dBm = -93;
    const int16_t s0_dBm = -141;

    uint8_t s_level    = 1;
    uint8_t overS9dBm  = 0;
    uint8_t overS9Bars = 0;

    if (rssi_dBm <= s9_dBm) {
        int16_t sn = map(rssi_dBm, s0_dBm, s9_dBm, 1, 9);
        if (sn < 1)
            sn = 1;
        if (sn > 9)
            sn = 9;
        s_level = (uint8_t)sn;
    } else {
        /* 相对 S9 的实际 dB；格条约每 10dB 一格，最多 4 格 */
        s_level = 9;
        {
            int32_t od = (int32_t)rssi_dBm - (int32_t)s9_dBm;
            if (od < 0)
                od = 0;
            if (od > 255)
                od = 255;
            overS9dBm = (uint8_t)od;
        }
        overS9Bars = MIN(overS9dBm / 10u, 4u);
    }
#else
    const int16_t s0_dBm   = -gEeprom.S0_LEVEL;                  // S0 .. base level
    const int16_t rssi_dBm =
        BK4819_GetRSSI_dBm()
#ifdef ENABLE_AM_FIX
        + ((gSetting_AM_fix && gRxVfo->Modulation == MODULATION_AM) ? AM_fix_get_gain_diff() : 0)
#endif
        + dBmCorrTable[gRxVfo->Band];

    int s0_9 = gEeprom.S0_LEVEL - gEeprom.S9_LEVEL;
    const uint8_t s_level = MIN(MAX((int32_t)(rssi_dBm - s0_dBm)*100 / (s0_9*100/9), 0), 9); // S0 - S9
    uint8_t overS9dBm = MIN(MAX(rssi_dBm + gEeprom.S9_LEVEL, 0), 99);
    uint8_t overS9Bars = MIN(overS9dBm/10, 4);
#endif

#ifdef ENABLE_FEAT_F4HWN
    if (!dualStatusRssi)
    {
        if (gSetting_set_gui)
        {
            sprintf(str, "%3d", rssi_dBm);
            UI_PrintStringSmallNormal(str, LCD_WIDTH + 8, 0, line - 1);
        }
        else
        {
            sprintf(str, "% 4d %s", rssi_dBm, "dBm");
            if (isMainOnly())
                GUI_DisplaySmallest(str, 2, 41, false, true);
            else
                GUI_DisplaySmallest(str, 2, 25, false, true);
        }

        if (overS9Bars == 0)
            sprintf(str, "S%d", s_level);
        else
            sprintf(str, "+%udB", (unsigned)DualVfoMapOverS9ToDisplayStep((unsigned)overS9dBm));

        UI_PrintStringSmallNormal(str, LCD_WIDTH + 38, 0, line - 1);
    }
#else
    if(overS9Bars == 0) {
        sprintf(str, "% 4d S%d", -rssi_dBm, s_level);
    }
    else {
        sprintf(str, "% 4d  %2d", -rssi_dBm, overS9dBm);
        memcpy(p_line + 2 + 7*5, &plus, ARRAY_SIZE(plus));
    }

    UI_PrintStringSmallNormal(str, 2, 0, line);
#endif
    if (!dualStatusRssi)
    {
        DrawLevelBar(bar_x, line, s_level + overS9Bars, 13);
        if (now)
            ST7565_BlitLine(line);
    }
#ifdef ENABLE_FEAT_F4HWN
    F4HWN_UpdateGvfoRssiBarLevelForStatusBar();
#else
    {
        const uint8_t raw = s_level + overS9Bars;
        gVFO_RSSI_bar_level[gEeprom.RX_VFO] = (raw * 6u + 6u) / 13u;
        if (gVFO_RSSI_bar_level[gEeprom.RX_VFO] > 6u)
            gVFO_RSSI_bar_level[gEeprom.RX_VFO] = 6u;
    }
#endif
#else
    int16_t rssi = BK4819_GetRSSI();
    uint8_t Level;

    if (rssi >= gEEPROM_RSSI_CALIB[gRxVfo->Band][3]) {
        Level = 6;
    } else if (rssi >= gEEPROM_RSSI_CALIB[gRxVfo->Band][2]) {
        Level = 4;
    } else if (rssi >= gEEPROM_RSSI_CALIB[gRxVfo->Band][1]) {
        Level = 2;
    } else if (rssi >= gEEPROM_RSSI_CALIB[gRxVfo->Band][0]) {
        Level = 1;
    } else {
        Level = 0;
    }

    uint8_t *pLine = (gEeprom.RX_VFO == 0)? gFrameBuffer[2] : gFrameBuffer[6];
    if (now)
        memset(pLine, 0, 23);
    DrawSmallPowerBars(pLine, Level);
    if (now)
#ifdef ENABLE_FEAT_F4HWN
        ST7565_BlitMainPerMode();
#else
        ST7565_BlitFullScreen();
#endif
    gVFO_RSSI_bar_level[gEeprom.RX_VFO] = Level;
#endif

}

#ifdef ENABLE_AGC_SHOW_DATA
void UI_MAIN_PrintAGC(bool now)
{
    char buf[20];
    memset(gFrameBuffer[3], 0, 128);
    union {
        struct {
            uint16_t _ : 5;
            uint16_t agcSigStrength : 7;
            int16_t gainIdx : 3;
            uint16_t agcEnab : 1;
        };
        uint16_t __raw;
    } reg7e;
    reg7e.__raw = BK4819_ReadRegister(0x7E);
    uint8_t gainAddr = reg7e.gainIdx < 0 ? 0x14 : 0x10 + reg7e.gainIdx;
    union {
        struct {
            uint16_t pga:3;
            uint16_t mixer:2;
            uint16_t lna:3;
            uint16_t lnaS:2;
        };
        uint16_t __raw;
    } agcGainReg;
    agcGainReg.__raw = BK4819_ReadRegister(gainAddr);
    int8_t lnaShortTab[] = {-28, -24, -19, 0};
    int8_t lnaTab[] = {-24, -19, -14, -9, -6, -4, -2, 0};
    int8_t mixerTab[] = {-8, -6, -3, 0};
    int8_t pgaTab[] = {-33, -27, -21, -15, -9, -6, -3, 0};
    int16_t agcGain = lnaShortTab[agcGainReg.lnaS] + lnaTab[agcGainReg.lna] + mixerTab[agcGainReg.mixer] + pgaTab[agcGainReg.pga];

    sprintf(buf, "%d%2d %2d %2d %3d", reg7e.agcEnab, reg7e.gainIdx, -agcGain, reg7e.agcSigStrength, BK4819_GetRSSI());
    UI_PrintStringSmallNormal(buf, 2, 0, 3);
    if(now)
        ST7565_BlitLine(3);
}
#endif

void UI_MAIN_TimeSlice500ms(void)
{
#ifdef ENABLE_FEAT_F4HWN
    {
        const bool status_bar_rssi_slice =
            (gScreenToDisplay == DISPLAY_MENU) || (gScreenToDisplay == DISPLAY_SCANNER)
#ifdef ENABLE_FMRADIO
            || (gScreenToDisplay == DISPLAY_FM)
#endif
            ;
        if (status_bar_rssi_slice) {
            F4HWN_UpdateGvfoRssiBarLevelForStatusBar();
            F4HWN_RequestStatusBarIfTopRssiSnapshotChanged();
        }
    }
#endif
    if (gScreenToDisplay == DISPLAY_MAIN) {
#ifdef ENABLE_AGC_SHOW_DATA
        UI_MAIN_PrintAGC(true);
        return;
#endif

        if(FUNCTION_IsRx()) {
            DisplayRSSIBar(true);
#ifdef ENABLE_FEAT_F4HWN_RX_TX_TIMER
            // 接收时每 500ms 触发主屏刷新，使 RX 正计时实时更新
            gUpdateDisplay = true;
#endif
        }
        else {
#ifdef ENABLE_FEAT_F4HWN_RX_TX_TIMER
            // 发射时每 500ms 触发主屏刷新，使 TX 倒计时实时更新
            gUpdateDisplay = true;
#endif
#ifdef ENABLE_FEAT_F4HWN // Blink Green Led for white...
            if(gSetting_set_eot > 0 && RxBlinkLed == 2)
        {
            if(RxBlinkLedCounter <= 8)
            {
                if(RxBlinkLedCounter % 2 == 0)
                {
                    if(gSetting_set_eot > 1 )
                    {
                        BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2_GREEN, false);
                    }
                }
                else
                {
                    if(gSetting_set_eot > 1 )
                    {
                        BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2_GREEN, true);
                    }

                    if(gSetting_set_eot == 1 || gSetting_set_eot == 3)
                    {
                        switch(RxBlinkLedCounter)
                        {
                            case 1:
                            AUDIO_PlayBeep(BEEP_400HZ_30MS);
                            break;

                            case 3:
                            AUDIO_PlayBeep(BEEP_400HZ_30MS);
                            break;

                            case 5:
                            AUDIO_PlayBeep(BEEP_500HZ_30MS);
                            break;

                            case 7:
                            AUDIO_PlayBeep(BEEP_600HZ_30MS);
                            break;
                        }
                    }
                }
                RxBlinkLedCounter += 1;
            }
            else
            {
                RxBlinkLed = 0;
            }
        }
#endif
        }
#ifdef ENABLE_FEAT_F4HWN
        if (isMainOnly())
        {
            if (!FUNCTION_IsRx())
                F4HWN_UpdateGvfoRssiBarLevelForStatusBar();
            F4HWN_RequestStatusBarIfTopRssiSnapshotChanged();
        }
#endif
    }
}

// ----------------------------------------

void UI_DisplayMain(void)
{
    char               String[22];
    const unsigned int activeTxVFO = gRxVfoIsActive ? gEeprom.RX_VFO : gEeprom.TX_VFO;

    center_line = CENTER_LINE_NONE;

    // clear the screen
    UI_DisplayClear();

    if(gLowBattery && !gLowBatteryConfirmed) {
        UI_DisplayPopup("LOW BATTERY");
        ST7565_BlitFullScreen();
        return;
    }

#ifndef ENABLE_FEAT_F4HWN
    if (gEeprom.KEY_LOCK && gKeypadLocked > 0)
    {   // tell user how to unlock the keyboard
        UI_PrintString("Long press #", 0, LCD_WIDTH, 1, 8);
        UI_PrintString("to unlock",    0, LCD_WIDTH, 3, 8);
        ST7565_BlitFullScreen();
        return;
    }
#else
    // 键盘锁定时也需要继续绘制主页内容，然后在上面叠加解锁提示框
    const bool keyboardLocked = (gEeprom.KEY_LOCK && gKeypadLocked > 0);
#endif

#ifdef ENABLE_FEAT_F4HWN
    if (DualVfoMainFreqEntryScreen() && !(gEeprom.KEY_LOCK && gKeypadLocked > 0))
    {
        UI_DisplayMain_FreqInputBare();
        goto display_main_after_vfo_loop;
    }
#endif

    // 主页面 (MAIN ONLY) 定制布局：横线、计时、大矩形 + 左侧条、信道/频率、底部两按钮
#ifdef ENABLE_FEAT_F4HWN
    if (isMainOnly() && !gAirCopyBootMode && gScreenToDisplay == DISPLAY_MAIN) {
        const uint8_t vfo = gEeprom.TX_VFO;
        const VFO_Info_t *pVfo = &gEeprom.VfoInfo[vfo];

        // 顶线 + 左侧小长条（RX 接收时空心，平时实心）+ 方框
        for (unsigned int i = 0; i < LCD_WIDTH; i++)
            gFrameBuffer[0][i] |= 0x01;

        const bool hollowBar = FUNCTION_IsRx();
        const int barX0 = 0, barX1 = 7, rectX0 = 7, rectY0 = 2, rectY1 = 33;
        const int contentX = rectX0 + 4;

        for (int y = rectY0; y <= rectY1; y++) {
            for (int x = barX0; x < barX1; x++) {
                if (!hollowBar) {
                    UI_DrawPixelBuffer(gFrameBuffer, x, y, true);   // 非 RX：实心
                } else {
                    const bool border =
                        (x == barX0) || (x == barX1 - 1) || (y == rectY0) || (y == rectY1);
                    if (border)
                        UI_DrawPixelBuffer(gFrameBuffer, x, y, true);   // RX：空心边框
                }
            }
        }

        UI_DrawLineBuffer(gFrameBuffer, rectX0, rectY0, LCD_WIDTH - 1, rectY0, true);
        UI_DrawLineBuffer(gFrameBuffer, LCD_WIDTH - 1, rectY0, LCD_WIDTH - 1, rectY1, true);

        // 右上角：信道模式默认显示信道号，接收时显示灵敏度；频率模式默认不显示，接收时显示灵敏度
        {
            const int slotY = 4;
            const int rightEdge = (int)(LCD_WIDTH - 1);
            if (FUNCTION_IsRx()) {
                char dBmStr[12];
                int16_t rssi_dBm =
                    BK4819_GetRSSI_dBm()
#ifdef ENABLE_AM_FIX
                    + ((gSetting_AM_fix && gRxVfo->Modulation == MODULATION_AM) ? AM_fix_get_gain_diff() : 0)
#endif
                    + dBmCorrTable[gRxVfo->Band];
                rssi_dBm = -rssi_dBm;
                if (rssi_dBm > 141) rssi_dBm = 141;
                if (rssi_dBm < 53) rssi_dBm = 53;
                const int16_t display_rssi_dBm = (int16_t)(-rssi_dBm);
                sprintf(dBmStr, "%d dBm", display_rssi_dBm);
                const unsigned int w = DualVfoU8g2_GetSmallTextWidth(dBmStr);
                const int right_aligned_x = (rightEdge - (int)w) > (int)contentX ? (rightEdge - (int)w) : (int)contentX;
                int shifted_x = right_aligned_x - 2;
                if (shifted_x < (int)contentX)
                    shifted_x = (int)contentX;
                DualVfoU8g2_DrawSmallText(dBmStr, (uint8_t)shifted_x, (uint8_t)slotY, true);
                {
                    uint8_t s_level, overS9Bars = 0;
                    if (rssi_dBm >= 93) {
                        s_level = (uint8_t)((141 - rssi_dBm) * 8u / 48u + 1u);
                        if (s_level > 9) s_level = 9;
                    } else {
                        s_level = 9;
                        uint8_t overS9dBm = (rssi_dBm >= 53) ? (uint8_t)(93 - rssi_dBm) : 40;
                        if (overS9dBm > 40) overS9dBm = 40;
                        overS9Bars = overS9dBm * 4u / 40u;
                    }
                    uint8_t raw = s_level + overS9Bars;
                    gVFO_RSSI_bar_level[vfo] = (raw * 6u + 6u) / 13u;
                    if (gVFO_RSSI_bar_level[vfo] > 6u) gVFO_RSSI_bar_level[vfo] = 6u;
                }
                gUpdateStatus = true;
            } else if (IS_MR_CHANNEL(gEeprom.ScreenChannel[vfo])) {
                sprintf(String, "%04u", gEeprom.ScreenChannel[vfo] + 1);
                const uint8_t chNumW = DualVfoU8g2_GetSmallTextWidth(String);
                const int right_aligned_x = (rightEdge - (int)chNumW) > (int)contentX ? (rightEdge - (int)chNumW) : (int)contentX;
                int shifted_x = right_aligned_x - 2;
                if (shifted_x < (int)contentX)
                    shifted_x = (int)contentX;
                DualVfoU8g2_DrawSmallText(String, (uint8_t)shifted_x, (uint8_t)slotY, true);
            }
            /* 频率模式且未接收：右上角不显示 */
        }

        // 先画频率（DualVfoU8g2_DrawMainFreqStrip 会清除较大区域），
        // 再画信道名覆盖被误清的部分。
        {
            uint32_t f = (gCurrentFunction == FUNCTION_TRANSMIT) ? pVfo->pTX->Frequency : pVfo->pRX->Frequency;
            DualVfoU8g2_DrawMainFreqStrip(f, contentX, 30);
        }

        if (IS_MR_CHANNEL(gEeprom.ScreenChannel[vfo])) {
            char chName[24];
            SETTINGS_FetchChannelName(chName, gEeprom.ScreenChannel[vfo]);
#ifdef ENABLE_CHINESE
            if (SETTINGS_ChannelNameHasCjkUtf8(chName))
                UI_PrintStringSmallAtPixel(chName, contentX, contentX, 1 * 8 + 4, 1 * 8 + 15 + 4, 0);
            else if (chName[0])
#else
            if (chName[0])
#endif
                UI_PrintStringSmallNormal(chName, contentX, contentX, 1);
        } else {
            UI_PrintStringSmallNormal("VFO", contentX, contentX, 1);
        }

        // 方框底边：与上边/右边同样用 1 像素线画
        UI_DrawLineBuffer(gFrameBuffer, rectX0, rectY1, LCD_WIDTH - 1, rectY1, true);

        // 方框下两行：第一行 time: 计时，第二行 亚音，均右对齐；整体上移 1px
        const int line1Y = 33, line2Y = 39;
#ifdef ENABLE_FEAT_F4HWN_RX_TX_TIMER
        {
            uint16_t t = (FUNCTION_IsRx()) ? (3600 - gRxTimerCountdown_500ms / 2) : (gTxTimerCountdown_500ms / 2);
            uint16_t m = t / 60;
            uint8_t s = (uint8_t)(t % 60);
            sprintf(String, "time: %02u:%02u", (unsigned)m, s);
            const int w1 = (int)DualVfoU8g2_GetSmallTextWidth(String);
            const int x1 = 127 - w1;
            DualVfoU8g2_DrawSmallText(String, (uint8_t)(x1 > 0 ? x1 : 0), (uint8_t)(line1Y + 2), true);
        }
#endif
        {
            char toneBuf[48];
            uint8_t pos = 0;
            const FREQ_Config_t *pRx = &pVfo->freq_config_RX;
            const FREQ_Config_t *pTx = &pVfo->freq_config_TX;
            if (pRx->CodeType != CODE_TYPE_OFF) {
                if (pRx->CodeType == CODE_TYPE_CONTINUOUS_TONE) {
                    pos += sprintf(
                        toneBuf + pos,
                        "RCT: %u.%u",
                        (unsigned)(CTCSS_Options[pRx->Code] / 10),
                        (unsigned)(CTCSS_Options[pRx->Code] % 10));
                } else if (pRx->CodeType == CODE_TYPE_DIGITAL) {
                    pos += sprintf(toneBuf + pos, "R: DCS%03oN", (unsigned)DCS_Options[pRx->Code]);
                } else {
                    pos += sprintf(toneBuf + pos, "R: DCS%03oI", (unsigned)DCS_Options[pRx->Code]);
                }
                if (pTx->CodeType != CODE_TYPE_OFF)
                    toneBuf[pos++] = ' ';
            }
            if (pTx->CodeType != CODE_TYPE_OFF) {
                if (pTx->CodeType == CODE_TYPE_CONTINUOUS_TONE) {
                    pos += sprintf(
                        toneBuf + pos,
                        "TCT: %u.%u",
                        (unsigned)(CTCSS_Options[pTx->Code] / 10),
                        (unsigned)(CTCSS_Options[pTx->Code] % 10));
                } else if (pTx->CodeType == CODE_TYPE_DIGITAL) {
                    pos += sprintf(toneBuf + pos, "T: DCS%03oN", (unsigned)DCS_Options[pTx->Code]);
                } else {
                    pos += sprintf(toneBuf + pos, "T: DCS%03oI", (unsigned)DCS_Options[pTx->Code]);
                }
            }
            toneBuf[pos] = '\0';
            if (pos > 0) {
                const int w2 = (int)DualVfoU8g2_GetSmallTextWidth(toneBuf);
                const int x2 = 127 - w2;
                DualVfoU8g2_DrawSmallText(toneBuf, (uint8_t)(x2 > 0 ? x2 : 0), (uint8_t)(line2Y + 2), true);
            }
        }

        // 底部两按钮：黑底先增高 1 像素（row5 底一行黑），再 row6 黑底，中间白线，笔画用 Negative 清空（字不动）
        const int btnLY = 6, btnL5 = 5, btnLX0 = 0, btnLX1 = 62, btnRX0 = 64, btnRX1 = 127;
        const int sepX = 63;
        for (int x = btnLX0; x <= btnLX1; x++)
            gFrameBuffer[btnL5][x] |= 0x80;
        for (int x = btnRX0; x <= btnRX1; x++)
            gFrameBuffer[btnL5][x] |= 0x80;
        gFrameBuffer[btnL5][sepX] &= (uint8_t)~0x80;
        for (int x = btnLX0; x <= btnLX1; x++)
            gFrameBuffer[btnLY][x] = 0xFF;
        for (int x = btnRX0; x <= btnRX1; x++)
            gFrameBuffer[btnLY][x] = 0xFF;
        gFrameBuffer[btnLY][sepX] = 0x00;

        UI_PrintStringSmallNormalNegative("Menu", btnLX0, btnLX1, btnLY);
        UI_PrintStringSmallNormalNegative(gModulationStr[pVfo->Modulation], btnRX0, btnRX1, btnLY);

        // 键盘锁定时，在主页内容之上显示解锁提示框
        // main only 和双守使用相同的偏移，确保显示位置完全一致
        if (keyboardLocked) {
            UI_DisplayUnlockKeyboard(-10);  // 统一使用 -10
        }
        
        ST7565_BlitFullScreen();
        return;
    }
#endif

#ifdef ENABLE_FEAT_F4HWN
    if (!isMainOnly() && !DualVfoShouldUseLegacyMain() && gScreenToDisplay == DISPLAY_MAIN && !gAirCopyBootMode)
    {
        UI_DisplayMain_DualVfoTwoPanel();
        
        // 键盘锁定时，在主页内容之上显示解锁提示框
        // 与 main only 使用相同的偏移，确保显示位置完全一致
        if (keyboardLocked) {
            UI_DisplayUnlockKeyboard(-10);  // 与 main only 统一使用 -10
        }
        
        goto display_main_after_vfo_loop;
    }
#endif

    for (unsigned int vfo_num = 0; vfo_num < 2; vfo_num++)
    {
#ifdef ENABLE_FEAT_F4HWN
        const unsigned int line0 = 0;  // text screen line
        const unsigned int line1 = 4;
        unsigned int line;
        if (isMainOnly())
        {
            line       = 0;
        }
        else
        {
            line       = (vfo_num == 0) ? line0 : line1;
        }
        const bool         isMainVFO  = (vfo_num == gEeprom.TX_VFO);
        uint8_t           *p_line0    = gFrameBuffer[line + 0];
        uint8_t           *p_line1    = gFrameBuffer[line + 1];
        enum Vfo_txtr_mode mode       = VFO_MODE_NONE;      
#else
        const unsigned int line0 = 0;  // text screen line
        const unsigned int line1 = 4;
        const unsigned int line       = (vfo_num == 0) ? line0 : line1;
        const bool         isMainVFO  = (vfo_num == gEeprom.TX_VFO);
        uint8_t           *p_line0    = gFrameBuffer[line + 0];
        uint8_t           *p_line1    = gFrameBuffer[line + 1];
        enum Vfo_txtr_mode mode       = VFO_MODE_NONE;
#endif

#ifdef ENABLE_FEAT_F4HWN
    if (isMainOnly())
    {
        if (activeTxVFO != vfo_num)
        {
            continue;
        }
    }
#endif

#ifdef ENABLE_FEAT_F4HWN
        if (activeTxVFO != vfo_num || isMainOnly())
#else
        if (activeTxVFO != vfo_num) // this is not active TX VFO
#endif
        {
#ifdef ENABLE_SCAN_RANGES
            if(gScanRangeStart) {

#ifdef ENABLE_FEAT_F4HWN
                //if(IS_FREQ_CHANNEL(gEeprom.ScreenChannel[0]) && IS_FREQ_CHANNEL(gEeprom.ScreenChannel[1])) {
                if(IS_FREQ_CHANNEL(gEeprom.ScreenChannel[activeTxVFO])) {

                    uint8_t shift = 0;

                    if (isMainOnly())
                    {
                        shift = 3;
                    }

                    UI_PrintString("ScnRng", 5, 0, line + shift, 8);
                    sprintf(String, "%3u.%05u", gScanRangeStart / 100000, gScanRangeStart % 100000);
                    UI_PrintStringSmallNormal(String, 56, 0, line + shift);
                    sprintf(String, "%3u.%05u", gScanRangeStop / 100000, gScanRangeStop % 100000);
                    UI_PrintStringSmallNormal(String, 56, 0, line + shift + 1);

                    if (!isMainOnly())
                        continue;
                }
                else
                {
                    gScanRangeStart = 0;
                }
#else
                UI_PrintString("ScnRng", 5, 0, line, 8);
                sprintf(String, "%3u.%05u", gScanRangeStart / 100000, gScanRangeStart % 100000);
                UI_PrintStringSmallNormal(String, 56, 0, line);
                sprintf(String, "%3u.%05u", gScanRangeStop / 100000, gScanRangeStop % 100000);
                UI_PrintStringSmallNormal(String, 56, 0, line + 1);
                continue;
#endif
            }
#endif


            if (
#ifdef ENABLE_DTMF_CALLING
                gDTMF_CallState != DTMF_CALL_STATE_NONE || gDTMF_IsTx
#else
                false
#endif
            ) {
                char *pPrintStr = "";
                // show DTMF stuff
#ifdef ENABLE_DTMF_CALLING
                char Contact[16];
                if (gDTMF_CallState == DTMF_CALL_STATE_CALL_OUT) {
                    pPrintStr = DTMF_FindContact(gDTMF_String, Contact) ? Contact : gDTMF_String;
                } else if (gDTMF_CallState == DTMF_CALL_STATE_RECEIVED || gDTMF_CallState == DTMF_CALL_STATE_RECEIVED_STAY){
                    pPrintStr = DTMF_FindContact(gDTMF_Callee, Contact) ? Contact : gDTMF_Callee;
                }else if (gDTMF_IsTx) {
                    pPrintStr = gDTMF_String;
                }

                UI_PrintString(pPrintStr, 2, 0, 2 + (vfo_num * 3), 8);

                pPrintStr = "";
                if (gDTMF_CallState == DTMF_CALL_STATE_CALL_OUT) {
                    pPrintStr = (gDTMF_State == DTMF_STATE_CALL_OUT_RSP) ? "CALL OUT(RSP)" : "CALL OUT";
                } else if (gDTMF_CallState == DTMF_CALL_STATE_RECEIVED || gDTMF_CallState == DTMF_CALL_STATE_RECEIVED_STAY) {
                    sprintf(String, "CALL FRM:%s", (DTMF_FindContact(gDTMF_Caller, Contact)) ? Contact : gDTMF_Caller);
                    pPrintStr = String;
                } else if (gDTMF_IsTx) {
                    pPrintStr = (gDTMF_State == DTMF_STATE_TX_SUCC) ? "DTMF TX(SUCC)" : "DTMF TX";
                }
#endif

#ifdef ENABLE_FEAT_F4HWN
                if (isMainOnly())
                {
                    UI_PrintString(pPrintStr, 2, 0, 5, 8);
                    center_line = CENTER_LINE_IN_USE;
                    continue;
                }
                else
                {
                    UI_PrintString(pPrintStr, 2, 0, 0 + (vfo_num * 3), 8);
                    center_line = CENTER_LINE_IN_USE;
                    continue;
                }
#else
                UI_PrintString(pPrintStr, 2, 0, 0 + (vfo_num * 3), 8);
                center_line = CENTER_LINE_IN_USE;
                continue;
#endif
            }

            // highlight the selected/used VFO with a marker
            if (isMainVFO)
                memcpy(p_line0 + 0, BITMAP_VFO_Default, sizeof(BITMAP_VFO_Default));
        }
        else // active TX VFO
        {   // highlight the selected/used VFO with a marker
            if (isMainVFO)
                memcpy(p_line0 + 0, BITMAP_VFO_Default, sizeof(BITMAP_VFO_Default));
            else
                memcpy(p_line0 + 0, BITMAP_VFO_NotDefault, sizeof(BITMAP_VFO_NotDefault));
        }

        uint32_t frequency = gEeprom.VfoInfo[vfo_num].pRX->Frequency;

        if(TX_freq_check(frequency) != 0 && gEeprom.VfoInfo[vfo_num].TX_LOCK == true && !FUNCTION_IsRx())
        {
            if(isMainOnly())
                memcpy(p_line0 + 25, BITMAP_VFO_Lock, sizeof(BITMAP_VFO_Lock));
            else
                memcpy(p_line0 + 25, BITMAP_VFO_Lock, sizeof(BITMAP_VFO_Lock));
        }

        if (gCurrentFunction == FUNCTION_TRANSMIT)
        {   // transmitting

#ifdef ENABLE_ALARM
            if (gAlarmState == ALARM_STATE_SITE_ALARM)
                mode = VFO_MODE_RX;
            else
#endif
            {
                if (activeTxVFO == vfo_num)
                {   // show the TX symbol
                    mode = VFO_MODE_TX;
                    //UI_PrintStringSmallBold("TX", 8, 0, line);
                    GUI_DisplaySmallest("TX", 10, line == 0 ? 1 : 33, false, true);

                }
            }
        }
        else
        {   // receiving .. show the RX symbol
            mode = VFO_MODE_RX;
            //if (FUNCTION_IsRx() && gEeprom.RX_VFO == vfo_num) {
            if (FUNCTION_IsRx()) {
                if (gEeprom.RX_VFO == vfo_num && VfoState[vfo_num] == VFO_STATE_NORMAL) {
#ifdef ENABLE_FEAT_F4HWN
                    RxBlinkLed = 1;
                    RxBlinkLedCounter = 0;
                    RxLine = line;
                    RxOnVfofrequency = frequency;
                    // if(!isMainVFO)
                    // {
                    //     RxBlink = 1;
                    // }
                    // else
                    // {
                    //     RxBlink = 0;
                    // }

                    // if (RxBlink == 0 || RxBlink == 1) {
                        if(gRxVfo->Modulation == MODULATION_AM) {
                            #ifdef ENABLE_FEAT_F4HWN_AUDIO
                                strcpy(String, gSubMenu_SET_AUD_AM[gSetting_set_audio_am]);
                            #else
                                strcpy(String, "AIR");
                            #endif
                        }
                        else if (gRxVfo->Modulation == MODULATION_USB) {
                            strcpy(String, "USB");
                        }
                        else {
                            #ifdef ENABLE_FEAT_F4HWN_AUDIO
                                strcpy(String, gSubMenu_SET_AUD_FM[gSetting_set_audio_fm]);
                            #else
                                strcpy(String, "RX");
                            #endif
                        }

                        GUI_DisplaySmallest(String, 10, RxLine == 0 ? 1 : 33, false, true);
                        //UI_PrintStringSmallBold("RX", 8, 0, RxLine);
                    // }
#else
                    UI_PrintStringSmallBold("RX", 8, 0, line);
#endif
                }
#ifdef ENABLE_FEAT_F4HWN
                else {
                    if(RxBlinkLed == 1)
                        RxBlinkLed = 2;
                }
            }
            else {
                if(RxOnVfofrequency == frequency && !isMainOnly()) {
                    //UI_PrintStringSmallNormal(">>", 8, 0, line);
                    //memcpy(p_line0 + 14, BITMAP_VFO_Default, sizeof(BITMAP_VFO_Default));
                    GUI_DisplaySmallest(">>", 8, RxLine == 0 ? 1 : 33, false, true);
                }

                if(RxBlinkLed == 1)
                    RxBlinkLed = 2;
            }
#endif
        }

        if (IS_MR_CHANNEL(gEeprom.ScreenChannel[vfo_num]))
        {   // channel mode
            const unsigned int x = 1;
            const bool inputting = gInputBoxIndex != 0 && gEeprom.TX_VFO == vfo_num;
            if (!inputting || gScanStateDir != SCAN_OFF)
                sprintf(String, "%04u", gEeprom.ScreenChannel[vfo_num] + 1);
            else
                sprintf(String, "%.4s", INPUTBOX_GetAscii());  // show the input text

            //if (gSetting_set_gui) {
                UI_PrintStringSmallNormalInverse(String, x, 0, line + 1);
            /*
            }
            else
            {
                GUI_DisplaySmallest(String, x + 1, line == 0 ? 9 : 41, false, true);
                gFrameBuffer[line + 1][0] ^= 0x1C;
                gFrameBuffer[line + 1][1] ^= 0x3E;
                for (uint8_t i = 2; i < 21; i++) {
                    gFrameBuffer[line + 1][i] ^= 0x7F;
                }
                gFrameBuffer[line + 1][21] ^= 0x3E;
                gFrameBuffer[line + 1][22] ^= 0x1C;

            }
            */
        }
        else if (IS_FREQ_CHANNEL(gEeprom.ScreenChannel[vfo_num]))
        {   // frequency mode
            // show the frequency band number
            const unsigned int x = 2;
            const uint8_t f = 1 + gEeprom.ScreenChannel[vfo_num] - FREQ_CHANNEL_FIRST;
            const bool over1GHz = gEeprom.VfoInfo[vfo_num].pRX->Frequency >= _1GHz_in_KHz;

            sprintf(String, over1GHz ? "F%u+" : "F%u", f);
            //if (gSetting_set_gui) {
                UI_PrintStringSmallNormalInverse(String, x, 0, line + 1);
            /*
            }
            else
            {
                GUI_DisplaySmallest(String, x + 2, line == 0 ? 9 : 41, false, true);
                uint8_t g = 13;
                if(over1GHz)
                    g = 17;

                gFrameBuffer[line + 1][0] ^= 0x1C;
                gFrameBuffer[line + 1][1] ^= 0x3E;
                for (uint8_t i = 2; i < g; i++) {
                    gFrameBuffer[line + 1][i] ^= 0x7F;
                }
                gFrameBuffer[line + 1][g] ^= 0x3E;
                gFrameBuffer[line + 1][g + 1] ^= 0x1C;

            }
            */
        }
#ifdef ENABLE_NOAA
        else
        {
            if (gInputBoxIndex == 0 || gEeprom.TX_VFO != vfo_num)
            {   // channel number
                sprintf(String, "N%u", 1 + gEeprom.ScreenChannel[vfo_num] - NOAA_CHANNEL_FIRST);
            }
            else
            {   // user entering channel number
                sprintf(String, "N%u%u", '0' + gInputBox[0], '0' + gInputBox[1]);
            }
            UI_PrintStringSmallNormal(String, 7, 0, line + 1);
        }
#endif

        // ----------------------------------------

        enum VfoState_t state = VfoState[vfo_num];

#ifdef ENABLE_ALARM
        if (gCurrentFunction == FUNCTION_TRANSMIT && gAlarmState == ALARM_STATE_SITE_ALARM) {
            if (activeTxVFO == vfo_num)
                state = VFO_STATE_ALARM;
        }
#endif
        if (state != VFO_STATE_NORMAL)
        {
            if (state < ARRAY_SIZE(VfoStateStr))
                UI_PrintString(VfoStateStr[state], 35, 0, line, 8);
        }
        else if (gInputBoxIndex > 0 && IS_FREQ_CHANNEL(gEeprom.ScreenChannel[vfo_num]) && gEeprom.TX_VFO == vfo_num)
        {   // user entering a frequency
            const char * ascii = INPUTBOX_GetAscii();
            bool isGigaF = frequency>=_1GHz_in_KHz;
            sprintf(String, "%.*s.%.3s", 3 + isGigaF, ascii, ascii + 3 + isGigaF);
#ifdef ENABLE_BIG_FREQ
            if(!isGigaF) {
                // show the remaining 2 small frequency digits
                UI_PrintStringSmallNormal(String + 7, 113, 0, line + 1);
                String[7] = 0;
                // show the main large frequency digits
                UI_DisplayFrequency(String, 32, line, false);
            }
            else
#endif
            {
                // show the frequency in the main font
                UI_PrintString(String, 32, 0, line, 8);
            }

            continue;
        }
        else
        {
            if (gCurrentFunction == FUNCTION_TRANSMIT)
            {   // transmitting
                if (activeTxVFO == vfo_num)
                    frequency = gEeprom.VfoInfo[vfo_num].pTX->Frequency;
            }

            if (IS_MR_CHANNEL(gEeprom.ScreenChannel[vfo_num]))
            {   // it's a channel

                #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
                    if(gEeprom.MENU_LOCK == false) {
                #endif

                const ChannelAttributes_t* att = MR_GetChannelAttributes(gEeprom.ScreenChannel[vfo_num]);


                if(att->exclude == false)
                {
                    // show the scan list assigment symbols
                    const ChannelAttributes_t* att = MR_GetChannelAttributes(gEeprom.ScreenChannel[vfo_num]);

                    uint8_t countList = att->scanlist;
                    if(countList > MR_CHANNELS_LIST + 1) {
                        countList = 0;
                    }

                    const char *displayStr;
                    uint8_t xStart, xDisplay;

                    if (countList == MR_CHANNELS_LIST + 1) {
                        displayStr = "ALL";
                        xStart = 113;
                        xDisplay = 115;
                    } 
                    else if (countList == 0) {
                        displayStr = "OFF";
                        xStart = 113;
                        xDisplay = 115;
                    } 
                    else {
                        // List 1 to MR_CHANNELS_LIST
                        const char *name = gListName[countList - 1];
                        
                        // If name is empty/invalid, display number
                        if (IsEmptyName(name, sizeof(gListName[0]))) {
                            sprintf(String, "%02d", countList);
                            xStart = 117;  // 2-digit number aligned right
                            xDisplay = 119;
                        } 
                        else {
                            sprintf(String, "%.3s", name);
                            xStart = 113;  // 3-char name aligned left
                            xDisplay = 115;
                        }
                        displayStr = String;
                    }

                    GUI_DisplaySmallest(displayStr, xDisplay, line == 0 ? 1 : 33, false, true);

                    gFrameBuffer[line][xStart] ^= 0x3E;
                    for (uint8_t x = xStart + 1; x < 127; x++) {
                        gFrameBuffer[line][x] ^= 0x7F;
                    }
                    gFrameBuffer[line][127] ^= 0x3E;

                }
                else
                {
                    const char *displayStr = "EX";

                    uint8_t xStart = 117;
                    uint8_t xDisplay = 119;
                    
                    GUI_DisplaySmallest(displayStr, xDisplay, line == 0 ? 1 : 33, false, true);

                    gFrameBuffer[line][xStart] ^= 0x3E;
                    for (uint8_t x = xStart + 1; x < 127; x++) {
                        gFrameBuffer[line][x] ^= 0x7F;
                    }
                    gFrameBuffer[line][127] ^= 0x3E;
                }

                #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
                {
                    }
                }
                #endif

                // compander symbol
#ifndef ENABLE_BIG_FREQ
                if (att->compander)
                    memcpy(p_line0 + 120 + LCD_WIDTH, BITMAP_compand, sizeof(BITMAP_compand));
#else
                // TODO:  // find somewhere else to put the symbol
#endif

                switch (gEeprom.CHANNEL_DISPLAY_MODE)
                {
                    case MDF_FREQUENCY: // show the channel frequency
                        sprintf(String, "%3u.%05u", frequency / 100000, frequency % 100000);
#ifdef ENABLE_BIG_FREQ
                        if(frequency < _1GHz_in_KHz) {
                            // show the remaining 2 small frequency digits
                            UI_PrintStringSmallNormal(String + 7, 113, 0, line + 1);
                            String[7] = 0;
                            // show the main large frequency digits
                            UI_DisplayFrequency(String, 32, line, false);
                        }
                        else
#endif
                        {
                            // show the frequency in the main font
                            UI_PrintString(String, 32, 0, line, 8);
                        }

                        break;

                    case MDF_CHANNEL:   // show the channel number
                        sprintf(String, "CH-%04u", gEeprom.ScreenChannel[vfo_num] + 1);
                        UI_PrintString(String, 36, 0, line, 8);
                        break;

                    case MDF_NAME:      // show the channel name
                    case MDF_NAME_FREQ: // show the channel name and frequency

                        SETTINGS_FetchChannelName(String, gEeprom.ScreenChannel[vfo_num]);
                        if (String[0] == 0)
                        {   // no channel name, show the channel number instead
                            sprintf(String, "CH-%04u", gEeprom.ScreenChannel[vfo_num] + 1);
                        }

                        if (gEeprom.CHANNEL_DISPLAY_MODE == MDF_NAME) {
#ifdef ENABLE_CHINESE
                            if (SETTINGS_ChannelNameHasCjkUtf8(String))
                            {
                                UI_PrintStringSmallAtPixel(String, 33, 127, line * 8, line * 8 + 15, 0);
                                break;
                            }
#endif
                            String[CHANNEL_NAME_MAX_BYTES] = 0;
                            UI_PrintString(String, 33, 0, line, 8);
                        }
                        else {
#ifdef ENABLE_FEAT_F4HWN
                            if (isMainOnly())
                            {
#ifdef ENABLE_CHINESE
                                if (SETTINGS_ChannelNameHasCjkUtf8(String))
                                {
                                    UI_PrintStringSmallAtPixel(String, 33, 127, line * 8, line * 8 + 15, 0);
                                    break;
                                }
                                if (String[0] == 0)
                                    sprintf(String, "CH-%04u", gEeprom.ScreenChannel[vfo_num] + 1);
#endif
                                String[CHANNEL_NAME_MAX_BYTES] = 0;
                                UI_PrintString(String, 33, 0, line, 8);
                            }
                            else
                            {
#ifdef ENABLE_CHINESE
                                if (String[0] == 0)
                                    sprintf(String, "CH-%04u", gEeprom.ScreenChannel[vfo_num] + 1);
#endif
                                String[CHANNEL_NAME_MAX_BYTES] = 0;
                                if(activeTxVFO == vfo_num) {
                                    UI_PrintStringSmallBold(String, 32 + 4, 0, line);
                                }
                                else
                                {
                                    UI_PrintStringSmallNormal(String, 32 + 4, 0, line);
                                }
                            }
#else
#ifdef ENABLE_CHINESE
                            if (String[0] == 0)
                                sprintf(String, "CH-%04u", gEeprom.ScreenChannel[vfo_num] + 1);
#endif
                            String[CHANNEL_NAME_MAX_BYTES] = 0;
                            UI_PrintStringSmallBold(String, 32 + 4, 0, line);
#endif

#ifdef ENABLE_FEAT_F4HWN
                            if (isMainOnly())
                            {
                                sprintf(String, "%3u.%05u", frequency / 100000, frequency % 100000);
                                if(frequency < _1GHz_in_KHz) {
                                    // show the remaining 2 small frequency digits
                                    UI_PrintStringSmallNormal(String + 7, 113, 0, line + 4);
                                    String[7] = 0;
                                    // show the main large frequency digits
                                    UI_DisplayFrequency(String, 32, line + 3, false);
                                }
                                else
                                {
                                    // show the frequency in the main font
                                    UI_PrintString(String, 32, 0, line + 3, 8);
                                }
                            }
                            else
                            {
                                sprintf(String, "%03u.%05u", frequency / 100000, frequency % 100000);
                                UI_PrintStringSmallNormal(String, 32 + 4, 0, line + 1);
                            }
#else                           // show the channel frequency below the channel number/name
                            sprintf(String, "%03u.%05u", frequency / 100000, frequency % 100000);
                            UI_PrintStringSmallNormal(String, 32 + 4, 0, line + 1);
#endif
                        }

                        break;
                }
            }
            else
            {   // frequency mode
                sprintf(String, "%3u.%05u", frequency / 100000, frequency % 100000);

#ifdef ENABLE_BIG_FREQ
                if(frequency < _1GHz_in_KHz) {
                    // show the remaining 2 small frequency digits
                    UI_PrintStringSmallNormal(String + 7, 113, 0, line + 1);
                    String[7] = 0;
                    // show the main large frequency digits
                    UI_DisplayFrequency(String, 32, line, false);
                }
                else
#endif
                {
                    // show the frequency in the main font
                    UI_PrintString(String, 32, 0, line, 8);
                }

                // show the channel symbols
                const ChannelAttributes_t* att = MR_GetChannelAttributes(gEeprom.ScreenChannel[vfo_num]);
                if (att->compander)
#ifdef ENABLE_BIG_FREQ
                    memcpy(p_line0 + 120, BITMAP_compand, sizeof(BITMAP_compand));
#else
                    memcpy(p_line0 + 120 + LCD_WIDTH, BITMAP_compand, sizeof(BITMAP_compand));
#endif
            }
        }

        // ----------------------------------------

        {   // show the TX/RX level
            int8_t Level = -1;

            if (mode == VFO_MODE_TX)
            {   // TX power level
                /*
                switch (gRxVfo->OUTPUT_POWER)
                {
                    case OUTPUT_POWER_LOW1:     Level = 2; break;
                    case OUTPUT_POWER_LOW2:     Level = 2; break;
                    case OUTPUT_POWER_LOW3:     Level = 2; break;
                    case OUTPUT_POWER_LOW4:     Level = 2; break;
                    case OUTPUT_POWER_LOW5:     Level = 2; break;
                    case OUTPUT_POWER_MID:      Level = 4; break;
                    case OUTPUT_POWER_HIGH:     Level = 6; break;
                }

                if (gRxVfo->OUTPUT_POWER == OUTPUT_POWER_MID) {
                    Level = 4;
                } else if (gRxVfo->OUTPUT_POWER == OUTPUT_POWER_HIGH) {
                    Level = 6;
                } else {
                    Level = 2;
                }
                */

                uint8_t currentPower = gRxVfo->OUTPUT_POWER;

                if(currentPower == OUTPUT_POWER_USER)
                    Level = gSetting_set_pwr;
                else
                    Level = currentPower - 1;
            }
            else 
            if (mode == VFO_MODE_RX)
            {   // RX signal level
                #ifndef ENABLE_RSSI_BAR
                    // bar graph
                    if (gVFO_RSSI_bar_level[vfo_num] > 0)
                        Level = gVFO_RSSI_bar_level[vfo_num];
                #endif
            }
            if(Level >= 0)
                DrawSmallPowerBars(p_line1 + LCD_WIDTH, Level);
        }

        // ----------------------------------------

        String[0] = '\0';
        const VFO_Info_t *vfoInfo = &gEeprom.VfoInfo[vfo_num];

        // show the modulation symbol
        const char * s = "";
#ifdef ENABLE_FEAT_F4HWN
        const char * t = "";
#endif
        const ModulationMode_t mod = vfoInfo->Modulation;
        switch (mod){
            case MODULATION_FM: {
                const FREQ_Config_t *pConfig = (mode == VFO_MODE_TX) ? vfoInfo->pTX : vfoInfo->pRX;
                const unsigned int code_type = pConfig->CodeType;
#ifdef ENABLE_FEAT_F4HWN
                const char *code_list[] = {"", "CT", "DC", "DC"};
#else
                const char *code_list[] = {"", "CT", "DCS", "DCR"};
#endif
                if (code_type < ARRAY_SIZE(code_list))
                    s = code_list[code_type];
#ifdef ENABLE_FEAT_F4HWN
                if(gCurrentFunction != FUNCTION_TRANSMIT || activeTxVFO != vfo_num)
                    t = gModulationStr[mod];
#endif
                break;
            }
            default:
                t = gModulationStr[mod];
            break;
        }

#if ENABLE_FEAT_F4HWN
        const FREQ_Config_t *pConfig = (mode == VFO_MODE_TX) ? vfoInfo->pTX : vfoInfo->pRX;
        int8_t shift = 0;

        switch((int)pConfig->CodeType)
        {
            case 1:
            sprintf(String, "%u.%u", CTCSS_Options[pConfig->Code] / 10, CTCSS_Options[pConfig->Code] % 10);
            break;

            case 2:
            case 3:
            sprintf(String, (int)pConfig->CodeType == 2 ? "%03oN" : "%03oI", DCS_Options[pConfig->Code]);
            break;

            default:
            sprintf(String, "%d.%02uK", vfoInfo->StepFrequency / 100, vfoInfo->StepFrequency % 100);
            shift = -10;
        }

        if (gSetting_set_gui)
        {
            UI_PrintStringSmallNormal(s, LCD_WIDTH + 22, 0, line + 1);
            UI_PrintStringSmallNormal(t, LCD_WIDTH + 2, 0, line + 1);

            if (isMainOnly() && !gDTMF_InputMode)
            {
                if(shift == 0)
                {
                    UI_PrintStringSmallNormal(String, 2, 0, 6);
                }

                if((vfoInfo->StepFrequency / 100) < 100)
                {
                    sprintf(String, "%d.%02uK", vfoInfo->StepFrequency / 100, vfoInfo->StepFrequency % 100);
                }
                else
                {
                    sprintf(String, "%dK", vfoInfo->StepFrequency / 100);               
                }
                UI_PrintStringSmallNormal(String, 46, 0, 6);
            }
        }
        else
        {
            if ((s != NULL) && (s[0] != '\0')) {
                GUI_DisplaySmallest(s, 58, line == 0 ? 17 : 49, false, true);
            }

            if ((t != NULL) && (t[0] != '\0')) {
                GUI_DisplaySmallest(t, 3, line == 0 ? 17 : 49, false, true);
            }

            GUI_DisplaySmallest(String, 68 + shift, line == 0 ? 17 : 49, false, true);

            //sprintf(String, "%d.%02u", vfoInfo->StepFrequency / 100, vfoInfo->StepFrequency % 100);
            //GUI_DisplaySmallest(String, 91, line == 0 ? 2 : 34, false, true);
        }
#else
        UI_PrintStringSmallNormal(s, LCD_WIDTH + 24, 0, line + 1);
#endif

        if (state == VFO_STATE_NORMAL || state == VFO_STATE_ALARM)
        {   // show the TX power
            uint8_t currentPower = vfoInfo->OUTPUT_POWER % 8;
            uint8_t arrowPos = 19;
            bool userPower = false;

            if(currentPower == OUTPUT_POWER_USER)
            {
                currentPower = gSetting_set_pwr;
                userPower = true;
            }
            else
            {
                currentPower--;
                userPower = false;
            }

            if (gSetting_set_gui)
            {
                const char pwr_short[][3] = {"L1", "L2", "L3", "L4", "L5", "M", "H"};
                //sprintf(String, "%s", pwr_short[currentPower]);
                //UI_PrintStringSmallNormal(String, LCD_WIDTH + 42, 0, line + 1);
                UI_PrintStringSmallNormal(pwr_short[currentPower], LCD_WIDTH + 42, 0, line + 1);

                arrowPos = 38;
            }
            else
            {
                const char pwr_long[][5] = {"LOW1", "LOW2", "LOW3", "LOW4", "LOW5", "MID", "HIGH"};
                //sprintf(String, "%s", pwr_long[currentPower]);
                //GUI_DisplaySmallest(String, 24, line == 0 ? 17 : 49, false, true);
                GUI_DisplaySmallest(pwr_long[currentPower], 24, line == 0 ? 17 : 49, false, true);
            }

            if(userPower == true)
            {
                memcpy(p_line0 + 256 + arrowPos, BITMAP_PowerUser, sizeof(BITMAP_PowerUser));
            }
        }

        if (vfoInfo->freq_config_RX.Frequency != vfoInfo->freq_config_TX.Frequency)
        {   // show the TX offset symbol
            int i = vfoInfo->TX_OFFSET_FREQUENCY_DIRECTION % 3;

            #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
                const char dir_list[][2] = {"", "+", "-", "D"};

                if(gTxVfo->TX_OFFSET_FREQUENCY_DIRECTION != 0 && gTxVfo->pTX == &gTxVfo->freq_config_RX && !vfoInfo->FrequencyReverse)
                {
                    i = 3;
                }
            #else
                const char dir_list[][2] = {"", "+", "-"};
            #endif

#if ENABLE_FEAT_F4HWN
        if (gSetting_set_gui)
        {
            UI_PrintStringSmallNormal(dir_list[i], LCD_WIDTH + 60, 0, line + 1);
        }
        else
        {
            #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
            if(i == 3)
            {
                GUI_DisplaySmallest(dir_list[i], 43, line == 0 ? 17 : 49, false, true);
            }
            else
            {
            #endif
            UI_PrintStringSmallNormal(dir_list[i], LCD_WIDTH + 41, 0, line + 1);
            #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
            }
            #endif
        }
#else
            UI_PrintStringSmallNormal(dir_list[i], LCD_WIDTH + 54, 0, line + 1);
#endif
        }

        // show the TX/RX reverse symbol
        if (vfoInfo->FrequencyReverse)
#if ENABLE_FEAT_F4HWN
        {
            if (gSetting_set_gui)
            {
                UI_PrintStringSmallNormal("R", LCD_WIDTH + 68, 0, line + 1);
            }
            else
            {
                GUI_DisplaySmallest("R", 51, line == 0 ? 17 : 49, false, true);
            }
        }
#else
            UI_PrintStringSmallNormal("R", LCD_WIDTH + 62, 0, line + 1);
#endif

#if ENABLE_FEAT_F4HWN
        #ifdef ENABLE_FEAT_F4HWN_NARROWER
            bool narrower = 0;

            if(vfoInfo->CHANNEL_BANDWIDTH == BANDWIDTH_NARROW && gSetting_set_nfm == 1)
            {
                narrower = 1;
            }

            if (gSetting_set_gui)
            {
                const char *bandWidthNames[] = {"W", "N", "N+"};
                UI_PrintStringSmallNormal(bandWidthNames[vfoInfo->CHANNEL_BANDWIDTH + narrower], LCD_WIDTH + 80, 0, line + 1);
            }
            else
            {
                const char *bandWidthNames[] = {"WIDE", "NAR", "NAR+"};
                GUI_DisplaySmallest(bandWidthNames[vfoInfo->CHANNEL_BANDWIDTH + narrower], 91, line == 0 ? 17 : 49, false, true);
            }
        #else
            if (gSetting_set_gui)
            {
                const char *bandWidthNames[] = {"W", "N"};
                UI_PrintStringSmallNormal(bandWidthNames[vfoInfo->CHANNEL_BANDWIDTH], LCD_WIDTH + 80, 0, line + 1);
            }
            else
            {
                const char *bandWidthNames[] = {"WIDE", "NAR"};
                GUI_DisplaySmallest(bandWidthNames[vfoInfo->CHANNEL_BANDWIDTH], 91, line == 0 ? 17 : 49, false, true);
            }
        #endif
#else
        if (vfoInfo->CHANNEL_BANDWIDTH == BANDWIDTH_NARROW)
            UI_PrintStringSmallNormal("N", LCD_WIDTH + 70, 0, line + 1);
#endif

#ifdef ENABLE_DTMF_CALLING
        // show the DTMF decoding symbol
        if (vfoInfo->DTMF_DECODING_ENABLE || gSetting_KILLED)
            UI_PrintStringSmallNormal("DTMF", LCD_WIDTH + 78, 0, line + 1);
#endif

#ifndef ENABLE_FEAT_F4HWN
        // show the audio scramble symbol
        if (vfoInfo->SCRAMBLING_TYPE > 0 && gSetting_ScrambleEnable)
            UI_PrintStringSmallNormal("SCR", LCD_WIDTH + 106, 0, line + 1);
#endif

#ifdef ENABLE_FEAT_F4HWN
        /*
        if(isMainVFO)   
        {
            if(gMonitor)
            {
                sprintf(String, "%s", "MONI");
            }
            
            if (gSetting_set_gui)
            {
                if(!gMonitor)
                {
                    sprintf(String, "SQL%d", gEeprom.SQUELCH_LEVEL);
                }
                UI_PrintStringSmallNormal(String, LCD_WIDTH + 98, 0, line + 1);
            }
            else
            {
                if(!gMonitor)
                {
                    sprintf(String, "SQL%d", gEeprom.SQUELCH_LEVEL);
                }
                GUI_DisplaySmallest(String, 110, line == 0 ? 17 : 49, false, true);
            }
        }
        */
        if (isMainVFO) {
           if (gMonitor) {
                strcpy(String, "MONI");
           } else {
                sprintf(String, "SQL%d", gEeprom.SQUELCH_LEVEL);
           }

           if (gSetting_set_gui) {
                UI_PrintStringSmallNormal(String, LCD_WIDTH + 98, 0, line + 1);
           } else {
                GUI_DisplaySmallest(String, 110, line == 0 ? 17 : 49, false, true);
           }
        }
#endif
    }

display_main_after_vfo_loop:

#ifdef ENABLE_FEAT_F4HWN
    if (DualVfoMainFreqEntryScreen() && !(gEeprom.KEY_LOCK && gKeypadLocked > 0))
    {
        ST7565_BlitMainPerMode();
        return;
    }
#endif

#ifdef ENABLE_AGC_SHOW_DATA
#ifdef ENABLE_FEAT_F4HWN
    if (!isMainOnly() && !DualVfoShouldUseLegacyMain()) {
        /* new dual layout uses row 3 for top VFO detail */
    } else
#endif
    {
        center_line = CENTER_LINE_IN_USE;
        UI_MAIN_PrintAGC(false);
    }
#endif

    if (center_line == CENTER_LINE_NONE)
    {   // we're free to use the middle line

        const bool rx = FUNCTION_IsRx();

#if defined(ENABLE_AM_FIX) && defined(ENABLE_AM_FIX_SHOW_DATA)
        if (rx && gEeprom.VfoInfo[gEeprom.RX_VFO].Modulation == MODULATION_AM && gSetting_AM_fix)
        {
            if (gScreenToDisplay != DISPLAY_MAIN
#ifdef ENABLE_DTMF_CALLING
                || gDTMF_CallState != DTMF_CALL_STATE_NONE
#endif
                )
                return;

#ifdef ENABLE_FEAT_F4HWN
            if (!isMainOnly() && !DualVfoShouldUseLegacyMain()) {
                /* new dual layout uses full framebuffer */
            } else
#endif
            {
                center_line = CENTER_LINE_AM_FIX_DATA;
                AM_fix_print_data(gEeprom.RX_VFO, String);
                UI_PrintStringSmallNormal(String, 2, 0, 3);
            }
        }
        else
#endif

#ifdef ENABLE_RSSI_BAR
        if (rx) {
#ifdef ENABLE_FEAT_F4HWN
            if (!isMainOnly() && !DualVfoShouldUseLegacyMain()) {
                DisplayRSSIBar(false);
            } else
#endif
            {
                center_line = CENTER_LINE_RSSI;
                DisplayRSSIBar(false);
            }
        }
        else
#endif
        if (rx || gCurrentFunction == FUNCTION_FOREGROUND || gCurrentFunction == FUNCTION_POWER_SAVE)
        {
            #if 1
                if (gSetting_live_DTMF_decoder && gDTMF_RX_live[0] != 0 && gKeypadLocked == 0)
                {   // show live DTMF decode
                    const unsigned int len = strlen(gDTMF_RX_live);
                    const unsigned int idx = (len > (17 - 5)) ? len - (17 - 5) : 0;  // limit to last 'n' chars

                    if (gScreenToDisplay != DISPLAY_MAIN
#ifdef ENABLE_DTMF_CALLING
                        || gDTMF_CallState != DTMF_CALL_STATE_NONE
#endif
                        )
                        return;

                    center_line = CENTER_LINE_DTMF_DEC;

                    sprintf(String, "DTMF %s", gDTMF_RX_live + idx);
#ifdef ENABLE_FEAT_F4HWN
                    if (!isMainOnly() && !DualVfoShouldUseLegacyMain()) {
                        /* new dual panel: no spare row for live DTMF strip */
                    }
                    else if (isMainOnly())
                    {
                        UI_PrintStringSmallNormal(String, 2, 0, 5);
                    }
                    else
                    {
                        UI_PrintStringSmallNormal(String, 2, 0, 3);
                    }
#else
                    UI_PrintStringSmallNormal(String, 2, 0, 3);

#endif
                }
            #else
                if (gSetting_live_DTMF_decoder && gDTMF_RX_index > 0)
                {   // show live DTMF decode
                    const unsigned int len = gDTMF_RX_index;
                    const unsigned int idx = (len > (17 - 5)) ? len - (17 - 5) : 0;  // limit to last 'n' chars

                    if (gScreenToDisplay != DISPLAY_MAIN ||
                        gDTMF_CallState != DTMF_CALL_STATE_NONE)
                        return;

                    center_line = CENTER_LINE_DTMF_DEC;

                    sprintf(String, "DTMF %s", gDTMF_RX_live + idx);
                    UI_PrintStringSmallNormal(String, 2, 0, 3);
                }
            #endif

#ifdef ENABLE_SHOW_CHARGE_LEVEL
            else if (gChargingWithTypeC)
            {   // charging .. show the battery state
                if (gScreenToDisplay != DISPLAY_MAIN
#ifdef ENABLE_DTMF_CALLING
                    || gDTMF_CallState != DTMF_CALL_STATE_NONE
#endif
                    )
                    return;

#ifdef ENABLE_FEAT_F4HWN
                if (!isMainOnly() && !DualVfoShouldUseLegacyMain()) {
                    /* new dual layout uses full framebuffer */
                } else
#endif
                {
                    center_line = CENTER_LINE_CHARGE_DATA;

                    sprintf(String, "Charge %u.%02uV %u%%",
                        gBatteryVoltageAverage / 100, gBatteryVoltageAverage % 100,
                        BATTERY_VoltsToPercent(gBatteryVoltageAverage));
                    UI_PrintStringSmallNormal(String, 2, 0, 3);
                }
            }
#endif
        }
    }

#ifdef ENABLE_FEAT_F4HWN
    //#ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
    //if(gEeprom.MENU_LOCK == false)
    //{
    //#endif
    if (isMainOnly() && !gDTMF_InputMode)
    {
        sprintf(String, "VFO %s", activeTxVFO ? "B" : "A");
        GUI_DisplaySmallest(String, 107, 50, false, true);

        gFrameBuffer[6][105] ^= 0x7C;
        for (uint8_t x = 106; x < 127; x++) {
            gFrameBuffer[6][x] ^= 0xFE;
        }
        gFrameBuffer[6][127] ^= 0x7C;

        /*
        UI_PrintStringSmallBold(String, 92, 0, 6);
        for (uint8_t i = 92; i < 128; i++)
        {
            gFrameBuffer[6][i] ^= 0x7F;
        }
        */
    }
    //#ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
    //}
    //#endif
#endif

#ifdef ENABLE_FEAT_F4HWN
    ST7565_BlitMainPerMode();
#else
    ST7565_BlitFullScreen();
#endif
}
