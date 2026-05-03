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

#if !defined(ENABLE_OVERLAY)
    #include "py32f0xx.h"
#endif
#include "app/dtmf.h"
#include "app/generic.h"
#include "app/menu.h"
#include "app/scanner.h"
#include "audio.h"
#include "board.h"
#include "driver/backlight.h"
#include "driver/bk4819.h"
#include "driver/eeprom.h"
#include "driver/gpio.h"
#include "driver/keyboard.h"
#ifdef ENABLE_CHINESE
#include "driver/py25q16.h"
#endif
#include "frequencies.h"
#include "helper/battery.h"
#include "misc.h"
#include "settings.h"
#include "../driver/st7565.h"
#if defined(ENABLE_OVERLAY)
    #include "sram-overlay.h"
#endif
#include "ui/inputbox.h"
#include "ui/menu.h"
#include "ui/ui.h"


uint8_t gUnlockAllTxConfCnt;
bool gMenuMainPageActive;
bool gMenuUseMainOnlyStatus;
uint8_t gMenuMainPageIconIndex;
static uint8_t gMenuFilteredCount;
static uint8_t gMenuFilteredMap[200];
static bool gMenuMainPageLastValid;
static uint8_t gMenuMainPageLastIconIndex;
static bool gMenuSecondPageLastValid[8];
static uint8_t gMenuSecondPageLastMenuId[8];
uint8_t gMemNameInputMode = MEM_NAME_INPUT_LOWER;
uint8_t gMemNameCandidateCount;
char gMemNameCandidates[5];
uint8_t gMemNameSymbolPage;

#ifdef ENABLE_CHINESE
char     gPinyinBuffer[PINYIN_MAX_LEN + 1];
uint8_t  gPinyinLen;
uint8_t  gPinyinKeyIndex[PINYIN_MAX_LEN];
uint16_t gCNCandidates[CN_CANDIDATE_MAX];
uint8_t  gCNCandidateCount;
uint8_t  gCNCandidateOffset;
uint8_t  gCNCandidateTotal;
uint8_t  gPinyinTimeout_500ms;
uint8_t  gPinyinLookupNoMatch;

static void MENU_PinyinSearch(void)
{
    if (gPinyinLen == 0)
    {
        gCNCandidateCount = 0;
        gCNCandidateTotal = 0;
        gCNCandidateOffset = 0;
        gPinyinLookupNoMatch = 0;
        return;
    }
    gMemNameCandidateCount = 0;
    {
        const int raw_total = SETTINGS_CNGetPinyinCandidates(
            gPinyinBuffer, gCNCandidates, CN_CANDIDATE_MAX, gCNCandidateOffset);
        gCNCandidateTotal = (uint8_t)(raw_total < 0 ? 0 : raw_total);
        gPinyinLookupNoMatch = (raw_total <= 0) ? 1u : 0u;
    }
    gCNCandidateCount = gCNCandidateTotal - gCNCandidateOffset;
    if (gCNCandidateCount > CN_CANDIDATE_MAX)
        gCNCandidateCount = CN_CANDIDATE_MAX;
}

static void MENU_PinyinReset(void)
{
    gPinyinLen = 0;
    gPinyinBuffer[0] = 0;
    gCNCandidateCount = 0;
    gCNCandidateOffset = 0;
    gCNCandidateTotal = 0;
    gMemNameCandidateCount = 0;
    gPinyinLookupNoMatch = 0;
    gPinyinTimeout_500ms = 0;
    memset(gPinyinKeyIndex, 0, sizeof(gPinyinKeyIndex));
}

#endif

static const char *MENU_GetMemNameLettersByKey(const KEY_Code_t key)
{
    static const char *const map[] = {
        "abc", "def", "ghi", "jkl", "mno", "pqrs", "tuv", "wxyz"
    };
    if (key < KEY_2 || key > KEY_9)
        return NULL;
    return map[key - KEY_2];
}

static void MENU_BuildMemNameCandidatesFromKey(const KEY_Code_t key)
{
    const char *letters = MENU_GetMemNameLettersByKey(key);
    gMemNameCandidateCount = 0;
    if (letters == NULL)
        return;

    while (letters[gMemNameCandidateCount] != 0 && gMemNameCandidateCount < ARRAY_SIZE(gMemNameCandidates))
    {
        const char c = letters[gMemNameCandidateCount];
        gMemNameCandidates[gMemNameCandidateCount] =
            (gMemNameInputMode == MEM_NAME_INPUT_UPPER) ? (char)(c - ('a' - 'A')) : c;
        gMemNameCandidateCount++;
    }
}

static void MENU_BuildMemNameSymbolCandidates(void)
{
    static const char symbols[] = ".,!?@#$%&*+-/=_:;()[]{}<>\"'\\|~^`";
    const uint8_t per_page = ARRAY_SIZE(gMemNameCandidates);
    const uint8_t total = (uint8_t)(sizeof(symbols) - 1u);
    const uint8_t pages = (uint8_t)((total + per_page - 1u) / per_page);
    const uint8_t base = (uint8_t)(gMemNameSymbolPage * per_page);
    gMemNameCandidateCount = 0;

    if (pages == 0u)
        return;
    if (gMemNameSymbolPage >= pages)
        gMemNameSymbolPage = 0u;

    for (uint8_t i = 0; i < per_page; i++)
    {
        const uint8_t idx = (uint8_t)(base + i);
        if (idx >= total)
            break;
        gMemNameCandidates[gMemNameCandidateCount++] = symbols[idx];
    }
}

static void MENU_MemNameAdvanceAfterInput(void)
{
    gMemNameCandidateCount = 0;

    if (++edit_index < 10)
        return;

    gFlagAcceptSetting  = false;
    gAskForConfirmation = 0;
    if (memcmp(edit_original, edit, sizeof(edit_original)) == 0)
        gIsInSubMenu = false;
}

/** Icon-mode menu list: hide all DTMF-related items (they fall under Settings and Other by index). */
static bool MENU_IsHiddenDtmfMenu(uint8_t menu_id)
{
    if (menu_id == MENU_D_ST || menu_id == MENU_D_PRE || menu_id == MENU_D_LIVE_DEC)
        return true;
#ifdef ENABLE_DTMF_CALLING
    if (menu_id == MENU_ANI_ID ||
        menu_id == MENU_D_RSP || menu_id == MENU_D_HOLD ||
        menu_id == MENU_D_DCD || menu_id == MENU_D_LIST)
        return true;
#endif
    return false;
}

bool MENU_IsMenuIdExcludedFromBrowse(uint8_t menu_id)
{
#ifdef ENABLE_AUDIO_BAR
    if (menu_id == MENU_MIC_BAR &&
        (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF || gEeprom.CROSS_BAND_RX_TX != CROSS_BAND_OFF))
        return true;
#endif
#if !defined(ENABLE_FEAT_F4HWN)
    if (menu_id == MENU_BAT_TXT)
        return true;
#endif
#ifdef ENABLE_CHINESE
    if (menu_id == MENU_CN_MEM_NAME && gUiLanguage != UI_LANGUAGE_CN)
        return true;
#endif
    return menu_id == MENU_PONMSG ||
           menu_id == MENU_UPCODE ||
           menu_id == MENU_DWCODE ||
           menu_id == MENU_PTT_ID ||
           menu_id == MENU_VOX ||
           menu_id == MENU_SET_INV ||
           menu_id == MENU_SET_PTT ||
           menu_id == MENU_SET_TOT ||
           menu_id == MENU_SET_EOT;
}

uint8_t MENU_GetVisibleCursorForActualIndex(uint8_t actual_menu_list_index)
{
    uint8_t vis = 0;
    for (uint8_t i = 0; i < actual_menu_list_index && i < gMenuListCount; i++)
        if (!MENU_IsMenuIdExcludedFromBrowse(MenuList[i].menu_id))
            vis++;
    return vis;
}

static bool MENU_IsMenuInIconGroup(uint8_t menu_number_1based, uint8_t menu_id, uint8_t icon_index)
{
    const bool is_channel_display_menu = (menu_id == MENU_MDF);

    if (is_channel_display_menu)
        return false;

    if (MENU_IsMenuIdExcludedFromBrowse(menu_id))
        return false;
    if (MENU_IsHiddenDtmfMenu(menu_id))
        return false;

#ifdef ENABLE_AUDIO_BAR
    /* 发射条 / Mic Bar：仅 Main Only 时在设置图标下显示；双信道时隐藏 */
    if (menu_id == MENU_MIC_BAR) {
        if (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF || gEeprom.CROSS_BAND_RX_TX != CROSS_BAND_OFF)
            return false;
        return (icon_index == 1u);
    }
#endif

    (void)menu_number_1based;

    bool in_channel = false;
    bool in_settings = false;
    bool in_display = false;

    /* 仅对明确指定的菜单做分组；未指定的一律归入“其它”。 */
    if (menu_id == MENU_STEP ||
        menu_id == MENU_TXP ||
        menu_id == MENU_R_DCS ||
        menu_id == MENU_R_CTCS ||
        menu_id == MENU_T_DCS ||
        menu_id == MENU_T_CTCS ||
        menu_id == MENU_SFT_D ||
        menu_id == MENU_OFFSET ||
        menu_id == MENU_W_N ||
        menu_id == MENU_BCL ||
        menu_id == MENU_COMPAND ||
        menu_id == MENU_AM ||
        menu_id == MENU_MEM_CH ||
        menu_id == MENU_DEL_CH ||
        menu_id == MENU_MEM_NAME ||
        menu_id == MENU_CN_MEM_NAME ||
        menu_id == MENU_MDF)
    {
        in_channel = true;
    }

    if (menu_id == MENU_SQL ||
        menu_id == MENU_ROGER ||
        menu_id == MENU_STE ||
        menu_id == MENU_RP_STE ||
        menu_id == MENU_BEEP ||
        menu_id == MENU_MIC ||
        menu_id == MENU_F1SHRT ||
        menu_id == MENU_F1LONG ||
        menu_id == MENU_F2SHRT ||
        menu_id == MENU_F2LONG ||
        menu_id == MENU_MLONG)
    {
        in_settings = true;
    }

    if (menu_id == MENU_LANGUAGE ||
        menu_id == MENU_TDR ||
        menu_id == MENU_BOOT_HINT ||
        menu_id == MENU_ABR ||
        menu_id == MENU_ABR_MIN ||
        menu_id == MENU_ABR_MAX ||
        menu_id == MENU_ABR_ON_TX_RX ||
        menu_id == MENU_SET_CTR ||
        menu_id == MENU_SET_INV)
    {
        in_display = true;
    }

    const bool in_about = (menu_id == MENU_VOL);
    const bool in_other = !(in_channel || in_settings || in_display || in_about);

    if (icon_index == 0)
        return in_channel;     // Channel
    if (icon_index == 1)
        return in_settings;    // Settings
    if (icon_index == 2u)
        return in_display;     // Display
    if (icon_index == 4u)
        return in_about;       // About
    if (!in_other)
        return false;
    if (icon_index == 3u && menu_id == MENU_TX_LOCK)
        return false;          // Other: hide TX_LOCK entry
    return true;
}

static uint8_t MENU_GetIconOrderPriority(uint8_t icon_index, uint8_t menu_id)
{
    if (icon_index == 0u)
    {
        if (menu_id == MENU_STEP) return 0u;
        if (menu_id == MENU_TXP) return 1u;
        if (menu_id == MENU_R_DCS) return 2u;
        if (menu_id == MENU_R_CTCS) return 3u;
        if (menu_id == MENU_T_DCS) return 4u;
        if (menu_id == MENU_T_CTCS) return 5u;
        if (menu_id == MENU_SFT_D) return 6u;
        if (menu_id == MENU_OFFSET) return 7u;
        if (menu_id == MENU_W_N) return 8u;
        if (menu_id == MENU_BCL) return 9u;
        if (menu_id == MENU_COMPAND) return 10u;
        if (menu_id == MENU_AM) return 11u;
        if (menu_id == MENU_MEM_CH) return 12u;
        if (menu_id == MENU_DEL_CH) return 13u;
        if (menu_id == MENU_MEM_NAME) return 14u;
        if (menu_id == MENU_CN_MEM_NAME) return 15u;
        if (menu_id == MENU_MDF) return 16u;
    }

    if (icon_index == 1u)
    {
        if (menu_id == MENU_SQL) return 0u;
        if (menu_id == MENU_ROGER) return 1u;
        if (menu_id == MENU_STE) return 2u;
        if (menu_id == MENU_RP_STE) return 3u;
        if (menu_id == MENU_BEEP) return 4u;
        if (menu_id == MENU_MIC) return 5u;
        if (menu_id == MENU_F1SHRT) return 6u;
        if (menu_id == MENU_F1LONG) return 7u;
        if (menu_id == MENU_F2SHRT) return 8u;
        if (menu_id == MENU_F2LONG) return 9u;
        if (menu_id == MENU_MLONG) return 10u;
    }

    if (icon_index == 2u)
    {
        if (menu_id == MENU_LANGUAGE) return 0u;
        if (menu_id == MENU_TDR) return 1u;
        if (menu_id == MENU_BOOT_HINT) return 2u;
        if (menu_id == MENU_ABR) return 3u;
        if (menu_id == MENU_ABR_MIN) return 4u;
        if (menu_id == MENU_ABR_MAX) return 5u;
        if (menu_id == MENU_ABR_ON_TX_RX) return 6u;
        if (menu_id == MENU_SET_CTR) return 7u;
        if (menu_id == MENU_SET_INV) return 8u;
    }

    return 255u;
}

