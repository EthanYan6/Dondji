/*

  u8x8_d_st7565.c
  also includes support for nt7534

  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

  Copyright (c) 2016, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list
    of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


*/
#include "u8x8.h"





static const uint8_t u8x8_d_st7565_powersave0_seq[] = {
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  U8X8_C(0x0a4),		                /* all pixel off, issue 142 */
  U8X8_C(0x0af),		                /* display on */
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

static const uint8_t u8x8_d_st7565_powersave1_seq[] = {
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x0a5),		                /* enter powersafe: all pixel on, issue 142 */
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

static const uint8_t u8x8_d_st7565_flip0_seq[] = {
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  U8X8_C(0x0a1),				/* segment remap a0/a1*/
  U8X8_C(0x0c0),				/* c0: scan dir normal, c8: reverse */
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

static const uint8_t u8x8_d_st7565_flip1_seq[] = {
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  U8X8_C(0x0a0),				/* segment remap a0/a1*/
  U8X8_C(0x0c8),				/* c0: scan dir normal, c8: reverse */
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

static const uint8_t u8x8_d_st7565_zflip0_seq[] = {
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  U8X8_C(0x0a1),				/* segment remap a0/a1*/
  U8X8_C(0x0c8),				/* c0: scan dir normal, c8: reverse */
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

static const uint8_t u8x8_d_st7565_zflip1_seq[] = {
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  U8X8_C(0x0a0),				/* segment remap a0/a1*/
  U8X8_C(0x0c0),				/* c0: scan dir normal, c8: reverse */
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

static const u8x8_display_info_t u8x8_st7565_128x64_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,

  /* post_chip_enable_wait_ns = */ 150,	/* st7565 datasheet, table 26, tcsh */
  /* pre_chip_disable_wait_ns = */ 50,	/* st7565 datasheet, table 26, tcss */
  /* reset_pulse_width_ms = */ 1,
  /* post_reset_wait_ms = */ 1,
  /* sda_setup_time_ns = */ 50,		/* st7565 datasheet, table 26, tsds */
  /* sck_pulse_width_ns = */ 120,	/* half of cycle time (100ns according to datasheet), AVR: below 70: 8 MHz, >= 70 --> 4MHz clock */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,	/* st7565 datasheet, table 24, tds8 */
  /* write_pulse_width_ns = */ 80,	/* st7565 datasheet, table 24, tcclw */
  /* tile_width = */ 16,		/* width of 16*8=128 pixel */
  /* tile_height = */ 8,
  /* default_x_offset = */ 0,
  /* flipmode_x_offset = */ 4,
  /* pixel_width = */ 128,
  /* pixel_height = */ 64
};

uint8_t u8x8_d_st7565_common(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, c;
  uint8_t *ptr;
  switch(msg)
  {
    case U8X8_MSG_DISPLAY_DRAW_TILE:
      u8x8_cad_StartTransfer(u8x8);

      x = ((u8x8_tile_t *)arg_ptr)->x_pos;
      x *= 8;
      x += u8x8->x_offset;
      u8x8_cad_SendCmd(u8x8, 0x010 | (x>>4) );
      u8x8_cad_SendCmd(u8x8, 0x000 | ((x&15)));
      u8x8_cad_SendCmd(u8x8, 0x0b0 | (((u8x8_tile_t *)arg_ptr)->y_pos));

      c = ((u8x8_tile_t *)arg_ptr)->cnt;
      c *= 8;
      ptr = ((u8x8_tile_t *)arg_ptr)->tile_ptr;
      /*
	The following if condition checks the hardware limits of the st7565
	controller: It is not allowed to write beyond the display limits.
	This is in fact an issue within flip mode.
      */
      if ( c + x > 132u )
      {
	c = 132u;
	c -= x;
      }
      do
      {
	u8x8_cad_SendData(u8x8, c, ptr);	/* note: SendData can not handle more than 255 bytes */
	arg_int--;
      } while( arg_int > 0 );

      u8x8_cad_EndTransfer(u8x8);
      break;
    /*	handled in the calling procedure
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
      u8x8_d_helper_display_setup_memory(u8x8, &u8x8_st7565_128x64_display_info);
      break;
    case U8X8_MSG_DISPLAY_INIT:
      u8x8_d_helper_display_init(u8x8);
      u8x8_cad_SendSequence(u8x8, u8x8_d_uc1701_dogs102_init_seq);
      break;
    */
    case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
      if ( arg_int == 0 )
	u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_powersave0_seq);
      else
	u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_powersave1_seq);
      break;
#ifdef U8X8_WITH_SET_CONTRAST
    case U8X8_MSG_DISPLAY_SET_CONTRAST:
      u8x8_cad_StartTransfer(u8x8);
      u8x8_cad_SendCmd(u8x8, 0x081 );
      u8x8_cad_SendArg(u8x8, arg_int >> 2 );	/* st7565 has range from 0 to 63 */
      u8x8_cad_EndTransfer(u8x8);
      break;
#endif
    default:
      return 0;
  }
  return 1;
}

/*================================================*/
/* DOGM128 */

static const uint8_t u8x8_d_st7565_dogm128_init_seq[] = {

  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  U8X8_C(0x0e2),            			/* soft reset */
  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x040),		                /* set display start line to 0 */

  U8X8_C(0x0a1),		                /* ADC set to reverse */
  U8X8_C(0x0c0),		                /* common output mode */
  // Flipmode
  // U8X8_C(0x0a0),		                /* ADC set to reverse */
  // U8X8_C(0x0c8),		                /* common output mode */

  U8X8_C(0x0a6),		                /* display normal, bit val 0: LCD pixel off. */
  U8X8_C(0x0a2),		                /* LCD bias 1/9 */
  U8X8_C(0x02f),		                /* all power  control circuits on (regulator, booster and follower) */
  U8X8_CA(0x0f8, 0x000),		/* set booster ratio to 4x */
  U8X8_C(0x027),		                /* set V0 voltage resistor ratio to max  */
  U8X8_CA(0x081, 0x018),		/* set contrast, contrast value, EA default: 0x016 */

  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x0a5),		                /* enter powersafe: all pixel on, issue 142 */

  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

uint8_t u8x8_d_st7565_ea_dogm128(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* call common procedure first and handle messages there */
  if ( u8x8_d_st7565_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case U8X8_MSG_DISPLAY_SETUP_MEMORY:
	u8x8_d_helper_display_setup_memory(u8x8, &u8x8_st7565_128x64_display_info);
	break;
      case U8X8_MSG_DISPLAY_INIT:
	u8x8_d_helper_display_init(u8x8);
	u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_dogm128_init_seq);
	break;
      case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	if ( arg_int == 0 )
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip0_seq);
	  u8x8->x_offset = u8x8->display_info->default_x_offset;
	}
	else
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip1_seq);
	  u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	}
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}



