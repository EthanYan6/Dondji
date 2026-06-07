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

#ifndef UI_MENU_H
#define UI_MENU_H

#include <stdbool.h>
#include <stdint.h>

#include "audio.h"     // VOICE_ID_t
#include "settings.h"

typedef struct {
    const char  name[8];    // up to 7 chars + '\0' (was [7] = max 6 + NUL; longer strings leaked into next item)
    uint8_t     menu_id;
} t_menu_item;

enum
{
    MENU_SQL = 0,
    MENU_STEP,
    MENU_TXP,
    MENU_R_DCS,
    MENU_R_CTCS,
    MENU_T_DCS,
    MENU_T_CTCS,
    MENU_SFT_D,
    MENU_OFFSET,
    MENU_TOT,
    MENU_W_N,
#ifndef ENABLE_FEAT_F4HWN
    MENU_SCR,
#endif
    MENU_BCL,
#ifdef ENABLE_FEAT_F4HWN
    MENU_TX_LOCK, 
#endif
    MENU_MEM_CH,
    MENU_DEL_CH,
    MENU_MEM_NAME,
    MENU_CN_MEM_NAME,
    MENU_MDF,
    MENU_SAVE,
    MENU_VOX,
    MENU_ABR,
    MENU_ABR_ON_TX_RX,
    MENU_ABR_MIN,
    MENU_ABR_MAX,
    MENU_TDR,
    MENU_BEEP,
#ifdef ENABLE_VOICE
    MENU_VOICE,
#endif
    MENU_SC_REV,
    MENU_AUTOLK,
    MENU_LIST_CH,
    MENU_STE,
    MENU_RP_STE,
    MENU_MIC,
    MENU_MIC_BAR,
    MENU_COMPAND,
    MENU_1_CALL,
    MENU_S_LIST,
    MENU_S_PRI,
    MENU_S_PRI_CH_1,
    MENU_S_PRI_CH_2,    
#ifdef ENABLE_ALARM
    MENU_AL_MOD,
#endif
#ifdef ENABLE_DTMF_CALLING
    MENU_ANI_ID,
#endif
    MENU_UPCODE,
    MENU_DWCODE,
    MENU_PTT_ID,
    MENU_D_ST,
#ifdef ENABLE_DTMF_CALLING
    MENU_D_RSP,
    MENU_D_HOLD,
#endif
    MENU_D_PRE,
#ifdef ENABLE_DTMF_CALLING  
    MENU_D_DCD,
    MENU_D_LIST,
#endif
    MENU_D_LIVE_DEC,
    MENU_PONMSG,
    MENU_BOOT_HINT,
    MENU_BOOT_SOUND,
    MENU_ROGER,
    MENU_VOL,
    MENU_BAT_TXT,
    MENU_AM,
#ifdef ENABLE_AM_FIX
    MENU_AM_FIX,
#endif
#ifndef ENABLE_FEAT_F4HWN
    #ifdef ENABLE_NOAA
        MENU_NOAA_S,
    #endif
#endif
    MENU_RESET,
    MENU_F_LOCK,
#ifndef ENABLE_FEAT_F4HWN
    MENU_200TX,
    MENU_350TX,
    MENU_500TX,
#endif
    MENU_350EN,
#ifndef ENABLE_FEAT_F4HWN
    MENU_SCREN,
#endif
#ifdef ENABLE_F_CAL_MENU
    MENU_F_CALI,  // reference xtal calibration
#endif
#ifdef ENABLE_FEAT_F4HWN_SLEEP
    MENU_SET_OFF,
#endif
#ifdef ENABLE_FEAT_F4HWN
    MENU_SET_PWR,
    MENU_SET_PTT,
    MENU_SET_TOT,
    MENU_SET_EOT,
#ifdef ENABLE_FEAT_F4HWN_CTR
    MENU_SET_CTR,
#endif
    MENU_SET_INV,
    MENU_SET_LCK,
    MENU_SET_MET,
    MENU_SET_GUI,
    MENU_SET_TMR,
    #ifdef ENABLE_FEAT_F4HWN_NARROWER
        MENU_SET_NFM,
    #endif
    #ifdef ENABLE_FEAT_F4HWN_VOL
        MENU_SET_VOL,
    #endif
    #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
        MENU_SET_KEY,
    #endif
    #ifdef ENABLE_NOAA
        MENU_NOAA_S,
    #endif
    MENU_SET_NAV,
    #ifdef ENABLE_FEAT_F4HWN_AUDIO
        MENU_SET_AUD,
    #endif
#endif
    MENU_BATCAL,  // battery voltage calibration
    MENU_F1SHRT,
    MENU_F1LONG,
    MENU_F2SHRT,
    MENU_F2LONG,
    MENU_MLONG,
    MENU_BATTYP,
    MENU_MDC_ID,
    MENU_LANGUAGE
};

