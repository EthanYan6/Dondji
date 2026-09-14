#ifndef APP_MDC_ADDRBOOK_H
#define APP_MDC_ADDRBOOK_H

#include <stdbool.h>
#include <stdint.h>

/* Dedicated 4KB SPI sector after calibration; not game/AUTH/Slot3/pinyin/voice. */
#define MDC_ADDRBOOK_SPI_BASE     0x011000u
#define MDC_ADDRBOOK_SPI_SIZE     0x1000u
#define MDC_ADDRBOOK_MAGIC        0x4144434Du /* "MDCA" LE */
#define MDC_ADDRBOOK_VERSION      1u
#define MDC_ADDRBOOK_NAME_LEN     6u
#define MDC_ADDRBOOK_ENTRY_SIZE   10u
#define MDC_ADDRBOOK_HEADER_SIZE  16u
#define MDC_ADDRBOOK_MAX_ENTRIES  400u

/* Returns true and NUL-terminated uppercase name[7] on hit. */
bool MDC_AddrBookLookup(uint16_t mdcId, char *nameOut);

#endif
