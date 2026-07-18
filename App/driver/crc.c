/* Copyright 2025 muzkr https://github.com/muzkr
 * Copyright 2023 Dual Tachyon
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

#include "crc.h"

void CRC_Init(void)
{
}

uint16_t CRC_Calculate(const void *pBuffer, uint16_t Size)
{
    const uint8_t *pData = (const uint8_t *)pBuffer;
    uint16_t i, Crc;

    Crc = 0;
    for (i = 0; i < Size; i++)
    {
        Crc ^= (pData[i] << 8);

        for (int j = 0; j < 8; j++)
        {
            // Check bit [15]
            if (Crc >> 15)
            {
                Crc = (Crc << 1) ^ 0x1021;
            }
            else
            {
                Crc = Crc << 1;
            }
        }
    }

    return Crc;
}

/* MDC1200 CRC — same as reference firmware software fallback / CRC_InitReverse path */
uint16_t compute_crc(const void *data, const unsigned int data_len)
{
    unsigned int i;
    const uint8_t *data8 = (const uint8_t *)data;
    uint16_t crc = 0;

    for (i = 0; i < data_len; i++) {
        unsigned int k;
        crc ^= data8[i];
        for (k = 8; k > 0; k--)
            crc = (crc & 1u) ? (crc >> 1) ^ 0x8408 : crc >> 1;
    }

    return crc ^ 0xFFFF;
}
