#pragma once

#include "sejin.h"

void hid_bridge_init(void);
void hid_bridge_task(void);
void hid_bridge_handle_sejin(const sejin_frame_t *frame);
