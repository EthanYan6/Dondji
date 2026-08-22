/*
 * Dondji Firmware — Yan ID FSK path (surgical port of mangosteen messenger_rf Yan ID)
 *
 * Critical interop note:
 * BK4819 AirCopy uses datasheet-default FSK sync REG_5A/5B = 0x85CF / 0xAB45.
 * SetupAircopy() never writes them. MDC disable zeroes them — if Yan TX/RX
 * leaves sync at 0, neither side can sync → both directions fail.
 */
#include <string.h>
#include "app/yan_id_packet.h"
#include "app/yan_id_rf.h"
#include "driver/bk4819.h"
#include "driver/bk4819-regs.h"
#include "driver/system.h"
#include "functions.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "ui/ui.h"
#define YAN_RF_WORDS              50u
#define YAN_RF_BYTES_CARRIED      YAN_PKT_WIRE_LEN
#define YAN_RF_REG59_TX_CLEAR     0x80F8u
#define YAN_RF_REG59_TX_READY     0x00F8u
#define YAN_RF_REG59_TX_START     0x28F8u
#define YAN_RF_REG5D_LEN_100      0x6300u
#define YAN_RF_REG59_RX_CLEAR     0x4068u
#define YAN_RF_REG59_RX_ENABLE    0x3068u
/* Datasheet power-on defaults — AirCopy / mangosteen messenger depend on these. */
#define YAN_RF_SYNC_5A            0x85CFu
#define YAN_RF_SYNC_5B            0xAB45u
#define YAN_RF_FSK_IRQ_MASK \
    (BK4819_REG_3F_FSK_RX_SYNC | BK4819_REG_3F_FSK_RX_FINISHED | BK4819_REG_3F_FSK_FIFO_ALMOST_FULL)