/*================================================*/
/* LM6063 https://github.com/olikraus/u8g2/issues/893 */

static const uint8_t u8x8_d_st7565_lm6063_init_seq[] = {

  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  U8X8_C(0x0e2),            			/* soft reset */
  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x040),		                /* set display start line to 0 */

  U8X8_C(0x0a1),		                /* ADC set to reverse */
  U8X8_C(0x0c0),		                /* common output mode */
  // Flipmode
  // U8X8_C(0x0a0),		                /* ADC set to reverse */
  // U8X8_C(0x0c8),		                /* common output mode */

  U8X8_C(0x0a6),		                /* display normal, bit val 0: LCD pixel off. */
  U8X8_C(0x0a2),		                /* LCD bias 1/9 */
  U8X8_C(0x02f),		                /* all power  control circuits on (regulator, booster and follower) */
  U8X8_CA(0x0f8, 0x000),		/* set booster ratio to 4x */
  U8X8_C(0x027),		                /* set V0 voltage resistor ratio to max  */
  U8X8_CA(0x081, 50/4),		/* set contrast, contrast value, 40..60 seems to be good */

  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x0a5),		                /* enter powersafe: all pixel on, issue 142 */

  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

uint8_t u8x8_d_st7565_lm6063(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* call common procedure first and handle messages there */
  if ( u8x8_d_st7565_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case U8X8_MSG_DISPLAY_SETUP_MEMORY:
	u8x8_d_helper_display_setup_memory(u8x8, &u8x8_st7565_128x64_display_info);
	break;
      case U8X8_MSG_DISPLAY_INIT:
	u8x8_d_helper_display_init(u8x8);
	u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_lm6063_init_seq);
	break;
      case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	if ( arg_int == 0 )
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip0_seq);
	  u8x8->x_offset = u8x8->display_info->default_x_offset;
	}
	else
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip1_seq);
	  u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	}
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}


/*================================================*/
/* Displaytech 64128n */

// Software reset
const uint8_t ST7565_CMD_SOFTWARE_RESET = 0xE2;
// Bias Select
// 1 0 1 0 0 0 1 BS
// Select bias setting 0=1/9; 1=1/7 (at 1/65 duty)
const uint8_t ST7565_CMD_BIAS_SELECT = 0xA2;
// COM Direction
// 1 1 0 0 MY - - -
// Set output direction of COM
// MY=1, reverse direction
// MY=0, normal direction
const uint8_t ST7565_CMD_COM_DIRECTION = 0xC0;
// SEG Direction
// 1 0 1 0 0 0 0 MX
// Set scan direction of SEG
// MX=1, reverse direction
// MX=0, normal direction
const uint8_t ST7565_CMD_SEG_DIRECTION = 0xA0;
// Inverse Display
// 1 0 1 0 0 1 1 INV
// INV =1, inverse display
// INV =0, normal display
const uint8_t ST7565_CMD_INVERSE_DISPLAY = 0xA6;
// All Pixel ON
// 1 0 1 0 0 1 0 AP
// AP=1, set all pixel ON
// AP=0, normal display
const uint8_t ST7565_CMD_ALL_PIXEL_ON = 0xA4;
// Regulation Ratio
// 0 0 1 0 0 RR2 RR1 RR0
// This instruction controls the regulation ratio of the built-in regulator
const uint8_t ST7565_CMD_REGULATION_RATIO = 0x20;
// Double command!! Set electronic volume (EV) level
// Send next: 0 0 EV5 EV4 EV3 EV2 EV1 EV0  contrast 0-63
const uint8_t ST7565_CMD_SET_EV = 0x81;
// Control built-in power circuit ON/OFF - 0 0 1 0 1 VB VR VF
// VB: Built-in Booster
// VR: Built-in Regulator
// VF: Built-in Follower
const uint8_t ST7565_CMD_POWER_CIRCUIT = 0x28;
// Set display start line 0-63
// 0 0 0 1 S5 S4 S3 S2 S1 S0
const uint8_t ST7565_CMD_SET_START_LINE = 0x40;
// Display ON/OFF
// 0 0 1 0 1 0 1 1 1 D
// D=1, display ON
// D=0, display OFF
const uint8_t ST7565_CMD_DISPLAY_ON_OFF = 0xAE;