void MENU_UpdateMenuFilterForIcon(uint8_t icon_index)
{
    gMenuFilteredCount = 0;
    for (uint8_t i = 0; i < gMenuListCount && gMenuFilteredCount < ARRAY_SIZE(gMenuFilteredMap); i++)
    {
        bool is_in_group = MENU_IsMenuInIconGroup((uint8_t)(i + 1), MenuList[i].menu_id, icon_index);
        if (is_in_group)
        {
            uint8_t new_priority = MENU_GetIconOrderPriority(icon_index, MenuList[i].menu_id);
            uint8_t insert_pos = gMenuFilteredCount;

            for (uint8_t j = 0; j < gMenuFilteredCount; j++)
            {
                uint8_t existing_index = gMenuFilteredMap[j];
                uint8_t existing_menu_id = MenuList[existing_index].menu_id;
                uint8_t existing_priority = MENU_GetIconOrderPriority(icon_index, existing_menu_id);

                if (new_priority < existing_priority)
                {
                    insert_pos = j;
                    break;
                }
            }

            if (insert_pos < gMenuFilteredCount)
            {
                for (uint8_t k = gMenuFilteredCount; k > insert_pos; k--)
                {
                    gMenuFilteredMap[k] = gMenuFilteredMap[k - 1u];
                }
            }
            gMenuFilteredMap[insert_pos] = i;
            gMenuFilteredCount++;
        }
    }
}

void MENU_RefreshIconFilterAfterRxModeChange(void)
{
#ifdef ENABLE_AUDIO_BAR
    if (gMenuUseMainOnlyStatus && !gMenuMainPageActive)
    {
        MENU_UpdateMenuFilterForIcon(gMenuMainPageIconIndex);
        if (gMenuFilteredCount > 0u && gMenuCursor >= gMenuFilteredCount)
            gMenuCursor = (uint8_t)(gMenuFilteredCount - 1u);
    }
#endif
}

void MENU_RecordSelectionBeforeLeaveMenuToMain(void)
{
    if (gMenuMainPageActive)
    {   // level-1: save current icon
        const uint8_t icon_count = MENU_MainPageIconCount();
        const uint8_t icon_index = (icon_count > 0) ? ((uint8_t)gSubMenuSelection % icon_count) : 0;
        gMenuMainPageLastValid = true;
        gMenuMainPageLastIconIndex = icon_index;
        return;
    }

    // level-2/3: save current level-2 menu for current icon group
    if (gMenuMainPageIconIndex < ARRAY_SIZE(gMenuSecondPageLastValid))
    {
        gMenuSecondPageLastValid[gMenuMainPageIconIndex] = true;
        gMenuSecondPageLastMenuId[gMenuMainPageIconIndex] = (uint8_t)UI_MENU_GetCurrentMenuId();
    }
}

void MENU_ActivateMainPage(void)
{
    const uint8_t icon_count = MENU_MainPageIconCount();

    gMenuMainPageActive = true;
    gMenuUseMainOnlyStatus = true;
    if (gMenuMainPageLastValid)
        gSubMenuSelection = gMenuMainPageLastIconIndex;
    else
        gSubMenuSelection = 0; // default Channel on cold start
    /* gSubMenuSelection is only a launcher icon index (0..4); never reuse raw menu values here */
    gMenuMainPageIconIndex = (icon_count > 0) ? ((uint8_t)gSubMenuSelection % icon_count) : 0u;
}

void MENU_OpenFromMainScreen(void)
{
    MENU_ActivateMainPage();
    gSubMenuSelection      = 0;
    gMenuMainPageIconIndex = 0;
}

uint8_t MENU_MainPageIconCount(void)
{
    return 5;
}

uint8_t MENU_GetActiveMenuCount(void)
{
    if (gMenuUseMainOnlyStatus && !gMenuMainPageActive)
        return gMenuFilteredCount;
    if (gMenuUseMainOnlyStatus && gMenuMainPageActive)
        return gMenuListCount;
    {
        uint8_t n = 0;
        for (uint8_t i = 0; i < gMenuListCount; i++)
            if (!MENU_IsMenuIdExcludedFromBrowse(MenuList[i].menu_id))
                n++;
        return n;
    }
}

uint8_t MENU_GetActualMenuIndexFromCursor(uint8_t cursor)
{
    if (gMenuUseMainOnlyStatus && !gMenuMainPageActive)
    {
        if (gMenuFilteredCount == 0)
            return 0;
        if (cursor >= gMenuFilteredCount)
            cursor = (uint8_t)(gMenuFilteredCount - 1);
        return gMenuFilteredMap[cursor];
    }
    if (gMenuUseMainOnlyStatus && gMenuMainPageActive)
        return cursor;
    {
        uint8_t vis = 0;
        for (uint8_t i = 0; i < gMenuListCount; i++)
        {
            if (MENU_IsMenuIdExcludedFromBrowse(MenuList[i].menu_id))
                continue;
            if (vis == cursor)
                return i;
            vis++;
        }
        if (gMenuListCount == 0)
            return 0;
        for (int j = (int)gMenuListCount - 1; j >= 0; j--)
            if (!MENU_IsMenuIdExcludedFromBrowse(MenuList[(uint8_t)j].menu_id))
                return (uint8_t)j;
        return 0;
    }
}

#ifdef ENABLE_F_CAL_MENU
    void writeXtalFreqCal(const int32_t value, const bool update_eeprom)
    {
        BK4819_WriteRegister(BK4819_REG_3B, 22656 + value);

        if (update_eeprom)
        {
            struct
            {
                int16_t  BK4819_XtalFreqLow;
                uint16_t EEPROM_1F8A;
                uint16_t EEPROM_1F8C;
                uint8_t  VOLUME_GAIN;
                uint8_t  DAC_GAIN;
            } __attribute__((packed)) misc;

            gEeprom.BK4819_XTAL_FREQ_LOW = value;

            // radio 1 .. 04 00 46 00 50 00 2C 0E
            // radio 2 .. 05 00 46 00 50 00 2C 0E
            //
            EEPROM_ReadBuffer(0x1F88, &misc, 8);
            misc.BK4819_XtalFreqLow = value;
            EEPROM_WriteBuffer(0x1F88, &misc);
        }
    }
#endif

void MENU_StartCssScan(void)
{
    SCANNER_Start(true);
    gUpdateStatus = true;
    gCssBackgroundScan = true;

    gRequestDisplayScreen = DISPLAY_MENU;
}

void MENU_CssScanFound(void)
{
    if(gScanCssResultType == CODE_TYPE_DIGITAL || gScanCssResultType == CODE_TYPE_REVERSE_DIGITAL) {
        gMenuCursor = UI_MENU_GetMenuIdx(MENU_R_DCS);
    }
    else if(gScanCssResultType == CODE_TYPE_CONTINUOUS_TONE) {
        gMenuCursor = UI_MENU_GetMenuIdx(MENU_R_CTCS);
    }

    MENU_ShowCurrentSetting();

    gUpdateStatus = true;
    gUpdateDisplay = true;
}

void MENU_StopCssScan(void)
{
    gCssBackgroundScan = false;

#ifdef ENABLE_VOICE
    gAnotherVoiceID       = VOICE_ID_SCANNING_STOP;
#endif
    gUpdateDisplay = true;
    gUpdateStatus = true;
}

int MENU_GetLimits(uint8_t menu_id, int32_t *pMin, int32_t *pMax)
{
    *pMin = 0;

    switch (menu_id)
    {
        case MENU_SQL:
            //*pMin = 0;
            *pMax = 9;
            break;

        case MENU_STEP:
            //*pMin = 0;
            *pMax = STEP_N_ELEM - 1;
            break;

        case MENU_ABR:
            //*pMin = 0;
            *pMax = 61;
            break;

        case MENU_ABR_MIN:
            //*pMin = 0;
            *pMax = 9;
            break;

        case MENU_ABR_MAX:
            *pMin = 1;
            *pMax = 10;
            break;

        case MENU_F_LOCK:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_F_LOCK) - 1;
            break;

        case MENU_MDF:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_MDF) - 1;
            break;

        case MENU_TXP:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_TXP) - 1;
            break;

        case MENU_SFT_D:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_SFT_D) - 1;
            break;

        case MENU_TDR:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_RXMode) - 1;
            break;

        #ifdef ENABLE_VOICE
            case MENU_VOICE:
                //*pMin = 0;
                *pMax = ARRAY_SIZE(gSubMenu_VOICE) - 1;
                break;
        #endif

        case MENU_SC_REV:
            //*pMin = 0;
            *pMax = 104;
            break;

        case MENU_ROGER:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_ROGER) - 1;
            break;

        case MENU_PONMSG:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_PONMSG) - 1;
            break;

        case MENU_BOOT_HINT:
            *pMax = ARRAY_SIZE(gSubMenu_BOOT_HINT) - 1;
            break;

        case MENU_R_DCS:
        case MENU_T_DCS:
            //*pMin = 0;
            *pMax = 208;
            //*pMax = (ARRAY_SIZE(DCS_Options) * 2);
            break;

        case MENU_R_CTCS:
        case MENU_T_CTCS:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(CTCSS_Options);
            break;

        case MENU_W_N:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_W_N) - 1;
            break;

        case MENU_VOL:
#ifdef ENABLE_FEAT_F4HWN
            *pMax = 2;
#else
            *pMax = 0;
#endif
            break;

        #ifdef ENABLE_ALARM
            case MENU_AL_MOD:
                //*pMin = 0;
                *pMax = ARRAY_SIZE(gSubMenu_AL_MOD) - 1;
                break;
        #endif

        case MENU_RESET:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_RESET) - 1;
            break;

        case MENU_COMPAND:
        case MENU_ABR_ON_TX_RX:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_RX_TX) - 1;
            break;

        #ifndef ENABLE_FEAT_F4HWN
            #ifdef ENABLE_AM_FIX
                case MENU_AM_FIX:
            #endif
        #endif
        #ifdef ENABLE_AUDIO_BAR
            case MENU_MIC_BAR:
        #endif
        case MENU_BCL:
        case MENU_BEEP:
        case MENU_STE:
        case MENU_D_ST:
#ifdef ENABLE_DTMF_CALLING
        case MENU_D_DCD:
#endif
        case MENU_D_LIVE_DEC:
        #ifdef ENABLE_NOAA
            case MENU_NOAA_S:
        #endif
#ifndef ENABLE_FEAT_F4HWN
        case MENU_350TX:
        case MENU_200TX:
        case MENU_500TX:
#endif
        case MENU_350EN:
#ifndef ENABLE_FEAT_F4HWN
        case MENU_SCREN:
#endif
#ifdef ENABLE_FEAT_F4HWN
        case MENU_SET_TMR:
        case MENU_S_PRI:
#endif
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_OFF_ON) - 1;
            break;
        case MENU_AM:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gModulationStr) - 1;
            break;

#ifndef ENABLE_FEAT_F4HWN
        case MENU_SCR:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_SCRAMBLER) - 1;
            break;
#endif

        case MENU_AUTOLK:
            *pMax = 40;
            break;

        case MENU_TOT:
            //*pMin = 0;
            *pMin = 5;
            *pMax = 179;
            break;

        #ifdef ENABLE_VOX
            case MENU_VOX:
        #endif
        case MENU_RP_STE:
            //*pMin = 0;
            *pMax = 10;
            break;

        case MENU_MEM_CH:
        case MENU_1_CALL:
        case MENU_DEL_CH:
        case MENU_MEM_NAME:
        case MENU_CN_MEM_NAME:
            //*pMin = 0;
            *pMax = MR_CHANNEL_LAST;
            break;

        case MENU_S_PRI_CH_1:
        case MENU_S_PRI_CH_2:
            //*pMin = 0;
            *pMax = MR_CHANNEL_LAST + 2;
            break;

        case MENU_SAVE:
            //*pMin = 0;
            *pMax = 5;
            break;

        case MENU_MIC:
            //*pMin = 0;
            *pMax = 8;
            break;

        case MENU_LIST_CH:
            //*pMin = 0;
            *pMax = MR_CHANNELS_LIST + 1;
            break;

        case MENU_S_LIST:
            *pMin = 1;
            *pMax = MR_CHANNELS_LIST + 1;
            break;

