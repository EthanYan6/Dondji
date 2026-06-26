#include "app/cw.h"
#include "keyboard_state.h"
#include "../driver/bk4819.h"
#include "../driver/gpio.h"
#include "../driver/backlight.h"
#include "../driver/system.h"
#include "../audio.h"
#include "../font.h"
#include "../ui/helper.h"
#include <string.h>

#ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
#include "screenshot.h"
#endif

static bool isInitialized;
static KeyboardState kbd;

#define CW_CHAR_BUFFER_SIZE 8
static char cwCharBuffer[CW_CHAR_BUFFER_SIZE];
static uint8_t cwCharIndex;

#define CW_TEXT_BUFFER_SIZE 48
static char cwTextBuffer[CW_TEXT_BUFFER_SIZE];
static uint8_t cwTextIndex;

// Auto-confirm timeout (15 loops * 40ms = 600ms)
#define AUTO_CONFIRM_TIMEOUT 15
static uint8_t idleCounter;

// Compact morse encoding: 3 bits length (1-5 -> 0-4) + 5 bits pattern (0=dot, 1=dash)
// Formula: encoded = ((len-1) << 5) | pattern
// Pattern: read morse from left to right, . = 0, - = 1
static const uint8_t morseEncode[36] = {
    // Letters A-Z (index 0-25)
    0x21, // A: .-       (len=2, 01=1)
    0x68, // B: -...     (len=4, 1000=8)
    0x6A, // C: -.-.     (len=4, 1010=10)
    0x44, // D: -..      (len=3, 100=4)
    0x00, // E: .        (len=1, 0)
    0x62, // F: ..-.     (len=4, 0010=2)
    0x46, // G: --.      (len=3, 110=6)
    0x60, // H: ....     (len=4, 0000=0)
    0x20, // I: ..       (len=2, 00=0)
    0x67, // J: .---     (len=4, 0111=7)
    0x45, // K: -.-      (len=3, 101=5)
    0x64, // L: .-..     (len=4, 0100=4)
    0x23, // M: --       (len=2, 11=3)
    0x22, // N: -.       (len=2, 10=2)
    0x47, // O: ---      (len=3, 111=7)
    0x66, // P: .--.     (len=4, 0110=6)
    0x6D, // Q: --.-     (len=4, 1101=13)
    0x42, // R: .-.      (len=3, 010=2)
    0x40, // S: ...      (len=3, 000=0)
    0x01, // T: -        (len=1, 1)
    0x41, // U: ..-      (len=3, 001=1)
    0x61, // V: ...-     (len=4, 0001=1)
    0x43, // W: .--      (len=3, 011=3)
    0x69, // X: -..-     (len=4, 1001=9)
    0x6B, // Y: -.--     (len=4, 1011=11)
    0x6C, // Z: --..     (len=4, 1100=12)
    // Numbers 0-9 (index 26-35)
    0x9F, // 0: -----    (len=5, 11111=31)
    0x8F, // 1: .----    (len=5, 01111=15)
    0x87, // 2: ..---    (len=5, 00111=7)
    0x83, // 3: ...--    (len=5, 00011=3)
    0x81, // 4: ....-    (len=5, 00001=1)
    0x80, // 5: .....    (len=5, 00000=0)
    0x90, // 6: -....    (len=5, 10000=16)
    0x98, // 7: --...    (len=5, 11000=24)
    0x9C, // 8: ---..    (len=5, 11100=28)
    0x9E  // 9: ----.    (len=5, 11110=30)
};

static char DecodeMorse(const char *code)
{
    uint8_t len = strlen(code);
    if (len < 1 || len > 5) return '?';

    uint8_t pattern = 0;
    for (uint8_t i = 0; i < len; i++) {
        pattern <<= 1;
        if (code[i] == '-') pattern |= 1;
    }

    uint8_t encoded = ((len - 1) << 5) | pattern;

    for (uint8_t i = 0; i < 36; i++) {
        if (morseEncode[i] == encoded) {
            if (i < 26) return 'A' + i;
            else return '0' + (i - 26);
        }
    }
    return '?';
}

static void ConfirmChar(void)
{
    if (cwCharIndex > 0) {
        char decoded = DecodeMorse(cwCharBuffer);
        if (cwTextIndex < CW_TEXT_BUFFER_SIZE - 1) {
            cwTextBuffer[cwTextIndex++] = decoded;
            cwTextBuffer[cwTextIndex] = '\0';
        }
        cwCharIndex = 0;
        cwCharBuffer[0] = '\0';
    }
    idleCounter = 0;
}

