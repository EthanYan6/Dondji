/*
 * Dondji Firmware — MDC1200 RX application layer
 * Port of UVK1 V2.0 dual-verified RX chain (FFFF matcher + 8-phase realign + dual decode).
 */
#ifndef APP_MDC1200_APP_H
#define APP_MDC1200_APP_H

#include <stdbool.h>
#include <stdint.h>

#include "app/mdc_addrbook.h"

extern uint16_t gMdcId_RX;
extern uint8_t  gMdcId_RX_timeout;
extern char     gMdcCallsign[MDC_ADDRBOOK_NAME_LEN + 1];

bool MDC1200_AppRxEnabled(void);
void MDC1200_AppEnableRx(void);
void MDC1200_AppDisableRx(void);
void MDC1200_AppOnRadioInterrupt(uint16_t irq02_bits);
void MDC1200_AppTick10ms(void);
void MDC1200_AppTick500ms(void);
void MDC1200_AppNoteOwnTx(void);

#endif
