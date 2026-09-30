#ifndef APP_FLASHLIGHT_H
#define APP_FLASHLIGHT_H

#ifdef ENABLE_FLASHLIGHT

#include <stdint.h>
#include <stdbool.h>

#if !defined(ENABLE_FEAT_F4HWN) || defined(ENABLE_FEAT_F4HWN_RESCUE_OPS)
    enum FlashlightMode_t {
        FLASHLIGHT_OFF = 0,
        FLASHLIGHT_ON,
        FLASHLIGHT_BLINK,
        FLASHLIGHT_SOS
    };

    extern enum FlashlightMode_t gFlashLightState;
    extern volatile uint16_t     gFlashLightBlinkCounter;

    void FlashlightTimeSlice(void);
#endif
void ACTION_FlashLight(void);
void Flashlight_BreathTick(void);
/* 飞机灯当前是否占着红/绿 LED（ON 相位且非发射/收信号）；供 RADIO_SetupRegisters 跳过灭灯 */
bool Flashlight_BreathLedsOn(void);

#endif

#endif
