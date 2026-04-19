/*
  u8g2_d_setup_minimal.c

  Minimal display setup for UV-KX firmware.
  Only includes the ST7565 128x64 full-buffer setup used by this project.
  Replaces the full u8g2_d_setup.c (9310 lines) to reduce LTO overhead.

  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
*/

#include "u8g2.h"

void u8g2_Setup_st7565_64128n_f(u8g2_t *u8g2, const u8g2_cb_t *rotation,
    u8x8_msg_cb byte_cb, u8x8_msg_cb gpio_and_delay_cb)
{
  uint8_t tile_buf_height;
  uint8_t *buf;
  u8g2_SetupDisplay(u8g2, u8x8_d_st7565_64128n, u8x8_cad_001,
                    byte_cb, gpio_and_delay_cb);
  buf = u8g2_m_16_8_f(&tile_buf_height);
  u8g2_SetupBuffer(u8g2, buf, tile_buf_height,
                   u8g2_ll_hvline_vertical_top_lsb, rotation);
}
