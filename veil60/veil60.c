#include "veil60.h"
#include "socd.h"
#include "analog.h"
#include "matrix.h"
#include "timer.h"
#include "print.h"
#include "config.h"

// Access the core matrix rows directly so we can inject hall-sensor presses
extern matrix_row_t matrix[MATRIX_ROWS];

// Hall sensor states
static hall_sensor_t hall_sensors[4] = {0};
static uint32_t last_adc_print = 0;
// (debug bottom-row tracking removed)
// Track previous mechanical states for WASD so we can detect changes
static bool prev_mech_w = false;
static bool prev_mech_a = false;
static bool prev_mech_s = false;
static bool prev_mech_d = false;
// Encoder button is handled in the keymap's matrix_scan_user to present as a normal key
// Forward declare per-keyboard physical input processor to avoid implicit declaration
void veil60_process_physical_inputs(void);

// Hall effect sensor threshold (single threshold, no hysteresis)
// DRV5055A4: typical mid-scale ~2048 for 12-bit; values here are raw ADC units
// Actuation when ADC > HALL_THRESHOLD, release when ADC <= HALL_THRESHOLD
#define HALL_THRESHOLD 545

#include "bootloader.h"
// Note: BOOT keycode is defined in the keymap as BOOT_KEY (SAFE_RANGE). Do not redefine here.
// Bootloader helper is available via bootloader.h
// ...existing code...


// Hall sensor scanning (runs alongside normal matrix)
void hall_sensor_scan(void) {
    // Scan every cycle for gaming (no delay)
    hall_sensors_task();
    
    // Print ADC values every 100ms for calibration
    if (timer_elapsed32(last_adc_print) > 100) {
        print_adc_values();
        last_adc_print = timer_read32();
    }
}

void hall_sensors_init(void) {
    // Initialize ADC pins
    analogReadPin(KEY_W);
    analogReadPin(KEY_A);
    analogReadPin(KEY_S);
    analogReadPin(KEY_D);
    
    // Initialize SOCD
    socd_init();
    // Init encoder switch if defined
#ifdef ENCODER_BUTTON_PIN
    setPinInputHigh(ENCODER_BUTTON_PIN); // enable pull-up
#endif
    // Initialize debounce state for hall sensors
    for (int i = 0; i < 4; ++i) {
        hall_sensors[i].last_debounce = timer_read32();
        hall_sensors[i].raw_pressed = false;
        hall_sensors[i].previous_state = false;
        hall_sensors[i].sent_state = false;
        hall_sensors[i].desired_state = false;
        hall_sensors[i].raw_value = 0;
    }
}

void hall_sensors_task(void) {
    // Read all hall sensors
    read_hall_sensor(ADC_W_CHANNEL, &hall_sensors[0]);
    read_hall_sensor(ADC_A_CHANNEL, &hall_sensors[1]);
    read_hall_sensor(ADC_S_CHANNEL, &hall_sensors[2]);
    read_hall_sensor(ADC_D_CHANNEL, &hall_sensors[3]);
    
    // Process key states with SOCD - treat these as hall-originated events
    for (uint8_t i = 0; i < 4; ++i) {
        if (hall_sensors[i].is_pressed != hall_sensors[i].previous_state) {
            // Map sensor index to keycode
            uint16_t kc = KC_NO;
            switch (i) {
                case 0: kc = KC_W; break;
                case 1: kc = KC_A; break;
                case 2: kc = KC_S; break;
                case 3: kc = KC_D; break;
            }
            // Keep ADC-specific printing only (handled elsewhere). Do not print here.
            socd_process_key(kc, hall_sensors[i].is_pressed, true);
            hall_sensors[i].sent_state = hall_sensors[i].is_pressed;
            hall_sensors[i].previous_state = hall_sensors[i].is_pressed;
        }
    }
}

bool read_hall_sensor(uint8_t channel, hall_sensor_t* sensor) {
    uint16_t adc_value;
    
    switch (channel) {
        case ADC_W_CHANNEL:
            adc_value = analogReadPin(KEY_W);
            break;
        case ADC_A_CHANNEL:
            adc_value = analogReadPin(KEY_A);
            break;
        case ADC_S_CHANNEL:
            adc_value = analogReadPin(KEY_S);
            break;
        case ADC_D_CHANNEL:
            adc_value = analogReadPin(KEY_D);
            break;
        default:
            return false;
    }
    
    sensor->raw_value = adc_value;
    // Simple debounce: detect raw pressed state and require it to be stable for 30ms
    bool currently_raw = (adc_value > HALL_THRESHOLD);
    if (currently_raw != sensor->raw_pressed) {
        // state changed, reset debounce timer
        sensor->last_debounce = timer_read32();
        sensor->raw_pressed = currently_raw;
    }
    // Only commit to is_pressed if stable for 30ms
    if (timer_elapsed32(sensor->last_debounce) >= 30) {
        sensor->is_pressed = sensor->raw_pressed;
    }
    
    return true;
}