#define MAX_DISPLAY_CHARS 18

static void DrawCW(void)
{
    for (int row = 0; row < FRAME_LINES; row++) {
        memset(gFrameBuffer[row], 0, LCD_WIDTH);
    }
    memset(gStatusLine, 0, sizeof(gStatusLine));

    // Title "CW" bold display at row 0
    UI_PrintStringSmallBold("CW", 0, 127, 0);

    // Separator line at y=8
    for (int x = 0; x < LCD_WIDTH; x++) {
        UI_DrawPixelBuffer(gFrameBuffer, (uint8_t)x, 8, true);
    }

    // Prompt line with small font
    GUI_DisplaySmallest("menu:. exit:-", 0, 12, false, true);

    // Current morse code (row 4) with normal font
    if (cwCharIndex > 0) {
        const char *s = cwCharBuffer;
        if (cwCharIndex > MAX_DISPLAY_CHARS)
            s += cwCharIndex - MAX_DISPLAY_CHARS;
        UI_PrintStringSmallNormalAt(s, 0, 26);
    }

    // Decoded text (row 5) with normal font
    if (cwTextIndex > 0) {
        const char *s = cwTextBuffer;
        if (cwTextIndex > MAX_DISPLAY_CHARS)
            s += cwTextIndex - MAX_DISPLAY_CHARS;
        UI_PrintStringSmallNormalAt(s, 0, 40);
    }
}

static void AppendCW(char c)
{
    if (cwCharIndex < CW_CHAR_BUFFER_SIZE - 1) {
        cwCharBuffer[cwCharIndex++] = c;
        cwCharBuffer[cwCharIndex] = '\0';
    }
    idleCounter = 0;  // Reset idle counter on input
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
        AppendCW('-');
        BK4819_PlayTone(800, true);
        AUDIO_AudioPathOn();
        BK4819_ExitTxMute();
        SYSTEM_DelayMs(150);
        BK4819_EnterTxMute();
        AUDIO_AudioPathOff();
        kbd.counter = 0;
    } else {
        kbd.counter = 0;
    }

    if (kbd.current == KEY_INVALID)
        return true;

    if (kbd.current != kbd.prev) {
        switch (kbd.current) {
        case KEY_MENU:
            AppendCW('.');
            BK4819_PlayTone(800, true);
            AUDIO_AudioPathOn();
            BK4819_ExitTxMute();
            SYSTEM_DelayMs(60);
            BK4819_EnterTxMute();
            AUDIO_AudioPathOff();
            break;
        default:
            break;
        }
    }
    return true;
}

void APP_RunCW(void)
{
    BACKLIGHT_UpdateTickless();

    // Fully initialize buffers
    memset(cwCharBuffer, 0, CW_CHAR_BUFFER_SIZE);
    memset(cwTextBuffer, 0, CW_TEXT_BUFFER_SIZE);
    cwCharIndex = 0;
    cwTextIndex = 0;
    idleCounter = 0;
    isInitialized = true;
    kbd.current = KEY_INVALID;
    kbd.prev = KEY_INVALID;
    kbd.counter = 0;

    DrawCW();
    ST7565_BlitStatusLine();
    ST7565_BlitFullScreen();

    // Wait for key release before starting main loop
    // This prevents the MENU key that entered this page from being processed
    while (KEYBOARD_GetKey() != KEY_INVALID) {
        SYSTEM_DelayMs(10);
    }
    // Additional delay to ensure key is fully released
    SYSTEM_DelayMs(100);

    while (isInitialized) {
        #ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
            SCREENSHOT_ParseInput();
        #endif
        HandleInput();

        // Auto-confirm after timeout
        if (cwCharIndex > 0) {
            idleCounter++;
            if (idleCounter >= AUTO_CONFIRM_TIMEOUT) {
                ConfirmChar();
            }
        }

        DrawCW();
        ST7565_BlitStatusLine();
        ST7565_BlitFullScreen();
        #ifdef ENABLE_FEAT_F4HWN_SCREENSHOT
            SCREENSHOT_Update(false);
        #endif
        SYSTEM_DelayMs(40);
    }
}