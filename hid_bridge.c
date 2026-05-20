#include <stdbool.h>
#include <stdint.h>
#include "tusb.h"
#include "class/hid/hid.h"
#include "hid_bridge.h"
#include "usb_descriptors.h"

typedef struct {
    uint8_t function;
    uint8_t keycode;
    uint8_t modifier;
} key_map_t;

typedef struct {
    uint8_t modifiers;
    uint8_t keys[6];
} kb_report_t;

static const key_map_t key_map[] = {
    {0x02, HID_KEY_B, 0},
    {0x03, HID_KEY_TAB, 0},
    {0x04, HID_KEY_ARROW_RIGHT, 0},
    {0x05, HID_KEY_SPACE, 0},
    {0x06, HID_KEY_APOSTROPHE, 0},
    {0x07, 0, KEYBOARD_MODIFIER_LEFTSHIFT},
    {0x08, HID_KEY_EQUAL, 0},
    {0x09, HID_KEY_BRACKET_LEFT, 0},
    {0x0B, HID_KEY_ENTER, 0},
    {0x0C, HID_KEY_ESCAPE, 0},
    {0x0D, HID_KEY_ARROW_LEFT, 0},
    {0x12, HID_KEY_A, 0},
    {0x13, HID_KEY_S, 0},
    {0x14, HID_KEY_D, 0},
    {0x15, HID_KEY_F, 0},
    {0x16, HID_KEY_J, 0},
    {0x18, HID_KEY_K, 0},
    {0x19, HID_KEY_L, 0},
    {0x1B, HID_KEY_SEMICOLON, 0},
    {0x1C, HID_KEY_PAGE_DOWN, 0},
    {0x1D, HID_KEY_END, 0},
    {0x1E, 0, KEYBOARD_MODIFIER_LEFTGUI},
    {0x21, 0, KEYBOARD_MODIFIER_RIGHTALT},
    {0x22, HID_KEY_Q, 0},
    {0x23, HID_KEY_W, 0},
    {0x24, HID_KEY_E, 0},
    {0x25, HID_KEY_R, 0},
    {0x26, HID_KEY_U, 0},
    {0x28, HID_KEY_I, 0},
    {0x29, HID_KEY_O, 0},
    {0x2B, HID_KEY_P, 0},
    {0x32, HID_KEY_Z, 0},
    {0x33, HID_KEY_X, 0},
    {0x34, HID_KEY_C, 0},
    {0x35, HID_KEY_V, 0},
    {0x36, HID_KEY_M, 0},
    {0x37, 0, KEYBOARD_MODIFIER_RIGHTSHIFT},
    {0x38, HID_KEY_COMMA, 0},
    {0x39, HID_KEY_PERIOD, 0},
    {0x3B, HID_KEY_SLASH, 0},
    {0x3C, HID_KEY_BACKSLASH, 0},
    {0x3D, HID_KEY_N, 0},
    {0x41, HID_KEY_ARROW_RIGHT, 0},
    {0x42, HID_KEY_F3, 0},
    {0x43, HID_KEY_ESCAPE, 0},
    {0x44, HID_KEY_F5, 0},
    {0x45, HID_KEY_5, 0},
    {0x46, HID_KEY_6, 0},
    {0x48, HID_KEY_F6, 0},
    {0x49, HID_KEY_F8, 0},
    {0x4C, HID_KEY_BACKSPACE, 0},
    {0x4D, HID_KEY_HOME, 0},
    {0x4E, HID_KEY_BACKSLASH, 0},
    {0x52, HID_KEY_F2, 0},
    {0x53, HID_KEY_F1, 0},
    {0x54, HID_KEY_F4, 0},
    {0x55, HID_KEY_G, 0},
    {0x56, HID_KEY_H, 0},
    {0x58, HID_KEY_F7, 0},
    {0x59, HID_KEY_F9, 0},
    {0x5C, HID_KEY_ARROW_UP, 0},
    {0x5D, HID_KEY_PAGE_UP, 0},
    {0x5F, 0, KEYBOARD_MODIFIER_LEFTCTRL},
    {0x62, HID_KEY_1, 0},
    {0x63, HID_KEY_2, 0},
    {0x64, HID_KEY_3, 0},
    {0x65, HID_KEY_4, 0},
    {0x66, HID_KEY_7, 0},
    {0x68, HID_KEY_8, 0},
    {0x69, HID_KEY_9, 0},
    {0x6B, HID_KEY_0, 0},
    {0x6C, HID_KEY_ARROW_DOWN, 0},
    {0x6D, HID_KEY_DELETE, 0},
    {0x71, 0, KEYBOARD_MODIFIER_LEFTALT},
    {0x72, HID_KEY_GRAVE, 0},
    {0x73, HID_KEY_CAPS_LOCK, 0},
    {0x75, HID_KEY_T, 0},
    {0x76, HID_KEY_Y, 0},
    {0x78, HID_KEY_F11, 0},
    {0x79, HID_KEY_F10, 0},
    {0x7B, HID_KEY_F12, 0},
    {0x7C, HID_KEY_PRINT_SCREEN, 0}
};

