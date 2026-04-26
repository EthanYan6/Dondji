/* UTF-8 Chinese strings for submenu values (ccc-style). */

#include "misc.h"
#include "radio.h"
#include "ui/menu_sub_values_cn.h"

#ifdef ENABLE_CHINESE

const char gSubMenu_OFF_ON_CN[][4] = {
    "\xe5\x85\xb3",     /* 关 */
    "\xe5\xbc\x80",     /* 开 */
};

const char gSubMenu_SFT_D_CN[][4] = {
    "\xe6\x97\xa0",     /* 无 */
    "+",
    "-",
};

const char gSubMenu_W_N_CN[][7] = {
    "\xe5\xae\xbd\xe5\xb8\xa6", /* 宽带 */
    "\xe7\xaa\x84\xe5\xb8\xa6", /* 窄带 */
};

const char *const gSubMenu_RXMode_CN[] = {
    "\xe4\xbb\x85\xe4\xb8\xbb\xe4\xbf\xa1\xe9\x81\x93",                 /* 仅主信道 */
    "\xe5\x8f\x8c\xe9\xa2\x91\xe5\xae\x88\xe5\x80\x99",                 /* 双频守候 */
    "\xe8\xb7\xa8\xe6\xae\xb5",                                         /* 跨段 */
    "\xe4\xb8\xbb\xe5\x8f\x91\xe5\x8f\x8c\xe6\x94\xb6",                 /* 主发双收 */
};

const char *const gSubMenu_MDF_CN[] = {
    "\xe9\xa2\x91\xe7\x8e\x87",                                         /* 频率 */
    "\xe4\xbf\xa1\xe9\x81\x93\xe5\x8f\xb7",                             /* 信道号 */
    "\xe5\x90\x8d\xe7\xa7\xb0",                                         /* 名称 */
    "\xe5\x90\x8d\xe7\xa7\xb0+\xe9\xa2\x91\xe7\x8e\x87",                 /* 名称+频率 */
};

#ifdef ENABLE_DTMF_CALLING
const char gSubMenu_D_RSP_CN[][11] = {
    "\xe6\x97\xa0\xe5\x8a\xa8\xe4\xbd\x9c",                             /* 无动作 */
    "\xe5\x93\x8d\xe9\x93\x83",                                         /* 响铃 */
    "\xe5\x9b\x9e\xe5\xa4\x8d",                                         /* 回复 */
    "\xe5\x8f\x8c\xe7\xab\xaf",                                         /* 双端 */
};
#endif

const char *const gSubMenu_PTT_ID_CN[] = {
    "\xe5\x85\xb3",
    "\xe4\xb8\x8a\xe8\xa1\x8c\xe7\xa0\x81",
    "\xe4\xb8\x8b\xe8\xa1\x8c\xe7\xa0\x81",
    "\xe4\xb8\x8a+\xe4\xb8\x8b\xe8\xa1\x8c\xe7\xa0\x81",
    "APOLLO\nQUINDAR",
};

#ifdef ENABLE_FEAT_F4HWN
const char gSubMenu_PONMSG_CN[][8] = {
    "\xe5\x85\xa8\xe9\x83\xa8",
    "\xe5\xa3\xb0\xe9\x9f\xb3",
    "\xe6\xb6\x88\xe6\x81\xaf",
    "\xe7\x94\xb5\xe5\x8e\x8b",
    "\xe6\x97\xa0",
};

const char *const gSubMenu_BOOT_HINT_CN[] = {
    "\xe5\x8f\xae\xe5\x92\x9a\xe9\xb8\xa1",
    "\xe9\xad\x85\xe5\x8a\x9b\xe5\x8c\x97\xe4\xba\xac",
    "\xe4\xba\x94\xe4\xba\x94\xe8\x8a\x82\xe7\xba\xaa\xe5\xbf\xb5\xe7\x89\x88",
};
#else
const char gSubMenu_PONMSG_CN[][8] = {
    "\xe5\x85\xa8\xe9\x83\xa8",
    "\xe6\xb6\x88\xe6\x81\xaf",
    "\xe7\x94\xb5\xe5\x8e\x8b",
    "\xe6\x97\xa0",
};

const char *const gSubMenu_BOOT_HINT_CN[] = {
    "\xe5\x8f\xae\xe5\x92\x9a\xe9\xb8\xa1",
    "\xe9\xad\x85\xe5\x8a\x9b\xe5\x8c\x97\xe4\xba\xac",
    "\xe4\xba\x94\xe4\xba\x94\xe8\x8a\x82\xe7\xba\xaa\xe5\xbf\xb5\xe7\x89\x88",
};
#endif

const char gSubMenu_ROGER_CN[][6] = {
    "\xe5\x85\xb3",
    "ROGER",
    "MDC",
};