#ifdef ENABLE_DTMF_CALLING
        case MENU_D_RSP:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_D_RSP) - 1;
            break;
#endif
        case MENU_PTT_ID:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_PTT_ID) - 1;
            break;

        case MENU_BAT_TXT:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_BAT_TXT) - 1;
            break;

        case MENU_LANGUAGE:
            *pMax = 1;
            break;

#ifdef ENABLE_DTMF_CALLING
        case MENU_D_HOLD:
            *pMin = 5;
            *pMax = 60;
            break;
#endif
        case MENU_D_PRE:
            *pMin = 3;
            *pMax = 99;
            break;

#ifdef ENABLE_DTMF_CALLING
        case MENU_D_LIST:
            *pMin = 1;
            *pMax = 16;
            break;
#endif
        #ifdef ENABLE_F_CAL_MENU
            case MENU_F_CALI:
                *pMin = -50;
                *pMax = +50;
                break;
        #endif

        case MENU_BATCAL:
            *pMin = 1500;
            *pMax = 3500;
            break;

        case MENU_BATTYP:
            //*pMin = 0;
            *pMax = 4;
            break;

        case MENU_SET_NAV:
            //*pMin = 0;
            *pMax = 1;
            break;

        case MENU_F1SHRT:
        case MENU_F1LONG:
        case MENU_F2SHRT:
        case MENU_F2LONG:
        case MENU_MLONG:
            //*pMin = 0;
            *pMax = gSubMenu_SIDEFUNCTIONS_size-1;
            break;

#ifdef ENABLE_FEAT_F4HWN_SLEEP
        case MENU_SET_OFF:
            *pMax = 120;
            break;
#endif

#ifdef ENABLE_FEAT_F4HWN
        case MENU_SET_PWR:
            *pMax = ARRAY_SIZE(gSubMenu_SET_PWR) - 1;
            break;
        case MENU_SET_PTT:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_SET_PTT) - 1;
            break;
        case MENU_SET_TOT:
        case MENU_SET_EOT:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_SET_TOT) - 1;
            break;
        #ifdef ENABLE_FEAT_F4HWN_CTR
        case MENU_SET_CTR:
            *pMin = 1;
            *pMax = 15;
            break;
        #endif
        case MENU_TX_LOCK:
        #ifdef ENABLE_FEAT_F4HWN_INV
        case MENU_SET_INV:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_OFF_ON) - 1;
            break;
        #endif
        case MENU_SET_LCK:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_SET_LCK) - 1;
            break;
        case MENU_SET_MET:
        case MENU_SET_GUI:
            //*pMin = 0;
            *pMax = ARRAY_SIZE(gSubMenu_SET_MET) - 1;
            break;
        #ifdef ENABLE_FEAT_F4HWN_AUDIO
        case MENU_SET_AUD:
            //*pMin = 0;
            if(gTxVfo->Modulation == MODULATION_AM)
                *pMax = ARRAY_SIZE(gSubMenu_SET_AUD_AM) - 1;
            else if (gTxVfo->Modulation == MODULATION_USB)
                *pMax = 0;
            else
                *pMax = ARRAY_SIZE(gSubMenu_SET_AUD_FM) - 1;
            break;
        #endif
        #ifdef ENABLE_FEAT_F4HWN_NARROWER
            case MENU_SET_NFM:
                //*pMin = 0;
                *pMax = ARRAY_SIZE(gSubMenu_SET_NFM) - 1;
                break;
        #endif
        #ifdef ENABLE_FEAT_F4HWN_VOL
            case MENU_SET_VOL:
                //*pMin = 0;
                *pMax = 63;
                break;
        #endif
        #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
            case MENU_SET_KEY:
                //*pMin = 0;
                *pMax = 4;
                break;
        #endif
#endif

        default:
            return -1;
    }

    return 0;
}