static const uint8_t u8x8_d_st7565_64128n_init_seq[] = {

  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  #ifdef NOT_WORKING

  U8X8_C(0x0e2),            			  /* soft reset */
  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x040),		                /* set display start line to 0 */

  U8X8_C(0x0a1),		                /* ADC set to reverse */
  U8X8_C(0x0c0),		                /* common output mode */
  // Flipmode
  // U8X8_C(0x0a0),		                /* ADC set to reverse */
  // U8X8_C(0x0c8),		                /* common output mode */

  U8X8_C(0x0a6),		                /* display normal, bit val 0: LCD pixel off. */
  U8X8_C(0x0a2),		                /* LCD bias 1/9 */
  U8X8_C(0x02f),		                /* all power  control circuits on */
  //U8X8_CA(0x0f8, 0x000),		/* set booster ratio to 4x */
  //U8X8_C(0x027),		                /* set V0 voltage resistor ratio to max  */

  U8X8_C(0x010),                   		/* Set V0 voltage resistor ratio. Setting for controlling brightness of Displaytech 64128N */


  U8X8_CA(0x081, 0x01e),		/* set contrast, contrast value */

  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x0a5),		                /* enter powersafe: all pixel on, issue 142 */
#else


  U8X8_C(ST7565_CMD_BIAS_SELECT | 0),
  U8X8_C(ST7565_CMD_COM_DIRECTION  | (0 << 3)),
  U8X8_C(ST7565_CMD_SEG_DIRECTION | 1),
  U8X8_C(ST7565_CMD_INVERSE_DISPLAY | 0),
  U8X8_C(ST7565_CMD_ALL_PIXEL_ON | 0),
  U8X8_C(ST7565_CMD_REGULATION_RATIO | (4 << 0)),
  U8X8_C(ST7565_CMD_SET_EV),
  U8X8_C(31),

  U8X8_C(ST7565_CMD_POWER_CIRCUIT | 0b011),
  U8X8_C(ST7565_CMD_POWER_CIRCUIT | 0b110),
  U8X8_C(ST7565_CMD_POWER_CIRCUIT | 0b111),

  U8X8_C(ST7565_CMD_SET_START_LINE | 0),

  U8X8_C(ST7565_CMD_DISPLAY_ON_OFF | 0),

#endif

  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

static const u8x8_display_info_t u8x8_st7565_64128n_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,

  /* post_chip_enable_wait_ns = */ 150,	/* st7565 datasheet, table 26, tcsh */
  /* pre_chip_disable_wait_ns = */ 50,	/* st7565 datasheet, table 26, tcss */
  /* reset_pulse_width_ms = */ 5,
  /* post_reset_wait_ms = */ 10,
  /* sda_setup_time_ns = */ 50,		/* st7565 datasheet, table 26, tsds */
  /* sck_pulse_width_ns = */ 120,	/* half of cycle time (100ns according to datasheet), AVR: below 70: 8 MHz, >= 70 --> 4MHz clock */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,	/* st7565 datasheet, table 24, tds8 */
  /* write_pulse_width_ns = */ 80,	/* st7565 datasheet, table 24, tcclw */
  /* tile_width = */ 16,		/* width of 16*8=128 pixel */
  /* tile_height = */ 8,
  /* default_x_offset = */ 4,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 128,
  /* pixel_height = */ 64
};

uint8_t u8x8_d_st7565_64128n(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* call common procedure first and handle messages there */
  if ( u8x8_d_st7565_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case U8X8_MSG_DISPLAY_SETUP_MEMORY:
	u8x8_d_helper_display_setup_memory(u8x8, &u8x8_st7565_64128n_display_info);
	break;
      case U8X8_MSG_DISPLAY_INIT:
	u8x8_d_helper_display_init(u8x8);
	u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_64128n_init_seq);
	break;
      case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	if ( arg_int == 0 )
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip0_seq);
	  u8x8->x_offset = u8x8->display_info->default_x_offset;
	}
	else
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip1_seq);
	  u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	}
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}

/*================================================*/
/* ZOLEN 128x64  */

static const uint8_t u8x8_d_st7565_zolen_128x64_init_seq[] = {

  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  U8X8_C(0x0e2),            			/* soft reset */
  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x040),		                /* set display start line to 0 */

  U8X8_C(0x0a1),		                /* ADC set to reverse */
  U8X8_C(0x0c8),		                /* common output mode */
  // Flipmode
  // U8X8_C(0x0a0),		                /* ADC set to reverse */
  // U8X8_C(0x0c0),		                /* common output mode */

  U8X8_C(0x0a6),		                /* display normal, bit val 0: LCD pixel off. */
  U8X8_C(0x0a2),		                /* LCD bias 1/9 */
  U8X8_C(0x02f),		                /* all power  control circuits on (regulator, booster and follower) */
  U8X8_CA(0x0f8, 0x000),		/* set booster ratio to 4x */
  U8X8_C(0x027),		                /* set V0 voltage resistor ratio to max  */
  U8X8_CA(0x081, 0x007),		/* set contrast, contrast value, EA default: 0x016 */

  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x0a5),		                /* enter powersafe: all pixel on, issue 142 */

  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