const char gSubMenu_RESET_CN[][8] = {
    "VFO",
    "\xe5\x85\xa8\xe9\x83\xa8",
};

const char *const gSubMenu_F_LOCK_CN[] = {
    "\xe9\xbb\x98\xe8\xae\xa4\n137-174\n400-470",
    "FCC\xe4\xb8\x9a\xe4\xbd\x99\n144-148\n420-450",
#ifdef ENABLE_FEAT_F4HWN_CA
    "CA\xe4\xb8\x9a\xe4\xbd\x99\n144-148\n430-450",
#endif
    "CE\xe4\xb8\x9a\xe4\xbd\x99\n144-146\n430-440",
    "GB\xe4\xb8\x9a\xe4\xbd\x99\n144-148\n430-440",
    "137-174\n400-430",
    "137-174\n400-438",
#ifdef ENABLE_FEAT_F4HWN_PMR
    "PMR 446",
#endif
#ifdef ENABLE_FEAT_F4HWN_GMRS_FRS_MURS
    "GMRS\nFRS\nMURS",
#endif
    "\xe5\x85\xa8\xe9\x83\xa8\xe7\xa6\x81\xe7\x94\xa8",
    "\xe5\x85\xa8\xe9\x83\xa8\xe8\xa7\xa3\xe9\x94\x81",
};

const char gSubMenu_RX_TX_CN[][16] = {
    "\xe5\x85\xb3",
    "\xe5\x8f\x91\xe5\xb0\x84",
    "\xe6\x8e\xa5\xe6\x94\xb6",
    "\xe6\x8e\xa5\xe6\x94\xb6/\xe5\x8f\x91\xe5\xb0\x84",
};

const char gSubMenu_BAT_TXT_CN[][10] = {
    "\xe6\x97\xa0",
    "\xe7\x94\xb5\xe5\x8e\x8b",
    "\xe7\x99\xbe\xe5\x88\x86\xe6\xaf\x94", /* 9 UTF-8 bytes — 行宽须 ≥10 含 '\\0' */
};

const char gSubMenu_SET_NAV_CN[][17] = {
    "\xe5\xb7\xa6\n\xe5\x8f\xb3\nUV-K1",
    "\xe4\xb8\x8a\n\xe4\xb8\x8b\nUV-K5(8)",
};

#ifdef ENABLE_VOICE
const char gSubMenu_VOICE_CN[][4] = {
    "\xe5\x85\xb3",
    "\xe4\xb8\xad",
    "\xe8\x8b\xb1",
};
#endif

#ifdef ENABLE_ALARM
const char gSubMenu_AL_MOD_CN[][5] = {
    "\xe7\x8e\xb0\xe5\x9c\xba",
    "\xe9\x9f\xb3\xe8\xb0\x83",
};
#endif

#ifndef ENABLE_FEAT_F4HWN
const char gSubMenu_SCRAMBLER_CN[][7] = {
    "\xe5\x85\xb3",
    "2600Hz",
    "2700Hz",
    "2800Hz",
    "2900Hz",
    "3000Hz",
    "3100Hz",
    "3200Hz",
    "3300Hz",
    "3400Hz",
    "3500Hz",
};
#endif

const char gSubMenu_TXP_CN[][6] = {
    "\xe7\x94\xa8\xe6\x88\xb7",
    "LOW 1",
    "LOW 2",
    "LOW 3",
    "LOW 4",
    "LOW 5",
    "MID",
    "HIGH",
};

#ifdef ENABLE_FEAT_F4HWN
const char gSubMenu_SET_PWR_CN[][6] = {
    "< 20m",
    "125m",
    "250m",
    "500m",
    "1",
    "2",
    "5",
};

const char gSubMenu_SET_PTT_CN[][8] = {
    "\xe7\xbb\x8f\xe5\x85\xb8",
    "\xe4\xb8\x80\xe9\x94\xae",
};

const char gSubMenu_SET_TOT_CN[][7] = {
    "\xe5\x85\xb3",
    "\xe5\xa3\xb0\xe9\x9f\xb3",
    "\xe8\xa7\x86\xe8\xa7\x89",
    "\xe5\x85\xa8\xe9\x83\xa8",
};

const char gSubMenu_SET_LCK_CN[][12] = {
    "\xe4\xbb\x85\xe6\x8c\x89\xe9\x94\xae",
    "\xe6\x8c\x89\xe9\x94\xae+PTT",
};

const char gSubMenu_SET_MET_CN[][8] = {
    "\xe7\xb2\xbe\xe7\xae\x80",
    "\xe7\xbb\x8f\xe5\x85\xb8",
};

