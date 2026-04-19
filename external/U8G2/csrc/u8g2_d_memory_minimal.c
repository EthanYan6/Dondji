/*
  u8g2_d_memory_minimal.c

  Minimal display memory allocation for UV-KX firmware.
  Only includes the 128x64 full-buffer memory function used by this project.
  Replaces the full u8g2_d_memory.c (2250 lines) to reduce LTO overhead.

  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
*/

#include "u8g2.h"

/* 16 tiles wide x 8 tile rows = 128x64 px = 1024 bytes in RAM */
uint8_t *u8g2_m_16_8_f(uint8_t *page_cnt)
{
#ifdef U8G2_USE_DYNAMIC_ALLOC
  *page_cnt = 8;
  return 0;
#else
  static uint8_t buf[1024];
  *page_cnt = 8;
  return buf;
#endif
}
