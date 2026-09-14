#ifndef APP_MDC1200_H
#define APP_MDC1200_H

#include <stdint.h>
#include <stdbool.h>

#define MDC1200_FEC_K   7

enum mdc1200_op_code_e {
    MDC1200_OP_CODE_PTT_ID = 0x01,
    MDC1200_OP_CODE_POST_ID = 0x01,
    MDC1200_OP_CODE_REMOTE_MONITOR = 0x11,
    MDC1200_OP_CODE_STATUS_REQ = 0x22,
    MDC1200_OP_CODE_RADIO_ENABLE = 0x2B,
    MDC1200_OP_CODE_RADIO_DISABLE = 0x2B,
    MDC1200_OP_CODE_CALL_ALERT = 0x35,
    MDC1200_OP_CODE_STS_XX = 0x46,
    MDC1200_OP_CODE_MSG_XX = 0x47,
    MDC1200_OP_CODE_RADIO_CHECK = 0x63
};
typedef enum mdc1200_op_code_e mdc1200_op_code_t;

extern const uint8_t mdc1200_sync[5];

extern uint16_t gMDC1200_ID;

#define MDC1200_ID_EEPROM_ADDR  0x00A172

unsigned int MDC1200_encode_single_packet(void *data, const uint8_t op, const uint8_t arg, const uint16_t unit_id);
bool MDC1200_process_rx_data(const void *buffer, const unsigned int size, uint8_t *op, uint8_t *arg, uint16_t *unit_id);
void MDC1200_reset_rx(void);
void MDC1200_init(void);
void MDC1200_LoadID(void);
void MDC1200_SaveID(void);
void MDC1200_SendPTTID(void);

#endif
