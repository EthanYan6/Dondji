#ifdef ENABLE_FLASHLIGHT

#include "driver/gpio.h"
#include "driver/bk4819.h"
#include "driver/keyboard.h"
#include "app/flashlight.h"
#include "functions.h"
#include "misc.h"
#include <stdbool.h>

static inline void Flashlight_TurnOn(){ GPIO_SetOutputPin(GPIO_PIN_FLASHLIGHT); }
static inline void Flashlight_TurnOff(){ GPIO_ResetOutputPin(GPIO_PIN_FLASHLIGHT); }
static inline void Flashlight_Toggle(){ GPIO_TogglePin(GPIO_PIN_FLASHLIGHT); }

static bool flash_on;
static bool s_breath_led_on; /* 飞机灯 ON 相位；双守轮询勿灭此灯 */

/* 飞机灯：无信号时红+绿同闪成黄色，节奏与休眠红灯一致（500ms 一拍，4 拍中亮 1 拍） */
static void AirplaneLight_Set(bool on)
{
    BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, on);
    BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2_GREEN, on);
}

bool Flashlight_BreathLedsOn(void)
{
    return s_breath_led_on
        && gSetting_breath_led
        && gCurrentFunction != FUNCTION_TRANSMIT
        && !g_SquelchLost
        && !flash_on;
}

static void AirplaneLight_Clear(bool led_on_by_us)
{
    if (!led_on_by_us)
        return;
    /* 发射中红灯归 TX 逻辑，不能灭 */
    if (gCurrentFunction != FUNCTION_TRANSMIT)
        BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, false);
    if (!g_SquelchLost)
        BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2_GREEN, false);
}

void Flashlight_BreathTick(void)
{
    static uint8_t counter;
    static uint8_t key_cooldown;
    static uint8_t last_beat;

    if (!gSetting_breath_led) {
        AirplaneLight_Clear(s_breath_led_on);
        s_breath_led_on = false;
        return;
    }

    if (gKeyReading0 != KEY_INVALID || gKeyReading1 != KEY_INVALID) {
        key_cooldown = 100; /* 松手后再停 1s */
        AirplaneLight_Clear(s_breath_led_on);
        s_breath_led_on = false;
        counter = 0;
        last_beat = gBreathBeat500ms;
        return;
    }
    if (key_cooldown > 0) {
        key_cooldown--;
        return;
    }

#ifdef ENABLE_FEAT_F4HWN_SLEEP
    /* 休眠红灯由 APP_TimeSlice500ms 控制，这里只收尾绿灯，不动红灯 */
    if (gWakeUp) {
        if (s_breath_led_on)
            BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2_GREEN, false);
        s_breath_led_on = false;
        counter = 0;
        return;
    }
#endif

    if (gCurrentFunction == FUNCTION_TRANSMIT || g_SquelchLost || flash_on) {
        AirplaneLight_Clear(s_breath_led_on);
        s_breath_led_on = false;
        counter = 0;
        return;
    }

    /* 用 ISR 实时 500ms 节拍（与休眠红灯同源），避免单守/双守主循环负载不同导致频率漂移 */
    if (gBreathBeat500ms != last_beat) {
        last_beat = gBreathBeat500ms;
        counter = (uint8_t)((counter + 1u) & 3u);
        s_breath_led_on = (counter == 0);
        AirplaneLight_Set(s_breath_led_on);
    }
}

#if !defined(ENABLE_FEAT_F4HWN) || defined(ENABLE_FEAT_F4HWN_RESCUE_OPS)
    enum FlashlightMode_t  gFlashLightState;

    void FlashlightTimeSlice()
    {
        if (gFlashLightState == FLASHLIGHT_BLINK && (gFlashLightBlinkCounter & 15u) == 0) {
            Flashlight_Toggle();
            return;
        }

        if (gFlashLightState == FLASHLIGHT_SOS) {
            const uint16_t u = 15;
            static uint8_t c;
            static uint16_t next;

            if (gFlashLightBlinkCounter - next > 7 * u) {
                c = 0;
                next = gFlashLightBlinkCounter + 1;
                return;
            }

            if (gFlashLightBlinkCounter == next) {
                if (c==0) {
                    Flashlight_TurnOff();
                } else {
                    Flashlight_Toggle();
                }

                if (c >= 18) {
                    next = gFlashLightBlinkCounter + 7 * u;
                    c = 0;
                } else if(c==7 || c==9 || c==11) {
                    next = gFlashLightBlinkCounter + 3 * u;
                } else {
                    next = gFlashLightBlinkCounter + u;
                }
                c++;
            }
        }
    }

    void ACTION_FlashLight(void)
    {
        if(gFlashLightState == FLASHLIGHT_OFF) {
            Flashlight_TurnOn();
        }
        else if (gFlashLightState == FLASHLIGHT_SOS) {
            Flashlight_TurnOff();
        }

        gFlashLightState = (gFlashLightState + 1) % 4;
        flash_on = (gFlashLightState != FLASHLIGHT_OFF);
    }
#else
    void ACTION_FlashLight(void)
    {
        static bool gFlashLightState = false;

        if(gFlashLightState)
        {
            Flashlight_TurnOff();
        }
        else
        {
            Flashlight_TurnOn();
        }

        gFlashLightState = (gFlashLightState) ? false : true;
        flash_on = gFlashLightState;
    }
#endif
#endif