void MENU_AcceptSetting(void)
{
    int32_t        Min;
    int32_t        Max;
    FREQ_Config_t *pConfig = &gTxVfo->freq_config_RX;

    if (!MENU_GetLimits(UI_MENU_GetCurrentMenuId(), &Min, &Max))
    {
        if (gSubMenuSelection < Min) gSubMenuSelection = Min;
        else
        if (gSubMenuSelection > Max) gSubMenuSelection = Max;
    }

    switch (UI_MENU_GetCurrentMenuId())
    {
        default:
            return;

        case MENU_SQL:
            gEeprom.SQUELCH_LEVEL = gSubMenuSelection;
            gVfoConfigureMode     = VFO_CONFIGURE;
            #ifdef ENABLE_FEAT_F4HWN
                gSquelchLevelOriginal = 10;
            #endif
            break;

        case MENU_STEP:
            gTxVfo->STEP_SETTING = FREQUENCY_GetStepIdxFromSortedIdx(gSubMenuSelection);
            if (IS_FREQ_CHANNEL(gTxVfo->CHANNEL_SAVE))
            {
                gRequestSaveChannel = 1;
            }
            return;

        case MENU_TXP:
            gTxVfo->OUTPUT_POWER = gSubMenuSelection;
            gRequestSaveChannel = 1;
            return;

        case MENU_T_DCS:
            pConfig = &gTxVfo->freq_config_TX;

            // Fallthrough

        case MENU_R_DCS: {
            if (gSubMenuSelection == 0) {
                if (pConfig->CodeType == CODE_TYPE_CONTINUOUS_TONE) {
                    return;
                }
                pConfig->Code = 0;
                pConfig->CodeType = CODE_TYPE_OFF;
            }
            else if (gSubMenuSelection < 105) {
                pConfig->CodeType = CODE_TYPE_DIGITAL;
                pConfig->Code = gSubMenuSelection - 1;
            }
            else {
                pConfig->CodeType = CODE_TYPE_REVERSE_DIGITAL;
                pConfig->Code = gSubMenuSelection - 105;
            }

            gRequestSaveChannel = 1;
            return;
        }
        case MENU_T_CTCS:
            pConfig = &gTxVfo->freq_config_TX;
            [[fallthrough]];
        case MENU_R_CTCS: {
            if (gSubMenuSelection == 0) {
                if (pConfig->CodeType != CODE_TYPE_CONTINUOUS_TONE) {
                    return;
                }
                pConfig->Code     = 0;
                pConfig->CodeType = CODE_TYPE_OFF;
            }
            else {
                pConfig->Code     = gSubMenuSelection - 1;
                pConfig->CodeType = CODE_TYPE_CONTINUOUS_TONE;
            }

            gRequestSaveChannel = 1;
            return;
        }
        case MENU_SFT_D:
            gTxVfo->TX_OFFSET_FREQUENCY_DIRECTION = gSubMenuSelection;
            gRequestSaveChannel                   = 1;
            return;

        case MENU_OFFSET:
            gTxVfo->TX_OFFSET_FREQUENCY = gSubMenuSelection;
            gRequestSaveChannel         = 1;
            return;

        case MENU_W_N:
            gTxVfo->CHANNEL_BANDWIDTH = gSubMenuSelection;
            gRequestSaveChannel       = 1;
            return;

#ifndef ENABLE_FEAT_F4HWN
        case MENU_SCR:
            gTxVfo->SCRAMBLING_TYPE = gSubMenuSelection;
            gRequestSaveChannel     = 1;
            return;
#endif

        case MENU_BCL:
            gTxVfo->BUSY_CHANNEL_LOCK = gSubMenuSelection;
            gRequestSaveChannel       = 1;
            return;

        case MENU_MEM_CH:
            gTxVfo->CHANNEL_SAVE = gSubMenuSelection;
                gEeprom.MrChannel[gEeprom.TX_VFO] = gSubMenuSelection;
            gRequestSaveChannel = 2;
            gVfoConfigureMode   = VFO_CONFIGURE_RELOAD;
            gFlagResetVfos      = true;
            return;

        case MENU_MEM_NAME:
            for (int i = 9; i >= 0; i--) {
                if (edit[i] != ' ' && edit[i] != '_' && edit[i] != 0x00 && edit[i] != 0xff)
                    break;
                edit[i] = ' ';
            }

            SETTINGS_SaveChannelName(gSubMenuSelection, edit);
            return;

#ifdef ENABLE_CHINESE
        case MENU_CN_MEM_NAME:
        {
            // Trim trailing padding
            for (int i = 9; i >= 0; i--) {
                uint8_t c = (uint8_t)edit[i];
                if (c != ' ' && c != '_' && c != 0x00 && c != 0xff)
                    break;
                edit[i] = ' ';
            }
            SETTINGS_SaveCNChannelName(gSubMenuSelection, edit);
            return;
        }
#endif

        case MENU_S_PRI_CH_1:
            gEeprom.SCANLIST_PRIORITY_CH[0] = gSubMenuSelection;
            break;

        case MENU_S_PRI_CH_2:
            gEeprom.SCANLIST_PRIORITY_CH[1] = gSubMenuSelection;
            break;

        case MENU_SAVE:
            gEeprom.BATTERY_SAVE = gSubMenuSelection;
            break;

        #ifdef ENABLE_VOX
            case MENU_VOX:
                gEeprom.VOX_SWITCH = gSubMenuSelection != 0;
                if (gEeprom.VOX_SWITCH)
                    gEeprom.VOX_LEVEL = gSubMenuSelection - 1;
                SETTINGS_LoadCalibration();
                gFlagReconfigureVfos = true;
                gUpdateStatus        = true;
                break;
        #endif

        case MENU_ABR:
            gEeprom.BACKLIGHT_TIME = gSubMenuSelection;
            #ifdef ENABLE_FEAT_F4HWN
                gBackLight = false;
            #endif
            break;

        case MENU_ABR_MIN:
            gEeprom.BACKLIGHT_MIN = gSubMenuSelection;
            gEeprom.BACKLIGHT_MAX = MAX(gSubMenuSelection + 1 , gEeprom.BACKLIGHT_MAX);
            break;

        case MENU_ABR_MAX:
            gEeprom.BACKLIGHT_MAX = gSubMenuSelection;
            gEeprom.BACKLIGHT_MIN = MIN(gSubMenuSelection - 1, gEeprom.BACKLIGHT_MIN);
            break;

        case MENU_ABR_ON_TX_RX:
            gSetting_backlight_on_tx_rx = gSubMenuSelection;
            break;

        case MENU_TDR:
            gEeprom.DUAL_WATCH = (gEeprom.TX_VFO + 1) * (gSubMenuSelection & 1);
            gEeprom.CROSS_BAND_RX_TX = (gEeprom.TX_VFO + 1) * ((gSubMenuSelection & 2) > 0);

            #ifdef ENABLE_FEAT_F4HWN
                gDW = gEeprom.DUAL_WATCH;
                gCB = gEeprom.CROSS_BAND_RX_TX;
                gSaveRxMode = true;
            #endif

            gFlagReconfigureVfos = true;
            gUpdateStatus        = true;
#ifdef ENABLE_AUDIO_BAR
            SETTINGS_ForceMicBarOffWhenNotMainOnly();
#endif
            MENU_RefreshIconFilterAfterRxModeChange();
            break;

        case MENU_BEEP:
            gEeprom.BEEP_CONTROL = gSubMenuSelection;
            break;

        case MENU_TOT:
            gEeprom.TX_TIMEOUT_TIMER = gSubMenuSelection;
            break;

        #ifdef ENABLE_VOICE
            case MENU_VOICE:
                gEeprom.VOICE_PROMPT = gSubMenuSelection;
                gUpdateStatus        = true;
                break;
        #endif

        case MENU_SC_REV:
            gEeprom.SCAN_RESUME_MODE = gSubMenuSelection;
            break;

        case MENU_MDF:
            gEeprom.CHANNEL_DISPLAY_MODE = gSubMenuSelection;
            break;

        case MENU_AUTOLK:
            gEeprom.AUTO_KEYPAD_LOCK = gSubMenuSelection;
            gKeyLockCountdown        = gEeprom.AUTO_KEYPAD_LOCK * 30; // 15 seconds step
            break;

        case MENU_LIST_CH:
            gTxVfo->SCANLIST_PARTICIPATION = gSubMenuSelection;
            SETTINGS_UpdateChannel(gTxVfo->CHANNEL_SAVE, gTxVfo, true, false, true);
            gVfoConfigureMode = VFO_CONFIGURE;
            gFlagResetVfos    = true;
            return;

        case MENU_STE:
            gEeprom.TAIL_TONE_ELIMINATION = gSubMenuSelection;
            break;

        case MENU_RP_STE:
            gEeprom.REPEATER_TAIL_TONE_ELIMINATION = gSubMenuSelection;
            break;

        case MENU_MIC:
            gEeprom.MIC_SENSITIVITY = gSubMenuSelection;
            SETTINGS_LoadCalibration();
            gFlagReconfigureVfos = true;
            break;

        #ifdef ENABLE_AUDIO_BAR
            case MENU_MIC_BAR:
                gSetting_mic_bar = gSubMenuSelection;
                break;
        #endif

        case MENU_COMPAND:
            gTxVfo->Compander = gSubMenuSelection;
            SETTINGS_UpdateChannel(gTxVfo->CHANNEL_SAVE, gTxVfo, true, false, true);
            gVfoConfigureMode = VFO_CONFIGURE;
            gFlagResetVfos    = true;
//          gRequestSaveChannel = 1;
            return;

        case MENU_1_CALL:
            gEeprom.CHAN_1_CALL = gSubMenuSelection;
            break;

        case MENU_S_LIST:
            gEeprom.SCAN_LIST_DEFAULT = gSubMenuSelection;
            break;

        case MENU_S_PRI:
            gEeprom.SCAN_LIST_ENABLED = gSubMenuSelection;
            break;

        #ifdef ENABLE_ALARM
            case MENU_AL_MOD:
                gEeprom.ALARM_MODE = gSubMenuSelection;
                break;
        #endif

        case MENU_D_ST:
            gEeprom.DTMF_SIDE_TONE = gSubMenuSelection;
            break;

#ifdef ENABLE_DTMF_CALLING
        case MENU_D_RSP:
            gEeprom.DTMF_DECODE_RESPONSE = gSubMenuSelection;
            break;

        case MENU_D_HOLD:
            gEeprom.DTMF_auto_reset_time = gSubMenuSelection;
            break;
#endif
        case MENU_D_PRE:
            gEeprom.DTMF_PRELOAD_TIME = gSubMenuSelection * 10;
            break;

        case MENU_PTT_ID:
            gTxVfo->DTMF_PTT_ID_TX_MODE = gSubMenuSelection;
            gRequestSaveChannel         = 1;
            return;

        case MENU_BAT_TXT:
            gSetting_battery_text = gSubMenuSelection;
            break;

        case MENU_LANGUAGE:
            gUiLanguage = gSubMenuSelection & 1u;
            SETTINGS_SaveSettings();
            gUpdateDisplay = true;
            return;

#ifdef ENABLE_DTMF_CALLING
        case MENU_D_DCD:
            gTxVfo->DTMF_DECODING_ENABLE = gSubMenuSelection;
            DTMF_clear_RX();
            gRequestSaveChannel = 1;
            return;
#endif

        case MENU_D_LIVE_DEC:
            gSetting_live_DTMF_decoder = gSubMenuSelection;
            gDTMF_RX_live_timeout = 0;
            memset(gDTMF_RX_live, 0, sizeof(gDTMF_RX_live));
            if (!gSetting_live_DTMF_decoder)
                BK4819_DisableDTMF();
            gFlagReconfigureVfos     = true;
            gUpdateStatus            = true;
            break;

#ifdef ENABLE_DTMF_CALLING
        case MENU_D_LIST:
            gDTMF_chosen_contact = gSubMenuSelection - 1;
            if (gIsDtmfContactValid)
            {
                GUI_SelectNextDisplay(DISPLAY_MAIN);
                gDTMF_InputMode       = true;
                gDTMF_InputBox_Index  = 3;
                memcpy(gDTMF_InputBox, gDTMF_ID, 4);
                gRequestDisplayScreen = DISPLAY_INVALID;
            }
            return;
#endif
        case MENU_PONMSG:
            gEeprom.POWER_ON_DISPLAY_MODE = gSubMenuSelection;
            break;

        case MENU_BOOT_HINT:
            gSetting_boot_hint = (uint8_t)gSubMenuSelection;
            break;

        case MENU_ROGER:
            gEeprom.ROGER = gSubMenuSelection;
            break;

        case MENU_AM:
            gTxVfo->Modulation     = gSubMenuSelection;
            gRequestSaveChannel = 1;
            return;

        #ifndef ENABLE_FEAT_F4HWN
            #ifdef ENABLE_AM_FIX
                case MENU_AM_FIX:
                    gSetting_AM_fix = gSubMenuSelection;
                    gVfoConfigureMode = VFO_CONFIGURE_RELOAD;
                    gFlagResetVfos    = true;
                    break;
            #endif
        #endif

        #ifdef ENABLE_NOAA
            case MENU_NOAA_S:
                gEeprom.NOAA_AUTO_SCAN = gSubMenuSelection;
                gFlagReconfigureVfos   = true;
                break;
        #endif

        case MENU_DEL_CH:
            SETTINGS_UpdateChannel(gSubMenuSelection, NULL, false, false, true);
            gVfoConfigureMode = VFO_CONFIGURE_RELOAD;
            gFlagResetVfos    = true;
            return;

        case MENU_RESET:
            SETTINGS_FactoryReset(gSubMenuSelection);
            return;

#ifndef ENABLE_FEAT_F4HWN
        case MENU_350TX:
            gSetting_350TX = gSubMenuSelection;
            break;
#endif

        case MENU_F_LOCK: {
            if(gSubMenuSelection == F_LOCK_NONE) { // select 10 times to enable
                gUnlockAllTxConfCnt++;
#ifdef ENABLE_FEAT_F4HWN
                if(gUnlockAllTxConfCnt < 3)
#else
                if(gUnlockAllTxConfCnt < 10)
#endif
                    return;
            }
            else
                gUnlockAllTxConfCnt = 0;

            gSetting_F_LOCK = gSubMenuSelection;

            #ifdef ENABLE_FEAT_F4HWN
            if(gSetting_F_LOCK == F_LOCK_ALL) {
                SETTINGS_ResetTxLock();
            }
            #endif
            break;
        }
#ifndef ENABLE_FEAT_F4HWN
        case MENU_200TX:
            gSetting_200TX = gSubMenuSelection;
            break;

        case MENU_500TX:
            gSetting_500TX = gSubMenuSelection;
            break;
#endif
        case MENU_350EN:
            gSetting_350EN       = gSubMenuSelection;
            gVfoConfigureMode    = VFO_CONFIGURE_RELOAD;
            gFlagResetVfos       = true;
            break;
#ifndef ENABLE_FEAT_F4HWN
        case MENU_SCREN:
            gSetting_ScrambleEnable = gSubMenuSelection;
            gFlagReconfigureVfos    = true;
            break;
#endif

        #ifdef ENABLE_F_CAL_MENU
            case MENU_F_CALI:
                writeXtalFreqCal(gSubMenuSelection, true);
                return;
        #endif

        case MENU_BATCAL:
        {                                                                   // voltages are averages between discharge curves of 1600 and 2200 mAh
            // gBatteryCalibration[0] = (520ul * gSubMenuSelection) / 760;  // 5.20V empty, blinking above this value, reduced functionality below
            // gBatteryCalibration[1] = (689ul * gSubMenuSelection) / 760;  // 6.89V,  ~5%, 1 bars above this value
            // gBatteryCalibration[2] = (724ul * gSubMenuSelection) / 760;  // 7.24V, ~17%, 2 bars above this value
            gBatteryCalibration[3] =          gSubMenuSelection;            // 7.6V,  ~29%, 3 bars above this value
            // gBatteryCalibration[4] = (771ul * gSubMenuSelection) / 760;  // 7.71V, ~65%, 4 bars above this value
            // gBatteryCalibration[5] = 2300;
            SETTINGS_SaveBatteryCalibration(gBatteryCalibration);
            return;
        }

        case MENU_BATTYP:
            gEeprom.BATTERY_TYPE = gSubMenuSelection;
            break;

        case MENU_SET_NAV:
            gEeprom.SET_NAV = gSubMenuSelection;
            break;

        case MENU_F1SHRT:
        case MENU_F1LONG:
        case MENU_F2SHRT:
        case MENU_F2LONG:
        case MENU_MLONG:
            {
                uint8_t * fun[]= {
                    &gEeprom.KEY_1_SHORT_PRESS_ACTION,
                    &gEeprom.KEY_1_LONG_PRESS_ACTION,
                    &gEeprom.KEY_2_SHORT_PRESS_ACTION,
                    &gEeprom.KEY_2_LONG_PRESS_ACTION,
                    &gEeprom.KEY_M_LONG_PRESS_ACTION};
                *fun[UI_MENU_GetCurrentMenuId()-MENU_F1SHRT] = gSubMenu_SIDEFUNCTIONS[gSubMenuSelection].id;
            }
            break;

#ifdef ENABLE_FEAT_F4HWN_SLEEP 
        case MENU_SET_OFF:
            gSetting_set_off = gSubMenuSelection;
            break;
#endif

#ifdef ENABLE_FEAT_F4HWN
        case MENU_SET_PWR:
            gSetting_set_pwr = gSubMenuSelection;
            gRequestSaveChannel = 1;
            break;
        case MENU_SET_PTT:
            gSetting_set_ptt = 0;
            gSetting_set_ptt_session = 0;
            break;
        case MENU_SET_TOT:
            gSetting_set_tot = 0;
            break;
        case MENU_SET_EOT:
            gSetting_set_eot = 0;
            break;
        #ifdef ENABLE_FEAT_F4HWN_CTR
        case MENU_SET_CTR:
            gSetting_set_ctr = gSubMenuSelection;
            break;
        #endif
        case MENU_SET_INV:
            gSetting_set_inv = 0;
            break;
        case MENU_SET_LCK:
            gSetting_set_lck = gSubMenuSelection;
            break;
        case MENU_SET_MET:
            gSetting_set_met = gSubMenuSelection;
            break;
        case MENU_SET_GUI:
            gSetting_set_gui = gSubMenuSelection;
            break;
        #ifdef ENABLE_FEAT_F4HWN_AUDIO
        case MENU_SET_AUD:
            if(gTxVfo->Modulation == MODULATION_AM)
                gSetting_set_audio_am = gSubMenuSelection;
            else if (gTxVfo->Modulation == MODULATION_FM)
                gSetting_set_audio_fm = gSubMenuSelection;

            RADIO_SetModulation(gTxVfo->Modulation);
            break;
        #endif
        #ifdef ENABLE_FEAT_F4HWN_NARROWER
            case MENU_SET_NFM:
                gSetting_set_nfm = gSubMenuSelection;
                RADIO_SetTxParameters();
                RADIO_SetupRegisters(true);
                break;
        #endif
        #ifdef ENABLE_FEAT_F4HWN_VOL
            case MENU_SET_VOL:
                gEeprom.VOLUME_GAIN = gSubMenuSelection;
                break;
        #endif
        #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
            case MENU_SET_KEY:
                gEeprom.SET_KEY = gSubMenuSelection;
                break;
        #endif
        case MENU_SET_TMR:
            gSetting_set_tmr = gSubMenuSelection;
            break;
        case MENU_TX_LOCK:
            gTxVfo->TX_LOCK = gSubMenuSelection;
            gRequestSaveChannel       = 1;
            return;
#endif
    }

    gRequestSaveSettings = true;
}

static void MENU_ClampSelection(int8_t Direction)
{
    int32_t Min;
    int32_t Max;

    if (!MENU_GetLimits(UI_MENU_GetCurrentMenuId(), &Min, &Max))
    {
        int32_t Selection = gSubMenuSelection;
        if (Selection < Min) Selection = Min;
        else
        if (Selection > Max) Selection = Max;
        gSubMenuSelection = NUMBER_AddWithWraparound(Selection, Direction, Min, Max);
    }
}