uint8_t u8x8_d_st7565_zolen_128x64(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* call common procedure first and handle messages there */
  if ( u8x8_d_st7565_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case U8X8_MSG_DISPLAY_SETUP_MEMORY:
	u8x8_d_helper_display_setup_memory(u8x8, &u8x8_st7565_128x64_display_info);
	break;
      case U8X8_MSG_DISPLAY_INIT:
	u8x8_d_helper_display_init(u8x8);
	u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_zolen_128x64_init_seq);
	break;
      case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	if ( arg_int == 0 )
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_zflip0_seq);
	  u8x8->x_offset = u8x8->display_info->default_x_offset;
	}
	else
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_zflip1_seq);
	  u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	}
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}



/*================================================*/
/* NHD-C12832 */

static const u8x8_display_info_t u8x8_st7565_128x32_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,

  /* post_chip_enable_wait_ns = */ 150,	/* st7565 datasheet, table 26, tcsh */
  /* pre_chip_disable_wait_ns = */ 50,	/* st7565 datasheet, table 26, tcss */
  /* reset_pulse_width_ms = */ 1,
  /* post_reset_wait_ms = */ 1,
  /* sda_setup_time_ns = */ 50,		/* st7565 datasheet, table 26, tsds */
  /* sck_pulse_width_ns = */ 120,	/* half of cycle time (100ns according to datasheet), AVR: below 70: 8 MHz, >= 70 --> 4MHz clock */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,	/* st7565 datasheet, table 24, tds8 */
  /* write_pulse_width_ns = */ 80,	/* st7565 datasheet, table 24, tcclw */
  /* tile_width = */ 16,		/* width of 16*8=128 pixel */
  /* tile_height = */ 4,
  /* default_x_offset = */ 4,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 128,
  /* pixel_height = */ 32
};


static const uint8_t u8x8_d_st7565_nhd_c12832_init_seq[] = {

  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  U8X8_C(0x0e2),            			/* soft reset */
  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x040),		                /* set display start line to 0 */

  U8X8_C(0x0a1),		                /* ADC set to reverse */
  U8X8_C(0x0c0),		                /* common output mode */
  // Flipmode
  //U8X8_C(0x0a0),		                /* ADC set to reverse */
  //U8X8_C(0x0c8),		                /* common output mode */

  U8X8_C(0x0a6),		                /* display normal, bit val 0: LCD pixel off. */
  U8X8_C(0x0a2),		                /* LCD bias 1/9 */
  U8X8_C(0x02f),		                /* all power  control circuits on */
  U8X8_CA(0x0f8, 0x000),		/* set booster ratio to 4x */
  U8X8_C(0x023),		                /* set V0 voltage resistor ratio to large*/
  U8X8_CA(0x081, 0x00a),		/* set contrast, contrast value NHD C12832 */

  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x0a5),		                /* enter powersafe: all pixel on, issue 142 */

  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

uint8_t u8x8_d_st7565_nhd_c12832(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* call common procedure first and handle messages there */
  if ( u8x8_d_st7565_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case U8X8_MSG_DISPLAY_SETUP_MEMORY:
	u8x8_d_helper_display_setup_memory(u8x8, &u8x8_st7565_128x32_display_info);
	break;
      case U8X8_MSG_DISPLAY_INIT:
	u8x8_d_helper_display_init(u8x8);
	u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_nhd_c12832_init_seq);
	break;
      case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	if ( arg_int == 0 )
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip0_seq);
	  u8x8->x_offset = u8x8->display_info->default_x_offset;
	}
	else
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip1_seq);
	  u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	}
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}

/*================================================*/
/* NHD-C12864 */

static const u8x8_display_info_t u8x8_st7565_nhd_c12864_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,

  /* post_chip_enable_wait_ns = */ 150,	/* st7565 datasheet, table 26, tcsh */
  /* pre_chip_disable_wait_ns = */ 50,	/* st7565 datasheet, table 26, tcss */
  /* reset_pulse_width_ms = */ 1,
  /* post_reset_wait_ms = */ 1,
  /* sda_setup_time_ns = */ 50,		/* st7565 datasheet, table 26, tsds */
  /* sck_pulse_width_ns = */ 120,	/* half of cycle time (100ns according to datasheet), AVR: below 70: 8 MHz, >= 70 --> 4MHz clock */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,	/* st7565 datasheet, table 24, tds8 */
  /* write_pulse_width_ns = */ 80,	/* st7565 datasheet, table 24, tcclw */
  /* tile_width = */ 16,		/* width of 16*8=128 pixel */
  /* tile_height = */ 8,
  /* default_x_offset = */ 4,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 128,
  /* pixel_height = */ 64
};