extern const uint8_t FIRST_HIDDEN_MENU_ITEM;
extern const t_menu_item MenuList[];

extern const char        gSubMenu_TXP[8][6];
extern const char        gSubMenu_SFT_D[3][4];
extern const char        gSubMenu_W_N[2][7];
extern const char        gSubMenu_OFF_ON[2][4];
#ifdef ENABLE_AUDIO_BAR
extern const char        gSubMenu_MIC_BAR_STYLE[3][10];
#endif
extern const char        gSubMenu_NA[4];
extern const char        gSubMenu_TOT[11][7];
extern const char* const gSubMenu_RXMode[4];

#ifdef ENABLE_VOICE
    extern const char    gSubMenu_VOICE[3][4];
#endif
extern const char* const gSubMenu_MDF[4];
#ifdef ENABLE_ALARM
    extern const char    gSubMenu_AL_MOD[2][5];
#endif
#ifdef ENABLE_DTMF_CALLING
extern const char        gSubMenu_D_RSP[4][11];
#endif

#ifdef ENABLE_FEAT_F4HWN
    extern const char    gSubMenu_SET_PWR[7][6];
    extern const char    gSubMenu_SET_PTT[2][8];
    extern const char    gSubMenu_SET_TOT[4][7];
    extern const char    gSubMenu_SET_LCK[2][9];
    extern const char    gSubMenu_SET_MET[2][8];
    #ifdef ENABLE_FEAT_F4HWN_NARROWER
        extern const char    gSubMenu_SET_NFM[2][9];
    #endif
    #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
        extern const char gSubMenu_SET_KEY[][9];
    #endif
    #ifdef ENABLE_FEAT_F4HWN_AUDIO
        extern const char    gSubMenu_SET_AUD_FM[5][6];
        extern const char    gSubMenu_SET_AUD_AM[3][6];
    #endif
#endif

extern const char* const gSubMenu_PTT_ID[5];
extern const char        gSubMenu_PONMSG[3][8];
extern const char        gSubMenu_ROGER[3][6];
extern const char        gSubMenu_RESET[2][4];
extern const char* const gSubMenu_F_LOCK[F_LOCK_LEN];
extern const char        gSubMenu_RX_TX[4][6];
extern const char        gSubMenu_BAT_TXT[3][8];
extern const char        gSubMenu_LANGUAGE[2][8];
extern const char        gSubMenu_BOOT_HINT[3][14];
extern const char        gSubMenu_BATTYP[5][12];

#ifndef ENABLE_FEAT_F4HWN
    extern const char        gSubMenu_SCRAMBLER[11][7];
#endif

typedef struct {char* name; uint8_t id;} t_sidefunction;
extern const uint8_t         gSubMenu_SIDEFUNCTIONS_size;
extern const t_sidefunction gSubMenu_SIDEFUNCTIONS[];
                         
extern bool              gIsInSubMenu;
                         
extern uint8_t           gMenuCursor;

extern int32_t           gSubMenuSelection;
                         
extern char              edit_original[24];
extern char              edit[24];
extern int               edit_index;

enum {
    MEM_NAME_INPUT_LOWER = 0,
    MEM_NAME_INPUT_UPPER,
    MEM_NAME_INPUT_DIGIT,
    MEM_NAME_INPUT_SYMBOL,
    MEM_NAME_INPUT_PINYIN
};
extern uint8_t           gMemNameInputMode;
extern uint8_t           gMemNameCandidateCount;
extern char              gMemNameCandidates[6];
extern uint8_t           gMemNameSymbolPage;
extern const char        gMemNameSymbolCharset[];
extern const uint8_t     gMemNameSymbolCharsetCount;

#ifdef ENABLE_CHINESE
// Pinyin input state for CN channel name
#define PINYIN_MAX_LEN      8
#define CN_CANDIDATE_MAX    6
extern char              gPinyinBuffer[PINYIN_MAX_LEN + 1];
extern uint8_t           gPinyinLen;
extern uint8_t           gPinyinKeyIndex[PINYIN_MAX_LEN]; // which key was pressed
extern uint16_t          gCNCandidates[CN_CANDIDATE_MAX];
extern uint8_t           gCNCandidateCount;
extern uint8_t           gCNCandidateOffset;
extern uint8_t           gCNCandidateTotal;
extern uint8_t           gPinyinTimeout_500ms; // multi-tap timeout counter
extern uint8_t           gPinyinLookupNoMatch; // last MENU lookup returned 0 Hanzi
#endif

void UI_DisplayMenu(void);
const char *UI_MENU_GetMenuTitle(const t_menu_item *item);
int UI_MENU_GetCurrentMenuId();
uint8_t UI_MENU_GetMenuIdx(uint8_t id);

#endif