void MENU_ShowCurrentSetting(void)
{
    /* On the level-1 icon page, gSubMenuSelection is the selected icon (0..n-1). The same
     * variable holds the edited value for level 2/3; refreshing here would replace the icon
     * index with e.g. SQL/TXP (often 1) and show the wrong icon (Setting). */
    if (gMenuMainPageActive)
        return;

    switch (UI_MENU_GetCurrentMenuId())
    {
        case MENU_SQL:
            gSubMenuSelection = gEeprom.SQUELCH_LEVEL;
            break;

        case MENU_STEP:
            gSubMenuSelection = FREQUENCY_GetSortedIdxFromStepIdx(gTxVfo->STEP_SETTING);
            break;

        case MENU_TXP:
            gSubMenuSelection = gTxVfo->OUTPUT_POWER;
            break;

        case MENU_RESET:
            gSubMenuSelection = 0;
            break;

        case MENU_R_DCS:
        case MENU_R_CTCS:
        {
            DCS_CodeType_t type = gTxVfo->freq_config_RX.CodeType;
            uint8_t code = gTxVfo->freq_config_RX.Code;
            int menuid = UI_MENU_GetCurrentMenuId();

            if(gScanUseCssResult) {
                gScanUseCssResult = false;
                type = gScanCssResultType;
                code = gScanCssResultCode;
            }
            if((menuid==MENU_R_CTCS) ^ (type==CODE_TYPE_CONTINUOUS_TONE)) { //not the same type
                gSubMenuSelection = 0;
                break;
            }

            switch (type) {
                case CODE_TYPE_CONTINUOUS_TONE:
                case CODE_TYPE_DIGITAL:
                    gSubMenuSelection = code + 1;
                    break;
                case CODE_TYPE_REVERSE_DIGITAL:
                    gSubMenuSelection = code + 105;
                    break;
                default:
                    gSubMenuSelection = 0;
                    break;
            }
        break;
        }

        case MENU_T_DCS:
            switch (gTxVfo->freq_config_TX.CodeType)
            {
                case CODE_TYPE_DIGITAL:
                    gSubMenuSelection = gTxVfo->freq_config_TX.Code + 1;
                    break;
                case CODE_TYPE_REVERSE_DIGITAL:
                    gSubMenuSelection = gTxVfo->freq_config_TX.Code + 105;
                    break;
                default:
                    gSubMenuSelection = 0;
                    break;
            }
            break;

        case MENU_T_CTCS:
            gSubMenuSelection = (gTxVfo->freq_config_TX.CodeType == CODE_TYPE_CONTINUOUS_TONE) ? gTxVfo->freq_config_TX.Code + 1 : 0;
            break;

        case MENU_SFT_D:
            gSubMenuSelection = gTxVfo->TX_OFFSET_FREQUENCY_DIRECTION;
            break;

        case MENU_OFFSET:
            gSubMenuSelection = gTxVfo->TX_OFFSET_FREQUENCY;
            break;

        case MENU_W_N:
            gSubMenuSelection = gTxVfo->CHANNEL_BANDWIDTH;
            break;

        case MENU_VOL:
            gSubMenuSelection = 0;
            break;

#ifndef ENABLE_FEAT_F4HWN
        case MENU_SCR:
            gSubMenuSelection = gTxVfo->SCRAMBLING_TYPE;
            break;
#endif

        case MENU_BCL:
            gSubMenuSelection = gTxVfo->BUSY_CHANNEL_LOCK;
            break;

        case MENU_MEM_CH:
                gSubMenuSelection = gEeprom.MrChannel[gEeprom.TX_VFO];
            break;

        case MENU_MEM_NAME:
        case MENU_CN_MEM_NAME:
            gSubMenuSelection = gEeprom.MrChannel[gEeprom.TX_VFO];
            break;

        case MENU_SAVE:
            gSubMenuSelection = gEeprom.BATTERY_SAVE;
            break;

#ifdef ENABLE_VOX
        case MENU_VOX:
            gSubMenuSelection = gEeprom.VOX_SWITCH ? gEeprom.VOX_LEVEL + 1 : 0;
            break;
#endif

        case MENU_ABR:
            #ifdef ENABLE_FEAT_F4HWN
                if(gBackLight)
                {
                    gSubMenuSelection = gBacklightTimeOriginal;
                }
                else
                {
                    gSubMenuSelection = gEeprom.BACKLIGHT_TIME;
                }
            #else
                gSubMenuSelection = gEeprom.BACKLIGHT_TIME;
            #endif
            break;

        case MENU_ABR_MIN:
            gSubMenuSelection = gEeprom.BACKLIGHT_MIN;
            break;

        case MENU_ABR_MAX:
            gSubMenuSelection = gEeprom.BACKLIGHT_MAX;
            break;

        case MENU_ABR_ON_TX_RX:
            gSubMenuSelection = gSetting_backlight_on_tx_rx;
            break;

        case MENU_TDR:
            gSubMenuSelection = (gEeprom.DUAL_WATCH != DUAL_WATCH_OFF) + (gEeprom.CROSS_BAND_RX_TX != CROSS_BAND_OFF) * 2;
            break;

        case MENU_BEEP:
            gSubMenuSelection = gEeprom.BEEP_CONTROL;
            break;

        case MENU_TOT:
            gSubMenuSelection = gEeprom.TX_TIMEOUT_TIMER;
            break;

#ifdef ENABLE_VOICE
        case MENU_VOICE:
            gSubMenuSelection = gEeprom.VOICE_PROMPT;
            break;
#endif

        case MENU_SC_REV:
            gSubMenuSelection = gEeprom.SCAN_RESUME_MODE;
            break;

        case MENU_MDF:
            gSubMenuSelection = gEeprom.CHANNEL_DISPLAY_MODE;
            break;

        case MENU_AUTOLK:
            gSubMenuSelection = gEeprom.AUTO_KEYPAD_LOCK;
            break;

        case MENU_LIST_CH:
            gSubMenuSelection = gTxVfo->SCANLIST_PARTICIPATION;
            break;

        case MENU_STE:
            gSubMenuSelection = gEeprom.TAIL_TONE_ELIMINATION;
            break;

        case MENU_RP_STE:
            gSubMenuSelection = gEeprom.REPEATER_TAIL_TONE_ELIMINATION;
            break;

        case MENU_MIC:
            gSubMenuSelection = gEeprom.MIC_SENSITIVITY;
            break;

#ifdef ENABLE_AUDIO_BAR
        case MENU_MIC_BAR:
            gSubMenuSelection = gSetting_mic_bar;
            break;
#endif

        case MENU_COMPAND:
            gSubMenuSelection = gTxVfo->Compander;
            return;

        case MENU_1_CALL:
            gSubMenuSelection = gEeprom.CHAN_1_CALL;
            break;

        case MENU_S_LIST:
            gSubMenuSelection = gEeprom.SCAN_LIST_DEFAULT;
            break;

        case MENU_S_PRI:
            gSubMenuSelection = gEeprom.SCAN_LIST_ENABLED;
            break;

        case MENU_S_PRI_CH_1:
            gSubMenuSelection = gEeprom.SCANLIST_PRIORITY_CH[0];
            break;

        case MENU_S_PRI_CH_2:
            gSubMenuSelection = gEeprom.SCANLIST_PRIORITY_CH[1];
            break;

        #ifdef ENABLE_ALARM
            case MENU_AL_MOD:
                gSubMenuSelection = gEeprom.ALARM_MODE;
                break;
        #endif

        case MENU_D_ST:
            gSubMenuSelection = gEeprom.DTMF_SIDE_TONE;
            break;

#ifdef ENABLE_DTMF_CALLING
        case MENU_D_RSP:
            gSubMenuSelection = gEeprom.DTMF_DECODE_RESPONSE;
            break;

        case MENU_D_HOLD:
            gSubMenuSelection = gEeprom.DTMF_auto_reset_time;
            break;
#endif
        case MENU_D_PRE:
            gSubMenuSelection = gEeprom.DTMF_PRELOAD_TIME / 10;
            break;

        case MENU_PTT_ID:
            gSubMenuSelection = gTxVfo->DTMF_PTT_ID_TX_MODE;
            break;

        case MENU_BAT_TXT:
            gSubMenuSelection = gSetting_battery_text;
            return;

        case MENU_LANGUAGE:
            gSubMenuSelection = gUiLanguage;
            return;

#ifdef ENABLE_DTMF_CALLING
        case MENU_D_DCD:
            gSubMenuSelection = gTxVfo->DTMF_DECODING_ENABLE;
            break;

        case MENU_D_LIST:
            gSubMenuSelection = gDTMF_chosen_contact + 1;
            break;
#endif
        case MENU_D_LIVE_DEC:
            gSubMenuSelection = gSetting_live_DTMF_decoder;
            break;

        case MENU_PONMSG:
            gSubMenuSelection = gEeprom.POWER_ON_DISPLAY_MODE;
            break;

        case MENU_BOOT_HINT:
            gSubMenuSelection = gSetting_boot_hint;
            return;

        case MENU_ROGER:
            gSubMenuSelection = gEeprom.ROGER;
            break;

        case MENU_AM:
            gSubMenuSelection = gTxVfo->Modulation;
            break;

#ifndef ENABLE_FEAT_F4HWN
    #ifdef ENABLE_AM_FIX
            case MENU_AM_FIX:
                gSubMenuSelection = gSetting_AM_fix;
                break;
    #endif
#endif
                
        #ifdef ENABLE_NOAA
            case MENU_NOAA_S:
                gSubMenuSelection = gEeprom.NOAA_AUTO_SCAN;
                break;
        #endif

        case MENU_DEL_CH:
                gSubMenuSelection = RADIO_FindNextChannel(gEeprom.MrChannel[gEeprom.TX_VFO], 1, false, 1);
            break;

#ifndef ENABLE_FEAT_F4HWN
        case MENU_350TX:
            gSubMenuSelection = gSetting_350TX;
            break;
#endif

        case MENU_F_LOCK:
            gSubMenuSelection = gSetting_F_LOCK;
            break;

#ifndef ENABLE_FEAT_F4HWN
        case MENU_200TX:
            gSubMenuSelection = gSetting_200TX;
            break;

        case MENU_500TX:
            gSubMenuSelection = gSetting_500TX;
            break;

#endif
        case MENU_350EN:
            gSubMenuSelection = gSetting_350EN;
            break;

#ifndef ENABLE_FEAT_F4HWN
        case MENU_SCREN:
            gSubMenuSelection = gSetting_ScrambleEnable;
            break;
#endif

        #ifdef ENABLE_F_CAL_MENU
            case MENU_F_CALI:
                gSubMenuSelection = gEeprom.BK4819_XTAL_FREQ_LOW;
                break;
        #endif

        case MENU_BATCAL:
            gSubMenuSelection = gBatteryCalibration[3];
            break;

        case MENU_BATTYP:
            gSubMenuSelection = gEeprom.BATTERY_TYPE;
            break;

        case MENU_SET_NAV:
            gSubMenuSelection = gEeprom.SET_NAV;
            break;

        case MENU_F1SHRT:
        case MENU_F1LONG:
        case MENU_F2SHRT:
        case MENU_F2LONG:
        case MENU_MLONG:
        {
            uint8_t * fun[]= {
                &gEeprom.KEY_1_SHORT_PRESS_ACTION,
                &gEeprom.KEY_1_LONG_PRESS_ACTION,
                &gEeprom.KEY_2_SHORT_PRESS_ACTION,
                &gEeprom.KEY_2_LONG_PRESS_ACTION,
                &gEeprom.KEY_M_LONG_PRESS_ACTION};
            uint8_t id = *fun[UI_MENU_GetCurrentMenuId()-MENU_F1SHRT];

            for(int i = 0; i < gSubMenu_SIDEFUNCTIONS_size; i++) {
                if(gSubMenu_SIDEFUNCTIONS[i].id==id) {
                    gSubMenuSelection = i;
                    break;
                }

            }
            break;
        }

#ifdef ENABLE_FEAT_F4HWN_SLEEP 
        case MENU_SET_OFF:
            gSubMenuSelection = gSetting_set_off;
            break;
#endif

#ifdef ENABLE_FEAT_F4HWN
        case MENU_SET_PWR:
            gSubMenuSelection = gSetting_set_pwr;
            break;
        case MENU_SET_PTT:
            gSetting_set_ptt = 0;
            gSetting_set_ptt_session = 0;
            gSubMenuSelection = 0;
            break;
        case MENU_SET_TOT:
            gSetting_set_tot = 0;
            gSubMenuSelection = 0;
            break;
        case MENU_SET_EOT:
            gSetting_set_eot = 0;
            gSubMenuSelection = 0;
            break;
        #ifdef ENABLE_FEAT_F4HWN_CTR
        case MENU_SET_CTR:
            gSubMenuSelection = gSetting_set_ctr;
            break;
        #endif
        case MENU_SET_INV:
            gSetting_set_inv = 0;
            gSubMenuSelection = 0;
            break;
        case MENU_SET_LCK:
            gSubMenuSelection = gSetting_set_lck;
            break;
        case MENU_SET_MET:
            gSubMenuSelection = gSetting_set_met;
            break;
        case MENU_SET_GUI:
            gSubMenuSelection = gSetting_set_gui;
            break;
        #ifdef ENABLE_FEAT_F4HWN_AUDIO
        case MENU_SET_AUD:
            if(gTxVfo->Modulation == MODULATION_AM)
                gSubMenuSelection = gSetting_set_audio_am;
            else if (gTxVfo->Modulation == MODULATION_USB)
                gSubMenuSelection = 0;
            else
                gSubMenuSelection = gSetting_set_audio_fm;
            break;
        #endif
        #ifdef ENABLE_FEAT_F4HWN_NARROWER
            case MENU_SET_NFM:
                gSubMenuSelection = gSetting_set_nfm;
                break;
        #endif
        #ifdef ENABLE_FEAT_F4HWN_VOL
            case MENU_SET_VOL:
                gSubMenuSelection = gEeprom.VOLUME_GAIN;
                break;
        #endif
        #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
            case MENU_SET_KEY:
                gSubMenuSelection = gEeprom.SET_KEY;
                break;
        #endif
        case MENU_SET_TMR:
            gSubMenuSelection = gSetting_set_tmr;
            break;
        case MENU_TX_LOCK:
            gSubMenuSelection = gTxVfo->TX_LOCK;
            break;
#endif

        default:
            return;
    }
}