static const uint8_t u8x8_d_st7565_nhd_c12864_init_seq[] = {

  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  U8X8_C(0x0e2),            			/* soft reset */
  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x040),		                /* set display start line to 0 */

  U8X8_C(0x0a1),		                /* ADC set to reverse */
  U8X8_C(0x0c0),		                /* common output mode */
  // Flipmode
  //U8X8_C(0x0a0),		                /* ADC set to reverse */
  //U8X8_C(0x0c8),		                /* common output mode */

  U8X8_C(0x0a6),		                /* display normal, bit val 0: LCD pixel off. */
  U8X8_C(0x0a2),		                /* LCD bias 1/9 */
  U8X8_C(0x02f),		                /* all power  control circuits on */
  U8X8_CA(0x0f8, 0x000),		/* set booster ratio to 4x */
  U8X8_C(0x025),		                /* set V0 voltage resistor ratio to large,  issue 1678: changed from 0x23 to 0x25 */
  U8X8_CA(0x081, 170),			/* set contrast, contrast value NHD C12864, see issue 186, increased contrast to 180 (issue 219), reduced to 170 (issue 1678) */

  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x0a5),		                /* enter powersafe: all pixel on, issue 142 */

  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

uint8_t u8x8_d_st7565_nhd_c12864(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* call common procedure first and handle messages there */
  if ( u8x8_d_st7565_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case U8X8_MSG_DISPLAY_SETUP_MEMORY:
	u8x8_d_helper_display_setup_memory(u8x8, &u8x8_st7565_nhd_c12864_display_info);
	break;
      case U8X8_MSG_DISPLAY_INIT:
	u8x8_d_helper_display_init(u8x8);
	u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_nhd_c12864_init_seq);
	break;
      case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	if ( arg_int == 0 )
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip0_seq);
	  u8x8->x_offset = u8x8->display_info->default_x_offset;
	}
	else
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip1_seq);
	  u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	}
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}
/*================================================*/
/* JLX12864 */

uint8_t u8x8_d_st7565_jlx12864(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  return u8x8_d_st7565_nhd_c12864(u8x8, msg, arg_int, arg_ptr);
}


/*================================================*/
/* LM6059 (Adafruit)... probably this is a ST7567 display */

static const uint8_t u8x8_d_st7565_lm6059_init_seq[] = {

  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  U8X8_C(0x0e2),            			/* soft reset */
  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x060),		                /* set display start line to ... */

  U8X8_C(0x0a0),		                /* ADC set to reverse */
  U8X8_C(0x0c8),		                /* common output mode */
  //U8X8_C(0x0a1),		                /* ADC set to reverse */
  //U8X8_C(0x0c0),		                /* common output mode */
  // Flipmode
  // U8X8_C(0x0a0),		                /* ADC set to reverse */
  // U8X8_C(0x0c8),		                /* common output mode */

  U8X8_C(0x0a6),		                /* display normal, bit val 0: LCD pixel off. */
  U8X8_C(0x0a3),		                /* LCD bias 1/9 */
  U8X8_C(0x02f),		                /* all power  control circuits on (regulator, booster and follower) */
  U8X8_CA(0x0f8, 0x000),		/* set booster ratio to 4x (ST7567 feature) */
  U8X8_C(0x027),		                /* set V0 voltage resistor ratio to max  */
  U8X8_CA(0x081, 0x018),		/* set contrast, contrast value, EA default: 0x016 */

  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x0a5),		                /* enter powersafe: all pixel on, issue 142 */

  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

static const u8x8_display_info_t u8x8_st7565_lm6059_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,

  /* post_chip_enable_wait_ns = */ 150,	/* st7565 datasheet, table 26, tcsh */
  /* pre_chip_disable_wait_ns = */ 50,	/* st7565 datasheet, table 26, tcss */
  /* reset_pulse_width_ms = */ 1,
  /* post_reset_wait_ms = */ 1,
  /* sda_setup_time_ns = */ 50,		/* st7565 datasheet, table 26, tsds */
  /* sck_pulse_width_ns = */ 120,	/* half of cycle time (100ns according to datasheet), AVR: below 70: 8 MHz, >= 70 --> 4MHz clock */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,	/* st7565 datasheet, table 24, tds8 */
  /* write_pulse_width_ns = */ 80,	/* st7565 datasheet, table 24, tcclw */
  /* tile_width = */ 16,		/* width of 16*8=128 pixel */
  /* tile_height = */ 8,
  /* default_x_offset = */ 1,	/* not sure... */
  /* flipmode_x_offset = */ 3,
  /* pixel_width = */ 128,
  /* pixel_height = */ 64
};

uint8_t u8x8_d_st7565_lm6059(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* call common procedure first and handle messages there */
  if ( u8x8_d_st7565_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case U8X8_MSG_DISPLAY_SETUP_MEMORY:
	u8x8_d_helper_display_setup_memory(u8x8, &u8x8_st7565_lm6059_display_info);
	break;
      case U8X8_MSG_DISPLAY_INIT:
	u8x8_d_helper_display_init(u8x8);
	u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_lm6059_init_seq);
	break;
      case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	if ( arg_int == 0 )
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip1_seq);
	  u8x8->x_offset = u8x8->display_info->default_x_offset;
	}
	else
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip0_seq);
	  u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	}
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}

/*================================================*/
/* https://github.com/olikraus/u8g2/issues/1314 */
/* KS0713 controller, takeover from LM6059 */

