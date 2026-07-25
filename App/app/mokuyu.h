/*
 * Dondji Firmware
 *
 * Copyright (c) 2026 BD1AHN
 *
 * Licensed under the Apache License, Version 2.0
 *
 * Project:
 *     叮咚鸡 (Dondji)
 *
 * Maintainer:
 *     BD1AHN
 *
 * Official Website:
 *     https://ethanyan6.github.io/Dondji/
 *
 * The Dondji name, logo, and official project identity
 * are protected separately from the source code license.
 */


#pragma once

#include "keyboard_state.h"

#include "../board.h"
#include "../driver/bk4819-regs.h"
#include "../driver/bk4819.h"
#include "../driver/gpio.h"
#include "../driver/keyboard.h"
#include "../driver/st7565.h"
#include "../driver/system.h"
#include "../driver/systick.h"
#include "../external/printf/printf.h"
#include "../font.h"
#include "../helper/battery.h"
#include "../misc.h"
#include "../radio.h"
#include "../settings.h"
#include "../ui/helper.h"
#include "../audio.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

void APP_RunMokuyu(void);
bool APP_IsMokuyuActive(void);