static void MENU_Key_0_to_9(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
    uint8_t  Offset;
    int32_t  Min;
    int32_t  Max;
    uint16_t Value = 0;

    if (bKeyHeld || !bKeyPressed)
        return;

    gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

    if (gMenuMainPageActive && !gIsInSubMenu)
    {
        const uint8_t icon_count = MENU_MainPageIconCount();
        const uint8_t value = (uint8_t)(Key - KEY_0);

        if (value >= 1 && value <= icon_count)
        {
            gSubMenuSelection = (uint8_t)(value - 1);
            gRequestDisplayScreen = DISPLAY_MENU;
        }
        else
        {
            gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
        }
        return;
    }

    if (UI_MENU_GetCurrentMenuId() == MENU_MEM_NAME && edit_index >= 0)
    {   // currently editing the channel name

        if (edit_index < 10)
        {
            if (gMemNameInputMode == MEM_NAME_INPUT_DIGIT)
            {
                edit[edit_index] = '0' + Key - KEY_0;
                gMemNameCandidateCount = 0;
                MENU_MemNameAdvanceAfterInput();
                gRequestDisplayScreen = DISPLAY_MENU;
                return;
            }
            if (gMemNameInputMode == MEM_NAME_INPUT_LOWER ||
                gMemNameInputMode == MEM_NAME_INPUT_UPPER)
            {
                if (Key >= KEY_1 && Key <= KEY_4 && gMemNameCandidateCount > 0u)
                {
                    const uint8_t idx = (uint8_t)(Key - KEY_1);
                    if (idx < gMemNameCandidateCount)
                    {
                        edit[edit_index] = gMemNameCandidates[idx];
                        MENU_MemNameAdvanceAfterInput();
                        gRequestDisplayScreen = DISPLAY_MENU;
                        return;
                    }
                }
                if (Key >= KEY_2 && Key <= KEY_9)
                {
                    MENU_BuildMemNameCandidatesFromKey(Key);
                    gRequestDisplayScreen = DISPLAY_MENU;
                    return;
                }
                gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
                return;
            }
            if (gMemNameInputMode == MEM_NAME_INPUT_SYMBOL)
            {
                if (Key >= KEY_1 && Key <= KEY_5 && gMemNameCandidateCount > 0u)
                {
                    const uint8_t idx = (uint8_t)(Key - KEY_1);
                    if (idx < gMemNameCandidateCount)
                    {
                        edit[edit_index] = gMemNameCandidates[idx];
                        MENU_MemNameAdvanceAfterInput();
                        gRequestDisplayScreen = DISPLAY_MENU;
                        return;
                    }
                }
                gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
                return;
            }
            if (Key <= KEY_9)
            {
                edit[edit_index] = '0' + Key - KEY_0;
                gRequestDisplayScreen = DISPLAY_MENU;
            }
        }

        return;
    }

#ifdef ENABLE_CHINESE
    if (UI_MENU_GetCurrentMenuId() == MENU_CN_MEM_NAME && edit_index >= 0)
    {
        if (edit_index >= 10)
        {
            gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
            return;
        }

        // Digit mode: keys 0-9 input digits directly
        if (gMemNameInputMode == MEM_NAME_INPUT_DIGIT)
        {
            if (Key <= KEY_9)
            {
                edit[edit_index] = '0' + Key - KEY_0;
                edit_index++;
                if (edit_index > 10)
                    edit_index = 10;
            }
            gCNCandidateCount = 0;
            gCNCandidateOffset = 0;
            gCNCandidateTotal = 0;
            gMemNameCandidateCount = 0;
            gRequestDisplayScreen = DISPLAY_MENU;
            return;
        }

        // Lowercase/Uppercase letter mode
        if (gMemNameInputMode == MEM_NAME_INPUT_LOWER ||
            gMemNameInputMode == MEM_NAME_INPUT_UPPER)
        {
            // Keys 1-4: select letter candidate
            if (Key >= KEY_1 && Key <= KEY_4 && gMemNameCandidateCount > 0)
            {
                const uint8_t idx = (uint8_t)(Key - KEY_1);
                if (idx < gMemNameCandidateCount)
                {
                    edit[edit_index] = gMemNameCandidates[idx];
                    edit_index++;
                    if (edit_index > 10)
                        edit_index = 10;
                    gMemNameCandidateCount = 0;
                    gRequestDisplayScreen = DISPLAY_MENU;
                    return;
                }
            }
            // Keys 2-9: show letter candidates
            if (Key >= KEY_2 && Key <= KEY_9)
            {
                MENU_BuildMemNameCandidatesFromKey(Key);
                gRequestDisplayScreen = DISPLAY_MENU;
                return;
            }
            gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
            return;
        }

        // Pinyin mode: 2-9 show letters below; 1-4 pick letter; 0 backspace or dismiss row;
        //               MENU then lookup; 1-6 pick Chinese (7-9 invalid while candidates shown)
        if (Key == KEY_0)
        {
            if (gCNCandidateCount > 0)
            {
                gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
                return;
            }
            if (gMemNameCandidateCount > 0)
            {
                gMemNameCandidateCount = 0;
                gRequestDisplayScreen = DISPLAY_MENU;
                return;
            }
            if (gPinyinLen > 0)
            {
                gPinyinLen--;
                gPinyinBuffer[gPinyinLen] = 0;
                gPinyinLookupNoMatch = 0;
                gRequestDisplayScreen = DISPLAY_MENU;
                return;
            }
            gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
            return;
        }

        if (Key <= KEY_9)
        {
            if (gCNCandidateCount > 0)
            {
                if (Key >= KEY_1 && Key <= KEY_6)
                {
                    const uint8_t pick_idx = (uint8_t)(Key - KEY_1);
                    if (pick_idx < gCNCandidateCount)
                    {
                        uint16_t unicode = gCNCandidates[pick_idx];
                        edit[edit_index]     = (char)(0xE0 | (unicode >> 12));
                        edit[edit_index + 1] = (char)(0x80 | ((unicode >> 6) & 0x3F));
                        edit[edit_index + 2] = (char)(0x80 | (unicode & 0x3F));
                        edit_index += 3;
                        if (edit_index > 10)
                            edit_index = 10;
                        MENU_PinyinReset();
                        gRequestDisplayScreen = DISPLAY_MENU;
                        return;
                    }
                }
                gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
                return;
            }

            if (gMemNameCandidateCount > 0)
            {
                if (Key >= KEY_1 && Key <= KEY_4)
                {
                    const uint8_t letter_idx = (uint8_t)(Key - KEY_1);
                    if (letter_idx < gMemNameCandidateCount && gPinyinLen < PINYIN_MAX_LEN)
                    {
                        gPinyinBuffer[gPinyinLen] = gMemNameCandidates[letter_idx];
                        gPinyinLen++;
                        gPinyinBuffer[gPinyinLen] = 0;
                        gMemNameCandidateCount = 0;
                        gPinyinLookupNoMatch = 0;
                        gRequestDisplayScreen = DISPLAY_MENU;
                        return;
                    }
                }
                if (Key >= KEY_2 && Key <= KEY_9)
                {
                    MENU_BuildMemNameCandidatesFromKey(Key);
                    gRequestDisplayScreen = DISPLAY_MENU;
                    return;
                }
                gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
                return;
            }

            if (Key >= KEY_2 && Key <= KEY_9)
            {
                MENU_BuildMemNameCandidatesFromKey(Key);
                gRequestDisplayScreen = DISPLAY_MENU;
                return;
            }

            gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
        }
        return;
    }
#endif

    INPUTBOX_Append(Key);

    gRequestDisplayScreen = DISPLAY_MENU;

    if (!gIsInSubMenu)
    {
        switch (gInputBoxIndex)
        {
            case 2:
                gInputBoxIndex = 0;

                Value = (gInputBox[0] * 10) + gInputBox[1];

                if (Value > 0 && Value <= MENU_GetActiveMenuCount())
                {
                    gMenuCursor         = Value - 1;
                    gFlagRefreshSetting = true;
                    return;
                }

                if (Value <= MENU_GetActiveMenuCount())
                    break;

                gInputBox[0]   = gInputBox[1];
                gInputBoxIndex = 1;
                [[fallthrough]];
            case 1:
                Value = gInputBox[0];
                if (Value > 0 && Value <= MENU_GetActiveMenuCount())
                {
                    gMenuCursor         = Value - 1;
                    gFlagRefreshSetting = true;
                    return;
                }
                break;
        }

        gInputBoxIndex = 0;

        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
        return;
    }

    if (UI_MENU_GetCurrentMenuId() == MENU_OFFSET)
    {
        uint32_t Frequency;

        if (gInputBoxIndex < 6)
        {   // invalid frequency
            #ifdef ENABLE_VOICE
                gAnotherVoiceID = (VOICE_ID_t)Key;
            #endif
            return;
        }

        #ifdef ENABLE_VOICE
            gAnotherVoiceID = (VOICE_ID_t)Key;
        #endif

        Frequency = StrToUL(INPUTBOX_GetAscii())*100;
        gSubMenuSelection = FREQUENCY_RoundToStep(Frequency, gTxVfo->StepFrequency);

        gInputBoxIndex = 0;
        return;
    }

    const int m = UI_MENU_GetCurrentMenuId();

    if (m == MENU_MEM_CH ||
        m == MENU_DEL_CH ||
        m == MENU_1_CALL ||
        m == MENU_S_PRI_CH_1 ||
        m == MENU_S_PRI_CH_2 ||
        m == MENU_MEM_NAME)
    {   // enter 4-digit channel number

        if (gInputBoxIndex < 4)
        {
            #ifdef ENABLE_VOICE
                gAnotherVoiceID   = (VOICE_ID_t)Key;
            #endif
            gRequestDisplayScreen = DISPLAY_MENU;
            return;
        }

        gInputBoxIndex = 0;

        //Value = ((gInputBox[0] * 100) + (gInputBox[1] * 10) + gInputBox[2]) - 1;
        Value = ((gInputBox[0] * 1000) + (gInputBox[1] * 100) + (gInputBox[2] * 10) + gInputBox[3]) - 1;

        if (IS_MR_CHANNEL(Value))
        {
            #ifdef ENABLE_VOICE
                gAnotherVoiceID = (VOICE_ID_t)Key;
            #endif
            gSubMenuSelection = Value;
            return;
        }

        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
        return;
    }

    if (MENU_GetLimits(UI_MENU_GetCurrentMenuId(), &Min, &Max))
    {
        gInputBoxIndex = 0;
        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
        return;
    }

    Offset = (Max >= 100) ? 3 : (Max >= 10) ? 2 : 1;

    /*
    switch (gInputBoxIndex)
    {
        case 1:
            Value = gInputBox[0];
            break;
        case 2:
            Value = (gInputBox[0] *  10) + gInputBox[1];
            break;
        case 3:
            Value = (gInputBox[0] * 100) + (gInputBox[1] * 10) + gInputBox[2];
            break;
    }
    */

    for (uint8_t i = 0; i < gInputBoxIndex; i++) {
        Value = (Value * 10) + gInputBox[i];
    }

    if (Offset == gInputBoxIndex)
        gInputBoxIndex = 0;

    if (Value <= Max)
    {
        gSubMenuSelection = Value;
        return;
    }

    gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
}

