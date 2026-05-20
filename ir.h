#pragma once
#include <stdint.h>
#include "pico/stdlib.h"


#define IR_MAX_PULSES 200

typedef struct {
    int32_t durations[IR_MAX_PULSES];
    int count;
} ir_frame_t;

void ir_init(uint gpio);


bool ir_get_frame(ir_frame_t *out);
