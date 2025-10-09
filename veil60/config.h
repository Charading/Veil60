#pragma once

// Console for debugging (enabled via keyboard.json)


#define ENCODER_BUTTON_PIN GP14


// ADC configuration for DRV5055A4 hall effect sensors
#define ADC_BUFFER_DEPTH 4
#define ADC_SAMPLING_RATE 10000


// SOCD configuration
#define SOCD_TIMEOUT 50 // ms timeout for SOCD cleaning

// Custom matrix scanning interval
#define MATRIX_SCAN_RATE 1000 // 1kHz scan rate for hall effect sensors