static void MENU_Key_EXIT(bool bKeyPressed, bool bKeyHeld)
{
    if (bKeyHeld || !bKeyPressed)
        return;

    gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

    if (!gCssBackgroundScan)
    {
        /* Backlight related menus set full brightness. Set it back to the configured value,
           just in case we are exiting from one of them. */
        BACKLIGHT_TurnOn();

        if (gIsInSubMenu)
        {
            // Handle channel name editing: delete character at current position, then move left
            if (UI_MENU_GetCurrentMenuId() == MENU_MEM_NAME && edit_index >= 0)
            {
                // Check if current position has a character to delete
                bool has_char_at_current = (edit[edit_index] != ' ' && edit[edit_index] != '_' && edit[edit_index] != '\0');
                
                if (has_char_at_current)
                {   // Delete character at current position
                    edit[edit_index] = '_';
                    gRequestDisplayScreen = DISPLAY_MENU;
                    return;
                }
                else if (edit_index > 0)
                {   // No char at current position, move left
                    edit_index--;
                    gRequestDisplayScreen = DISPLAY_MENU;
                    return;
                }
                else
                {   // At first position and no char - exit editing mode
                    edit_index = -1;
                    gIsInSubMenu = false;
                    gInputBoxIndex = 0;
                    gFlagRefreshSetting = true;
                    #ifdef ENABLE_VOICE
                        gAnotherVoiceID = VOICE_ID_CANCEL;
                    #endif
                    gRequestDisplayScreen = DISPLAY_MENU;
                    return;
                }
            }

#ifdef ENABLE_CHINESE
            // CN channel name editing: backspace
            if (UI_MENU_GetCurrentMenuId() == MENU_CN_MEM_NAME && edit_index >= 0)
            {
                // If letter candidates are visible, dismiss them
                if (gMemNameCandidateCount > 0)
                {
                    gMemNameCandidateCount = 0;
                    gRequestDisplayScreen = DISPLAY_MENU;
                    return;
                }

                // If Chinese candidates are visible, dismiss them (keep pinyin)
                if (gCNCandidateCount > 0)
                {
                    gCNCandidateCount = 0;
                    gCNCandidateOffset = 0;
                    gCNCandidateTotal = 0;
                    gRequestDisplayScreen = DISPLAY_MENU;
                    return;
                }

                // If pinyin is being typed, clear pinyin
                if (gPinyinLen > 0)
                {
                    MENU_PinyinReset();
                    gRequestDisplayScreen = DISPLAY_MENU;
                    return;
                }

                // EXIT: first clear current char, then move cursor back
                if (edit_index == 10)
                {
                    // Past end: move cursor back to last character
                    if (edit_index >= 3 &&
                        (uint8_t)edit[9] >= 0xE4 && (uint8_t)edit[9] <= 0xEF)
                        edit_index = 7;
                    else
                        edit_index = 9;
                    gRequestDisplayScreen = DISPLAY_MENU;
                    return;
                }
                if (edit_index >= 0 && edit_index < 10)
                {
                    uint8_t c = (uint8_t)edit[edit_index];
                    if (c != '_')
                    {
                        // Clear current character (Chinese: 3 bytes, ASCII: 1 byte)
                        if (c >= 0xE4 && c <= 0xEF)
                        {
                            edit[edit_index]     = '_';
                            edit[edit_index + 1] = '_';
                            edit[edit_index + 2] = '_';
                        }
                        else
                        {
                            edit[edit_index] = '_';
                        }
                        gRequestDisplayScreen = DISPLAY_MENU;
                        return;
                    }
                    else if (edit_index > 0)
                    {
                        // Current is '_', move cursor back
                        if (edit_index >= 3 &&
                            (uint8_t)edit[edit_index - 3] >= 0xE4 &&
                            (uint8_t)edit[edit_index - 3] <= 0xEF)
                            edit_index -= 3;
                        else
                            edit_index--;
                        gRequestDisplayScreen = DISPLAY_MENU;
                        return;
                    }
                    else
                    {   // At position 0 with '_' - exit editing mode
                        edit_index = -1;
                        gIsInSubMenu = false;
                        gInputBoxIndex = 0;
                        gFlagRefreshSetting = true;
                        gRequestDisplayScreen = DISPLAY_MENU;
                        return;
                    }
                }
            }
#endif

            if (gInputBoxIndex == 0 || UI_MENU_GetCurrentMenuId() != MENU_OFFSET)
            {
                gAskForConfirmation = 0;
                gIsInSubMenu        = false;
                gInputBoxIndex      = 0;
                gFlagRefreshSetting = true;

                #ifdef ENABLE_VOICE
                    gAnotherVoiceID = VOICE_ID_CANCEL;
                #endif
            }
            else
                gInputBox[--gInputBoxIndex] = 10;

            // ***********************

            gRequestDisplayScreen = DISPLAY_MENU;
            return;
        }

        if (gMenuMainPageActive)
        {
            MENU_RecordSelectionBeforeLeaveMenuToMain();
            gMenuMainPageActive    = false;
            gRequestDisplayScreen = DISPLAY_MAIN;
            return;
        }

        #ifdef ENABLE_VOICE
            gAnotherVoiceID = VOICE_ID_CANCEL;
        #endif

        MENU_RecordSelectionBeforeLeaveMenuToMain();
        // Keep level-1 selection in sync with the currently active icon group.
        gMenuMainPageLastValid = true;
        gMenuMainPageLastIconIndex = gMenuMainPageIconIndex;

        MENU_ActivateMainPage();
        gRequestDisplayScreen = DISPLAY_MENU;

        if (gEeprom.BACKLIGHT_TIME == 0) // backlight set to always off
        {
            BACKLIGHT_TurnOff();    // turn the backlight OFF
        }
    }
    else
    {
        MENU_StopCssScan();

        #ifdef ENABLE_VOICE
            gAnotherVoiceID   = VOICE_ID_SCANNING_STOP;
        #endif

        gRequestDisplayScreen = DISPLAY_MENU;
    }

    gPttWasReleased = true;
}

static void MENU_Key_MENU(const bool bKeyPressed, const bool bKeyHeld)
{
    if (bKeyHeld || !bKeyPressed)
        return;

    gBeepToPlay           = BEEP_1KHZ_60MS_OPTIONAL;
    gRequestDisplayScreen = DISPLAY_MENU;

    if (gMenuMainPageActive && !gIsInSubMenu)
    {
        const uint8_t icon_count = MENU_MainPageIconCount();
        const uint8_t icon_index = (icon_count > 0) ? ((uint8_t)gSubMenuSelection % icon_count) : 0;
        gMenuMainPageLastValid = true;
        gMenuMainPageLastIconIndex = icon_index;
        gMenuMainPageIconIndex = icon_index;
        MENU_UpdateMenuFilterForIcon(gMenuMainPageIconIndex);
        gMenuMainPageActive = false;
        gIsInSubMenu = false;
        gInputBoxIndex = 0;
        gAskForConfirmation = 0;
        gFlagRefreshSetting = true;
        if (gMenuMainPageIconIndex < ARRAY_SIZE(gMenuSecondPageLastValid) && gMenuSecondPageLastValid[gMenuMainPageIconIndex])
            gMenuCursor = UI_MENU_GetMenuIdx(gMenuSecondPageLastMenuId[gMenuMainPageIconIndex]);
        else
            gMenuCursor = 0;
        /* About (icon 4): list is only MENU_VOL (SysInf / firmware) — avoid stale cursor mapping to another group */
        if (gMenuMainPageIconIndex == 4u)
            gMenuCursor = 0;
        gRequestDisplayScreen = DISPLAY_MENU;
        return;
    }

    if (!gIsInSubMenu)
    {
        const int m = UI_MENU_GetCurrentMenuId();

        #ifdef ENABLE_VOICE
            if (m != MENU_SCR)
            {
                const uint8_t actual_idx = MENU_GetActualMenuIndexFromCursor(gMenuCursor);
                gAnotherVoiceID = VOICE_ID_MENU;
            }
        #endif
        if (m == MENU_UPCODE
            || m == MENU_DWCODE
            || m == MENU_PTT_ID
#ifdef ENABLE_DTMF_CALLING
            || m == MENU_ANI_ID
#endif
            )
            return;
        #if 1
            if (m == MENU_DEL_CH || m == MENU_MEM_NAME || m == MENU_CN_MEM_NAME)
                if (!RADIO_CheckValidChannel(gSubMenuSelection, false, 0))
                    return;  // invalid channel
        #endif

        gAskForConfirmation = 0;
        gIsInSubMenu        = true;

//      if (m != MENU_D_LIST)
        {
            gInputBoxIndex      = 0;
            edit_index          = -1;
            gMemNameCandidateCount = 0;
        }

        return;
    }

    if (UI_MENU_GetCurrentMenuId() == MENU_MEM_NAME)
    {
        if (edit_index < 0)
        {   // enter channel name edit mode
            if (!RADIO_CheckValidChannel(gSubMenuSelection, false, 0))
                return;

            SETTINGS_FetchChannelName(edit, gSubMenuSelection);

            // pad the channel name out with '_'
            edit_index = strlen(edit);
            while (edit_index < 10)
                edit[edit_index++] = '_';
            edit[edit_index] = 0;
            edit_index = 0;  // 'edit_index' is going to be used as the cursor position

            // make a copy so we can test for change when exiting the menu item
            memcpy(edit_original, edit, sizeof(edit_original));
            gMemNameInputMode = MEM_NAME_INPUT_LOWER;
            gMemNameCandidateCount = 0;
            gMemNameSymbolPage = 0;

            return;
        }
        else
        if (edit_index >= 0 && edit_index < 10)
        {   // editing the channel name characters

            if (++edit_index < 10)
                return; // next char

            // exit
            gFlagAcceptSetting  = false;
            gAskForConfirmation = 0;
            if (memcmp(edit_original, edit, sizeof(edit_original)) == 0) {
                // no change - drop it
                gIsInSubMenu = false;
            }
        }
    }

#ifdef ENABLE_CHINESE
    if (UI_MENU_GetCurrentMenuId() == MENU_CN_MEM_NAME)
    {
        if (edit_index < 0)
        {   // enter CN channel name edit mode
            if (!RADIO_CheckValidChannel(gSubMenuSelection, false, 0))
                return;

            SETTINGS_FetchCNChannelName(edit, gSubMenuSelection);

            // pad with '_' up to 10 bytes (max 3 Chinese chars = 9 bytes + 1 for alignment)
            edit_index = strlen(edit);
            while (edit_index < 10)
                edit[edit_index++] = '_';
            edit[edit_index] = 0;
            edit_index = 0;

            memcpy(edit_original, edit, sizeof(edit_original));
            gMemNameInputMode = MEM_NAME_INPUT_PINYIN;
            gCNCandidateCount = 0;
            gCNCandidateOffset = 0;
            gCNCandidateTotal = 0;
            gPinyinTimeout_500ms = 0;
            MENU_PinyinReset();

            return;
        }
        else if (edit_index >= 0)
        {
            if (gMemNameCandidateCount > 0)
            {
                gMemNameCandidateCount = 0;
                gRequestDisplayScreen = DISPLAY_MENU;
                return;
            }
            if (gPinyinLen > 0 && gCNCandidateCount > 0)
            {
                gCNCandidateCount = 0;
                gCNCandidateOffset = 0;
                gCNCandidateTotal = 0;
                gRequestDisplayScreen = DISPLAY_MENU;
                return;
            }
            if (gPinyinLen > 0 && gCNCandidateCount == 0)
            {
                gCNCandidateOffset = 0;
                MENU_PinyinSearch();
                gRequestDisplayScreen = DISPLAY_MENU;
                return;
            }
            if (gPinyinLen == 0 && gCNCandidateCount == 0)
            {
                // Pinyin empty, no candidates: advance cursor by one character
                if (edit_index < 10)
                {
                    uint8_t c = (uint8_t)edit[edit_index];
                    uint8_t cw = (c >= 0xE4 && c <= 0xEF) ? 3u : 1u;
                    edit_index += cw;

                    if (edit_index >= 10)
                    {
                        edit_index = 10;
                        gFlagAcceptSetting  = false;
                        gAskForConfirmation = 0;
                        if (memcmp(edit_original, edit, sizeof(edit_original)) == 0)
                            gIsInSubMenu = false;
                        // fall through to save flow below
                    }
                    else
                    {
                        gRequestDisplayScreen = DISPLAY_MENU;
                        return;
                    }
                }
            }
        }
    }
#endif

    // exiting the sub menu

    if (gIsInSubMenu)
    {
        const int m = UI_MENU_GetCurrentMenuId();

        if (m == MENU_RESET  ||
            m == MENU_MEM_CH ||
            m == MENU_DEL_CH ||
            m == MENU_MEM_NAME ||
            m == MENU_CN_MEM_NAME)
        {
            switch (gAskForConfirmation)
            {
                case 0:
                    gAskForConfirmation = 1;
                    UI_DisplayMenu();
                    break;

                case 1:
                    gAskForConfirmation = 2;

                    UI_DisplayMenu();

                    if (m == MENU_RESET)
                    {
                        #ifdef ENABLE_VOICE
                            AUDIO_SetVoiceID(0, VOICE_ID_CONFIRM);
                            AUDIO_PlaySingleVoice(true);
                        #endif

                        MENU_AcceptSetting();

                        #if defined(ENABLE_OVERLAY)
                            overlay_FLASH_RebootToBootloader();
                        #else
                            NVIC_SystemReset();
                        #endif
                    }

                    gFlagAcceptSetting  = true;
                    gIsInSubMenu        = false;
                    gAskForConfirmation = 0;
            }
        }
        else
        {
            gFlagAcceptSetting = true;
            gIsInSubMenu       = false;
        }
    }

    SCANNER_Stop();

    #ifdef ENABLE_VOICE
        if (UI_MENU_GetCurrentMenuId() == MENU_SCR)
            gAnotherVoiceID = (gSubMenuSelection == 0) ? VOICE_ID_SCRAMBLER_OFF : VOICE_ID_SCRAMBLER_ON;
        else
            gAnotherVoiceID = VOICE_ID_CONFIRM;
    #endif

    gInputBoxIndex = 0;
}

