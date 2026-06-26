#include "app/toolbox_menu.h"
#include "app/mokuyu.h"
#include "app/cw.h"
#include "keyboard_state.h"
#include "../driver/bk4819.h"
#include "../driver/gpio.h"
#include "../audio.h"
#include "../font.h"
#include "../ui/helper.h"
#include <string.h>

static bool isInitialized;
static uint8_t selectedIndex;
static KeyboardState kbd;

static void DrawMenu(void)
{
    // Clear screen
    for (int row = 0; row < FRAME_LINES; row++) {
        memset(gFrameBuffer[row], 0, LCD_WIDTH);
    }
    memset(gStatusLine, 0, sizeof(gStatusLine));

    // Title "工具箱" at row 0, centered
    UI_PrintStringSmallAtPixel("工具箱", 51, 86, 0, 11, 0);

    // Separator line at y=13
    for (int x = 0; x < LCD_WIDTH; x++) {
        UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)x, 13, true);
    }

    // Menu items - shifted down 3 pixels from row positions
    const char *items[] = {"电子木鱼", "cw"};
    const uint8_t row_start[] = {3, 5};

    for (int i = 0; i < 2; i++) {
        uint8_t row = row_start[i];
        UI_PrintStringSmallAtPixel(items[i], 0, 127, row * 8 + 3, row * 8 + 14, 0);
        if (i == selectedIndex) {
            // Invert all framebuffer lines the text spans: black background + white text
            uint8_t fb_start = (row * 8 >= 8) ? (row * 8 - 8) / 8 : 0;
            uint8_t fb_end = (row * 8 + 11 - 8) / 8;
            for (uint8_t r = fb_start; r <= fb_end; r++) {
                for (int x = 0; x < LCD_WIDTH; x++) {
                    gFrameBuffer[r][x] ^= 0xFF;
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
    } else if (kbd.prev == KEY_EXIT && kbd.counter > 0) {
        // Short press EXIT - exit menu
        isInitialized = false;
        return false;
    } else {
        kbd.counter = 0;
    }

    if (kbd.current == KEY_INVALID)
        return true;

    if (kbd.current != kbd.prev) {
        switch (kbd.current) {
        case KEY_UP:
            // Move up
            if (selectedIndex > 0)
                selectedIndex--;
            break;
        case KEY_DOWN:
            // Move down
            if (selectedIndex < 1)
                selectedIndex++;
            break;
        case KEY_MENU:
            // Enter selected item
            if (selectedIndex == 0) {
                APP_RunMokuyu();
            } else if (selectedIndex == 1) {
                APP_RunCW();
            }
            break;
        default:
            break;
        }
    }

    return true;
}

void APP_RunToolboxMenu(void)
{
    BACKLIGHT_UpdateTickless();

    selectedIndex = 0;
    isInitialized = true;
    kbd.current = KEY_INVALID;
    kbd.prev = KEY_INVALID;
    kbd.counter = 0;

    DrawMenu();
    ST7565_BlitStatusLine();
    ST7565_BlitFullScreen();

    while (isInitialized) {
        HandleInput();
        DrawMenu();
        ST7565_BlitStatusLine();
        ST7565_BlitFullScreen();
        SYSTEM_DelayMs(40);
    }
}