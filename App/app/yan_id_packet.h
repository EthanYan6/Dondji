/*
 * Dondji Firmware — Yan ID GGM2 packet (mangosteen-compatible)
 */
#ifndef APP_YAN_ID_PACKET_H
#define APP_YAN_ID_PACKET_H

#include <stdbool.h>
#include <stdint.h>

#define YAN_PKT_MAGIC0          'G'
#define YAN_PKT_MAGIC1          'G'
#define YAN_PKT_MAGIC2          'M'
#define YAN_PKT_MAGIC3          '2'
#define YAN_PKT_VERSION         2u
#define YAN_PKT_TYPE_YAN_ID     5u
#define YAN_PKT_TO_ALL          "ALL"
#define YAN_PKT_WIRE_LEN        94u
#define YAN_PKT_CALLSIGN_LEN    8u

typedef struct {
    uint8_t type;
    char    from[YAN_PKT_CALLSIGN_LEN + 1];
} YAN_Packet_t;

uint16_t YAN_PACKET_Crc16(const uint8_t *data, uint16_t len);
uint8_t  YAN_PACKET_BuildYanId(uint8_t *out, uint8_t out_len, const char *from);
bool     YAN_PACKET_ParseYanId(const uint8_t *data, uint8_t len, YAN_Packet_t *pkt);

#endif
