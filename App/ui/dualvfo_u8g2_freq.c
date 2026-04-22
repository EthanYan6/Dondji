/* u8g2 off-screen render for dual-VFO frequency digits only (no SPI). */

#include "dualvfo_u8g2_freq.h"

#include <stdio.h>
#include <string.h>

#include "driver/st7565.h"
#include "ui/helper.h"
#include "u8g2.h"

#include "font_u8g2/font_bn_tn.h"
#include "font_u8g2/font_10_tr.h"
#include "font_u8g2/font_5_tr.h"

/* 顶栏反色黑底：DV_Y_TOP_HDR=0 时 DualVfoDrawInvertedHeaderPx 画到 yBot=y+6=6，行 0..6 为黑。擦除频率区时 yTop 不得 ≤6，否则会擦掉黑底最下一行、出现白条。 */
#define DUALVFO_MAIN_FREQ_CLEAR_YTOP_MIN 7u
/* Blocktopia 主段与末两位 SmallBold 之间的额外间隔（末两位整体右移） */
#define DUALVFO_MAIN_FREQ_TAIL_GAP_PX 3u
/* 主频整串（含末两位）相对布局名义左缘再右移；若超出屏宽则夹紧 */
#define DUALVFO_MAIN_FREQ_SHIFT_RIGHT_PX 5u
/* 下面板反色条 y=29..35；副频擦除勿碰该行 */
#define DUALVFO_SUB_FREQ_CLEAR_YTOP_MIN 36u
/* 副频 blit 底边；基线下移后需含 baseline+3；左侧 S 表区由底栏单独清屏 */
#define DUALVFO_SUB_FREQ_BLIT_YMAX 50u

static u8g2_t s_u8g2;
static uint8_t s_u8g2_ready;

static void dualvfo_u8g2_prepare_small_font_text(const char *input_text, char *output_text,
                                                 size_t output_capacity)
{
    if (output_text == NULL || output_capacity == 0u)
        return;

    output_text[0] = '\0';
    if (input_text == NULL)
        return;

    size_t write_index = 0u;
    while (input_text[write_index] != '\0' && write_index + 1u < output_capacity)
    {
        const char source_char = input_text[write_index];
        char       mapped_char = source_char;

        if (source_char >= 'a' && source_char <= 'z')
            mapped_char = (char)(source_char - 'a' + 'A');

        if (mapped_char < 0x20 || mapped_char > 0x5f)
            mapped_char = ' ';

        output_text[write_index] = mapped_char;
        write_index++;
    }

    output_text[write_index] = '\0';
}

static uint8_t u8x8_gpio_all_ok(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg,
                                U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr)
{
    return 1;
}

static void dualvfo_u8g2_ensure_init(void)
{
    if (s_u8g2_ready)
        return;
    u8g2_Setup_st7565_64128n_f(&s_u8g2, U8G2_R0, u8x8_byte_empty, u8x8_gpio_all_ok);
    s_u8g2_ready = 1;
}

/* u8g2 ST7565 64128n full buffer: column-major pages, LSB = top pixel of byte (same as gFrameBuffer). */
static void dualvfo_clear_framebuffer_rect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    if (y0 > y1 || x0 > x1)
        return;
    for (uint8_t yy = y0; yy <= y1; yy++)
        for (uint8_t xx = x0; xx <= x1 && xx < LCD_WIDTH; xx++)
            UI_DrawPixelBuffer(gFrameBuffer, xx, yy, false);
}

static void dualvfo_u8g2_blit_rect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    const uint8_t *buf = u8g2_GetBufferPtr(&s_u8g2);

    if (y0 > y1 || x0 > x1)
        return;
    for (uint8_t y = y0; y <= y1; y++)
    {
        const uint8_t row = (uint8_t)(y / 8u);
        const uint8_t bit = (uint8_t)(1u << (y % 8u));
        const uint16_t row_off = (uint16_t)row * 128u;
        for (uint8_t x = x0; x <= x1 && x < LCD_WIDTH; x++)
        {
            if (buf[row_off + x] & bit)
                UI_DrawPixelBuffer(gFrameBuffer, x, y, true);
        }
    }
}