static const uint8_t u8x8_d_st7565_ks0713_init_seq[] = {

  U8X8_START_TRANSFER(),             /* enable chip, delay is part of the transfer start */
  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x0e2),            			 /* soft reset */
  U8X8_C(0x0a3),		                /* LCD bias 1/9 */
  U8X8_C(0x0a0),		                /* ADC set to reverse */
  U8X8_C(0x0c0),		                /* common output mode */
  U8X8_C(0x02f),		                /* all power  control circuits on (regulator, booster and follower) */
  U8X8_C(0x026),		                /* set V0 voltage resistor ratio to max  */
  U8X8_C(0x0a6),		                /* display normal, bit val 0: LCD pixel off. */
  U8X8_CA(0x081, 0x010),		       /* set contrast, contrast value, EA default: 0x016 */
  U8X8_C(0x0af),		                /* display on */
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

uint8_t u8x8_d_st7565_ks0713(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* call common procedure first and handle messages there */
  if ( u8x8_d_st7565_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case U8X8_MSG_DISPLAY_SETUP_MEMORY:
	u8x8_d_helper_display_setup_memory(u8x8, &u8x8_st7565_lm6059_display_info);
	break;
      case U8X8_MSG_DISPLAY_INIT:
	u8x8_d_helper_display_init(u8x8);
	u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_ks0713_init_seq);
	break;
      case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	if ( arg_int == 0 )
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_zflip1_seq);
	  u8x8->x_offset = u8x8->display_info->default_x_offset;
	}
	else
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_zflip0_seq);
	  u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	}
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}


/*================================================*/
/* LX12864 issue 576 */

static const uint8_t u8x8_d_st7565_lx12864_init_seq[] = {

  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  U8X8_C(0x0e2),            			/* soft reset */
  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x060),		                /* set display start line to ... */

  U8X8_C(0x0a0),		                /* ADC set to reverse */
  U8X8_C(0x0c8),		                /* common output mode */
  //U8X8_C(0x0a1),		                /* ADC set to reverse */
  //U8X8_C(0x0c0),		                /* common output mode */
  // Flipmode
  // U8X8_C(0x0a0),		                /* ADC set to reverse */
  // U8X8_C(0x0c8),		                /* common output mode */

  U8X8_C(0x0a6),		                /* display normal, bit val 0: LCD pixel off. */
  U8X8_C(0x0a2),		                /* LCD bias 1/9 */
  U8X8_C(0x02f),		                /* all power  control circuits on (regulator, booster and follower) */
  U8X8_CA(0x0f8, 0x000),		/* set booster ratio to 4x (ST7567 feature) */
  U8X8_C(0x027),		                /* set V0 voltage resistor ratio to max  */
  U8X8_CA(0x081, 0x008),		/* set contrast, contrast value */

  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x0a5),		                /* enter powersafe: all pixel on, issue 142 */

  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

static const u8x8_display_info_t u8x8_st7565_lx12864_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,

  /* post_chip_enable_wait_ns = */ 150,	/* st7565 datasheet, table 26, tcsh */
  /* pre_chip_disable_wait_ns = */ 50,	/* st7565 datasheet, table 26, tcss */
  /* reset_pulse_width_ms = */ 1,
  /* post_reset_wait_ms = */ 1,
  /* sda_setup_time_ns = */ 50,		/* st7565 datasheet, table 26, tsds */
  /* sck_pulse_width_ns = */ 120,	/* half of cycle time (100ns according to datasheet), AVR: below 70: 8 MHz, >= 70 --> 4MHz clock */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,	/* st7565 datasheet, table 24, tds8 */
  /* write_pulse_width_ns = */ 80,	/* st7565 datasheet, table 24, tcclw */
  /* tile_width = */ 16,		/* width of 16*8=128 pixel */
  /* tile_height = */ 8,
  /* default_x_offset = */ 1,	/* not sure... */
  /* flipmode_x_offset = */ 3,
  /* pixel_width = */ 128,
  /* pixel_height = */ 64
};

uint8_t u8x8_d_st7565_lx12864(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* call common procedure first and handle messages there */
  if ( u8x8_d_st7565_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case U8X8_MSG_DISPLAY_SETUP_MEMORY:
	u8x8_d_helper_display_setup_memory(u8x8, &u8x8_st7565_lx12864_display_info);
	break;
      case U8X8_MSG_DISPLAY_INIT:
	u8x8_d_helper_display_init(u8x8);
	u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_lx12864_init_seq);
	break;
      case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	if ( arg_int == 0 )
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip1_seq);
	  u8x8->x_offset = u8x8->display_info->default_x_offset;
	}
	else
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip0_seq);
	  u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	}
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}


/*================================================*/
/* ERC12864-1 (buydisplay.com) */

static const uint8_t u8x8_d_st7565_erc12864_init_seq[] = {

  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  U8X8_C(0x0e2),            			/* soft reset */
  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x040),		                /* set display start line to ... */

  U8X8_C(0x0a0),		                /* ADC set to reverse */
  U8X8_C(0x0c8),		                /* common output mode */
  //U8X8_C(0x0a1),		                /* ADC set to reverse */
  //U8X8_C(0x0c0),		                /* common output mode */
  // Flipmode
  // U8X8_C(0x0a0),		                /* ADC set to reverse */
  // U8X8_C(0x0c8),		                /* common output mode */

  U8X8_C(0x0a6),		                /* display normal, bit val 0: LCD pixel off. */
  U8X8_C(0x0a3),		                /* LCD bias 1/9 */
  U8X8_C(0x02f),		                /* all power  control circuits on (regulator, booster and follower) */
  U8X8_CA(0x0f8, 0x000),		/* set booster ratio to 4x (ST7567 feature)*/
  U8X8_C(0x027),		                /* set V0 voltage resistor ratio to max  */
  U8X8_CA(0x081, 0x018),		/* set contrast, contrast value, EA default: 0x016 */

  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x0a5),		                /* enter powersafe: all pixel on, issue 142 */

  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

