#pragma once

#include "quantum.h"
#include "analog.h"


// ADC pins for WASD hall effect sensors (DRV5055A4)
#define KEY_W GP26
#define KEY_A GP27
#define KEY_S GP28
#define KEY_D GP29

// ADC channel mapping for RP2040
#define ADC_W_CHANNEL 0  // GP26 = ADC0
#define ADC_A_CHANNEL 1  // GP27 = ADC1
#define ADC_S_CHANNEL 2  // GP28 = ADC2
#define ADC_D_CHANNEL 3  // GP29 = ADC3

// Hall effect sensor state
typedef struct {
    uint16_t raw_value;
    bool is_pressed;
    bool previous_state;
    // Whether we've sent a key press to the host for this sensor
    bool sent_state;
    // Desired active state determined by SOCD processing (applied to matrix later)
    bool desired_state;
    // Debounce bookkeeping
    uint32_t last_debounce;
    bool raw_pressed;
} hall_sensor_t;

// Global flag for ADC printing (controlled by keymap)
extern bool adc_printing_enabled;

// Function declarations
void hall_sensors_init(void);
void hall_sensors_task(void);
bool read_hall_sensor(uint8_t channel, hall_sensor_t* sensor);
void print_adc_values(void);
void hall_sensor_scan(void);

