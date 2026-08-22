
#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

extern const uint8_t gFontPowerSave[2][6];
extern const uint8_t gFontPttOnePush[2][6];
extern const uint8_t gFontPttClassic[2][6];
extern const uint8_t gFontF[8];
extern const uint8_t gFontS[6];

extern const uint8_t gFontKeyLock[9];
extern const uint8_t gFontLight[9];
extern const uint8_t gFontMute[12];

extern const uint8_t gFontXB[2][6];
extern const uint8_t gFontMO[2][6];
extern const uint8_t gFontDWR[3][6];
#ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
    extern const uint8_t gFontRO[2][6];
#endif
extern const uint8_t gFontHold[2][5];

extern const uint8_t BITMAP_USB_C[9];
extern const uint8_t BITMAP_Antenna[6];

/*
extern const uint8_t BITMAP_Ready[7];
extern const uint8_t BITMAP_NotReady[7];
*/

#ifdef ENABLE_VOX
    extern const uint8_t gFontVox[2][6];
#endif

extern const uint8_t BITMAP_VFO_Default[7];
extern const uint8_t BITMAP_VFO_NotDefault[7];
extern const uint8_t BITMAP_VFO_Empty[7];
extern const uint8_t BITMAP_VFO_Lock[7];
extern const uint8_t BITMAP_PowerUser[3];
extern const uint8_t BITMAP_compand[6];

extern const uint8_t BITMAP_NOAA[12];

/* Yan ID popup phone icon — page-major 16×16 (2 pages × 16 cols). */
#define BITMAP_CALL_PHONE_WIDTH  16
#define BITMAP_CALL_PHONE_HEIGHT 16
#define BITMAP_CALL_PHONE_PAGES  (BITMAP_CALL_PHONE_HEIGHT / 8)
extern const uint8_t BITMAP_CALL_PHONE[BITMAP_CALL_PHONE_PAGES * BITMAP_CALL_PHONE_WIDTH];

#ifndef ENABLE_CUSTOM_MENU_LAYOUT
    extern const uint8_t BITMAP_CurrentIndicator[8];
#endif

#endif
