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

#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "font.h"
#include "ui/helper.h"
#include "ui/inputbox.h"
#include "misc.h"
#include "settings.h"
#include "bitmaps.h"

#ifdef ENABLE_CHINESE

#endif

void UI_GenerateChannelString(char *pString, const uint16_t Channel)
{
    unsigned int i;

    if (gInputBoxIndex == 0)
    {
        sprintf(pString, "CH-%02u", Channel + 1);
        return;
    }

    pString[0] = 'C';
    pString[1] = 'H';
    pString[2] = '-';
    for (i = 0; i < 2; i++)
        pString[i + 3] = (gInputBox[i] == 10) ? '-' : gInputBox[i] + '0';
}

void UI_GenerateChannelStringEx(char *pString, const bool bShowPrefix, const uint16_t ChannelNumber)
{
    if (gInputBoxIndex > 0) {
        for (unsigned int i = 0; i < 4; i++) {
            pString[i] = (gInputBox[i] == 10) ? '-' : gInputBox[i] + '0';
        }

        pString[4] = 0;
        return;
    }

    if (bShowPrefix) {
        // BUG here? Prefixed NULLs are allowed
        sprintf(pString, "CH-%04u", ChannelNumber + 1);
    } else if (ChannelNumber == MR_CHANNEL_LAST + 1) {
        strcpy(pString, "None");
    } else if (ChannelNumber == 0xFFFF) {
        strcpy(pString, "NULL");
    } else {
        sprintf(pString, "%04u", ChannelNumber + 1);
    }
}

void UI_PrintStringBuffer(const char *pString, uint8_t * buffer, uint32_t char_width, const uint8_t *font)
{
    const size_t Length = strlen(pString);
    const unsigned int char_spacing = char_width + 1;
    for (size_t i = 0; i < Length; i++) {
        const unsigned int index = pString[i] - ' ' - 1;
        if (pString[i] > ' ' && pString[i] < 127) {
            const uint32_t offset = i * char_spacing + 1;
            memcpy(buffer + offset, font + index * char_width, char_width);
        }
    }
}

void UI_PrintString(const char *pString, uint8_t Start, uint8_t End, uint8_t Line, uint8_t Width)
{
    size_t i;
    size_t Length = strlen(pString);

    if (End > Start)
        Start += (((End - Start) - (Length * Width)) + 1) / 2;

    for (i = 0; i < Length; i++)
    {
        const unsigned int ofs   = (unsigned int)Start + (i * Width);
        if (pString[i] > ' ' && pString[i] < 127)
        {
            const unsigned int index = pString[i] - ' ' - 1;
            memcpy(gFrameBuffer[Line + 0] + ofs, &gFontBig[index][0], 7);
            memcpy(gFrameBuffer[Line + 1] + ofs, &gFontBig[index][7], 7);
        }
    }
}

void UI_PrintStringSmall(const char *pString, uint8_t Start, uint8_t End, uint8_t Line, uint8_t char_width, const uint8_t *font)
{
    const size_t Length = strlen(pString);
    const unsigned int char_spacing = char_width + 1;

    if (End > Start) {
        Start += (((End - Start) - Length * char_spacing) + 1) / 2;
    }

    UI_PrintStringBuffer(pString, gFrameBuffer[Line] + Start, char_width, font);
}

/* 黑底白字：仅在字体笔画处清空像素，不整块反色；适用于任意长度与字体 */
static void UI_PrintStringBufferNegative(const char *pString, uint8_t *buffer, uint32_t char_width, const uint8_t *font)
{
    const size_t Length = strlen(pString);
    const unsigned int char_spacing = char_width + 1;
    for (size_t i = 0; i < Length; i++) {
        if (pString[i] <= ' ' || pString[i] >= 127) continue;
        const unsigned int index = (unsigned int)(pString[i] - ' ' - 1);
        const uint32_t offset = i * char_spacing + 1;
        for (uint32_t c = 0; c < char_width; c++) {
            buffer[offset + c] &= ~(font[index * char_width + c]);
        }
    }
}


void UI_PrintStringSmallNormal(const char *pString, uint8_t Start, uint8_t End, uint8_t Line)
{
    UI_PrintStringSmall(pString, Start, End, Line, ARRAY_SIZE(gFontSmall[0]), (const uint8_t *)gFontSmall);
}

#ifdef ENABLE_FEAT_F4HWN
/* 小号正文字体画在单行内且底对齐：字体落在该行下 7 像素，与 8 像素高大字底边对齐，不跨行 */
void UI_PrintStringSmallNormalBottomInRow(const char *pString, uint8_t Start, uint8_t End, uint8_t Line)
{
    if (Line >= 8) return;
    const size_t Length = strlen(pString);
    const uint8_t char_width = ARRAY_SIZE(gFontSmall[0]);
    const unsigned int char_spacing = char_width + 1;
    uint8_t x_start = Start;
    if (End > Start && Length > 0)
        x_start += (((End - Start) - (unsigned int)Length * char_spacing) + 1) / 2;
    uint8_t *buffer = gFrameBuffer[Line];
    for (size_t i = 0; i < Length; i++) {
        if (pString[i] <= ' ' || pString[i] >= 127) continue;
        const unsigned int index = (unsigned int)(pString[i] - ' ' - 1);
        const uint32_t col_offset = i * char_spacing + 1;
        for (uint8_t c = 0; c < char_width && (x_start + col_offset + c) < 128; c++) {
            uint8_t V = gFontSmall[index][c];
            buffer[x_start + col_offset + c] |= (uint8_t)(V >> 1);  /* 下 7 像素，底对齐 */
        }
    }
}