#define KB_REPORT_QUEUE_SIZE 32

static uint8_t kb_modifiers;
static uint8_t kb_keys[6];
static kb_report_t kb_queue[KB_REPORT_QUEUE_SIZE];
static uint8_t kb_q_head;
static uint8_t kb_q_tail;
static uint8_t mouse_buttons;
static int16_t mouse_pending_x;
static int16_t mouse_pending_y;
static bool mouse_dirty;
static bool was_mounted;

static bool hid_ready(void) {
    return tud_mounted() && tud_hid_ready();
}

static bool kb_queue_empty(void) {
    return kb_q_head == kb_q_tail;
}

static bool kb_reports_equal(const kb_report_t *a, const kb_report_t *b) {
    if (a->modifiers != b->modifiers) return false;
    for (uint32_t i = 0; i < 6; i++) {
        if (a->keys[i] != b->keys[i]) return false;
    }
    return true;
}

static bool key_in_slots(uint8_t keycode) {
    for (uint32_t i = 0; i < 6; i++) {
        if (kb_keys[i] == keycode) return true;
    }
    return false;
}

static bool add_key_to_slots(uint8_t keycode) {
    if (keycode == 0 || key_in_slots(keycode)) return false;
    for (uint32_t i = 0; i < 6; i++) {
        if (kb_keys[i] == 0) {
            kb_keys[i] = keycode;
            return true;
        }
    }
    return false;
}

static bool remove_key_from_slots(uint8_t keycode) {
    if (keycode == 0) return false;
    for (uint32_t i = 0; i < 6; i++) {
        if (kb_keys[i] == keycode) {
            kb_keys[i] = 0;
            return true;
        }
    }
    return false;
}

static void clear_key_slots(void) {
    for (uint32_t i = 0; i < 6; i++) kb_keys[i] = 0;
}

static void enqueue_keyboard_state(void) {
    kb_report_t r = {0};
    r.modifiers = kb_modifiers;
    for (uint32_t i = 0; i < 6; i++) r.keys[i] = kb_keys[i];

    if (!kb_queue_empty()) {
        uint8_t last = (uint8_t)((kb_q_tail + KB_REPORT_QUEUE_SIZE - 1) % KB_REPORT_QUEUE_SIZE);
        if (kb_reports_equal(&kb_queue[last], &r)) return;
    }

    uint8_t next_tail = (uint8_t)((kb_q_tail + 1) % KB_REPORT_QUEUE_SIZE);
    if (next_tail == kb_q_head) {
        kb_q_head = (uint8_t)((kb_q_head + 1) % KB_REPORT_QUEUE_SIZE);
    }

    kb_queue[kb_q_tail] = r;
    kb_q_tail = next_tail;
}

static void flush_keyboard_queue(void) {
    if (kb_queue_empty() || !hid_ready()) return;
    kb_report_t *r = &kb_queue[kb_q_head];
    if (tud_hid_keyboard_report(USB_REPORT_ID_KEYBOARD, r->modifiers, r->keys)) {
        kb_q_head = (uint8_t)((kb_q_head + 1) % KB_REPORT_QUEUE_SIZE);
    }
}

static int8_t clamp_i16_to_i8(int16_t v) {
    if (v > 127) return 127;
    if (v < -127) return -127;
    return (int8_t)v;
}

