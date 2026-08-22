/*
 * Dondji Firmware — Yan ID AirCopy FSK TX/RX (mangosteen-compatible)
 */
#ifndef APP_YAN_ID_RF_H
#define APP_YAN_ID_RF_H

#include <stdbool.h>
#include <stdint.h>

#include "settings.h"

extern char    gYanId_RX[YAN_ID_LEN + 1];
extern uint8_t gYanId_RX_timeout;

bool YAN_RF_ReceiveEnabled(void);
bool YAN_RF_Send(void);
void YAN_RF_EnableRx(void);
void YAN_RF_DisableRx(void);
void YAN_RF_OnRadioInterrupt(uint16_t irq02_bits);
void YAN_RF_Tick10ms(void);
void YAN_RF_Tick500ms(void);

#endif