/* 小号正文在像素 (x,y) 处整体绘制，y 需为 8 的倍数；整段文字一起移动，不拆行 */
void UI_PrintStringSmallNormalAt(const char *pString, uint8_t x, uint8_t y)
{
    if (y >= 64) return;
    const uint8_t row = (uint8_t)(y / 8u);
    const uint8_t char_width = ARRAY_SIZE(gFontSmall[0]);
    UI_PrintStringBuffer(pString, gFrameBuffer[row] + x, char_width, (const uint8_t *)gFontSmall);
}
#endif

/* 黑底白字：仅在笔画处清空，不整块反色；与 UI_PrintStringSmallNormal 同参数、同居中 */
void UI_PrintStringSmallNormalNegative(const char *pString, uint8_t Start, uint8_t End, uint8_t Line)
{
    const size_t Length = strlen(pString);
    const uint8_t char_width = ARRAY_SIZE(gFontSmall[0]);
    const unsigned int char_spacing = char_width + 1;
    uint8_t x_start = Start;
    if (End > Start && Length > 0)
        x_start += (((End - Start) - (unsigned int)Length * char_spacing) + 1) / 2;
    UI_PrintStringBufferNegative(pString, gFrameBuffer[Line] + x_start, char_width, (const uint8_t *)gFontSmall);
}

#ifdef ENABLE_FEAT_F4HWN
/* 黑底白字且行顶留 1 像素黑（字体下移 1 像素）：用于按钮内文字视觉居中 */
static void UI_PrintStringBufferNegativeTopMargin1(const char *pString, uint8_t *buffer, uint32_t char_width, const uint8_t *font)
{
    const size_t Length = strlen(pString);
    const unsigned int char_spacing = char_width + 1;
    for (size_t i = 0; i < Length; i++) {
        if (pString[i] <= ' ' || pString[i] >= 127) continue;
        const unsigned int index = (unsigned int)(pString[i] - ' ' - 1);
        const uint32_t offset = i * char_spacing + 1;
        for (uint32_t c = 0; c < char_width; c++) {
            const uint8_t V = font[index * char_width + c];
            buffer[offset + c] &= (uint8_t)~((V >> 1) << 1);  /* 顶 1 bit 不清，字体下移 1 像素 */
        }
    }
}
void UI_PrintStringSmallNormalNegativeTopMargin1(const char *pString, uint8_t Start, uint8_t End, uint8_t Line)
{
    const size_t Length = strlen(pString);
    const uint8_t char_width = ARRAY_SIZE(gFontSmall[0]);
    const unsigned int char_spacing = char_width + 1;
    uint8_t x_start = Start;
    if (End > Start && Length > 0)
        x_start += (((End - Start) - (unsigned int)Length * char_spacing) + 1) / 2;
    UI_PrintStringBufferNegativeTopMargin1(pString, gFrameBuffer[Line] + x_start, char_width, (const uint8_t *)gFontSmall);
}
#endif

#ifdef ENABLE_FEAT_F4HWN
/* 小号正文字体，带垂直像素偏移 vOffset(0~7)，用于小幅度上下移动 */
void UI_PrintStringSmallNormalVOffset(const char *pString, uint8_t Start, uint8_t End, uint8_t Line, uint8_t vOffset)
{
    if (Line >= 7) return;
    const size_t Length = strlen(pString);
    const uint8_t char_width = ARRAY_SIZE(gFontSmall[0]);
    const unsigned int char_spacing = char_width + 1;
    uint8_t x_start = Start;
    if (End > Start && Length > 0)
        x_start += (((End - Start) - (unsigned int)Length * char_spacing) + 1) / 2;

    if (vOffset == 0) {
        UI_PrintStringSmallNormal(pString, Start, End, Line);
        return;
    }
    /* 下移 vOffset 像素：字顶 (8-vOffset) bit -> 当前行底 (8-vOffset) bit，字底 vOffset bit -> 下一行顶 vOffset bit */
    for (size_t i = 0; i < Length; i++) {
        if (pString[i] <= ' ' || pString[i] >= 127) continue;
        const unsigned int index = (unsigned int)(pString[i] - ' ' - 1);
        const uint32_t col_offset = i * char_spacing + 1;
        for (uint8_t c = 0; c < char_width && (x_start + col_offset + c) < 128; c++) {
            uint8_t V = gFontSmall[index][c];
            uint8_t *p0 = &gFrameBuffer[Line][x_start + col_offset + c];
            uint8_t *p1 = &gFrameBuffer[Line + 1][x_start + col_offset + c];
            *p0 |= (V >> vOffset) << vOffset;
            *p1 |= (uint8_t)((V & ((1u << vOffset) - 1u)) << (8u - vOffset));
        }
    }
}

/* 小号正文字体，向上偏移 upOffset 像素(1~7)，仅上移不跑到顶行；MSB=屏顶 */
void UI_PrintStringSmallNormalVOffsetUp(const char *pString, uint8_t Start, uint8_t End, uint8_t Line, uint8_t upOffset)
{
    if (Line < 1 || Line >= 7 || upOffset == 0 || upOffset > 7) return;
    const size_t Length = strlen(pString);
    const uint8_t char_width = ARRAY_SIZE(gFontSmall[0]);
    const unsigned int char_spacing = char_width + 1;
    uint8_t x_start = Start;
    if (End > Start && Length > 0)
        x_start += (((End - Start) - (unsigned int)Length * char_spacing) + 1) / 2;
    /* 上移：字顶 upOffset bit(MSB) -> 上一行底；字底 (8-upOffset) bit -> 当前行顶 */
    for (size_t i = 0; i < Length; i++) {
        if (pString[i] <= ' ' || pString[i] >= 127) continue;
        const unsigned int index = (unsigned int)(pString[i] - ' ' - 1);
        const uint32_t col_offset = i * char_spacing + 1;
        for (uint8_t c = 0; c < char_width && (x_start + col_offset + c) < 128; c++) {
            uint8_t V = gFontSmall[index][c];
            uint8_t *p0 = &gFrameBuffer[Line - 1][x_start + col_offset + c];
            uint8_t *p1 = &gFrameBuffer[Line][x_start + col_offset + c];
            *p0 |= (V >> (8u - upOffset));  /* 字顶 upOffset bit -> 上一行底 */
            *p1 |= (uint8_t)((V << upOffset) & 0xFF);  /* 字底 (8-upOffset) bit -> 当前行顶 */
        }
    }
}
#endif

