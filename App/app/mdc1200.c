#include "../driver/bk4819.h"
#include "../driver/crc.h"
#include "../driver/py25q16.h"
#include "mdc1200.h"
#include "../misc.h"
#include <string.h>

const uint8_t mdc1200_pre_amble[] = {0x00, 0x00, 0x00};
const uint8_t mdc1200_sync[5] = {0x07, 0x09, 0x2a, 0x44, 0x6f};

static uint16_t mdc1200_compute_crc(const void *data, const unsigned int data_len)
{
    unsigned int i;
    const uint8_t *data8 = (const uint8_t *)data;
    uint16_t crc = 0;

    for (i = 0; i < data_len; i++)
    {
        unsigned int k;
        crc ^= data8[i];
        for (k = 8; k > 0; k--)
            crc = (crc & 1u) ? (crc >> 1) ^ 0x8408 : crc >> 1;
    }

    return crc ^ 0xFFFF;
}

const uint8_t mdc1200_sync_suc_xor[5] = {0xfb, 0x72, 0x40, 0x99, 0xa7};

uint16_t gMDC1200_ID = 0x0000;

void error_correction(void *data) {
    int i;
    uint8_t shift_reg;
    uint8_t syn;
    uint8_t *data8 = (uint8_t *) data;

    for (i = 0, shift_reg = 0, syn = 0; i < MDC1200_FEC_K; i++) {
        const uint8_t bi = data8[i];
        int bit_num;
        for (bit_num = 0; bit_num < 8; bit_num++) {
            uint8_t b;
            unsigned int k = 0;

            shift_reg = (shift_reg << 1) | ((bi >> bit_num) & 1u);
            b = ((shift_reg >> 6) ^ (shift_reg >> 5) ^ (shift_reg >> 2) ^ (shift_reg >> 0)) & 1u;
            syn = (syn << 1) | (((b ^ (data8[i + MDC1200_FEC_K] >> bit_num)) & 1u) ? 1u : 0u);

            if (syn & 0x80) k++;
            if (syn & 0x20) k++;
            if (syn & 0x04) k++;
            if (syn & 0x02) k++;

            if (k >= 3) {
                int ii = i;
                int bn = bit_num - 7;
                if (bn < 0) {
                    bn += 8;
                    ii--;
                }
                if (ii >= 0)
                    data8[ii] ^= 1u << bn;
                syn ^= 0xA6;
            }
        }
    }
}

bool decode_data(void *data) {
    uint16_t crc1;
    uint16_t crc2;
    uint8_t *data8 = (uint8_t *) data;

    {
        unsigned int i;
        unsigned int k;
        unsigned int m;
        uint8_t deinterleaved[(MDC1200_FEC_K * 2) * 8];

        for (i = 0, k = 0; i < 16; i++) {
            for (m = 0; m < MDC1200_FEC_K; m++) {
                const unsigned int n = (m * 16) + i;
                deinterleaved[k++] = (data8[n >> 3] >> ((7 - n) & 7u)) & 1u;
            }
        }

        for (i = 0, m = 0; i < (MDC1200_FEC_K * 2); i++) {
            unsigned int k;
            uint8_t b = 0;
            for (k = 0; k < 8; k++)
                if (deinterleaved[m++])
                    b |= 1u << k;
            data8[i] = b;
        }
    }

    error_correction(data);

    crc1 = mdc1200_compute_crc(data, 4);
    crc2 = ((uint16_t) data8[5] << 8) | (data8[4] << 0);

    return (crc1 == crc2) ? true : false;
}

void xor_modulation(void *data, const unsigned int size) {
    unsigned int i;
    uint8_t *data8 = (uint8_t *) data;
    uint8_t prev_bit = 0;
    for (i = 0; i < size; i++) {
        int bit_num;
        uint8_t in = data8[i];
        uint8_t out = 0;
        for (bit_num = 7; bit_num >= 0; bit_num--) {
            const uint8_t new_bit = (in >> bit_num) & 1u;
            if (new_bit != prev_bit)
                out |= 1u << bit_num;
            prev_bit = new_bit;
        }
        data8[i] = out ^ 0xff;
    }
}

uint8_t *encode_data(void *data) {
    uint8_t *data8 = (uint8_t *) data;

    {
        unsigned int i;
        uint8_t shift_reg = 0;
        for (i = 0; i < MDC1200_FEC_K; i++) {
            unsigned int bit_num;
            const uint8_t bi = data8[i];
            uint8_t bo = 0;
            for (bit_num = 0; bit_num < 8; bit_num++) {
                shift_reg = (shift_reg << 1) | ((bi >> bit_num) & 1u);
                bo |= (((shift_reg >> 6) ^ (shift_reg >> 5) ^ (shift_reg >> 2) ^ (shift_reg >> 0)) & 1u) << bit_num;
            }
            data8[MDC1200_FEC_K + i] = bo;
        }
    }

    {
        unsigned int i;
        unsigned int k;
        uint8_t interleaved[(MDC1200_FEC_K * 2) * 8];

        for (i = 0, k = 0; i < (MDC1200_FEC_K * 2); i++) {
            unsigned int bit_num;
            const uint8_t b = data8[i];
            for (bit_num = 0; bit_num < 8; bit_num++) {
                interleaved[k] = (b >> bit_num) & 1u;
                k += 16;
                if (k >= sizeof(interleaved))
                    k -= sizeof(interleaved) - 1;
            }
        }

        for (i = 0, k = 0; i < (MDC1200_FEC_K * 2); i++) {
            int bit_num;
            uint8_t b = 0;
            for (bit_num = 7; bit_num >= 0; bit_num--)
                if (interleaved[k++])
                    b |= 1u << bit_num;
            data8[i] = b;
        }
    }

    return data8 + (MDC1200_FEC_K * 2);
}

