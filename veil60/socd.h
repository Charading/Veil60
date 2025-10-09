// Simple SOCD header for last-input-wins behavior on W/A/S/D
#pragma once

#include "quantum.h"

void socd_init(void);
// Process a key event. If is_hall is true this module will send HID
// register/unregister calls so hall sensors behave as host keypresses.
bool socd_process_key(uint16_t keycode, bool pressed, bool is_hall);
void socd_task(void);
bool socd_is_active(uint16_t keycode);