void UI_PrintStringSmallNormalInverse(const char *pString, uint8_t Start, uint8_t End, uint8_t Line)
{
    // First draw the string normally (this may center and thus change effective Start)
    UI_PrintStringSmallNormal(pString, Start, End, Line);

    // Invert the framebuffer over the *actual* text region (same centering as UI_PrintStringSmall)
    const size_t len = strlen(pString);
    const unsigned int char_width = 6;  // gFontSmall width
    const unsigned int char_spacing = char_width + 1;  // 7
    uint8_t x_start = Start;
    if (End > Start && len > 0) {
        x_start += (((End - Start) - (unsigned int)len * char_spacing) + 1) / 2;
    }
    uint8_t x_end = x_start + (uint8_t)(len * char_spacing) + 1;
    if (End != 0 && x_end > End)
        x_end = End;

    // Clamp to avoid out-of-bounds (e.g. x_start-3 underflow)
    if (x_start >= 3) {
        gFrameBuffer[Line][x_start - 3] ^= 0x3E;
        gFrameBuffer[Line][x_start - 2] ^= 0x7F;
        gFrameBuffer[Line][x_start - 1] ^= 0xFF;
    }
    /* 只反色文字所在行；小字体只画在这一行，反色上一行会破坏分隔线/按钮边框 */
    for (uint8_t x = x_start; x < x_end && x < 128; x++) {
        gFrameBuffer[Line][x] ^= 0xFF;
    }
    if (x_end < 128) {
        gFrameBuffer[Line][x_end + 0] ^= 0xFF;
        if (x_end + 1 < 128) gFrameBuffer[Line][x_end + 1] ^= 0x7F;
        if (x_end + 2 < 128) gFrameBuffer[Line][x_end + 2] ^= 0x3E;
    }
}


void UI_PrintStringSmallBold(const char *pString, uint8_t Start, uint8_t End, uint8_t Line)
{
#ifdef ENABLE_SMALL_BOLD
    const uint8_t *font = (uint8_t *)gFontSmallBold;
    const uint8_t char_width = ARRAY_SIZE(gFontSmallBold[0]);
#else
    const uint8_t *font = (uint8_t *)gFontSmall;
    const uint8_t char_width = ARRAY_SIZE(gFontSmall[0]);
#endif

    UI_PrintStringSmall(pString, Start, End, Line, char_width, font);
}

/* 黑底白字，Bold 小字：仅在笔画处清空 */
void UI_PrintStringSmallBoldNegative(const char *pString, uint8_t Start, uint8_t End, uint8_t Line)
{
#ifdef ENABLE_SMALL_BOLD
    const uint8_t *font = (uint8_t *)gFontSmallBold;
    const uint8_t char_width = ARRAY_SIZE(gFontSmallBold[0]);
#else
    const uint8_t *font = (uint8_t *)gFontSmall;
    const uint8_t char_width = ARRAY_SIZE(gFontSmall[0]);
#endif
    const size_t Length = strlen(pString);
    const unsigned int char_spacing = char_width + 1;
    uint8_t x_start = Start;
    if (End > Start && Length > 0)
        x_start += (((End - Start) - (unsigned int)Length * char_spacing) + 1) / 2;
    UI_PrintStringBufferNegative(pString, gFrameBuffer[Line] + x_start, char_width, font);
}

void UI_PrintStringSmallBoldInverse(const char *pString, uint8_t Start, uint8_t End, uint8_t Line)
{
    // First draw the string normally
    UI_PrintStringSmallBold(pString, Start, End, Line);

    // Now invert the framebuffer bits for the rendered area
    uint8_t len = strlen(pString);
    uint8_t char_width = 7; // small font is typically 6px wide

    uint8_t x_start = Start;
    uint8_t x_end   = Start + (len * char_width);

    if (End != 0 && x_end > End)
        x_end = End;

    gFrameBuffer[Line][x_start] ^= 0x7F;
    for (uint8_t x = x_start + 1; x < x_end; x++)
    {
        gFrameBuffer[Line][x] ^= 0x41;
    }
    gFrameBuffer[Line][x_end + 1] ^= 0x7F;
}


void UI_PrintStringSmallBufferNormal(const char *pString, uint8_t * buffer)
{
    UI_PrintStringBuffer(pString, buffer, ARRAY_SIZE(gFontSmall[0]), (uint8_t *)gFontSmall);
}

void UI_PrintStringSmallBufferBold(const char *pString, uint8_t * buffer)
{
#ifdef ENABLE_SMALL_BOLD
    const uint8_t *font = (uint8_t *)gFontSmallBold;
    const uint8_t char_width = ARRAY_SIZE(gFontSmallBold[0]);
#else
    const uint8_t *font = (uint8_t *)gFontSmall;
    const uint8_t char_width = ARRAY_SIZE(gFontSmall[0]);
#endif
    UI_PrintStringBuffer(pString, buffer, char_width, font);
}