static void MENU_Key_STAR(const bool bKeyPressed, const bool bKeyHeld)
{
    if (bKeyHeld || !bKeyPressed)
        return;

    gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

    if (UI_MENU_GetCurrentMenuId() == MENU_MEM_NAME && edit_index >= 0)
    {   // currently editing the channel name
        if (edit_index < 10)
        {
            edit[edit_index] = '-';
            gMemNameCandidateCount = 0;
            gRequestDisplayScreen = DISPLAY_MENU;
        }

        return;
    }

    /* CN mode switch moved to KEY_F handler */

    RADIO_SelectVfos();

    #ifdef ENABLE_NOAA
        if (!IS_NOAA_CHANNEL(gRxVfo->CHANNEL_SAVE) && gRxVfo->Modulation == MODULATION_FM)
    #else
        if (gRxVfo->Modulation ==  MODULATION_FM)
    #endif
    {
        const int m = UI_MENU_GetCurrentMenuId();
        if ((m == MENU_R_CTCS || m == MENU_R_DCS) && gIsInSubMenu)
        {   // scan CTCSS or DCS to find the tone/code of the incoming signal
            if (!SCANNER_IsScanning())
                MENU_StartCssScan();
            else
                MENU_StopCssScan();
        }

        gPttWasReleased = true;
        return;
    }

    gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
}

static void MENU_Key_UP_DOWN(bool bKeyPressed, bool bKeyHeld, int8_t Direction)
{
    uint8_t VFO;
    uint16_t Channel;
    bool    bCheckScanList;

    if (!gEeprom.SET_NAV && gIsInSubMenu) {
        Direction = -Direction;
    }

#ifdef ENABLE_CHINESE
    if (UI_MENU_GetCurrentMenuId() == MENU_CN_MEM_NAME && gIsInSubMenu && edit_index >= 0)
    {
        // Pinyin mode: UP/DOWN pages through Chinese candidates
        if (gMemNameInputMode == MEM_NAME_INPUT_PINYIN &&
            gCNCandidateTotal > CN_CANDIDATE_MAX &&
            bKeyPressed && Direction != 0)
        {
            const uint8_t per_page = CN_CANDIDATE_MAX;
            const uint8_t pages = (uint8_t)((gCNCandidateTotal + per_page - 1u) / per_page);
            const uint8_t cur_page = (uint8_t)(gCNCandidateOffset / per_page);
            const uint8_t new_page = (uint8_t)NUMBER_AddWithWraparound(cur_page, Direction > 0 ? 1 : -1, 0, pages - 1u);
            gCNCandidateOffset = (uint8_t)(new_page * per_page);
            MENU_PinyinSearch();
            gRequestDisplayScreen = DISPLAY_MENU;
            return;
        }
    }
#endif

    if (UI_MENU_GetCurrentMenuId() == MENU_MEM_NAME && gIsInSubMenu && edit_index >= 0)
    {   // symbol mode uses up/down to page candidates
        if (gMemNameInputMode == MEM_NAME_INPUT_SYMBOL && bKeyPressed && Direction != 0)
        {
            static const uint8_t per_page = ARRAY_SIZE(gMemNameCandidates);
            static const uint8_t total_symbols = sizeof(".,!?@#$%&*+-/=_:;()[]{}<>\"'\\|~^`") - 1u;
            const uint8_t pages = (uint8_t)((total_symbols + per_page - 1u) / per_page);
            if (pages > 0u)
            {
                gMemNameSymbolPage = NUMBER_AddWithWraparound(gMemNameSymbolPage, Direction > 0 ? 1 : -1, 0, pages - 1u);
                MENU_BuildMemNameSymbolCandidates();
                gRequestDisplayScreen = DISPLAY_MENU;
            }
            return;
        }
        // keep old behavior as fallback
        if (bKeyPressed && edit_index < 10 && Direction != 0)
        {
            const char   unwanted[] = "$%&!\"':;?^`|{}";
            char         c          = edit[edit_index] + Direction;
            unsigned int i          = 0;
            while (i < sizeof(unwanted) && c >= 32 && c <= 126)
            {
                if (c == unwanted[i++])
                {   // choose next character
                    c += Direction;
                    i = 0;
                }
            }
            edit[edit_index] = (c < 32) ? 126 : (c > 126) ? 32 : c;

            gRequestDisplayScreen = DISPLAY_MENU;
        }
        return;
    }

    if (!bKeyHeld)
    {
        if (!bKeyPressed)
            return;

        gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;

        gInputBoxIndex = 0;
    }
    else
    if (!bKeyPressed)
        return;

    if (SCANNER_IsScanning()) {
        return;
    }

    if (!gIsInSubMenu)
    {
        if (gMenuMainPageActive)
        {
            const uint8_t icon_count = MENU_MainPageIconCount();
            gSubMenuSelection = NUMBER_AddWithWraparound(gSubMenuSelection, -Direction, 0, icon_count > 0 ? (icon_count - 1) : 0);
            gRequestDisplayScreen = DISPLAY_MENU;
            return;
        }

        const uint8_t menu_count = MENU_GetActiveMenuCount();
        gMenuCursor = NUMBER_AddWithWraparound(gMenuCursor, -Direction, 0, menu_count > 0 ? (menu_count - 1) : 0);

        gFlagRefreshSetting = true;

        gRequestDisplayScreen = DISPLAY_MENU;

        const int m = UI_MENU_GetCurrentMenuId();

        if (m != MENU_ABR 
            && m != MENU_ABR_MIN 
            && m != MENU_ABR_MAX 
            && gEeprom.BACKLIGHT_TIME == 0) // backlight always off and not in the backlight menu
        {
            BACKLIGHT_TurnOff();
        }

        return;
    }

    if (UI_MENU_GetCurrentMenuId() == MENU_OFFSET)
    {
        int32_t Offset = (Direction * gTxVfo->StepFrequency) + gSubMenuSelection;
        if (Offset < 99999990)
        {
            if (Offset < 0)
                Offset = 99999990;
        }
        else
            Offset = 0;

        gSubMenuSelection     = FREQUENCY_RoundToStep(Offset, gTxVfo->StepFrequency);
        gRequestDisplayScreen = DISPLAY_MENU;
        return;
    }

    VFO = 0;
    
    const int m = UI_MENU_GetCurrentMenuId();

    switch (m)
    {
        case MENU_DEL_CH:
        case MENU_1_CALL:
        case MENU_S_PRI_CH_1:
        case MENU_S_PRI_CH_2:
        case MENU_MEM_NAME:
        case MENU_CN_MEM_NAME:
            bCheckScanList = false;
            break;

        default:
            MENU_ClampSelection(Direction);
            gRequestDisplayScreen = DISPLAY_MENU;
            return;
    }

    if(m == MENU_S_PRI_CH_1 || m == MENU_S_PRI_CH_2)
    {
        static int16_t last;

        if(gSubMenuSelection == MR_CHANNELS_MAX) {
            if(Direction > 0)
            {
                gSubMenuSelection = -1;
                last = -1;
            }
            else if(Direction < 0)
            {
                gSubMenuSelection = MR_CHANNELS_MAX;
                last = MR_CHANNELS_MAX;
            }
        }

        Channel = RADIO_FindNextChannel(gSubMenuSelection + Direction, Direction, bCheckScanList, VFO);
        if (Channel != 0xFFFF)
            gSubMenuSelection = Channel;

        if(Direction > 0 && gSubMenuSelection < last)
        {
            gSubMenuSelection = MR_CHANNELS_MAX;
        }
        else if(Direction < 0 && gSubMenuSelection > last)
        {
            gSubMenuSelection = MR_CHANNELS_MAX;           
        }
        else
        {
            last = Channel;
        }

        gRequestDisplayScreen = DISPLAY_MENU;
    }
    else
    {
        Channel = RADIO_FindNextChannel(gSubMenuSelection + Direction, Direction, bCheckScanList, VFO);
        if (Channel != 0xFFFF)
            gSubMenuSelection = Channel;

        gRequestDisplayScreen = DISPLAY_MENU;
    }
}

void MENU_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
    switch (Key)
    {
        case KEY_0...KEY_9:
            MENU_Key_0_to_9(Key, bKeyPressed, bKeyHeld);
            break;
        case KEY_MENU:
            MENU_Key_MENU(bKeyPressed, bKeyHeld);
            break;
        case KEY_UP:
        case KEY_DOWN:
            MENU_Key_UP_DOWN(bKeyPressed, bKeyHeld, Key == KEY_UP ? 1 : -1);
            break;
        case KEY_EXIT:
            MENU_Key_EXIT(bKeyPressed, bKeyHeld);
            break;
        case KEY_STAR:
            MENU_Key_STAR(bKeyPressed, bKeyHeld);
            break;
        case KEY_F:
            if (UI_MENU_GetCurrentMenuId() == MENU_MEM_NAME && edit_index >= 0)
            {   // currently editing the channel name
                if (!bKeyHeld && bKeyPressed)
                {
                    gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
                    gMemNameInputMode = (uint8_t)((gMemNameInputMode + 1u) % 4u);
                    gMemNameCandidateCount = 0;
                    if (gMemNameInputMode == MEM_NAME_INPUT_SYMBOL)
                        MENU_BuildMemNameSymbolCandidates();
                    gRequestDisplayScreen = DISPLAY_MENU;
                }
                break;
            }

#ifdef ENABLE_CHINESE
            if (UI_MENU_GetCurrentMenuId() == MENU_CN_MEM_NAME && edit_index >= 0)
            {   // # key: cycle through pinyin → digit → lower → upper → pinyin
                if (!bKeyHeld && bKeyPressed)
                {
                    gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
                    MENU_PinyinReset();
                    gMemNameCandidateCount = 0;
                    switch (gMemNameInputMode) {
                        case MEM_NAME_INPUT_PINYIN: gMemNameInputMode = MEM_NAME_INPUT_DIGIT;  break;
                        case MEM_NAME_INPUT_DIGIT:  gMemNameInputMode = MEM_NAME_INPUT_LOWER;  break;
                        case MEM_NAME_INPUT_LOWER:  gMemNameInputMode = MEM_NAME_INPUT_UPPER;  break;
                        default:                    gMemNameInputMode = MEM_NAME_INPUT_PINYIN; break;
                    }
                    gRequestDisplayScreen = DISPLAY_MENU;
                }
                break;
            }
#endif

            GENERIC_Key_F(bKeyPressed, bKeyHeld);
            break;
        case KEY_PTT:
            GENERIC_Key_PTT(bKeyPressed);
            break;
        default:
            if (!bKeyHeld && bKeyPressed)
                gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
            break;
    }

    if (gScreenToDisplay == DISPLAY_MENU)
    {
        const int m = UI_MENU_GetCurrentMenuId();

        if (m == MENU_VOL ||
            #ifdef ENABLE_F_CAL_MENU
                m == MENU_F_CALI ||
            #endif
            m == MENU_BATCAL)
        {
            gMenuCountdown = menu_timeout_long_500ms;
        }
        else
        {
            gMenuCountdown = menu_timeout_500ms;
        }
    }
}
