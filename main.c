#include <stdio.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "ir.h"
#include "sejin.h"
#include "hid_bridge.h"
#include "serial_log.h"

#define IR_GPIO 28

static void print_sejin_decoded(const sejin_frame_t *s) {
    if (s->is_sejin1) {
        LOG_PRINTF("SEJIN1 device=0x%02X subdevice=0x%02X function=0x%02X%s\r\n",
                   s->device, s->subdevice, s->function,
                   s->toggle ? "" : " HOLD");
        return;
    }

    if (s->is_button_event) {
        if (s->button_down) {
            LOG_PRINTF("SEJIN2 device=%u btn=%u DOWN seed=%u rmobc=%u\r\n",
                       s->device, s->obc, s->seed, s->rmobc);
        } else {
            LOG_PRINTF("SEJIN2 device=%u BTN_UP seed=%u rmobc=%u\r\n",
                       s->device, s->seed, s->rmobc);
        }
        return;
    }

    if (s->rmobc_supported) {
        LOG_PRINTF("SEJIN2 device=%u delta=(%d,%d) seed=%u rmobc=%u\r\n",
                   s->device, s->x, s->y, s->seed, s->rmobc);
    } else {
        LOG_PRINTF("SEJIN2 device=%u delta=(%d,%d) seed=%u rmobc=UNSUPPORTED\r\n",
                   s->device, s->x, s->y, s->seed);
    }
}

int main(void) {
#if POLSAT_ENABLE_SERIAL
    stdio_init_all();
#endif
    tusb_init();
    hid_bridge_init();
    ir_init(IR_GPIO);

    while (1) {
        tight_loop_contents();
        tud_task();
        hid_bridge_task();

        ir_frame_t frame;
        while (ir_get_frame(&frame)) {
            // Keep USB stack serviced during IR bursts so HID endpoint does not starve.
            tud_task();
            hid_bridge_task();
            sejin_frame_t s;
            if (!decode_sejin_38(&frame, &s)) continue;

            print_sejin_decoded(&s);
            hid_bridge_handle_sejin(&s);

            LOG_PRINTF("========================================\r\n");
        }
    }
}