void UI_DisplayFrequency(const char *string, uint8_t X, uint8_t Y, bool center)
{
    const unsigned int char_width  = 13;
    uint8_t           *pFb0        = gFrameBuffer[Y] + X;
    uint8_t           *pFb1        = pFb0 + 128;
    bool               bCanDisplay = false;

    uint8_t len = strlen(string);
    for(int i = 0; i < len; i++) {
        char c = string[i];
        if(c=='-') c = '9' + 1;
        if (bCanDisplay || c != ' ')
        {
            bCanDisplay = true;
            if(c>='0' && c<='9' + 1) {
                memcpy(pFb0 + 2, gFontBigDigits[c-'0'],                  char_width - 3);
                memcpy(pFb1 + 2, gFontBigDigits[c-'0'] + char_width - 3, char_width - 3);
            }
            else if(c=='.') {
                *pFb1 = 0x60; pFb0++; pFb1++;
                *pFb1 = 0x60; pFb0++; pFb1++;
                *pFb1 = 0x60; pFb0++; pFb1++;
                continue;
            }

        }
        else if (center) {
            pFb0 -= 6;
            pFb1 -= 6;
        }
        pFb0 += char_width;
        pFb1 += char_width;
    }
}

#ifdef ENABLE_FEAT_F4HWN
/* 大号频率整体上移 2 像素：多画一行(Y-1)接住字形顶部 2 像素，整字一起上移。maxXTop 表示 row Y-1 只写到列 < maxXTop，避免盖住信道名 */
void UI_DisplayFrequencyUp2(const char *string, uint8_t X, uint8_t Y, bool center, uint8_t maxXTop)
{
    if (Y < 1 || Y + 1 >= 7) {
        UI_DisplayFrequency(string, X, Y, center);
        return;
    }
    const unsigned int char_width = 13;
    uint8_t *pFbM1 = gFrameBuffer[Y - 1] + X;
    uint8_t *pFb0  = gFrameBuffer[Y] + X;
    uint8_t *pFb1  = gFrameBuffer[Y + 1] + X;
    bool bCanDisplay = false;
    uint8_t len = strlen(string);
    for (int i = 0; i < len; i++) {
        char c = string[i];
        if (c == '-') c = '9' + 1;
        if (bCanDisplay || c != ' ') {
            bCanDisplay = true;
            if (c >= '0' && c <= '9' + 1) {
                const uint8_t *glyph = gFontBigDigits[c - '0'];
                for (int col = 0; col < 10; col++) {
                    uint8_t b0 = glyph[col];
                    uint8_t b1 = glyph[10 + col];
                    uint16_t xcol = (uint16_t)(X + 2 + col);
                    if (maxXTop == 0 || xcol < maxXTop)
                        pFbM1[2 + col] |= (uint8_t)(b0 >> 6);
                    pFb0[2 + col]  = (uint8_t)((b0 << 2) | (b1 >> 6));
                    pFb1[2 + col]  = (uint8_t)(b1 << 2);
                }
            } else if (c == '.') {
                uint8_t v = 0x60;
                for (int k = 0; k < 3; k++) {
                    if (maxXTop == 0 || (uint16_t)(pFb0 - gFrameBuffer[Y]) < maxXTop)
                        *pFbM1 |= (uint8_t)(v >> 6);
                    *pFb0  = (uint8_t)(v << 2);
                    *pFb1  = 0u;
                    pFbM1++; pFb0++; pFb1++;
                }
                continue;
            }
        } else if (center) {
            pFbM1 -= 6;
            pFb0 -= 6;
            pFb1 -= 6;
        }
        pFbM1 += char_width;
        pFb0 += char_width;
        pFb1 += char_width;
    }
}
#endif

/*
void UI_DisplayFrequency(const char *string, uint8_t X, uint8_t Y, bool center)
{
    const unsigned int char_width  = 13;
    uint8_t           *pFb0        = gFrameBuffer[Y] + X;
    uint8_t           *pFb1        = pFb0 + 128;
    bool               bCanDisplay = false;

    if (center) {
        uint8_t len = 0;
        for (const char *ptr = string; *ptr; ptr++)
            if (*ptr != ' ') len++; // Ignores spaces for centering

        X -= (len * char_width) / 2; // Centering adjustment
        pFb0 = gFrameBuffer[Y] + X;
        pFb1 = pFb0 + 128;
    }

    for (; *string; string++) {
        char c = *string;
        if (c == '-') c = '9' + 1; // Remap of '-' symbol

        if (bCanDisplay || c != ' ') {
            bCanDisplay = true;
            if (c >= '0' && c <= '9' + 1) {
                memcpy(pFb0 + 2, gFontBigDigits[c - '0'], char_width - 3);
                memcpy(pFb1 + 2, gFontBigDigits[c - '0'] + char_width - 3, char_width - 3);
            } else if (c == '.') {
                memset(pFb1, 0x60, 3); // Replaces the three assignments
                pFb0 += 3;
                pFb1 += 3;
                continue;
            }
        }
        pFb0 += char_width;
        pFb1 += char_width;
    }
}
*/

void UI_DrawPixelBuffer(uint8_t (*buffer)[128], uint8_t x, uint8_t y, bool black)
{
    const uint8_t pattern = 1 << (y % 8);
    if(black)
        buffer[y/8][x] |= pattern;
    else
        buffer[y/8][x] &= ~pattern;
}

static void sort(int16_t *a, int16_t *b)
{
    if(*a > *b) {
        int16_t t = *a;
        *a = *b;
        *b = t;
    }
}

