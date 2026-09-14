/*
 * Dondji Firmware — MDC1200 RX application layer
 *
 * Ported from UVK1 mdc1200_app.c (V2.0 dual-machine verified):
 *   FFFF/FFFF matcher (dead-zone start, wide-in) → FIFO full-frame capture
 *   → raw dump XOR 0xFF → losehu sliding-window decode (40-bit fuzzy sync)
 *   → polarity-flip retry.
 *
 * Decode uses UVK1 method-2 (raw pass) only — phase/byte-align independent.
 * Sidecar style follows yan_id_rf.c.
 */
#include "app/mdc1200.h"
#include "app/mdc1200_app.h"
#include "app/yan_id_rf.h"
#include "driver/bk4819.h"
#include "driver/bk4819-regs.h"
#include "functions.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"

#define MDC_RX_WORDS            32u
#define MDC_RX_FRAME_WORDS      22u
#define MDC_RX_REG59_CLEAR      0x4068u
#define MDC_RX_REG59_ENABLE     0x1068u
#define MDC_RX_FSK_IRQ_MASK \
    (BK4819_REG_3F_FSK_RX_SYNC | BK4819_REG_3F_FSK_RX_FINISHED | BK4819_REG_3F_FSK_FIFO_ALMOST_FULL)

uint16_t gMdcId_RX;
uint8_t  gMdcId_RX_timeout;

static uint16_t s_fsk_buf[MDC_RX_WORDS];
static uint8_t  s_rx_words;
static bool     s_rx_capture_active;
static bool     s_sidecar_armed;
static bool     s_ignore_next_self_rx;
static uint8_t  s_ignore_self_ticks;
static uint8_t  s_rearm_delay_ticks;
static uint8_t  s_rx_idle_ticks;

bool MDC1200_AppRxEnabled(void)
{
    return gEeprom.mdc_id_rx;
}

static void mdc_ensure_fsk_irq_mask(void)
{
    if (!MDC1200_AppRxEnabled() || gCurrentFunction == FUNCTION_TRANSMIT)
        return;
    const uint16_t r3f = BK4819_ReadRegister(BK4819_REG_3F);
    const uint16_t wanted = (uint16_t)(r3f | MDC_RX_FSK_IRQ_MASK);
    if (r3f != wanted)
        BK4819_WriteRegister(BK4819_REG_3F, wanted);
}

static void mdc_keep_rx_enabled(void)
{
    mdc_ensure_fsk_irq_mask();
    BK4819_WriteRegister(BK4819_REG_59, MDC_RX_REG59_ENABLE);
}

static void mdc_align_fill(void)
{
    while (s_rx_words < MDC_RX_FRAME_WORDS && s_rx_words < MDC_RX_WORDS)
        s_fsk_buf[s_rx_words++] = BK4819_ReadRegister(BK4819_REG_5F);
}

/* UVK1 method-2: dump XOR 0xFF then losehu sliding-window decode.
 * Second try flips first bit for unknown cum-domain parity. */
static bool mdc_frame_decode(uint16_t *id, uint8_t *op, uint8_t *arg)
{
    static uint8_t buf[MDC_RX_FRAME_WORDS * 2u];
    uint8_t n = 0u;
    uint8_t n_words = s_rx_words;
    if (n_words > MDC_RX_FRAME_WORDS)
        n_words = MDC_RX_FRAME_WORDS;

    for (uint8_t w = 0u; w < n_words; w++) {
        buf[n++] = (uint8_t)((s_fsk_buf[w]      ) ^ 0xFFu);
        buf[n++] = (uint8_t)((s_fsk_buf[w] >> 8 ) ^ 0xFFu);
    }
    if (n < 19u)
        return false;

    if (MDC1200_process_rx_data(buf, n, op, arg, id))
        return true;
    buf[0] ^= 0x80u;
    return MDC1200_process_rx_data(buf, n, op, arg, id);
}

static void mdc_try_accept_rx(void)
{
    uint16_t id = 0;
    uint8_t op = 0, arg = 0;

    if (s_ignore_next_self_rx) {
        s_ignore_next_self_rx = false;
        s_ignore_self_ticks = 0;
        return;
    }
    if (!mdc_frame_decode(&id, &op, &arg))
        return;

    gMdcId_RX = id;
    gMdcId_RX_timeout = 12; /* 6 s @ 500 ms — same window as Yan ID popup */
    gUpdateDisplay = true;
}

void MDC1200_AppNoteOwnTx(void)
{
    s_sidecar_armed = false;
    s_rx_capture_active = false;
    s_rx_words = 0;
    BK4819_WriteRegister(BK4819_REG_59, 0x0068);
    s_ignore_next_self_rx = true;
    s_ignore_self_ticks = 4;   /* ~2 s @ 500 ms */
    s_rearm_delay_ticks = 20;  /* 200 ms then re-arm */
}