static const u8x8_display_info_t u8x8_st7565_erc12864_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,

  /* post_chip_enable_wait_ns = */ 150,	/* st7565 datasheet, table 26, tcsh */
  /* pre_chip_disable_wait_ns = */ 50,	/* st7565 datasheet, table 26, tcss */
  /* reset_pulse_width_ms = */ 1,
  /* post_reset_wait_ms = */ 1,
  /* sda_setup_time_ns = */ 50,		/* st7565 datasheet, table 26, tsds */
  /* sck_pulse_width_ns = */ 120,	/* half of cycle time (100ns according to datasheet), AVR: below 70: 8 MHz, >= 70 --> 4MHz clock */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,	/* st7565 datasheet, table 24, tds8 */
  /* write_pulse_width_ns = */ 80,	/* st7565 datasheet, table 24, tcclw */
  /* tile_width = */ 16,		/* width of 16*8=128 pixel */
  /* tile_height = */ 8,
  /* default_x_offset = */ 0,
  /* flipmode_x_offset = */ 4,
  /* pixel_width = */ 128,
  /* pixel_height = */ 64
};

uint8_t u8x8_d_st7565_erc12864(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* call common procedure first and handle messages there */
  if ( u8x8_d_st7565_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case U8X8_MSG_DISPLAY_SETUP_MEMORY:
	u8x8_d_helper_display_setup_memory(u8x8, &u8x8_st7565_erc12864_display_info);
	break;
      case U8X8_MSG_DISPLAY_INIT:
	u8x8_d_helper_display_init(u8x8);
	u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_erc12864_init_seq);
	break;
      case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	if ( arg_int == 0 )
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip1_seq);
	  u8x8->x_offset = u8x8->display_info->default_x_offset;
	}
	else
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip0_seq);
	  u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	}
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}




/*================================================*/
/* ERC12864-1 alternative version, suggested in issue 790 */

static const uint8_t u8x8_d_st7565_erc12864_alt_init_seq[] = {


  // original sequence

  // U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  // U8X8_C(0x0e2),            			/* soft reset */
  // U8X8_C(0x0ae),		                /* display off */
  // U8X8_C(0x040),		                /* set display start line to ... */

  // U8X8_C(0x0a0),		                /* ADC set to reverse */
  // U8X8_C(0x0c8),		                /* common output mode */

  // U8X8_C(0x0a6),		                /* display normal, bit val 0: LCD pixel off. */
  // U8X8_C(0x0a3),		                /* LCD bias 1/9 */
  // U8X8_C(0x02f),		                /* all power  control circuits on (regulator, booster and follower) */
  // U8X8_CA(0x0f8, 0x000),		/* set booster ratio to 4x (ST7567 feature)*/
  // U8X8_C(0x027),		                /* set V0 voltage resistor ratio to max  */
  // U8X8_CA(0x081, 0x018),		/* set contrast, contrast value, EA default: 0x016 */

  // U8X8_C(0x0ae),		                /* display off */
  // U8X8_C(0x0a5),		                /* enter powersafe: all pixel on, issue 142 */

  // U8X8_END_TRANSFER(),             	/* disable chip */
  // U8X8_END()             			/* end of sequence */



  // suggested in https://github.com/olikraus/u8g2/issues/790

  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  U8X8_C(0x0e2),            			/* soft reset */
  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x040),		                /* set display start line to ... */

  U8X8_C(0x0a0),		                /* ADC set to reverse */
  U8X8_C(0x0c8),		                /* common output mode */

  U8X8_C(0x0a6),		                /* display normal, bit val 0: LCD pixel off. */
  U8X8_C(0x0a2),		                /* LCD bias 1/9 - *** Changed by Ismail - was 0xa3 - 1/7 bias we were getting dark pixel off */
  U8X8_C(0x02f),		                /* all power  control circuits on (regulator, booster and follower) */
  U8X8_CA(0x0f8, 0x000),		/* set booster ratio to 4x (ST7567 feature)*/
  U8X8_C(0x027),		                /* set V0 voltage resistor ratio to max  */
  U8X8_CA(0x081, 0x05),		       /* set contrast, contrast value, EA default: 0x016 - *** Changed by Ismail to 0x05 */

  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x0a5),		                /* enter powersafe: all pixel on, issue 142 */

  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */

};


uint8_t u8x8_d_st7565_erc12864_alt(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* call common procedure first and handle messages there */
  if ( u8x8_d_st7565_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case U8X8_MSG_DISPLAY_SETUP_MEMORY:
	u8x8_d_helper_display_setup_memory(u8x8, &u8x8_st7565_erc12864_display_info);
	break;
      case U8X8_MSG_DISPLAY_INIT:
	u8x8_d_helper_display_init(u8x8);
	u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_erc12864_alt_init_seq);
	break;
      case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	if ( arg_int == 0 )
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip1_seq);
	  u8x8->x_offset = u8x8->display_info->default_x_offset;
	}
	else
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip0_seq);
	  u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	}
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}




/*================================================*/
/* NT7534, TG12864R */
/* The NT7534 has an extended command set for the ST7565, however this is not used. */
/* The TG12864R display is also shifted in lines, like the LM6059/Adafruit display */
/* However contrast seems to be different */