static void flush_mouse_report(void) {
    if (!mouse_dirty || !hid_ready()) return;

    int8_t step_x = clamp_i16_to_i8(mouse_pending_x);
    int8_t step_y = clamp_i16_to_i8(mouse_pending_y);

    if (tud_hid_mouse_report(USB_REPORT_ID_MOUSE, mouse_buttons, step_x, step_y, 0, 0)) {
        mouse_pending_x = (int16_t)(mouse_pending_x - step_x);
        mouse_pending_y = (int16_t)(mouse_pending_y - step_y);
        mouse_dirty = (mouse_pending_x != 0) || (mouse_pending_y != 0);
    }
}

static void queue_mouse_delta(int8_t x, int8_t y) {
    mouse_pending_x = (int16_t)(mouse_pending_x + x);
    mouse_pending_y = (int16_t)(mouse_pending_y + y);
    mouse_dirty = true;
    flush_mouse_report();
}

static void queue_mouse_buttons(void) {
    mouse_dirty = true;
    flush_mouse_report();
}

static const key_map_t *find_mapping(uint8_t function) {
    for (uint32_t i = 0; i < (sizeof(key_map) / sizeof(key_map[0])); i++) {
        if (key_map[i].function == function) return &key_map[i];
    }
    return NULL;
}

static void clear_keyboard_state(void) {
    kb_modifiers = 0;
    clear_key_slots();
}

void hid_bridge_init(void) {
    clear_keyboard_state();
    kb_q_head = 0;
    kb_q_tail = 0;
    mouse_buttons = 0;
    mouse_pending_x = 0;
    mouse_pending_y = 0;
    mouse_dirty = false;
    was_mounted = false;
}

void hid_bridge_task(void) {
    bool mounted = tud_mounted();
    if (was_mounted && !mounted) {
        clear_keyboard_state();
        kb_q_head = 0;
        kb_q_tail = 0;
        mouse_buttons = 0;
        mouse_pending_x = 0;
        mouse_pending_y = 0;
        mouse_dirty = false;
    }
    was_mounted = mounted;

    flush_keyboard_queue();
    flush_mouse_report();
}

void hid_bridge_handle_sejin(const sejin_frame_t *frame) {
    if (frame->is_sejin1) {
        if (frame->function == 0xFF) {
            clear_keyboard_state();
            enqueue_keyboard_state();
            return;
        }

        const key_map_t *m = find_mapping(frame->function);
        if (!m) return;

        // Sejin-1 frame without "HOLD" is treated as key release.
        if (frame->toggle) {
            if (m->modifier != 0) {
                uint8_t next = (uint8_t)(kb_modifiers & (uint8_t)~m->modifier);
                if (next != kb_modifiers) {
                    kb_modifiers = next;
                    enqueue_keyboard_state();
                }
                return;
            }

            if (m->keycode != 0) {
                if (remove_key_from_slots(m->keycode)) {
                    enqueue_keyboard_state();
                } else {
                    // If we got only a release-like packet, synthesize tap.
                    if (add_key_to_slots(m->keycode)) {
                        enqueue_keyboard_state();
                        remove_key_from_slots(m->keycode);
                        enqueue_keyboard_state();
                    }
                }
            }
            return;
        }

        if (m->modifier != 0) {
            uint8_t next = (uint8_t)(kb_modifiers | m->modifier);
            if (next != kb_modifiers) {
                kb_modifiers = next;
                enqueue_keyboard_state();
            }
            return;
        }

        if (m->keycode != 0 && add_key_to_slots(m->keycode)) {
            enqueue_keyboard_state();
        }
        return;
    }

    if (frame->is_button_event) {
        if (frame->button_down) {
            if (frame->obc == 1) mouse_buttons |= MOUSE_BUTTON_LEFT;
            else if (frame->obc == 2) mouse_buttons |= MOUSE_BUTTON_RIGHT;
            else if (frame->obc == 3) mouse_buttons |= MOUSE_BUTTON_MIDDLE;
        } else {
            mouse_buttons = 0;
        }
        queue_mouse_buttons();
        return;
    }

    queue_mouse_delta(frame->x * 5, -frame->y * 5);
}
