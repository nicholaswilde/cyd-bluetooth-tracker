#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include "catppuccin.h"

// Display Performance Settings
#define DISPLAY_DRAW_BUF_ROWS 30
#define DISPLAY_REFR_PERIOD_MS 20
#define DISPLAY_INDEV_READ_PERIOD_MS 10
#define DEFAULT_SCREEN_ORIENTATION 1  // Landscape (0 deg)

// Catppuccin Flavor Definitions
#define CATPPUCCIN_MOCHA      1
#define CATPPUCCIN_MACCHIATO  2
#define CATPPUCCIN_FRAPPE     3
#define CATPPUCCIN_LATTE      4

// Default Theme Flavor
#define DEFAULT_THEME_FLAVOR CATPPUCCIN_MOCHA

// Default Backlight Settings
#define DEFAULT_BRIGHTNESS 100
#define DEFAULT_AUTO_BRIGHTNESS false

#define TO_LV_COLOR(c) (lv_color_t){.full = (c)}

// Bluetooth Tracker Settings
#define DEFAULT_SCAN_INTERVAL_MS 2000
#define DEFAULT_MIN_RSSI -90
#define DEFAULT_AUDIO_GEIGER true
#define DEFAULT_AUDIO_VOLUME 80

// RSSI Distance Zones (dBm)
#define RSSI_CEILING -30
#define RSSI_FLOOR -100
#define RSSI_ZONE_IMMEDIATE -45
#define RSSI_ZONE_NEAR -65
#define RSSI_ZONE_MID -80
#define RSSI_ZONE_FAR -95

// Proximity Trend Detection
// Least-squares RSSI slope (dB per sample) beyond which the signal is considered
// improving/degrading; smaller values are treated as noise (STABLE).
#define RSSI_TREND_SLOPE_THRESHOLD 1.0f
#define RSSI_TREND_MIN_SAMPLES 3

// Device Catalog Sort & Filter Defaults
#define DEFAULT_DEVICE_SORT_MODE 0    // 0: RSSI (Strongest), 1: Name (A-Z), 2: Last Seen
#define DEFAULT_DEVICE_FILTER_MODE 0  // 0: All, 1: Named Only, 2: Pinned Only
#define MAX_PINNED_DEVICES 8

// Hardware Pins
#define LDR_PIN 34
#define BOOT_BUTTON_PIN 0
#define SPEAKER_PIN 26

// MicroSD Card SPI Settings (VSPI)
#ifndef SD_CS_PIN
#define SD_CS_PIN 5
#endif
#ifndef SD_SCK_PIN
#define SD_SCK_PIN 18
#endif
#ifndef SD_MISO_PIN
#define SD_MISO_PIN 19
#endif
#ifndef SD_MOSI_PIN
#define SD_MOSI_PIN 23
#endif

// RGB LED Pins (Active LOW)
#ifndef CYD_LED_RED
#define CYD_LED_RED 4
#endif

#ifndef CYD_LED_GREEN
#define CYD_LED_GREEN 16
#endif

#ifndef CYD_LED_BLUE
#define CYD_LED_BLUE 17
#endif

// Default LED Settings
#define DEFAULT_LED_ENABLED true
#define DEFAULT_LED_BRIGHTNESS 255

#endif // CONFIG_H
