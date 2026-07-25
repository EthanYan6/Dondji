/*
 * Dondji Firmware
 *
 * Copyright (c) 2026 BD1AHN
 *
 * Licensed under the Apache License, Version 2.0
 *
 * Project:
 *     叮咚鸡 (Dondji)
 *
 * Maintainer:
 *     BD1AHN
 *
 * Official Website:
 *     https://ethanyan6.github.io/Dondji/
 *
 * The Dondji name, logo, and official project identity
 * are protected separately from the source code license.
 */


/* Dual-VFO main screen: draw MHz strings with u8g2 fonts (UV-KX style), blit into gFrameBuffer. */
#ifndef DUALVFO_U8G2_FREQ_H
#define DUALVFO_U8G2_FREQ_H

#include <stdbool.h>
#include <stdint.h>

/** Left edge of main frequency (Blocktopia + gap + tail) after optional right shift and screen clamp. */
uint8_t DualVfoU8g2_MainFreqComputeDrawX(uint32_t frequency, uint8_t x_nominal);

void DualVfoU8g2_DrawMainFreqStrip(uint32_t frequency, uint8_t x_left, uint8_t baseline_y);
/** 副信道频率：加粗 10px 体，右对齐；返回绘制左缘 x（供 XOR 对齐）。 */
uint8_t DualVfoU8g2_DrawSubFreqStrip(uint32_t frequency, uint8_t baseline_y);
uint8_t DualVfoU8g2_GetSmallTextWidth(const char *text);
void DualVfoU8g2_DrawSmallText(const char *text, uint8_t x_left, uint8_t y_top, bool set_black);
void DualVfoU8g2_DrawSmallTextStatus(const char *text, uint8_t x_left, uint8_t y_top, bool set_black);

#endif
