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

#ifdef ENABLE_AIRCOPY

#include <string.h>

#include "app/aircopy.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "misc.h"
#include "radio.h"
#include "ui/aircopy.h"
#include "ui/helper.h"
#include "ui/inputbox.h"

static void set_bit(uint8_t* array, int bit_index) {
    array[bit_index / 8] |= (1 << (bit_index % 8));
}

static int get_bit(uint8_t* array, int bit_index) {
    return (array[bit_index / 8] >> (bit_index % 8)) & 1;
}

void UI_DisplayAircopy(void)
{
    char String[16];
    char *pPrintStr;

    UI_DisplayClear();

    if (gAircopyState == AIRCOPY_READY) {
        pPrintStr = "AIR COPY(RDY)";
    } else if (gAircopyState == AIRCOPY_TRANSFER) {
        pPrintStr = "AIR COPY";
    } else {
        pPrintStr = "AIR COPY(CMP)";
        gAircopyState = AIRCOPY_READY;
    }

    UI_PrintString(pPrintStr, 2, 127, 0, 8);

    if (gInputBoxIndex == 0) {
        uint32_t frequency = gRxVfo->freq_config_RX.Frequency;
        sprintf(String, "%3u.%05u", frequency / 100000, frequency % 100000);
        // show the remaining 2 small frequency digits
        UI_PrintStringSmallNormal(String + 7, 97, 0, 3);
        String[7] = 0;
    } else {
        const char *ascii = INPUTBOX_GetAscii();
        sprintf(String, "%.3s.%.3s", ascii, ascii + 3);
    }

    // show the main large frequency digits
    UI_DisplayFrequency(String, 16, 2, false);

    // Get the current map and calculate percentage based on its total blocks
    const AIRCOPY_TransferMap_t *currentMap = AIRCOPY_GetCurrentMap();

    uint16_t doneBlocks = gAirCopyBlockNumber + gErrorsDuringAirCopy;

    if (doneBlocks > currentMap->total_blocks)
        doneBlocks = currentMap->total_blocks;

    // Draw memory selection
    if (gAircopyState == AIRCOPY_READY) 
    {
        doneBlocks = 0;

        if(gAircopyCurrentMapIndex < AIRCOPY_NUM_BANKS) {   
            sprintf(String, "MEM %03u - %03u", (gAircopyCurrentMapIndex * 128) + 1, (gAircopyCurrentMapIndex + 1) * 128);
        } else {
            strcpy(String, "Settings");            
        }
        UI_PrintString(String, 2, 127, 5, 8);
    } 
    else 
    {
        uint16_t percent = (doneBlocks * 10000) / currentMap->total_blocks;

        if (gAirCopyIsSendMode == 0) {
            sprintf(String, "RCV:%02u.%02u%% E:%d", percent / 100, percent % 100, gErrorsDuringAirCopy);
        } else {
            sprintf(String, "SND:%02u.%02u%%", percent / 100, percent % 100);
        }

        // Draw gauge
        if(gAircopyStep != 0)
        {
            UI_PrintString(String, 2, 127, 5, 8);

            gFrameBuffer[4][1] = 0x3c;
            gFrameBuffer[4][2] = 0x42;

            for(uint8_t i = 1; i <= AIRCOPY_BAR_WIDTH + 2; i++)
            {
                gFrameBuffer[4][2 + i] = 0x81;
            }

            gFrameBuffer[4][125] = 0x42;
            gFrameBuffer[4][126] = 0x3c;
        }
    }

    if (doneBlocks > 0)
    {
        // Track CRC errors per real block index
        if (gErrorsDuringAirCopy != lErrorsDuringAirCopy)
        {
            // Mark the last processed block as faulty
            set_bit(crc, doneBlocks - 1);
            lErrorsDuringAirCopy = gErrorsDuringAirCopy;
        }

        uint16_t b = 0;
        uint16_t fraction_accumulator = 0;

        for (uint8_t col = 0; col < AIRCOPY_BAR_WIDTH; col++)
        {
            bool processed = (b < doneBlocks);
            bool error     = processed && get_bit(crc, b);

            if (!processed)
                gFrameBuffer[4][col + 4] = 0x81;   // not yet processed
            else if (error)
                gFrameBuffer[4][col + 4] = 0x81;   // error gap (intentional hole)
            else
                gFrameBuffer[4][col + 4] = 0xBD;   // ok filled

            // DDA/Bresenham algorythm
            fraction_accumulator += currentMap->total_blocks;
            while (fraction_accumulator >= AIRCOPY_BAR_WIDTH) {
                fraction_accumulator -= AIRCOPY_BAR_WIDTH;
                b++;
            }
        }
    }

    ST7565_BlitFullScreen();
}

#endif