/*
 * Dondji Firmware
 *
 * Copyright (c) 2026 BD1AHN
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Project:
 *     叮咚鸡 (Dondji)
 *
 * Maintainer:
 *     BD1AHN
 *
 * Commercial products using the Dondji brand require separate authorization.
 */

/* UV-KX smeter face (fonts_icons/smeter.xbm → icons.h), 1bpp XBM：与 u8g2 相同，每字节 bit0 为最左像素 */
#ifndef DUALVFO_SMETER_XBM_H
#define DUALVFO_SMETER_XBM_H

#include <stdint.h>

#define DUALVFO_SMETER_XBM_WIDTH 35
#define DUALVFO_SMETER_XBM_HEIGHT 9

static const uint8_t dualvfo_smeter_xbm_bits[] = {
    0x03, 0x07, 0x07, 0x07, 0x07, 0x02, 0x04, 0x01, 0x04, 0x05, 0x72, 0x77,
    0x77, 0x74, 0x07, 0x02, 0x04, 0x04, 0x04, 0x04, 0x07, 0x07, 0x07, 0x04,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x22, 0x02, 0x22,
    0x22, 0x22, 0x22, 0x02, 0x22, 0x22, 0x22, 0x22, 0x02,
};

#endif
