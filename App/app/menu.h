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

#ifndef APP_MENU_H
#define APP_MENU_H

#include <stdbool.h>
#include "driver/keyboard.h"

#ifdef ENABLE_F_CAL_MENU
    void writeXtalFreqCal(const int32_t value, const bool update_eeprom);
#endif

extern uint8_t gUnlockAllTxConfCnt;
extern bool gMenuMainPageActive;
extern bool gMenuUseMainOnlyStatus;
extern uint8_t gMenuMainPageIconIndex;

int MENU_GetLimits(uint8_t menu_id, int32_t *pMin, int32_t *pMax);
void MENU_AcceptSetting(void);
void MENU_ShowCurrentSetting(void);
void MENU_StartCssScan(void);
void MENU_CssScanFound(void);
void MENU_StopCssScan(void);
void MENU_ActivateMainPage(void);
/** Open level-1 icon page from the main radio screen; always selects Channel (icon 0). */
void MENU_OpenFromMainScreen(void);
uint8_t MENU_MainPageIconCount(void);
uint8_t MENU_GetActiveMenuCount(void);
uint8_t MENU_GetActualMenuIndexFromCursor(uint8_t cursor);
bool MENU_IsMenuIdExcludedFromBrowse(uint8_t menu_id);
uint8_t MENU_GetVisibleCursorForActualIndex(uint8_t actual_menu_list_index);
void MENU_UpdateMenuFilterForIcon(uint8_t icon_index);
void MENU_RefreshIconFilterAfterRxModeChange(void);
void MENU_RecordSelectionBeforeLeaveMenuToMain(void);

void MENU_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);

#endif

