# MCU and bootloader
MCU = RP2040
BOOTLOADER = rp2040

ENCODER_MAP_ENABLE = yes

# Analog driver for RP2040
ANALOG_DRIVER_REQUIRED = yes
ANALOG_DRIVER = rp2040_adc

# Enable VIA and persistent dynamic keymaps so VIA can save layout changes to EEPROM
DYNAMIC_KEYMAP_ENABLE = yes

# Include SOCD source files
SRC += socd.c