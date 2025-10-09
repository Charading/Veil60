
/*
 * socd.c - Shego-style last-input-wins SOCD adapted for QMK
 *
 * Behavior summary:
 * - Tracks physical pressed state for W/A/S/D and suppressed state when both
 *   directions are held.
 * - When an event originates from a hall sensor (is_hall==true), this module
 *   will perform register_code/unregister_code to the host so hall presses
 *   act like regular keypresses.
 * - Last-input-wins: when opposing direction is already held, the newest
 *   press wins (we unregister the opposite and mark it suppressed). When the
 *   winner is released, the suppressed opposite (if still physically held)
 *   will be re-registered.
 */

#include "socd.h"
#include "quantum.h" // register_code/unregister_code, KC_*
#include "print.h"

static bool socd_enabled = true;

// track pressed state (physical sensors or mechanical)
static bool a_pressed = false;
static bool d_pressed = false;
static bool w_pressed = false;
static bool s_pressed = false;

// suppressed flags (true when the opposite was unregistered due to last-input-wins)
static bool a_suppressed = false;
static bool d_suppressed = false;
static bool w_suppressed = false;
static bool s_suppressed = false;

void socd_init(void) {
    a_pressed = d_pressed = w_pressed = s_pressed = false;
    a_suppressed = d_suppressed = w_suppressed = s_suppressed = false;
}

// Process a press/release for one of the monitored keys. If is_hall is true,
// this function will also perform register_code/unregister_code so hall
// sensors behave like real keypresses at the host. The return value indicates
// whether higher-level code should continue processing the event (true) or
// suppress it (false). For our usage we generally return true for mechanical
// events and return value isn't used for hall events since we send HID events
// directly here.
bool socd_process_key(uint16_t keycode, bool pressed, bool is_hall) {
    // Only handle W/A/S/D here
    if (keycode == KC_A) {
        if (pressed) {
            a_pressed = true;
            if (socd_enabled && is_hall && d_pressed) {
                if (!d_suppressed) {
                    unregister_code(KC_D);
                    d_suppressed = true;
                }
            }
            // If this is a hall event, also send host code now
            if (is_hall) register_code(KC_A);
            return true;
        } else {
            a_pressed = false;
            // If A had been suppressed earlier, clear the flag and suppress this release
            if (a_suppressed) {
                a_suppressed = false;
                return false;
            }
            // If this was a hall event, release host code now
            if (is_hall) unregister_code(KC_A);
            // If D is physically held and was suppressed, reassert it
            if (is_hall && d_pressed && d_suppressed) {
                register_code(KC_D);
                d_suppressed = false;
            }
            return true;
        }
    }

    if (keycode == KC_D) {
        if (pressed) {
            d_pressed = true;
            if (socd_enabled && is_hall && a_pressed) {
                if (!a_suppressed) {
                    unregister_code(KC_A);
                    a_suppressed = true;
                }
            }
            if (is_hall) register_code(KC_D);
            return true;
        } else {
            d_pressed = false;
            if (d_suppressed) {
                d_suppressed = false;
                return false;
            }
            if (is_hall) unregister_code(KC_D);
            if (is_hall && a_pressed && a_suppressed) {
                register_code(KC_A);
                a_suppressed = false;
            }
            return true;
        }
    }

    if (keycode == KC_W) {
        if (pressed) {
            w_pressed = true;
            if (socd_enabled && is_hall && s_pressed) {
                if (!s_suppressed) {
                    unregister_code(KC_S);
                    s_suppressed = true;
                }
            }
            if (is_hall) register_code(KC_W);
            return true;
        } else {
            w_pressed = false;
            if (w_suppressed) {
                w_suppressed = false;
                return false;
            }
            if (is_hall) unregister_code(KC_W);
            if (is_hall && s_pressed && s_suppressed) {
                register_code(KC_S);
                s_suppressed = false;
            }
            return true;
        }
    }

    if (keycode == KC_S) {
        if (pressed) {
            s_pressed = true;
            if (socd_enabled && is_hall && w_pressed) {
                if (!w_suppressed) {
                    unregister_code(KC_W);
                    w_suppressed = true;
                }
            }
            if (is_hall) register_code(KC_S);
            return true;
        } else {
            s_pressed = false;
            if (s_suppressed) {
                s_suppressed = false;
                return false;
            }
            if (is_hall) unregister_code(KC_S);
            if (is_hall && w_pressed && w_suppressed) {
                register_code(KC_W);
                w_suppressed = false;
            }
            return true;
        }
    }

    return true;
}

void socd_task(void) { /* no periodic work required */ }

bool socd_is_active(uint16_t keycode) {
    switch (keycode) {
        case KC_W: return w_pressed && !w_suppressed;
        case KC_A: return a_pressed && !a_suppressed;
        case KC_S: return s_pressed && !s_suppressed;
        case KC_D: return d_pressed && !d_suppressed;
        default: return false;
    }
}