static void dualvfo_u8g2_apply_text_rect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool set_black)
{
    const uint8_t *buf = u8g2_GetBufferPtr(&s_u8g2);

    if (y0 > y1 || x0 > x1)
        return;

    for (uint8_t y = y0; y <= y1; y++)
    {
        const uint8_t row = (uint8_t)(y / 8u);
        const uint8_t bit = (uint8_t)(1u << (y % 8u));
        const uint16_t row_off = (uint16_t)row * 128u;
        for (uint8_t x = x0; x <= x1 && x < LCD_WIDTH; x++)
        {
            if (buf[row_off + x] & bit)
                UI_DrawPixelBuffer(gFrameBuffer, x, y, set_black);
        }
    }
}

uint8_t DualVfoU8g2_GetSmallTextWidth(const char *text)
{
    if (text == NULL || text[0] == '\0')
        return 0u;

    char prepared_text[64];
    dualvfo_u8g2_prepare_small_font_text(text, prepared_text, sizeof(prepared_text));
    if (prepared_text[0] == '\0')
        return 0u;

    dualvfo_u8g2_ensure_init();
    u8g2_SetFont(&s_u8g2, u8g2_font_5_tr);
    const u8g2_uint_t width = u8g2_GetStrWidth(&s_u8g2, prepared_text);
    if (width > 255u)
        return 255u;

    return (uint8_t)width;
}

void DualVfoU8g2_DrawSmallText(const char *text, uint8_t x_left, uint8_t y_top, bool set_black)
{
    if (text == NULL || text[0] == '\0')
        return;

    char prepared_text[64];
    dualvfo_u8g2_prepare_small_font_text(text, prepared_text, sizeof(prepared_text));
    if (prepared_text[0] == '\0')
        return;

    dualvfo_u8g2_ensure_init();
    u8g2_ClearBuffer(&s_u8g2);
    u8g2_SetDrawColor(&s_u8g2, 1);
    u8g2_SetFont(&s_u8g2, u8g2_font_5_tr);

    {
        const uint8_t baseline_y = (uint8_t)(y_top + 5u);
        u8g2_DrawStr(&s_u8g2, x_left, baseline_y, prepared_text);
    }

    {
        const uint8_t width = DualVfoU8g2_GetSmallTextWidth(prepared_text);
        if (width == 0u)
            return;

        const uint8_t x_right = (uint8_t)(x_left + width - 1u);
        const uint8_t y_bottom = (uint8_t)(y_top + 5u);
        dualvfo_u8g2_apply_text_rect(x_left, y_top, x_right, y_bottom, set_black);
    }
}

uint8_t DualVfoU8g2_MainFreqComputeDrawX(uint32_t frequency, uint8_t x_nominal)
{
    char           partA[12];
    char           partB[4];
    const uint32_t rem = frequency % 100000UL;

    snprintf(partA, sizeof(partA), "%3u.%03u", (unsigned)(frequency / 100000u), (unsigned)(rem / 100u));
    snprintf(partB, sizeof(partB), "%02u", (unsigned)(frequency % 100u));

    dualvfo_u8g2_ensure_init();

    u8g2_SetFont(&s_u8g2, u8g2_font_bn_tn);
    const u8g2_uint_t w1 = u8g2_GetStrWidth(&s_u8g2, partA);
    u8g2_SetFont(&s_u8g2, u8g2_font_10_tr);
    const u8g2_uint_t w2 = u8g2_GetStrWidth(&s_u8g2, partB);
    const u8g2_uint_t total = w1 + DUALVFO_MAIN_FREQ_TAIL_GAP_PX + w2;

    if ((unsigned)total >= LCD_WIDTH)
        return x_nominal;

    {
        unsigned       x     = (unsigned)x_nominal + DUALVFO_MAIN_FREQ_SHIFT_RIGHT_PX;
        const unsigned max_l = (unsigned)LCD_WIDTH - 1u - (unsigned)total;
        if (x > max_l)
            x = max_l;
        if (x < (unsigned)x_nominal)
            x = (unsigned)x_nominal;
        return (uint8_t)x;
    }
}