void MDC1200_AppEnableRx(void)
{
    if (!MDC1200_AppRxEnabled())
        return;
#ifdef ENABLE_AIRCOPY
    if (gScreenToDisplay == DISPLAY_AIRCOPY)
        return;
#endif
    if (gCurrentFunction == FUNCTION_TRANSMIT)
        return;
    if (YAN_RF_ReceiveEnabled())
        return; /* FSK modem is exclusive with Yan ID RX */

    s_rx_words = 0;
    s_rx_capture_active = false;

    BK4819_EnableMDC1200Rx();
    BK4819_WriteRegister(BK4819_REG_02, 0x0000);
    mdc_ensure_fsk_irq_mask();
    BK4819_WriteRegister(BK4819_REG_59, MDC_RX_REG59_CLEAR);
    BK4819_WriteRegister(BK4819_REG_59, MDC_RX_REG59_ENABLE);
    s_sidecar_armed = true;
}

void MDC1200_AppDisableRx(void)
{
    s_sidecar_armed = false;
    s_rx_capture_active = false;
    s_rx_words = 0;
    BK4819_WriteRegister(BK4819_REG_70, 0);
    BK4819_WriteRegister(BK4819_REG_72, 0);
    BK4819_WriteRegister(BK4819_REG_58, 0);
    BK4819_WriteRegister(BK4819_REG_59, 0x0068);
}

void MDC1200_AppOnRadioInterrupt(uint16_t status)
{
    const bool fsk_sync    = (status & BK4819_REG_02_FSK_RX_SYNC) != 0;
    const bool fifo_full   = (status & BK4819_REG_02_FSK_FIFO_ALMOST_FULL) != 0;
    const bool rx_finished = (status & BK4819_REG_02_FSK_RX_FINISHED) != 0;

    if (!s_sidecar_armed || !MDC1200_AppRxEnabled())
        return;
    if (YAN_RF_ReceiveEnabled())
        return;

    if (fsk_sync || (BK4819_ReadRegister(BK4819_REG_0B) & ((1u << 6) | (1u << 7)))) {
        if (!s_rx_capture_active)
            s_rx_words = 0;
        s_rx_capture_active = true;
    }

    if (fifo_full) {
        const uint8_t count = (uint8_t)(BK4819_ReadRegister(BK4819_REG_5E) & 7u);
        for (uint8_t i = 0; i < count && s_rx_words < MDC_RX_WORDS; i++)
            s_fsk_buf[s_rx_words++] = BK4819_ReadRegister(BK4819_REG_5F);
        s_rx_idle_ticks = 0;
        s_rx_capture_active = true;
        if (s_rx_words >= MDC_RX_WORDS) {
            mdc_try_accept_rx();
            s_rx_capture_active = false;
            s_rx_words = 0;
            mdc_keep_rx_enabled();
        }
    }

    if (rx_finished) {
        if (s_rx_capture_active) {
            mdc_align_fill();
            mdc_try_accept_rx();
            s_rx_capture_active = false;
        }
        mdc_keep_rx_enabled();
        s_rx_words = 0;
    }
}

void MDC1200_AppTick10ms(void)
{
    if (!MDC1200_AppRxEnabled() || YAN_RF_ReceiveEnabled()) {
        if (s_sidecar_armed)
            MDC1200_AppDisableRx();
        return;
    }
    if (gCurrentFunction == FUNCTION_TRANSMIT)
        return;
    if (s_rearm_delay_ticks > 0 && --s_rearm_delay_ticks == 0)
        s_sidecar_armed = false;
    if (!s_sidecar_armed) {
        MDC1200_AppEnableRx();
        return;
    }
    if (s_rx_capture_active && s_rx_words > 0) {
        if (++s_rx_idle_ticks >= 20u) {
            s_rx_idle_ticks = 0;
            mdc_align_fill();
            mdc_try_accept_rx();
            s_rx_capture_active = false;
            s_rx_words = 0;
            mdc_keep_rx_enabled();
        }
    } else {
        s_rx_idle_ticks = 0;
    }
    if (((BK4819_ReadRegister(BK4819_REG_59) & 0x1000u) == 0u) ||
        ((BK4819_ReadRegister(BK4819_REG_3F) & MDC_RX_FSK_IRQ_MASK) != MDC_RX_FSK_IRQ_MASK)) {
        s_sidecar_armed = false;
        MDC1200_AppEnableRx();
    } else {
        mdc_ensure_fsk_irq_mask();
    }
}

void MDC1200_AppTick500ms(void)
{
    if (gMdcId_RX_timeout > 0) {
        if (--gMdcId_RX_timeout == 0) {
            gMdcId_RX = 0;
            gUpdateDisplay = true;
        }
    }
    if (s_ignore_next_self_rx && s_ignore_self_ticks > 0) {
        if (--s_ignore_self_ticks == 0)
            s_ignore_next_self_rx = false;
    }
}