#ifdef ENABLE_FEAT_F4HWN
    /*
    void UI_DrawLineDottedBuffer(uint8_t (*buffer)[128], int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool black)
    {
        if(x2==x1) {
            sort(&y1, &y2);
            for(int16_t i = y1; i <= y2; i+=2) {
                UI_DrawPixelBuffer(buffer, x1, i, black);
            }
        } else {
            const int multipl = 1000;
            int a = (y2-y1)*multipl / (x2-x1);
            int b = y1 - a * x1 / multipl;

            sort(&x1, &x2);
            for(int i = x1; i<= x2; i+=2)
            {
                UI_DrawPixelBuffer(buffer, i, i*a/multipl +b, black);
            }
        }
    }
    */

    void PutPixel(uint8_t x, uint8_t y, bool fill) {
      UI_DrawPixelBuffer(gFrameBuffer, x, y, fill);
    }

    void PutPixelStatus(uint8_t x, uint8_t y, bool fill) {
      UI_DrawPixelBuffer(&gStatusLine, x, y, fill);
    }

    void GUI_DisplaySmallest(const char *pString, uint8_t x, uint8_t y,
                                    bool statusbar, bool fill) {
      uint8_t c;
      uint8_t pixels;
      const uint8_t *p = (const uint8_t *)pString;

      while ((c = *p++) && c != '\0') {
        c -= 0x20;
        for (int i = 0; i < 3; ++i) {
          pixels = gFont3x5[c][i];
          for (int j = 0; j < 6; ++j) {
            if (pixels & 1) {
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

    /* 最小字反色：黑底白字，先涂黑再在笔画处清空，画到 gFrameBuffer */
    void GUI_DisplaySmallestNegative(const char *pString, uint8_t x, uint8_t y) {
      const size_t len = strlen(pString);
      if (len == 0) return;
      const uint8_t w = (uint8_t)(len * 4);
      const uint8_t h = 6;
      for (uint8_t py = 0; py < h && (y + py) < 64; py++)
        for (uint8_t px = 0; px < w && (x + px) < 128; px++)
          UI_DrawPixelBuffer(gFrameBuffer, x + px, y + py, true);
      const uint8_t *p = (const uint8_t *)pString;
      uint8_t cx = x;
      uint8_t c;
      while ((c = *p++) && c != '\0') {
        c -= 0x20;
        for (int i = 0; i < 3; ++i) {
          uint8_t pixels = gFont3x5[c][i];
          for (int j = 0; j < 6; ++j) {
            if (pixels & 1)
              UI_DrawPixelBuffer(gFrameBuffer, cx + i, y + j, false);
            pixels >>= 1;
          }
        }
        cx += 4;
      }
    }

    void UI_DisplayUnlockKeyboard(int16_t yOffset) {
        if (gEeprom.KEY_LOCK && gKeypadLocked > 0)
        {   // tell user how to unlock the keyboard
            // 在屏幕中间绘制长方形框
            // 尺寸：宽度 +30(两边各 +15), 高度 18px
            // X: 9~118 (宽 109), Y 根据 yOffset 调整
            const int16_t rectX1 = 9;    // 左边 X 坐标
            const int16_t rectY1 = 26 + yOffset;   // 上边 Y 坐标（可调整）
            const int16_t rectX2 = 118;  // 右边 X 坐标
            const int16_t rectY2 = rectY1 + 18;   // 下边 Y 坐标（框高 18px）
            
            // 清除弹窗区域（向外 1 像素边框）
            // 只清除指定 Y 范围内的像素，每行只清除需要的 X 范围
            const int16_t clearX1 = rectX1 - 1;
            const int16_t clearX2 = rectX2 + 1;
            const int16_t clearY1 = rectY1 - 1;  // 框外上 1 像素
            const int16_t clearY2 = rectY2 + 1;  // 框外下 1 像素
            
            for (int16_t y = clearY1; y <= clearY2 && y < 64; y++) {
                if (y < 0) continue;
                const uint8_t row = y / 8;  // 计算行号
                const uint8_t bit = (uint8_t)(1 << (y % 8));  // 计算位掩码
                for (int16_t x = clearX1; x <= clearX2 && x < 128; x++) {
                    if (x < 0) continue;
                    gFrameBuffer[row][x] &= ~bit;  // 只清除指定 Y 坐标的像素
                }
            }
            
            // 绘制上边和左边（细线）
            UI_DrawLineBuffer(gFrameBuffer, rectX1, rectY1, rectX2 - 1, rectY1, true);  // 上边
            UI_DrawLineBuffer(gFrameBuffer, rectX1, rectY1, rectX1, rectY2 - 1, true);  // 左边
            
            // 绘制右边和下边（加粗：画两条线）
            UI_DrawLineBuffer(gFrameBuffer, rectX2, rectY1, rectX2, rectY2, true);      // 右边外线
            UI_DrawLineBuffer(gFrameBuffer, rectX2 - 1, rectY1, rectX2 - 1, rectY2, true); // 右边内线
            UI_DrawLineBuffer(gFrameBuffer, rectX1, rectY2, rectX2, rectY2, true);      // 下边外线
            UI_DrawLineBuffer(gFrameBuffer, rectX1, rectY2 - 1, rectX2, rectY2 - 1, true); // 下边内线
            
            // 在长方形框中间第一行绘制锁的形状
            // 锁和文字的 Y 坐标固定不变
            const uint8_t lockX = 56;  // 锁图标 X 坐标（居中）
            const uint8_t lockY = 21;  // 锁图标 Y 坐标（固定位置）
            const uint8_t lockRow = lockY / 8;
            memcpy(gFrameBuffer[lockRow] + lockX, gFontKeyLock, sizeof(gFontKeyLock));
            
            // 在长方形框中间第二行显示解锁提示文字
            const uint8_t textY = 28;  // 文字 Y 坐标（固定位置）
            const uint8_t textRow = textY / 8;
            const char *unlock_hint_text = "UNLOCK KEYBOARD";

#ifdef ENABLE_CHINESE
            if (gUiLanguage == UI_LANGUAGE_CN) {
                const uint8_t chinese_hint_base_x_start = 0u;
                const uint8_t chinese_hint_base_x_end = 119u;
                const uint8_t chinese_hint_offset_x = 1u;
                const uint8_t chinese_hint_x_start = (uint8_t)(chinese_hint_base_x_start + chinese_hint_offset_x);
                const uint8_t chinese_hint_x_end = (uint8_t)(chinese_hint_base_x_end + chinese_hint_offset_x);

                unlock_hint_text = "长按#解锁";
                UI_PrintStringSmallAtPixelKeyLockUnlockHint(
                    unlock_hint_text,
                    chinese_hint_x_start,
                    chinese_hint_x_end,
                    28u,
                    42u
                );
            } else
#endif
            {
                UI_PrintStringSmallBold(unlock_hint_text, 36, 92, textRow);
            }
        }
    }

    bool IsEmptyName(const char *name, uint8_t len) {
        if (name[0] == '\0' || name[0] == '\xff')
            return true;
        for (uint8_t i = 0; i < len; i++) {
            if (name[i] != ' ' && name[i] != '\xff' && name[i] != '\0')
                return false;
        }
        return true;
    }
#endif
    
void UI_DrawLineBuffer(uint8_t (*buffer)[128], int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool black)
{
    if(x2==x1) {
        sort(&y1, &y2);
        for(int16_t i = y1; i <= y2; i++) {
            UI_DrawPixelBuffer(buffer, x1, i, black);
        }
    } else {
        const int multipl = 1000;
        int a = (y2-y1)*multipl / (x2-x1);
        int b = y1 - a * x1 / multipl;

        sort(&x1, &x2);
        for(int i = x1; i<= x2; i++)
        {
            UI_DrawPixelBuffer(buffer, i, i*a/multipl +b, black);
        }
    }
}

void UI_DrawRectangleBuffer(uint8_t (*buffer)[128], int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool black)
{
    UI_DrawLineBuffer(buffer, x1,y1, x1,y2, black);
    UI_DrawLineBuffer(buffer, x1,y1, x2,y1, black);
    UI_DrawLineBuffer(buffer, x2,y1, x2,y2, black);
    UI_DrawLineBuffer(buffer, x1,y2, x2,y2, black);
}


void UI_DisplayPopup(const char *string)
{
    UI_DisplayClear();

    // for(uint8_t i = 1; i < 5; i++) {
    //  memset(gFrameBuffer[i]+8, 0x00, 111);
    // }

    // for(uint8_t x = 10; x < 118; x++) {
    //  UI_DrawPixelBuffer(x, 10, true);
    //  UI_DrawPixelBuffer(x, 46-9, true);
    // }

    // for(uint8_t y = 11; y < 37; y++) {
    //  UI_DrawPixelBuffer(10, y, true);
    //  UI_DrawPixelBuffer(117, y, true);
    // }
    // DrawRectangle(9,9, 118,38, true);
    UI_PrintString(string, 9, 118, 2, 8);
    UI_PrintStringSmallNormal("Press EXIT", 9, 118, 6);
}

void UI_DisplayClear()
{
    memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
}

#ifdef ENABLE_CHINESE

static bool IsChineseChar(const char *pStr)
{
    uint8_t c = (uint8_t)pStr[0];
    return (c >= 0xE4 && c <= 0xEF);
}

static uint16_t Utf8ToUnicode(const char *pStr)
{
    uint8_t c1 = (uint8_t)pStr[0];
    uint8_t c2 = (uint8_t)pStr[1];
    uint8_t c3 = (uint8_t)pStr[2];
    return ((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
}

static void DrawChineseChar(uint16_t unicode, uint8_t x, uint8_t y_pixel, uint8_t y_pixel_end)
{
    int16_t spi_index = SETTINGS_CNCharToIndex(unicode);
    if (spi_index < 0)
        return;

    const uint8_t char_height = 12;
    const uint8_t char_width = 12;
    const uint16_t y_range = (uint16_t)y_pixel_end - (uint16_t)y_pixel + 1u;
    uint8_t y_offset = 0;
    if (y_range >= char_height)
        y_offset = (uint8_t)((y_range - char_height) / 2u);
    uint8_t y = y_pixel + y_offset;
    if (y < 8)
        y = 8;

    uint16_t spi_bitmap[12];
    SETTINGS_ReadCNFontBitmap((uint16_t)spi_index, spi_bitmap);

    for (uint8_t row = 0; row < char_height; row++) {
        uint16_t row_data = spi_bitmap[row];
        uint8_t current_y = y + row;
        uint8_t line = (current_y - 8) / 8;
        uint8_t bit_offset = (current_y - 8) % 8;
        for (uint8_t col = 0; col < char_width; col++) {
            if (x + col >= LCD_WIDTH)
                break;
            if (row_data & (0x8000 >> col))
                gFrameBuffer[line][x + col] |= (1 << bit_offset);
        }
    }
}

size_t UI_SmallStringPixelWidth(const char *pString)
{
    const uint8_t eng_char_width = 6;
    const uint8_t chn_char_width = 12;
    size_t        total_width    = 0;
    size_t        i              = 0;
    while (pString[i]) {
        if (IsChineseChar(&pString[i])) {
            total_width += chn_char_width + 1;
            i += 3;
        } else {
            total_width += eng_char_width + 1;
            i++;
        }
    }
    if (total_width > 0)
        total_width--;
    return total_width;
}

void UI_PrintStringSmallAtPixel(const char *pString, uint8_t x_start, uint8_t x_end, uint8_t y_pixel_start, uint8_t y_pixel_end, uint8_t latin_down_when_mixed)
{
    const uint8_t eng_char_width = 6;
    const uint8_t eng_char_height = 7;
    const uint8_t chn_char_width = 12;
    size_t total_width = 0;
    size_t i = 0;
    bool has_chinese = false;
    while (pString[i]) {
        if (IsChineseChar(&pString[i])) {
            has_chinese = true;
            total_width += chn_char_width + 1;
            i += 3;
        } else {
            total_width += eng_char_width + 1;
            i++;
        }
    }
    if (total_width > 0)
        total_width--;
    uint8_t x = x_start;
    if (x_end > x_start && total_width < (x_end - x_start))
        x += (x_end - x_start - total_width) / 2;
    i = 0;
    while (pString[i]) {
        if (IsChineseChar(&pString[i])) {
            uint16_t unicode = Utf8ToUnicode(&pString[i]);
            DrawChineseChar(unicode, x, y_pixel_start, y_pixel_end);
            x += chn_char_width + 1;
            i += 3;
        } else {
            const uint16_t y_range = (uint16_t)y_pixel_end - (uint16_t)y_pixel_start + 1u;
            unsigned y_offset = 0;
            if (y_range >= eng_char_height)
                y_offset = (unsigned)((y_range - eng_char_height) / 2u);
            if (y_range >= eng_char_height) {
                const unsigned max_off = (unsigned)(y_range - eng_char_height);
                if (y_offset > max_off)
                    y_offset = max_off;
            }
            uint8_t y_pixel = y_pixel_start + (uint8_t)y_offset;
            /* Mixed CJK + Latin: move Latin down to align baseline with Han */
            if (has_chinese) {
                const unsigned add = (unsigned)(latin_down_when_mixed + 1u);
                if (y_pixel <= (uint8_t)(255u - add))
                    y_pixel = (uint8_t)(y_pixel + add);
            }
            if (y_pixel < 8)
                y_pixel = 8;
            uint8_t line = (y_pixel - 8) / 8;
            uint8_t bit_offset = (y_pixel - 8) % 8;
            if (pString[i] >= '!' && pString[i] < 127) {
                const unsigned int index = pString[i] - ' ' - 1;
                if (index < ARRAY_SIZE(gFontSmall)) {
                    const uint8_t *font_data = gFontSmall[index];
                    for (uint8_t col = 0; col < eng_char_width; col++) {
                        if (x + col >= LCD_WIDTH)
                            break;
                        uint8_t pixel_col = font_data[col];
                        gFrameBuffer[line][x + col] |= (pixel_col << bit_offset);
                        if (bit_offset + eng_char_height > 8 && line + 1 < FRAME_LINES)
                            gFrameBuffer[line + 1][x + col] |= (pixel_col >> (8 - bit_offset));
                    }
                }
            }
            x += eng_char_width + 1;
            i++;
        }
    }
}

void UI_PrintStringSmallAtPixelKeyLockUnlockHint(const char *pString, uint8_t x_start, uint8_t x_end, uint8_t y_pixel_start, uint8_t y_pixel_end)
{
    const uint8_t latin_down_key_lock_unlock_hint = 0u;

    UI_PrintStringSmallAtPixel(pString, x_start, x_end, y_pixel_start, y_pixel_end, latin_down_key_lock_unlock_hint);
}

void UI_PrintStringSmallChannelNameBand(const char *pString, uint8_t x_start, uint8_t x_end, uint8_t y_pixel_top)
{
    const uint8_t band_bottom_inclusive = (uint8_t)((unsigned)y_pixel_top + 11u);

    UI_PrintStringSmallAtPixel(pString, x_start, x_end, y_pixel_top, band_bottom_inclusive, 0u);
}

void UI_PrintStringSmallAtPixelInverse(const char *pString, uint8_t x_start, uint8_t x_end, uint8_t y_pixel_start, uint8_t y_pixel_end)
{
    const uint8_t eng_char_width = 6;
    const uint8_t eng_char_height = 7;
    const uint8_t chn_char_width = 12;
    size_t total_width = 0;
    size_t i = 0;
    bool has_chinese = false;
    while (pString[i]) {
        if (IsChineseChar(&pString[i])) {
            has_chinese = true;
            total_width += chn_char_width + 1;
            i += 3;
        } else {
            total_width += eng_char_width + 1;
            i++;
        }
    }
    if (total_width > 0)
        total_width--;
    uint8_t x = x_start;
    if (x_end > x_start && total_width < (x_end - x_start))
        x += (x_end - x_start - total_width) / 2;
    i = 0;
    while (pString[i]) {
        if (IsChineseChar(&pString[i])) {
            uint16_t unicode = Utf8ToUnicode(&pString[i]);
            int16_t spi_idx = SETTINGS_CNCharToIndex(unicode);
            if (spi_idx >= 0) {
                uint16_t spi_bitmap[12];
                SETTINGS_ReadCNFontBitmap((uint16_t)spi_idx, spi_bitmap);
                const uint16_t y_range_inv = (uint16_t)y_pixel_end - (uint16_t)y_pixel_start + 1u;
                const uint8_t chn_top = (y_range_inv >= 12u)
                    ? (uint8_t)(y_pixel_start + (uint8_t)((y_range_inv - 12u) / 2u))
                    : y_pixel_start;
                for (uint8_t row = 0; row < 12; row++) {
                    uint16_t row_data = spi_bitmap[row];
                    uint8_t y = chn_top + row;
                    if (y < 8)
                        y = 8;
                    uint8_t line = (y - 8) / 8;
                    uint8_t bit_offset = (y - 8) % 8;
                    for (uint8_t col = 0; col < 12; col++) {
                        if (x + col >= LCD_WIDTH)
                            break;
                        if (row_data & (0x8000 >> col))
                            gFrameBuffer[line][x + col] &= ~(1 << bit_offset);
                    }
                }
            }
            x += chn_char_width + 1;
            i += 3;
        } else {
            const uint16_t y_range = (uint16_t)y_pixel_end - (uint16_t)y_pixel_start + 1u;
            unsigned y_offset = 0;
            if (y_range >= eng_char_height)
                y_offset = (unsigned)((y_range - eng_char_height) / 2u);
            if (y_range >= eng_char_height) {
                const unsigned max_off = (unsigned)(y_range - eng_char_height);
                if (y_offset > max_off)
                    y_offset = max_off;
            }
            uint8_t y_pixel = y_pixel_start + (uint8_t)y_offset;
            if (has_chinese) {
                if (y_pixel >= y_pixel_start + 4u)
                    y_pixel -= 4u;
                else if (y_pixel > y_pixel_start)
                    y_pixel = y_pixel_start;
            }
            if (y_pixel < 8)
                y_pixel = 8;
            uint8_t line = (y_pixel - 8) / 8;
            uint8_t bit_offset = (y_pixel - 8) % 8;
            if (pString[i] >= '!' && pString[i] < 127) {
                const unsigned int char_index = pString[i] - ' ' - 1;
                if (char_index < ARRAY_SIZE(gFontSmall)) {
                    const uint8_t *font_data = gFontSmall[char_index];
                    for (uint8_t col = 0; col < eng_char_width; col++) {
                        if (x + col >= LCD_WIDTH)
                            break;
                        uint8_t pixel_col = font_data[col];
                        gFrameBuffer[line][x + col] &= ~(pixel_col << bit_offset);
                        if (bit_offset + eng_char_height > 8 && line + 1 < FRAME_LINES)
                            gFrameBuffer[line + 1][x + col] &= ~(pixel_col >> (8 - bit_offset));
                    }
                }
            }
            x += eng_char_width + 1;
            i++;
        }
    }
}

void UI_PrintStringSmallAtPixelCnInverse(const char *pString, uint8_t x_start, uint8_t x_end, uint8_t y_pixel_start, uint8_t y_pixel_end)
{
    const uint8_t chn_char_width = 12;
    /* 计算总宽度并居中 */
    size_t total_width = 0;
    size_t i = 0;
    while (pString[i]) {
        if (IsChineseChar(&pString[i])) {
            total_width += chn_char_width + 1;
            i += 3;
        } else {
            total_width += 7;
            i++;
        }
    }
    if (total_width > 0)
        total_width--;
    uint8_t x = x_start;
    if (x_end > x_start && total_width < (x_end - x_start))
        x += (uint8_t)((x_end - x_start - total_width) / 2);

    /* 先填充黑色背景 */
    for (uint8_t yy = y_pixel_start; yy <= y_pixel_end; yy++)
        for (uint8_t xx = x_start; xx <= x_end && xx < LCD_WIDTH; xx++)
            PutPixel(xx, yy, true);

    /* 绘制白色中文字符 */
    i = 0;
    while (pString[i]) {
        if (IsChineseChar(&pString[i])) {
            uint16_t unicode = Utf8ToUnicode(&pString[i]);
            int16_t spi_index = SETTINGS_CNCharToIndex(unicode);
            if (spi_index >= 0) {
                uint16_t spi_bitmap[12];
                SETTINGS_ReadCNFontBitmap((uint16_t)spi_index, spi_bitmap);
                const uint16_t y_range = (uint16_t)y_pixel_end - (uint16_t)y_pixel_start + 1u;
                uint8_t y_off = 0;
                if (y_range >= 12u)
                    y_off = (uint8_t)((y_range - 12u) / 2u);
                uint8_t y_base = (uint8_t)(y_pixel_start + y_off);
                if (y_base < 8)
                    y_base = 8;
                for (uint8_t row = 0; row < 12; row++) {
                    uint16_t row_data = spi_bitmap[row];
                    uint8_t y = y_base + row;
                    for (uint8_t col = 0; col < 12; col++) {
                        if (x + col >= LCD_WIDTH)
                            break;
                        if (row_data & (0x8000 >> col))
                            PutPixel((uint8_t)(x + col), y, false);
                    }
                }
            }
            x += chn_char_width + 1;
            i += 3;
        } else {
            /* 绘制白色 ASCII 字符（用 PutPixel 保持与中文一致） */
            const uint8_t eng_char_width = 6;
            const uint8_t eng_char_height = 7;
            const uint16_t y_range = (uint16_t)y_pixel_end - (uint16_t)y_pixel_start + 1u;
            unsigned y_offset = 0;
            if (y_range >= eng_char_height)
                y_offset = (unsigned)((y_range - eng_char_height) / 2u);
            uint8_t y_pixel = (uint8_t)(y_pixel_start + (uint8_t)y_offset);
            if (y_pixel < 8)
                y_pixel = 8;
            if (pString[i] >= '!' && pString[i] < 127) {
                const unsigned int index = pString[i] - ' ' - 1;
                if (index < ARRAY_SIZE(gFontSmall)) {
                    const uint8_t *font_data = gFontSmall[index];
                    for (uint8_t col = 0; col < eng_char_width; col++) {
                        if (x + col >= LCD_WIDTH)
                            break;
                        uint8_t pixel_col = font_data[col];
                        for (uint8_t row = 0; row < eng_char_height; row++) {
                            if (pixel_col & (1u << row))
                                PutPixel((uint8_t)(x + col), (uint8_t)(y_pixel + row), false);
                        }
                    }
                }
            }
            x += eng_char_width + 1;
            i++;
        }
    }
}

#endif /* ENABLE_CHINESE */
