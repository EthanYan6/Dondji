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

#ifndef APP_ACTION_H
#define APP_ACTION_H

#include "driver/keyboard.h"
#include "settings.h"

void ACTION_Power(void);
void ACTION_Monitor(void);
void ACTION_Scan(bool bRestart);
#ifdef ENABLE_VOX
    void ACTION_Vox(void);
#endif

#ifdef ENABLE_FMRADIO
    void ACTION_FM(void);
#endif
void ACTION_SwitchDemodul(void);

#ifdef ENABLE_BLMIN_TMP_OFF
    void ACTION_BlminTmpOff(void);
#endif

#ifdef ENABLE_FEAT_F4HWN
    void ACTION_RxMode(void);
    void ACTION_MainOnly(void);
    void ACTION_Ptt(void);
    void ACTION_SyncDualPttKeyActions(void);
    void ACTION_ClearSide1PttIfMainOnly(void);
    void ACTION_HandleSide1Ptt(bool bKeyPressed, bool bKeyHeld);
    void ACTION_DualPttStop(void);
    #define ACTION_IsMainOnlyMode() \
        (gEeprom.DUAL_WATCH == DUAL_WATCH_OFF && \
         gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF)
    #define ACTION_DualPttEnabled() \
        (!ACTION_IsMainOnlyMode() && \
         (gEeprom.KEY_1_SHORT_PRESS_ACTION == ACTION_OPT_PTT || \
          gEeprom.KEY_1_LONG_PRESS_ACTION  == ACTION_OPT_PTT || \
          gEeprom.KEY_2_SHORT_PRESS_ACTION == ACTION_OPT_PTT || \
          gEeprom.KEY_2_LONG_PRESS_ACTION  == ACTION_OPT_PTT))
    /* Side key that owns dual-PTT (whole key TX on bottom channel): 1 or 2; 0 if none. */
    #define ACTION_DualPttSideKey() \
        ((gEeprom.KEY_1_SHORT_PRESS_ACTION == ACTION_OPT_PTT || \
          gEeprom.KEY_1_LONG_PRESS_ACTION  == ACTION_OPT_PTT) ? 1u : \
         (gEeprom.KEY_2_SHORT_PRESS_ACTION == ACTION_OPT_PTT || \
          gEeprom.KEY_2_LONG_PRESS_ACTION  == ACTION_OPT_PTT) ? 2u : 0u)
    void ACTION_Wn(void);
    void ACTION_BackLightOnDemand(void);
    void ACTION_BackLight(void);
    void ACTION_Mute(void);
    #ifdef ENABLE_FEAT_F4HWN_AUDIO
        void ACTION_RxA(void);
    #endif
    #ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
        void ACTION_Power_High(void);
        void ACTION_Remove_Offset(void);
    #endif
#endif

void ACTION_Handle(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);

#endif