static const uint8_t u8x8_d_nt7534_tg12864r_init_seq[] = {

  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  U8X8_C(0x0e2),            			/* soft reset */
  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x060),		                /* set display start line to ... */

  U8X8_C(0x0a0),		                /* ADC set to reverse */
  U8X8_C(0x0c8),		                /* common output mode */
  // Flipmode
  //U8X8_C(0x0a1),		                /* ADC set to reverse */
  //U8X8_C(0x0c0),		                /* common output mode */

  U8X8_C(0x0a6),		                /* display normal, bit val 0: LCD pixel off. */
  U8X8_C(0x0a3),		                /* LCD bias 1/9 */
  U8X8_C(0x02f),		                /* all power  control circuits on (regulator, booster and follower) */
  //U8X8_CA(0x0f8, 0x000),		/* set booster ratio to 4x (ST7567 feature)*/
  U8X8_C(0x027),		                /* set V0 voltage resistor ratio to max  */
  U8X8_CA(0x081, 0x009),		/* set contrast, contrast value, EA default: 0x016 */

  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x0a5),		                /* enter powersafe: all pixel on, issue 142 */

  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

uint8_t u8x8_d_nt7534_tg12864r(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* call common procedure first and handle messages there */
  if ( u8x8_d_st7565_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case U8X8_MSG_DISPLAY_SETUP_MEMORY:
	/* reuse the LM6059 data structure... this display seems to have similar shifts and offsets */
	u8x8_d_helper_display_setup_memory(u8x8, &u8x8_st7565_lm6059_display_info);
	break;
      case U8X8_MSG_DISPLAY_INIT:
	u8x8_d_helper_display_init(u8x8);

	//u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_lm6059_init_seq);
	u8x8_cad_SendSequence(u8x8, u8x8_d_nt7534_tg12864r_init_seq);
	break;
      case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	if ( arg_int == 0 )
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip1_seq);
	  u8x8->x_offset = u8x8->display_info->default_x_offset;
	}
	else
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip0_seq);
	  u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	}
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}


/*================================================*/
/* EA DOGM132 */

static const u8x8_display_info_t u8x8_st7565_dogm132_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,

  /* post_chip_enable_wait_ns = */ 150,	/* st7565 datasheet, table 26, tcsh */
  /* pre_chip_disable_wait_ns = */ 50,	/* st7565 datasheet, table 26, tcss */
  /* reset_pulse_width_ms = */ 1,
  /* post_reset_wait_ms = */ 1,
  /* sda_setup_time_ns = */ 50,		/* st7565 datasheet, table 26, tsds */
  /* sck_pulse_width_ns = */ 120,	/* half of cycle time (100ns according to datasheet), AVR: below 70: 8 MHz, >= 70 --> 4MHz clock */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,	/* st7565 datasheet, table 24, tds8 */
  /* write_pulse_width_ns = */ 80,	/* st7565 datasheet, table 24, tcclw */
  /* tile_width = */ 17,		/* width of 16*8=136 pixel */
  /* tile_height = */ 4,
  /* default_x_offset = */ 0,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 132,
  /* pixel_height = */ 32
};


static const uint8_t u8x8_d_st7565_dogm132_init_seq[] = {

  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */

  U8X8_C(0x0e2),            			/* soft reset */
  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x040),		                /* set display start line to 0 */

  U8X8_C(0x0a1),		                /* ADC set to reverse */
  U8X8_C(0x0c0),		                /* common output mode */
  // Flipmode
  //U8X8_C(0x0a0),		                /* ADC set to reverse */
  //U8X8_C(0x0c8),		                /* common output mode */

  U8X8_C(0x0a6),		                /* display normal, bit val 0: LCD pixel off. */
  U8X8_C(0x0a2),		                /* LCD bias 1/9 */
  U8X8_C(0x02f),		                /* all power  control circuits on */
  U8X8_CA(0x0f8, 0x000),		/* set booster ratio to 4x */
  U8X8_C(0x023),		                /* set V0 voltage resistor ratio to large*/
  U8X8_CA(0x081, 0x01f),		/* set contrast, contrast value EA DOGM132 */

  U8X8_C(0x0ae),		                /* display off */
  U8X8_C(0x0a5),		                /* enter powersafe: all pixel on, issue 142 */

  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

uint8_t u8x8_d_st7565_ea_dogm132(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* call common procedure first and handle messages there */
  if ( u8x8_d_st7565_common(u8x8, msg, arg_int, arg_ptr) == 0 )
  {
    /* msg not handled, then try here */
    switch(msg)
    {
      case U8X8_MSG_DISPLAY_SETUP_MEMORY:
	u8x8_d_helper_display_setup_memory(u8x8, &u8x8_st7565_dogm132_display_info);
	break;
      case U8X8_MSG_DISPLAY_INIT:
	u8x8_d_helper_display_init(u8x8);
	u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_dogm132_init_seq);
	break;
      case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
	if ( arg_int == 0 )
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip0_seq);
	  u8x8->x_offset = u8x8->display_info->default_x_offset;
	}
	else
	{
	  u8x8_cad_SendSequence(u8x8, u8x8_d_st7565_flip1_seq);
	  u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
	}
	break;
      default:
	return 0;		/* msg unknown */
    }
  }
  return 1;
}
