#include "ir.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"


#define FRAME_GAP_US 2200

#define QUEUE_SIZE 10

static volatile int32_t cap_buf[IR_MAX_PULSES];
static volatile int cap_count = 0;
static volatile uint64_t last_edge_us = 0;
static volatile bool capturing = false;


static ir_frame_t frame_queue[QUEUE_SIZE];
static volatile uint8_t q_head = 0;
static volatile uint8_t q_tail = 0;

static inline void enqueue_captured_frame_from_irq(void) {
    if (cap_count <= 0) return;

    uint8_t next_tail = (q_tail + 1) % QUEUE_SIZE;
    if (next_tail == q_head) {
        // Queue full: drop oldest so newest transitions are kept.
        q_head = (q_head + 1) % QUEUE_SIZE;
    }

    ir_frame_t *f = &frame_queue[q_tail];
    f->count = cap_count;
    for (int i = 0; i < cap_count; i++) {
        f->durations[i] = cap_buf[i];
    }
    q_tail = next_tail;

    cap_count = 0;
    capturing = false;
}

static void gpio_irq_handler(uint gpio, uint32_t events) {
    (void)gpio;
    uint64_t now = time_us_64();

   
    if (capturing && cap_count > 0 && (now - last_edge_us) > FRAME_GAP_US) {
        enqueue_captured_frame_from_irq();
    }

    if (events & GPIO_IRQ_EDGE_FALL) {
        if (!capturing) {
            capturing = true;
            cap_count = 0;
        } else {
            int32_t dur = (int32_t)(now - last_edge_us);
            if (cap_count < IR_MAX_PULSES)
                cap_buf[cap_count++] = -dur; // negative = space
        }
        last_edge_us = now;
    }

    if ((events & GPIO_IRQ_EDGE_RISE) && capturing) {
        int32_t dur = (int32_t)(now - last_edge_us);
        last_edge_us = now;
        if (cap_count < IR_MAX_PULSES)
            cap_buf[cap_count++] = dur; // positive = mark
    }
}

void ir_init(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
    gpio_set_irq_enabled_with_callback(gpio,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, gpio_irq_handler);
}

bool ir_get_frame(ir_frame_t *out) {
    uint32_t saved = save_and_disable_interrupts();
    if (q_head == q_tail) {
        restore_interrupts(saved);
        return false;
    }

    *out = frame_queue[q_head];
    q_head = (q_head + 1) % QUEUE_SIZE;
    restore_interrupts(saved);
    return true;
}
