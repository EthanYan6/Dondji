/* UTF-8 submenu value strings when UI language is Chinese (aligned with ccc). */
#ifndef MENU_SUB_VALUES_CN_H
#define MENU_SUB_VALUES_CN_H

#include "misc.h"

#ifdef ENABLE_CHINESE

extern const char gSubMenu_OFF_ON_CN[][4];
extern const char gSubMenu_SFT_D_CN[][4];
extern const char gSubMenu_W_N_CN[][7];
extern const char *const gSubMenu_RXMode_CN[];
extern const char *const gSubMenu_MDF_CN[];

#ifdef ENABLE_DTMF_CALLING
extern const char gSubMenu_D_RSP_CN[][11];
#endif

extern const char *const gSubMenu_PTT_ID_CN[];
extern const char gSubMenu_PONMSG_CN[][8];
extern const char *const gSubMenu_BOOT_HINT_CN[];
extern const char gSubMenu_ROGER_CN[][6];
extern const char gSubMenu_RESET_CN[][8];
extern const char *const gSubMenu_F_LOCK_CN[];
extern const char gSubMenu_RX_TX_CN[][16];
extern const char gSubMenu_BAT_TXT_CN[][10]; /* 「百分比」UTF-8 共 9 字节 + '\\0'，[9] 会截断导致串连相邻只读数据 */
extern const char gSubMenu_SET_NAV_CN[][17];

#ifdef ENABLE_VOICE
extern const char gSubMenu_VOICE_CN[][4];
#endif

#ifdef ENABLE_ALARM
extern const char gSubMenu_AL_MOD_CN[][5];
#endif

#ifndef ENABLE_FEAT_F4HWN
extern const char gSubMenu_SCRAMBLER_CN[][7];
#endif

extern const char gSubMenu_TXP_CN[][6];

#ifdef ENABLE_FEAT_F4HWN
extern const char gSubMenu_SET_PWR_CN[][6];
extern const char gSubMenu_SET_PTT_CN[][8];
extern const char gSubMenu_SET_TOT_CN[][7];
extern const char gSubMenu_SET_LCK_CN[][12];
extern const char gSubMenu_SET_MET_CN[][8];
# ifdef ENABLE_FEAT_F4HWN_AUDIO
extern const char gSubMenu_SET_AUD_FM_CN[][6];
extern const char gSubMenu_SET_AUD_AM_CN[][6];
# endif
# ifdef ENABLE_FEAT_F4HWN_NARROWER
extern const char gSubMenu_SET_NFM_CN[][16];
# endif
# ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
extern const char gSubMenu_SET_KEY_CN[][16];
# endif
#endif

extern const char *const gSubMenu_SIDEFUNCTIONS_CN[];

extern const char gSubMenu_TX_LOCK_INSIDE_CN[];
extern const char gSubMenu_LIST_CN_OFF[];
extern const char gSubMenu_LIST_CN_ALL[];
extern const char gSubMenu_SC_REV_STOP_CN[];
extern const char gSubMenu_ABR_ON_CN[];
extern const char gSubMenu_MEM_NONE_CN[];

extern const char gSubMenu_MODULATION_CN[][8];

#endif /* ENABLE_CHINESE */

#endif
