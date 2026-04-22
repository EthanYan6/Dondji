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

#include <stddef.h>
#include <string.h>

#include "driver/st7565.h"
#include "functions.h"
#include "ui/battery.h"
#include "../misc.h"

void UI_DrawBattery(uint8_t* bitmap, uint8_t level, uint8_t blink)
{
    const uint8_t battery_icon_width = UI_BATTERY_ICON_WIDTH;
    const uint8_t battery_body_left_x = 0u;
    const uint8_t battery_body_right_x = 10u;
    const uint8_t battery_body_top_y = 1u;
    const uint8_t battery_body_bottom_y = 5u;
    const uint8_t battery_tip_left_x = 11u;
    const uint8_t battery_tip_right_x = 12u;
    const uint8_t battery_tip_top_y = 2u;
    const uint8_t battery_tip_bottom_y = 4u;
    const uint8_t battery_fill_left_x = 1u;
    const uint8_t battery_fill_right_limit = 9u;
    const uint8_t battery_fill_top_y = 2u;
    const uint8_t battery_fill_bottom_y = 4u;

    memset(bitmap, 0, battery_icon_width);

    if (level < 2 && blink == 1) {
        return;
    }

    for (uint8_t x = battery_body_left_x; x <= battery_body_right_x; x++) {
        bitmap[x] |= (uint8_t)(1u << battery_body_top_y);
        bitmap[x] |= (uint8_t)(1u << battery_body_bottom_y);
    }
    for (uint8_t y = battery_body_top_y; y <= battery_body_bottom_y; y++) {
        bitmap[battery_body_left_x] |= (uint8_t)(1u << y);
        bitmap[battery_body_right_x] |= (uint8_t)(1u << y);
    }
    for (uint8_t y = battery_tip_top_y; y <= battery_tip_bottom_y; y++) {
        bitmap[battery_tip_left_x] |= (uint8_t)(1u << y);
        bitmap[battery_tip_right_x] |= (uint8_t)(1u << y);
    }

    {
        const uint8_t battery_level_clamped = (level > 7u) ? 7u : level;
        const uint8_t battery_fill_pixel_capacity =
            (uint8_t)(battery_fill_right_limit - battery_fill_left_x + 1u);
        const uint8_t battery_fill_pixels =
            (uint8_t)((battery_level_clamped * battery_fill_pixel_capacity) / 7u);

        for (uint8_t fill_index = 0u; fill_index < battery_fill_pixels; fill_index++) {
            const uint8_t fill_x = (uint8_t)(battery_fill_left_x + fill_index);
            for (uint8_t fill_y = battery_fill_top_y; fill_y <= battery_fill_bottom_y; fill_y++) {
                bitmap[fill_x] |= (uint8_t)(1u << fill_y);
            }
        }
    }
}

void UI_DisplayBattery(uint8_t level, uint8_t blink)
{
    uint8_t bitmap[UI_BATTERY_ICON_WIDTH];
    UI_DrawBattery(bitmap, level, blink);
    ST7565_DrawLine(LCD_WIDTH - sizeof(bitmap), 0, bitmap, sizeof(bitmap));
}