void print_adc_values(void) {
    // Only print if enabled by keymap (ADC debug prints are allowed)
    if (adc_printing_enabled) {
        uprintf("ADC: W=%d A=%d S=%d D=%d | Pressed: W=%d A=%d S=%d D=%d\n",
                hall_sensors[0].raw_value,
                hall_sensors[1].raw_value,
                hall_sensors[2].raw_value,
                hall_sensors[3].raw_value,
                hall_sensors[0].is_pressed,
                hall_sensors[1].is_pressed,
                hall_sensors[2].is_pressed,
                hall_sensors[3].is_pressed);
    }
}

// QMK callback: Initialize keyboard
void keyboard_pre_init_kb(void) {
    hall_sensors_init();
    keyboard_pre_init_user();
}

// QMK callback: Called every matrix scan cycle
void matrix_scan_kb(void) {
    // Scan hall effect sensors (runs on top of normal matrix)
    hall_sensor_scan();
    
    // Run SOCD task
    socd_task();
    
    // Inject hall sensor states directly into the matrix AFTER default scan completes
    // W at [1,2], A at [2,1], S at [2,2], D at [2,3]
    if (hall_sensors[0].is_pressed) {
        matrix[HALL_W_ROW] |= (1UL << HALL_W_COL);
    } else {
        matrix[HALL_W_ROW] &= ~(1UL << HALL_W_COL);
    }
    
    if (hall_sensors[1].is_pressed) {
        matrix[HALL_A_ROW] |= (1UL << HALL_A_COL);
    } else {
        matrix[HALL_A_ROW] &= ~(1UL << HALL_A_COL);
    }
    
    if (hall_sensors[2].is_pressed) {
        matrix[HALL_S_ROW] |= (1UL << HALL_S_COL);
    } else {
        matrix[HALL_S_ROW] &= ~(1UL << HALL_S_COL);
    }
    
    if (hall_sensors[3].is_pressed) {
        matrix[HALL_D_ROW] |= (1UL << HALL_D_COL);
    } else {
        matrix[HALL_D_ROW] &= ~(1UL << HALL_D_COL);
    }
    
    // Continue to per-keyboard physical input processing
    veil60_process_physical_inputs();
    
    // Call user's matrix_scan callback
    matrix_scan_user();
}

// (previous implementation that directly applied desired_state removed)

// Helper to check a mechanical matrix bit
static bool mechanical_pressed_at(uint8_t row, uint8_t col) {
    // Read the current matrix state from QMK's internal matrix
    // (we can't use the extern matrix[] directly in custom matrix mode)
    // Instead, read via matrix_get_row if available, or return false as fallback
    #ifdef MATRIX_ROWS
    if (row < MATRIX_ROWS) {
        matrix_row_t row_state = matrix_get_row(row);
        return (row_state & (1 << col)) != 0;
    }
    #endif
    return false;
}

// Apply SOCD using combined mechanical+hall sensors
// and ensure last-input-wins is enforced
void veil60_process_physical_inputs(void) {
    // W
    bool mech_w = mechanical_pressed_at(HALL_W_ROW, HALL_W_COL);
    if (mech_w != prev_mech_w) {
        prev_mech_w = mech_w;
        bool phys_w = mech_w || hall_sensors[0].is_pressed;
        socd_process_key(KC_W, phys_w, false);
    }

    // A
    bool mech_a = mechanical_pressed_at(HALL_A_ROW, HALL_A_COL);
    if (mech_a != prev_mech_a) {
        prev_mech_a = mech_a;
        bool phys_a = mech_a || hall_sensors[1].is_pressed;
        socd_process_key(KC_A, phys_a, false);
    }

    // S
    bool mech_s = mechanical_pressed_at(HALL_S_ROW, HALL_S_COL);
    if (mech_s != prev_mech_s) {
        prev_mech_s = mech_s;
        bool phys_s = mech_s || hall_sensors[2].is_pressed;
        socd_process_key(KC_S, phys_s, false);
    }

    // D
    bool mech_d = mechanical_pressed_at(HALL_D_ROW, HALL_D_COL);
    if (mech_d != prev_mech_d) {
        prev_mech_d = mech_d;
        bool phys_d = mech_d || hall_sensors[3].is_pressed;
        socd_process_key(KC_D, phys_d, false);
    }

}

// Trigger RP2040 bootloader safely from code or keymap
void veil60_bootloader(void) {
#if defined(RESET_USB_BOOT) || defined(usb_boot)
    // Prefer QMK helper if available
    reset_usb_boot(0, 0);
#elif defined(bootloader_jump)
    bootloader_jump();
#else
    // Fallback: use generic reset (may not enter UF2)
    reset_keyboard();
#endif
}

// (moved earlier)