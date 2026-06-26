#include "app/mokuyu.h"
#include "app/mokuyu_bitmap.h"

#ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
#include "screenshot.h"
#endif

static bool isInitialized;
static uint32_t count;
static int8_t plusOneY;
static uint8_t plusOneAlpha;
static KeyboardState kbd;

static void DrawCounter(void)
{
    char str[12];
    sprintf(str, "%lu", (unsigned long)count);
    uint8_t len = strlen(str);
    uint8_t totalWidth = len * 5;
    uint8_t startX = (LCD_WIDTH - totalWidth) / 2 - 40;
    UI_PrintStringSmallBold(str, startX, LCD_WIDTH - 1, 6);
}

static void DrawBottomLabels(void)
{
    GUI_DisplaySmallest("Add", 0, 49, false, true);
    GUI_DisplaySmallest("Reset/Exit", 87, 49, false, true);
}

static void DrawPlusOneAnim(void)
{
    if (plusOneAlpha == 0)
        return;

    static const uint8_t font_plus[7] = {
        0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00
    };
    static const uint8_t font_one[7] = {
        0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E
    };

    int baseX = 90;
    int baseY = plusOneY;

    const uint8_t *chars[2] = { font_plus, font_one };

    for (int ci = 0; ci < 2; ci++) {
        int cx = baseX + ci * 10;
        for (int cy = 0; cy < 7; cy++) {
            uint8_t row = chars[ci][cy];
            for (int bx = 0; bx < 5; bx++) {
                if (row & (1 << (4 - bx))) {
                    for (int dy = 0; dy < 2; dy++) {
                        for (int dx = 0; dx < 2; dx++) {
                            int px = cx + bx * 2 + dx;
                            int py = baseY + cy * 2 + dy;
                            if (px >= 0 && px < LCD_WIDTH && py >= 0 && py < LCD_HEIGHT) {
                                UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)px, (uint8_t)py, true);
                            }
                        }
                    }
                }
            }
        }
    }
}

static bool HandleInput(void)
{
    kbd.prev = kbd.current;
    kbd.current = KEYBOARD_GetKey();

    if (kbd.current == KEY_EXIT) {
        kbd.counter++;
        if (kbd.counter > 20) {
            isInitialized = false;
            return false;
        }
    } else if (kbd.prev == KEY_EXIT && kbd.counter > 0 && kbd.counter <= 20) {
        count = 0;
        plusOneY = 10;
        plusOneAlpha = 0;
        kbd.counter = 0;
    } else {
        kbd.counter = 0;
    }

    if (kbd.current == KEY_INVALID)
        return true;

    if (kbd.current != kbd.prev) {
        switch (kbd.current) {
        case KEY_MENU:
            count++;
            plusOneY = 10;
            plusOneAlpha = 255;
            BK4819_PlayTone(1200, true);
            AUDIO_AudioPathOn();
            BK4819_ExitTxMute();
            SYSTEM_DelayMs(100);
            BK4819_EnterTxMute();
            AUDIO_AudioPathOff();
            BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2_GREEN, true);
            SYSTEM_DelayMs(80);
            BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2_GREEN, false);
            break;
        default:
            break;
        }
    }

    return true;
}

bool APP_IsMokuyuActive(void)
{
    return isInitialized;
}

void APP_RunMokuyu(void)
{
    BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2_GREEN, false);
    BACKLIGHT_UpdateTickless();

    UI_DisplayClear();
    memset(gStatusLine, 0, sizeof(gStatusLine));

    count = 0;
    plusOneY = 10;
    plusOneAlpha = 0;
    isInitialized = true;
    kbd.current = KEY_INVALID;
    kbd.prev = KEY_INVALID;
    kbd.counter = 0;

    DrawCounter();
    DrawBottomLabels();

    // Wait for the MENU key that entered this page to be released
    while (KEYBOARD_GetKey() != KEY_INVALID) {
        SYSTEM_DelayMs(10);
    }
    SYSTEM_DelayMs(100);

    while (isInitialized) {
        #ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
            SCREENSHOT_ParseInput();
        #endif
        HandleInput();

        if (plusOneAlpha > 0) {
            plusOneY -= 1;
            if (plusOneAlpha > 8)
                plusOneAlpha -= 8;
            else
                plusOneAlpha = 0;
        }

        for (int row = 0; row < FRAME_LINES; row++) {
            memset(gFrameBuffer[row], 0, LCD_WIDTH);
        }
        memset(gStatusLine, 0, sizeof(gStatusLine));

        for (int row = 0; row < 6; row++) {
            for (int col = 0; col < LCD_WIDTH; col++) {
                if (BITMAP_MOKUYU[row][col]) {
                    gFrameBuffer[row][col] |= BITMAP_MOKUYU[row][col];
                }
            }
        }

        DrawCounter();
        DrawBottomLabels();
        DrawPlusOneAnim();

        ST7565_BlitStatusLine();
        ST7565_BlitFullScreen();

        #ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
            SCREENSHOT_Update(false);
        #endif

        SYSTEM_DelayMs(40);
    }
}
