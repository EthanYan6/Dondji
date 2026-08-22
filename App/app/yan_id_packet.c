/*
 * Dondji Firmware — Yan ID GGM2 packet (ported from mangosteen messenger_packet)
 */
#include <string.h>
#include "app/yan_id_packet.h"

static void put_u16_le(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)(v >> 8);
}

static uint16_t get_u16_le(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

uint16_t YAN_PACKET_Crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFu;
    while (len--) {
        crc ^= (uint16_t)(*data++) << 8;
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x8000u)
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            else
                crc <<= 1;
        }
    }
    return crc;
}

static void copy_field(uint8_t *dst, uint8_t dst_len, const char *src)
{
    memset(dst, 0, dst_len);
    if (!src)
        return;
    for (uint8_t i = 0; i < dst_len && src[i]; i++)
        dst[i] = (uint8_t)src[i];
}

uint8_t YAN_PACKET_BuildYanId(uint8_t *out, uint8_t out_len, const char *from)
{
    if (!out || out_len < YAN_PKT_WIRE_LEN)
        return 0;
    memset(out, 0, YAN_PKT_WIRE_LEN);
    out[0] = YAN_PKT_MAGIC0;
    out[1] = YAN_PKT_MAGIC1;
    out[2] = YAN_PKT_MAGIC2;
    out[3] = YAN_PKT_MAGIC3;
    out[4] = YAN_PKT_VERSION;
    out[5] = YAN_PKT_TYPE_YAN_ID;
    out[6] = 0;
    put_u16_le(&out[7], 0);
    out[9]  = 1;
    out[10] = 1;
    copy_field(&out[11], YAN_PKT_CALLSIGN_LEN, from && from[0] ? from : "UVK1");
    copy_field(&out[19], YAN_PKT_CALLSIGN_LEN, YAN_PKT_TO_ALL);
    out[27] = 0;
    put_u16_le(&out[YAN_PKT_WIRE_LEN - 2u], YAN_PACKET_Crc16(out, YAN_PKT_WIRE_LEN - 2u));
    return YAN_PKT_WIRE_LEN;
}

bool YAN_PACKET_ParseYanId(const uint8_t *data, uint8_t len, YAN_Packet_t *pkt)
{
    if (!data || !pkt || len < YAN_PKT_WIRE_LEN)
        return false;

    for (uint8_t off = 0; off + YAN_PKT_WIRE_LEN <= len; off++) {
        const uint8_t *p = &data[off];
        if (p[0] != YAN_PKT_MAGIC0 || p[1] != YAN_PKT_MAGIC1 ||
            p[2] != YAN_PKT_MAGIC2 || p[3] != YAN_PKT_MAGIC3)
            continue;
        if (p[4] != YAN_PKT_VERSION || p[5] != YAN_PKT_TYPE_YAN_ID)
            continue;
        if (get_u16_le(&p[YAN_PKT_WIRE_LEN - 2u]) != YAN_PACKET_Crc16(p, YAN_PKT_WIRE_LEN - 2u))
            continue;

        memset(pkt, 0, sizeof(*pkt));
        pkt->type = p[5];
        memcpy(pkt->from, &p[11], YAN_PKT_CALLSIGN_LEN);
        pkt->from[YAN_PKT_CALLSIGN_LEN] = 0;
        /* trim trailing NULs already zero-padded */
        for (int i = YAN_PKT_CALLSIGN_LEN - 1; i >= 0; i--) {
            if (pkt->from[i] == 0 || pkt->from[i] == ' ')
                pkt->from[i] = 0;
            else
                break;
        }
        return pkt->from[0] != 0;
    }
    return false;
}