# ifdef ENABLE_FEAT_F4HWN_AUDIO
const char gSubMenu_SET_AUD_FM_CN[][6] = {
    "FLAT",
    "CLEAN",
    "MID",
    "BOOST",
    "MAX",
};

const char gSubMenu_SET_AUD_AM_CN[][6] = {
    "SHARP",
    "STOCK",
    "OPEN",
};
# endif

# ifdef ENABLE_FEAT_F4HWN_NARROWER
const char gSubMenu_SET_NFM_CN[][16] = {
    "\xe7\xaa\x84\xe5\xb8\xa6",
    "\xe6\x9b\xb4\xe7\xaa\x84",
};
# endif

# ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
const char gSubMenu_SET_KEY_CN[][16] = {
    "MENU\xe9\x94\xae",
    "\xe4\xb8\x8a\xe9\x94\xae",
    "\xe4\xb8\x8b\xe9\x94\xae",
    "EXIT\xe9\x94\xae",
    "*\xe9\x94\xae",
};
# endif
#endif /* ENABLE_FEAT_F4HWN */

const char *const gSubMenu_SIDEFUNCTIONS_CN[] = {
    "\xe6\x97\xa0",
#ifdef ENABLE_FLASHLIGHT
    "\xe6\x89\x8b\xe7\x94\xb5",
#endif
    "\xe5\x8a\x9f\xe7\x8e\x87",
    "\xe7\x9b\x91\xe5\x90\xac",
    "\xe6\x89\xab\xe6\x8f\x8f",
#ifdef ENABLE_VOX
    "\xe5\xa3\xb0\xe6\x8e\xa7",
#endif
#ifdef ENABLE_ALARM
    "\xe6\x8a\xa5\xe8\xad\xa6",
#endif
#ifdef ENABLE_FMRADIO
    "\xe6\x94\xb6\xe9\x9f\xb3\xe6\x9c\xba",
#endif
#ifdef ENABLE_TX1750
    "1750Hz",
#endif
#ifdef ENABLE_REGA
    "REGA\n\xe6\x8a\xa5\xe8\xad\xa6",
    "REGA\n\xe6\xb5\x8b\xe8\xaf\x95",
#endif
    "\xe9\x94\x81\xe9\x94\xae\xe7\x9b\x98",
    "VFO A\nVFO B",
    "VFO\n\xe4\xbf\xa1\xe9\x81\x93",
    "\xe8\xb0\x83\xe5\x88\xb6",
#ifdef ENABLE_BLMIN_TMP_OFF
    "\xe8\x83\x8c\xe5\x85\x89\xe4\xb8\xb4\xe6\x97\xb6\xe5\x85\xb3",
#endif
#ifdef ENABLE_FEAT_F4HWN
    "\xe6\x8e\xa5\xe6\x94\xb6\xe6\xa8\xa1\xe5\xbc\x8f",
    "\xe4\xbb\x85\xe4\xb8\xbb\xe4\xbf\xa1\xe9\x81\x93",
    "PTT",
    "\xe5\xae\xbd\n\xe7\xaa\x84",
    "\xe9\x9d\x99\xe9\x9f\xb3",
# ifdef ENABLE_FEAT_F4HWN_AUDIO
    "RxA",
# endif
# ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
    "\xe5\x8a\x9f\xe7\x8e\x87\n\xe9\xab\x98",
    "\xe7\xa7\xbb\xe9\x99\xa4\n\xe9\xa2\x91\xe5\xb7\xae",
# endif
#endif
};

/* 内、计、划 不在嵌入中文字库；第一行用 Inside，第三行用「频段」（字库有 频、段）。 */
const char gSubMenu_TX_LOCK_INSIDE_CN[] =
    "Inside\nF Lock\n\xe9\xa2\x91\xe6\xae\xb5";

const char gSubMenu_LIST_CN_OFF[] = "\xe5\x85\xb3";
const char gSubMenu_LIST_CN_ALL[] = "\xe5\x85\xa8\xe9\x83\xa8";
const char gSubMenu_SC_REV_STOP_CN[] = "\xe5\x81\x9c\xe6\xad\xa2";
const char gSubMenu_ABR_ON_CN[] = "\xe5\xbc\x80";
const char gSubMenu_MEM_NONE_CN[] = "\xe6\x97\xa0";

/* Same order as ModulationMode_t / gModulationStr (size MODULATION_UKNOWN). */
const char gSubMenu_MODULATION_CN[][8] = {
    "FM",
    "AM",
    "USB",
#ifdef ENABLE_BYP_RAW_DEMODULATORS
    "BYP",
    "RAW",
#endif
};

_Static_assert(ARRAY_SIZE(gSubMenu_MODULATION_CN) == MODULATION_UKNOWN, "gSubMenu_MODULATION_CN");

#endif /* ENABLE_CHINESE */