char    gYanId_RX[YAN_ID_LEN + 1];
uint8_t gYanId_RX_timeout;
static uint16_t s_fsk_buf[YAN_RF_WORDS];
static uint8_t  s_rx_words;
static bool     s_rx_capture_active;
static bool     s_sidecar_armed;
static bool     s_ignore_next_self_rx;
static uint8_t  s_ignore_self_ticks;
static bool     s_bw_lock_active;
static uint8_t  s_bw_lock_tx_old;
static uint8_t  s_bw_lock_rx_old;
static uint8_t  s_rearm_delay_ticks;
bool YAN_RF_ReceiveEnabled(void)
{
    return gEeprom.ROGER == ROGER_MODE_YAN_ID && gEeprom.yan_id_rx;
}
static void yan_narrow_lock_begin(void)
{
    if (s_bw_lock_active || !gTxVfo)
        return;
    s_bw_lock_tx_old = gTxVfo->CHANNEL_BANDWIDTH;
    s_bw_lock_rx_old = gRxVfo ? gRxVfo->CHANNEL_BANDWIDTH : s_bw_lock_tx_old;
    gTxVfo->CHANNEL_BANDWIDTH = BANDWIDTH_NARROW;
    if (gRxVfo)
        gRxVfo->CHANNEL_BANDWIDTH = BANDWIDTH_NARROW;
    s_bw_lock_active = true;
}
static void yan_narrow_lock_end(void)
{
    if (!s_bw_lock_active)
        return;
    if (gTxVfo)
        gTxVfo->CHANNEL_BANDWIDTH = s_bw_lock_tx_old;
    if (gRxVfo)
        gRxVfo->CHANNEL_BANDWIDTH = s_bw_lock_rx_old;
    s_bw_lock_active = false;
}
static void yan_ensure_fsk_irq_mask(void)
{
    if (!YAN_RF_ReceiveEnabled() || gCurrentFunction == FUNCTION_TRANSMIT)
        return;
    const uint16_t r3f = BK4819_ReadRegister(BK4819_REG_3F);
    const uint16_t wanted = (uint16_t)(r3f | YAN_RF_FSK_IRQ_MASK);
    if (r3f != wanted)
        BK4819_WriteRegister(BK4819_REG_3F, wanted);
}
static void yan_setup_aircopy_modem(void)
{
    /* Must match mangosteen BK4819_SetupAircopy() + restore AirCopy sync words. */
    BK4819_WriteRegister(BK4819_REG_70, 0x00C3); /* Tone2 enable, gain 48 */
    BK4819_WriteRegister(BK4819_REG_72, 0x3065); /* 1200 baud */
    BK4819_WriteRegister(BK4819_REG_58, 0x00C1); /* FSK 1.2K */
    BK4819_WriteRegister(BK4819_REG_5C, 0x5665);
    BK4819_WriteRegister(BK4819_REG_5A, YAN_RF_SYNC_5A);
    BK4819_WriteRegister(BK4819_REG_5B, YAN_RF_SYNC_5B);
    BK4819_WriteRegister(BK4819_REG_5E, 0x3204);
    BK4819_WriteRegister(BK4819_REG_5D, YAN_RF_REG5D_LEN_100);
}
static void yan_keep_fsk_rx_enabled(void)
{
    yan_ensure_fsk_irq_mask();
    BK4819_WriteRegister(BK4819_REG_59, YAN_RF_REG59_RX_CLEAR);
    BK4819_WriteRegister(BK4819_REG_59, YAN_RF_REG59_RX_ENABLE);
}
static void bytes_to_aircopy_words(const uint8_t *bytes)
{
    memset(s_fsk_buf, 0, sizeof(s_fsk_buf));
    s_fsk_buf[0] = 0xABCDu;
    for (uint8_t i = 0; i < (YAN_RF_BYTES_CARRIED / 2u); i++) {
        s_fsk_buf[1u + i] = (uint16_t)bytes[i * 2u] | ((uint16_t)bytes[i * 2u + 1u] << 8);
    }
    s_fsk_buf[1u + (YAN_RF_BYTES_CARRIED / 2u)] = YAN_PACKET_Crc16(bytes, YAN_PKT_WIRE_LEN);
    s_fsk_buf[YAN_RF_WORDS - 1u] = 0xDCBAu;
}
static void send_fsk_long_preamble(const uint16_t *pData)
{
    unsigned int i;
    uint8_t timeout = 200;
    SYSTEM_DelayMs(30);
    BK4819_WriteRegister(BK4819_REG_3F, BK4819_REG_3F_FSK_TX_FINISHED);
    BK4819_WriteRegister(BK4819_REG_5D, YAN_RF_REG5D_LEN_100);
    BK4819_WriteRegister(BK4819_REG_59, YAN_RF_REG59_TX_CLEAR);
    BK4819_WriteRegister(BK4819_REG_59, YAN_RF_REG59_TX_READY);
    for (i = 0; i < YAN_RF_WORDS; i++)
        BK4819_WriteRegister(BK4819_REG_5F, pData[i]);
    SYSTEM_DelayMs(20);
    BK4819_WriteRegister(BK4819_REG_59, YAN_RF_REG59_TX_START);
    while (timeout-- && (BK4819_ReadRegister(BK4819_REG_0C) & 1u) == 0u)
        SYSTEM_DelayMs(5);
    BK4819_WriteRegister(BK4819_REG_02, 0x0000);
    SYSTEM_DelayMs(30);
    BK4819_ResetFSK();
}
static bool parse_aircopy_yan(YAN_Packet_t *pkt)
{
    const uint8_t data_words = (uint8_t)(YAN_RF_BYTES_CARRIED / 2u);
    const uint8_t words_available =
        (s_rx_words > 0u && s_rx_words <= YAN_RF_WORDS) ? s_rx_words : YAN_RF_WORDS;
    for (uint8_t off = 0u; off + 1u + data_words <= words_available; off++) {
        if (s_fsk_buf[off] != 0xABCDu)
            continue;
        uint8_t bytes[YAN_RF_BYTES_CARRIED];
        for (uint8_t i = 0u; i < data_words; i++) {
            const uint16_t w = s_fsk_buf[off + 1u + i];
            bytes[i * 2u]      = (uint8_t)(w & 0xFFu);
            bytes[i * 2u + 1u] = (uint8_t)(w >> 8);
        }
        if (YAN_PACKET_ParseYanId(bytes, YAN_PKT_WIRE_LEN, pkt))
            return true;
    }
    return false;
}
static void try_accept_rx(void)
{
    YAN_Packet_t pkt;
    if (!parse_aircopy_yan(&pkt))
        return;
    /* mangosteen: drop the next RX after own TX regardless of callsign match */
    if (s_ignore_next_self_rx) {
        s_ignore_next_self_rx = false;
        s_ignore_self_ticks = 0;
        return;
    }
    memset(gYanId_RX, 0, sizeof(gYanId_RX));
    strncpy(gYanId_RX, pkt.from, YAN_ID_LEN);
    gYanId_RX[YAN_ID_LEN] = 0;
    gYanId_RX_timeout = 12; /* 6 s @ 500 ms — same as mangosteen YanId_RX_timeout_500ms */
    gUpdateDisplay = true;
}
bool YAN_RF_Send(void)
{
    uint8_t packet[YAN_PKT_WIRE_LEN];
    if (gEeprom.yan_id[0] == 0)
        return false;
    if (YAN_PACKET_BuildYanId(packet, sizeof(packet), gEeprom.yan_id) != YAN_PKT_WIRE_LEN)
        return false;
    s_sidecar_armed = false;
    s_rx_capture_active = false;
    s_rx_words = 0;
    BK4819_WriteRegister(BK4819_REG_59, 0x0068);
    BK4819_ResetFSK();
    yan_narrow_lock_begin();
    BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, true);
    RADIO_SetTxParameters();
    yan_setup_aircopy_modem();
    bytes_to_aircopy_words(packet);
    send_fsk_long_preamble(s_fsk_buf);
    BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, false);
    yan_narrow_lock_end();
    s_ignore_next_self_rx = true;
    s_ignore_self_ticks = 4; /* ~2 s @ 500 ms */
    s_rearm_delay_ticks = 20; /* 200 ms then re-arm RX */
    return true;
}
void YAN_RF_EnableRx(void)
{
    if (!YAN_RF_ReceiveEnabled())
        return;
#ifdef ENABLE_AIRCOPY
    if (gScreenToDisplay == DISPLAY_AIRCOPY)
        return;
#endif
    if (gCurrentFunction == FUNCTION_TRANSMIT)
        return;
    memset(s_fsk_buf, 0, sizeof(s_fsk_buf));
    s_rx_words = 0;
    s_rx_capture_active = false;
    yan_setup_aircopy_modem();
    BK4819_WriteRegister(BK4819_REG_02, 0x0000);
    yan_ensure_fsk_irq_mask();
    BK4819_WriteRegister(BK4819_REG_59, YAN_RF_REG59_RX_CLEAR);
    BK4819_WriteRegister(BK4819_REG_59, YAN_RF_REG59_RX_ENABLE);
    s_sidecar_armed = true;
}
void YAN_RF_DisableRx(void)
{
    /* Soft disarm like mangosteen — do NOT zero REG_5A/5B (AirCopy sync). */
    s_sidecar_armed = false;
    s_rx_capture_active = false;
    s_rx_words = 0;
    BK4819_WriteRegister(BK4819_REG_59, 0x0068);
}
void YAN_RF_OnRadioInterrupt(uint16_t status)
{
    const bool fsk_sync    = (status & BK4819_REG_02_FSK_RX_SYNC) != 0;
    const bool fifo_full   = (status & BK4819_REG_02_FSK_FIFO_ALMOST_FULL) != 0;
    const bool rx_finished = (status & BK4819_REG_02_FSK_RX_FINISHED) != 0;
    if (!s_sidecar_armed || !YAN_RF_ReceiveEnabled())
        return;
    if (fsk_sync || (BK4819_ReadRegister(BK4819_REG_0B) & ((1u << 6) | (1u << 7)))) {
        if (!s_rx_capture_active) {
            s_rx_words = 0;
            memset(s_fsk_buf, 0, sizeof(s_fsk_buf));
        }
        s_rx_capture_active = true;
        yan_narrow_lock_begin();
    }
    if (fifo_full) {
        for (uint8_t i = 0; i < 4u && s_rx_words < YAN_RF_WORDS; i++)
            s_fsk_buf[s_rx_words++] = BK4819_ReadRegister(BK4819_REG_5F);
        s_rx_capture_active = true;
        yan_narrow_lock_begin();
    }
    if (rx_finished) {
        while (s_rx_words < YAN_RF_WORDS)
            s_fsk_buf[s_rx_words++] = BK4819_ReadRegister(BK4819_REG_5F);
        try_accept_rx();
        s_rx_capture_active = false;
        yan_narrow_lock_end();
        yan_keep_fsk_rx_enabled();
        s_rx_words = 0;
    }
}
void YAN_RF_Tick10ms(void)
{
    if (!YAN_RF_ReceiveEnabled())
        return;
    if (gCurrentFunction == FUNCTION_TRANSMIT)
        return;
    if (s_rearm_delay_ticks > 0) {
        if (--s_rearm_delay_ticks == 0)
            s_sidecar_armed = false; /* force full re-arm below */
    }
    if (!s_sidecar_armed) {
        YAN_RF_EnableRx();
        return;
    }
    /* Keep IRQ mask + RX enable alive (mangosteen EnsureFskIrqMask / keepalive). */
    {
        const uint16_t r3f = BK4819_ReadRegister(BK4819_REG_3F);
        const uint16_t r59 = BK4819_ReadRegister(BK4819_REG_59);
        if ((r59 & 0x1000u) == 0u || (r3f & YAN_RF_FSK_IRQ_MASK) != YAN_RF_FSK_IRQ_MASK) {
            s_sidecar_armed = false;
            YAN_RF_EnableRx();
        } else {
            yan_ensure_fsk_irq_mask();
        }
    }
}
void YAN_RF_Tick500ms(void)
{
    if (gYanId_RX_timeout > 0) {
        if (--gYanId_RX_timeout == 0) {
            gYanId_RX[0] = 0;
            gUpdateDisplay = true;
        }
    }
    if (s_ignore_next_self_rx && s_ignore_self_ticks > 0) {
        if (--s_ignore_self_ticks == 0)
            s_ignore_next_self_rx = false;
    }
}