unsigned int MDC1200_encode_single_packet(void *data, const uint8_t op, const uint8_t arg, const uint16_t unit_id) {
    unsigned int size;
    uint16_t crc;
    uint8_t *p = (uint8_t *) data;

    p[0] = op;
    p[1] = arg;
    p[2] = (unit_id >> 8) & 0x00ff;
    p[3] = (unit_id >> 0) & 0x00ff;
    crc = mdc1200_compute_crc(p, 4);
    p[4] = (crc >> 0) & 0x00ff;
    p[5] = (crc >> 8) & 0x00ff;
    p[6] = 0;

    p = encode_data(p);

    size = (unsigned int) (p - (uint8_t *) data);

    xor_modulation(data, size);

    return size;
}

struct {
    uint8_t bit;
    uint8_t prev_bit;
    uint8_t xor_bit;
    uint64_t shift_reg;
    unsigned int bit_count;
    unsigned int stage;
    bool inverted_sync;
    unsigned int data_index;
    uint8_t data[40];
} rx;

void MDC1200_reset_rx(void) {
    memset(&rx, 0, sizeof(rx));
}

bool MDC1200_process_rx_data(
        const void *buffer,
        const unsigned int size,
        uint8_t *op,
        uint8_t *arg,
        uint16_t *unit_id) {
    const uint8_t *buffer8 = (const uint8_t *) buffer;
    unsigned int index;

    memset(&rx, 0, sizeof(rx));

    for (index = 0; index < size; index++) {
        int bit;
        const uint8_t rx_byte = buffer8[index];

        for (bit = 7; bit >= 0; bit--) {
            unsigned int i;

            rx.prev_bit = rx.bit;

            rx.bit = (rx_byte >> bit) & 1u;

            rx.xor_bit = (rx.xor_bit ^ rx.bit ^ 1u) & 1u;

            rx.shift_reg = (rx.shift_reg << 1) | rx.xor_bit;
            rx.bit_count++;

            if (rx.stage == 0) {

                const unsigned int sync_bit_ok_threshold = 32;

                if (rx.bit_count >= 40) {
                    uint64_t sync_nor = 0x07092a446fu;
                    uint64_t sync_inv = 0xffffffffffu ^ sync_nor;

                    sync_nor ^= rx.shift_reg;
                    sync_inv ^= rx.shift_reg;

                    unsigned int nor_count = 0;
                    unsigned int inv_count = 0;
                    for (i = 40; i > 0; i--, sync_nor >>= 1, sync_inv >>= 1) {
                        nor_count += sync_nor & 1u;
                        inv_count += sync_inv & 1u;
                    }
                    nor_count = 40 - nor_count;
                    inv_count = 40 - inv_count;

                    if (nor_count >= sync_bit_ok_threshold || inv_count >= sync_bit_ok_threshold) {

                        rx.inverted_sync = (inv_count > nor_count) ? true : false;
                        rx.data_index = 0;
                        rx.bit_count = 0;
                        rx.stage = 1;

                    }
                }

                continue;
            }

            if (rx.bit_count < 8)
                continue;

            rx.bit_count = 0;

            rx.data[rx.data_index++] = rx.shift_reg & 0xff;

            if (rx.data_index < (MDC1200_FEC_K * 2))
                continue;

            if (!decode_data(rx.data)) {
                MDC1200_reset_rx();
                continue;
            }

            *op = rx.data[0];
            *arg = rx.data[1];
            *unit_id = ((uint16_t) rx.data[2] << 8) | (rx.data[3] << 0);

            MDC1200_reset_rx();

            return true;
        }
    }

    MDC1200_reset_rx();

    return false;
}

uint8_t mdc1200_rx_buffer[sizeof(mdc1200_sync_suc_xor) + (MDC1200_FEC_K * 2)];
unsigned int mdc1200_rx_buffer_index = 0;

uint8_t mdc1200_op;
uint8_t mdc1200_arg;
uint16_t mdc1200_unit_id;
uint8_t mdc1200_rx_ready_tick_500ms;

void MDC1200_init(void) {
    MDC1200_reset_rx();
    MDC1200_LoadID();
}

void MDC1200_LoadID(void) {
    uint8_t data[2];
    PY25Q16_ReadBuffer(MDC1200_ID_EEPROM_ADDR, data, 2);
    gMDC1200_ID = ((uint16_t)data[0] << 8) | data[1];
    if (gMDC1200_ID == 0xFFFF) {
        gMDC1200_ID = 0x0000;
    }
}

void MDC1200_SaveID(void) {
    uint8_t data[2];
    data[0] = (gMDC1200_ID >> 8) & 0xFF;
    data[1] = gMDC1200_ID & 0xFF;
    PY25Q16_WriteBuffer(MDC1200_ID_EEPROM_ADDR, data, 2);
}

void MDC1200_SendPTTID(void)
{
    uint8_t packet[40];
    unsigned int size;

    size = MDC1200_encode_single_packet(packet, MDC1200_OP_CODE_PTT_ID, 0x80, gMDC1200_ID);
    BK4819_PlayMDC1200(packet, size, true);
}
