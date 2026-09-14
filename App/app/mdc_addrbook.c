#include <string.h>

#include "app/mdc_addrbook.h"
#include "driver/py25q16.h"

static bool mdc_ab_is_name_char(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

bool MDC_AddrBookLookup(uint16_t mdcId, char *nameOut)
{
    uint8_t hdr[MDC_ADDRBOOK_HEADER_SIZE];
    uint16_t count;
    uint16_t i;

    if (nameOut == NULL || mdcId == 0)
        return false;

    PY25Q16_ReadBuffer(MDC_ADDRBOOK_SPI_BASE, hdr, sizeof(hdr));

    if (hdr[0] != 'M' || hdr[1] != 'D' || hdr[2] != 'C' || hdr[3] != 'A')
        return false;
    if (hdr[4] != MDC_ADDRBOOK_VERSION)
        return false;

    count = (uint16_t)hdr[6] | ((uint16_t)hdr[7] << 8);
    if (count > MDC_ADDRBOOK_MAX_ENTRIES)
        count = MDC_ADDRBOOK_MAX_ENTRIES;

    for (i = 0; i < count; i++) {
        uint8_t entry[MDC_ADDRBOOK_ENTRY_SIZE];
        uint16_t id;
        uint8_t n;

        PY25Q16_ReadBuffer(
            MDC_ADDRBOOK_SPI_BASE + MDC_ADDRBOOK_HEADER_SIZE + (uint32_t)i * MDC_ADDRBOOK_ENTRY_SIZE,
            entry, MDC_ADDRBOOK_ENTRY_SIZE);

        /* ID stored big-endian, same as MDC1200_SaveID */
        id = ((uint16_t)entry[0] << 8) | entry[1];
        if (id != mdcId)
            continue;

        for (n = 0; n < MDC_ADDRBOOK_NAME_LEN; n++) {
            char c = (char)entry[2 + n];
            if (c == 0)
                break;
            if (!mdc_ab_is_name_char(c))
                break;
            nameOut[n] = c;
        }
        nameOut[n] = 0;
        return n > 0;
    }

    return false;
}
