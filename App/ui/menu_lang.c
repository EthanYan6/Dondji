/* UTF-8 menu titles when UI language is Chinese */

#include <stddef.h>

#include "settings.h"
#include "ui/menu.h"

const char *UI_MENU_GetMenuTitle(const t_menu_item *item)
{
    if (item == NULL)
        return "";
    if (gUiLanguage != UI_LANGUAGE_CN)
        return item->name;

    switch (item->menu_id)
    {
    case MENU_STEP:         return "\xe6\xad\xa5\xe8\xbf\x9b";
    case MENU_TXP:          return "\xe5\x8a\x9f\xe7\x8e\x87";
    case MENU_R_DCS:        return "接收数字亚音";
    case MENU_R_CTCS:       return "接收模拟亚音";
    case MENU_T_DCS:        return "发射数字亚音";
    case MENU_T_CTCS:       return "发射模拟亚音";
    case MENU_SFT_D:        return "频差方向";
    case MENU_OFFSET:       return "频差频率";
    case MENU_W_N:          return "\xe5\xae\xbd\xe7\xaa\x84\xe5\xb8\xa6";
#ifndef ENABLE_FEAT_F4HWN
    case MENU_SCR:          return "\xe5\x8a\xa0\xe5\xaf\x86";
#endif
    case MENU_BCL:          return "\xe7\xb9\x81\xe5\xbf\x99\xe9\x94\x81\xe5\xae\x9a";
    case MENU_COMPAND:      return "\xe8\xaf\xad\xe9\x9f\xb3\xe5\x8e\x8b\xe6\x89\xa9";
    case MENU_AM:           return "\xe8\xb0\x83\xe5\x88\xb6\xe6\xa8\xa1\xe5\xbc\x8f";
#ifdef ENABLE_FEAT_F4HWN
    case MENU_TX_LOCK:      return "\xe6\xae\xb5\xe5\xa4\x96\xe5\x8f\x91\xe5\xb0\x84\xe9\x94\x81";
#endif
    case MENU_LIST_CH:      return "\xe4\xbf\xa1\xe9\x81\x93\xe5\x88\x97\xe8\xa1\xa8";
    case MENU_MEM_CH:       return "\xe5\xad\x98\xe4\xbf\xa1\xe9\x81\x93";
    case MENU_DEL_CH:       return "\xe5\x88\xa0\xe4\xbf\xa1\xe9\x81\x93";
    case MENU_MEM_NAME:     return "\xe5\x91\xbd\xe5\x90\x8d\xe4\xbf\xa1\xe9\x81\x93";
    case MENU_CN_MEM_NAME:  return "\xe5\x91\xbd\xe5\x90\x8d\xe4\xbf\xa1\xe9\x81\x93";
    case MENU_S_LIST:       return "\xe6\x89\xab\xe6\x8f\x8f\xe5\x88\x97\xe8\xa1\xa8";
    case MENU_S_PRI:        return "\xe4\xbc\x98\xe5\x85\x88\xe6\x89\xab\xe6\x8f\x8f";
    case MENU_S_PRI_CH_1:   return "\xe4\xbc\x98\xe5\x85\x88\xe4\xbf\xa1\xe9\x81\x93""1";
    case MENU_S_PRI_CH_2:   return "\xe4\xbc\x98\xe5\x85\x88\xe4\xbf\xa1\xe9\x81\x93""2";
    case MENU_SC_REV:       return "\xe6\x89\xab\xe6\x8f\x8f\xe6\x81\xa2\xe5\xa4\x8d";
#ifndef ENABLE_FEAT_F4HWN
    #ifdef ENABLE_NOAA
    case MENU_NOAA_S:       return "NOAA-S";
    #endif
#endif
    case MENU_F1SHRT:       return "\xe4\xbe\xa7\xe9\x94\xae""1\xe7\x9f\xad\xe6\x8c\x89";
    case MENU_F1LONG:       return "\xe4\xbe\xa7\xe9\x94\xae""1\xe9\x95\xbf\xe6\x8c\x89";
    case MENU_F2SHRT:       return "\xe4\xbe\xa7\xe9\x94\xae""2\xe7\x9f\xad\xe6\x8c\x89";
    case MENU_F2LONG:       return "\xe4\xbe\xa7\xe9\x94\xae""2\xe9\x95\xbf\xe6\x8c\x89";
    case MENU_MLONG:        return "MENU长按";
    case MENU_AUTOLK:       return "\xe8\x87\xaa\xe5\x8a\xa8\xe9\x94\x81\xe9\x94\xae";
    case MENU_LANGUAGE:     return "\xe6\x98\xbe\xe7\xa4\xba\xe8\xaf\xad\xe8\xa8\x80";
    case MENU_TOT:          return "\xe5\x8f\x91\xe5\xb0\x84\xe9\x99\x90\xe6\x97\xb6";
    case MENU_SAVE:         return "省电模式";
    case MENU_BAT_TXT:      return "\xe7\x94\xb5\xe6\xb1\xa0\xe6\x98\xbe\xe7\xa4\xba";
    case MENU_MIC:          return "\xe9\xba\xa6\xe5\x85\x8b\xe9\xa3\x8e\xe5\xa2\x9e\xe7\x9b\x8a"; /* 麦克风增益 — SPI 字库 */
    case MENU_MDF:          return "\xe4\xbf\xa1\xe9\x81\x93\xe6\x98\xbe\xe7\xa4\xba";
    case MENU_PONMSG:       return "\xe5\xbc\x80\xe6\x9c\xba\xe7\x94\xbb\xe9\x9d\xa2";
    case MENU_BOOT_HINT:    return "\xe5\xbc\x80\xe6\x9c\xba\xe6\x8f\x90\xe7\xa4\xba";
    case MENU_BOOT_SOUND:   return "\xe5\xbc\x80\xe6\x9c\xba\xe9\x9f\xb3\xe6\x95\x88";
    case MENU_ABR:          return "\xe8\x83\x8c\xe5\x85\x89\xe6\x97\xb6\xe9\x97\xb4";
    case MENU_ABR_MIN:      return "\xe8\x83\x8c\xe5\x85\x89\xe6\x9c\x80\xe6\x9a\x97";
    case MENU_ABR_MAX:      return "\xe8\x83\x8c\xe5\x85\x89\xe6\x9c\x80\xe4\xba\xae";
    case MENU_ABR_ON_TX_RX: return "\xe6\x94\xb6\xe5\x8f\x91\xe8\x83\x8c\xe5\x85\x89";
    case MENU_BEEP:         return "\xe6\x8c\x89\xe9\x94\xae\xe9\x9f\xb3";
#ifdef ENABLE_VOICE
    case MENU_VOICE:        return "\xe8\xaf\xad\xe9\x9f\xb3";
#endif
    case MENU_ROGER:        return "\xe5\x8f\x91\xe5\xb0\x84\xe5\xb0\xbe\xe9\x9f\xb3";
    case MENU_STE:          return "\xe5\xb0\xbe\xe9\x9f\xb3\xe6\xb6\x88\xe9\x99\xa4";
    case MENU_RP_STE:       return "过中继尾音消除";
    case MENU_1_CALL:       return "按键即呼";
#ifdef ENABLE_ALARM
    case MENU_AL_MOD:       return "\xe6\x8a\xa5\xe8\xad\xa6";
#endif
#ifdef ENABLE_DTMF_CALLING
    case MENU_ANI_ID:       return "ANI ID";
#endif
    case MENU_UPCODE:       return "\xe4\xb8\x8a\xe8\xa1\x8c\xe7\xa0\x81";
    case MENU_DWCODE:       return "\xe4\xb8\x8b\xe8\xa1\x8c\xe7\xa0\x81";
    case MENU_PTT_ID:       return "PTT ID";
    case MENU_D_ST:         return "DTMF ST";
#ifdef ENABLE_DTMF_CALLING
    case MENU_D_RSP:        return "DTMF\xe5\x93\x8d\xe5\xba\x94";
    case MENU_D_HOLD:       return "DTMF\xe4\xbf\x9d\xe6\x8c\x81";
#endif
    case MENU_D_PRE:        return "DTMF\xe5\x89\x8d\xe5\xaf\xbc";
#ifdef ENABLE_DTMF_CALLING
    case MENU_D_DCD:        return "DTMF\xe8\xa7\xa3\xe7\xa0\x81";
    case MENU_D_LIST:       return "DTMF\xe8\x81\x94\xe7\xb3\xbb\xe4\xba\xba";
#endif
    case MENU_D_LIVE_DEC:   return "DTMF\xe7\x9b\xb4\xe8\xa7\xa3";
#ifndef ENABLE_FEAT_F4HWN
    #ifdef ENABLE_AM_FIX
    case MENU_AM_FIX:       return "AM Fix";
    #endif
#endif
    case MENU_VOX:          return "\xe5\xa3\xb0\xe6\x8e\xa7";
#ifdef ENABLE_FEAT_F4HWN
    case MENU_VOL:          return "\xe7\xb3\xbb\xe7\xbb\x9f\xe4\xbf\xa1\xe6\x81\xaf";
#else
    case MENU_VOL:          return "\xe7\x94\xb5\xe6\xb1\xa0\xe7\x94\xb5\xe5\x8e\x8b";
#endif
    case MENU_TDR:          return "\xe6\x8e\xa5\xe6\x94\xb6\xe6\xa8\xa1\xe5\xbc\x8f<\xe8\xae\xbe\xe4\xb8\xbb\xe9\xa1\xb5>";
    case MENU_SQL:          return "\xe9\x9d\x99\xe5\x99\xaa";
#ifdef ENABLE_FEAT_F4HWN
    case MENU_SET_PWR:      return "\xe8\xae\xbe\xe7\xbd\xae\xe5\x8a\x9f\xe7\x8e\x87";
    case MENU_SET_PTT:      return "\xe8\xae\xbe\xe7\xbd\xae""PTT";
    case MENU_SET_TOT:      return "\xe8\xae\xbe\xe7\xbd\xae""TOT";
    case MENU_SET_EOT:      return "\xe8\xae\xbe\xe7\xbd\xae""EOT";
#ifdef ENABLE_FEAT_F4HWN_CTR
    case MENU_SET_CTR:      return "\xe8\xae\xbe\xe7\xbd\xae\xe5\xaf\xb9\xe6\xaf\x94\xe5\xba\xa6";
#endif
    case MENU_SET_INV:      return "\xe8\xae\xbe\xe7\xbd\xae\xe5\x8f\x8d\xe8\x89\xb2";
    case MENU_SET_LCK:      return "\xe9\x94\x81\xe9\x94\xae\xe8\x8c\x83\xe5\x9b\xb4";
    case MENU_SET_MET:      return "\xe8\xae\xbe\xe7\xbd\xae\xe4\xbb\xaa\xe8\xa1\xa8";
    case MENU_SET_GUI:      return "\xe8\xae\xbe\xe7\xbd\xae""GUI";
    case MENU_SET_TMR:      return "\xe8\xae\xbe\xe7\xbd\xae\xe5\xae\x9a\xe6\x97\xb6";
#ifdef ENABLE_FEAT_F4HWN_SLEEP
    case MENU_SET_OFF:      return "\xe8\xbf\x9b\xe5\x85\xa5\xe4\xbc\x91\xe7\x9c\xa0";
#endif
#ifdef ENABLE_FEAT_F4HWN_NARROWER
    case MENU_SET_NFM:      return "\xe8\xae\xbe\xe7\xbd\xae\xe7\xaa\x84\xe5\xb8\xa6";
#endif
#ifdef ENABLE_FEAT_F4HWN_VOL
    case MENU_SET_VOL:      return "\xe8\xae\xbe\xe7\xbd\xae\xe9\x9f\xb3\xe9\x87\x8f";
#endif
#ifdef ENABLE_FEAT_F4HWN_RESCUE_OPS
    case MENU_SET_KEY:      return "\xe8\xae\xbe\xe7\xbd\xae\xe6\x8c\x89\xe9\x94\xae";
#endif
#ifdef ENABLE_NOAA
    case MENU_NOAA_S:       return "\xe8\xae\xbe""NWR";
#endif
#ifdef ENABLE_FEAT_F4HWN_AUDIO
    case MENU_SET_AUD:      return "\xe9\x9f\xb3\xe9\xa2\x91\xe9\xa2\x84\xe8\xae\xbe";
#endif
    case MENU_SET_NAV:      return "\xe5\xaf\xbc\xe8\x88\xaa\xe9\x94\xae";
#endif
    case MENU_RESET:        return "\xe6\x81\xa2\xe5\xa4\x8d\xe5\x87\xba\xe5\x8e\x82";
    case MENU_F_LOCK:       return "\xe9\x94\x81\xe5\xae\x9a\xe9\xa2\x91\xe6\xae\xb5";
#ifndef ENABLE_FEAT_F4HWN
    case MENU_200TX:        return "Tx200";
    case MENU_350TX:        return "Tx350";
    case MENU_500TX:        return "Tx500";
#endif
    case MENU_350EN:        return "350""\xe5\x90\xaf\xe7\x94\xa8";
#ifndef ENABLE_FEAT_F4HWN
    case MENU_SCREN:        return "\xe5\x8a\xa0\xe5\xaf\x86\xe5\x90\xaf\xe7\x94\xa8";
#endif
#ifdef ENABLE_F_CAL_MENU
    case MENU_F_CALI:       return "\xe9\xa2\x91\xe7\x8e\x87\xe6\xa0\xa1\xe5\x87\x86";
#endif
    case MENU_BATCAL:       return "\xe7\x94\xb5\xe6\xb1\xa0\xe6\xa0\xa1\xe5\x87\x86";
    case MENU_BATTYP:       return "\xe7\x94\xb5\xe6\xb1\xa0\xe7\xb1\xbb\xe5\x9e\x8b";
#ifdef ENABLE_AUDIO_BAR
    case MENU_MIC_BAR:      return "\xe5\x8f\x91\xe5\xb0\x84\xe6\x8f\x90\xe7\xa4\xba"; /* 发射提示 */
#endif
    default:
        return item->name;
    }
}