void DualVfoU8g2_DrawMainFreqStrip(uint32_t frequency, uint8_t x_left, uint8_t baseline_y)
{
    char            partA[12];
    char            partB[4];
    const uint32_t  rem = frequency % 100000UL;

    snprintf(partA, sizeof(partA), "%3u.%03u", (unsigned)(frequency / 100000u),
             (unsigned)(rem / 100u));
    snprintf(partB, sizeof(partB), "%02u", (unsigned)(frequency % 100u));

    dualvfo_u8g2_ensure_init();

    u8g2_ClearBuffer(&s_u8g2);
    u8g2_SetDrawColor(&s_u8g2, 1);

    u8g2_SetFont(&s_u8g2, u8g2_font_bn_tn);
    const u8g2_uint_t w1 = u8g2_GetStrWidth(&s_u8g2, partA);
    u8g2_SetFont(&s_u8g2, u8g2_font_10_tr);
    const u8g2_uint_t w2 = u8g2_GetStrWidth(&s_u8g2, partB);
    const u8g2_uint_t total = w1 + DUALVFO_MAIN_FREQ_TAIL_GAP_PX + w2;

    u8g2_SetFont(&s_u8g2, u8g2_font_bn_tn);
    u8g2_DrawStr(&s_u8g2, x_left, baseline_y, partA);
    u8g2_SetFont(&s_u8g2, u8g2_font_10_tr);
    u8g2_DrawStr(&s_u8g2, (u8g2_uint_t)(x_left + w1 + DUALVFO_MAIN_FREQ_TAIL_GAP_PX), baseline_y, partB);

    {
        uint8_t yTop = (baseline_y > 14u) ? (uint8_t)(baseline_y - 14u) : 0u;
        if (yTop < DUALVFO_MAIN_FREQ_CLEAR_YTOP_MIN)
            yTop = DUALVFO_MAIN_FREQ_CLEAR_YTOP_MIN;
        const uint8_t yBot = (uint8_t)(baseline_y + 2u);
        unsigned      xr_u = (unsigned)x_left + (unsigned)total;
        if (xr_u >= LCD_WIDTH)
            xr_u = LCD_WIDTH - 1u;
        else if (xr_u < LCD_WIDTH - 1u)
            xr_u++;
        dualvfo_clear_framebuffer_rect(x_left, yTop, (uint8_t)xr_u, yBot);
        dualvfo_u8g2_blit_rect(x_left, yTop, (uint8_t)xr_u, yBot);
    }
}

uint8_t DualVfoU8g2_DrawSubFreqStrip(uint32_t frequency, uint8_t baseline_y)
{
    char fs[16];

    snprintf(fs, sizeof(fs), "%3u.%05u", (unsigned)(frequency / 100000u),
             (unsigned)(frequency % 100000u));

    dualvfo_u8g2_ensure_init();

    u8g2_ClearBuffer(&s_u8g2);
    u8g2_SetDrawColor(&s_u8g2, 1);
    u8g2_SetFont(&s_u8g2, u8g2_font_10_tr);
    const u8g2_uint_t total = u8g2_GetStrWidth(&s_u8g2, fs);
    uint8_t           x_left =
        ((unsigned)total < LCD_WIDTH - 2u) ? (uint8_t)((unsigned)LCD_WIDTH - 2u - (unsigned)total) : 2u;

    u8g2_DrawStr(&s_u8g2, x_left, baseline_y, fs);

    {
        uint8_t yTop = (baseline_y > 12u) ? (uint8_t)(baseline_y - 12u) : 0u;
        if (yTop < DUALVFO_SUB_FREQ_CLEAR_YTOP_MIN)
            yTop = DUALVFO_SUB_FREQ_CLEAR_YTOP_MIN;
        uint8_t yBot = (uint8_t)(baseline_y + 3u);
        if (yBot > DUALVFO_SUB_FREQ_BLIT_YMAX)
            yBot = DUALVFO_SUB_FREQ_BLIT_YMAX;
        unsigned xr_u = (unsigned)x_left + (unsigned)total;
        if (xr_u >= LCD_WIDTH)
            xr_u = LCD_WIDTH - 1u;
        else if (xr_u < LCD_WIDTH - 1u)
            xr_u++;
        dualvfo_clear_framebuffer_rect(x_left, yTop, (uint8_t)xr_u, yBot);
        dualvfo_u8g2_blit_rect(x_left, yTop, (uint8_t)xr_u, yBot);
    }

    return x_left;
}
