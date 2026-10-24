# CYD Hardware Variants & Controller Guide

This document tracks hardware differences, controller variations, and troubleshooting findings across the ESP32 Cheap Yellow Display (CYD) boards supported by the Bluetooth Tracker, particularly boards labeled `ESP32-2432S028`.

---

## Supported Environments

| PlatformIO env | Board | Display (driver) | Touch | Backlight | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `cyd_28r` | `ESP32-2432S028R` 2.8" 320x240 | ILI9341 | XPT2046 (SPI) | GPIO 21 | Default / reference board. |
| `cyd_28r_inv` | Same as above | ILI9341 | XPT2046 (SPI) | GPIO 21 | Colors inverted (`TFT_INVERSION_ON`). |
| `cyd_28c` | `JC2432W328C` 2.8" 320x240 | ILI9341_2 | Capacitive (I2C) | GPIO 27 | Inversion on by default. |
| `cyd_28c_inv` | Same as above | ILI9341_2 | Capacitive (I2C) | GPIO 27 | Inversion off. |
| `cyd_28_nm` | `ESP32-2432S028` NodeMiner / Rockbase | ST7789 (BGR) | XPT2046 (SPI) | GPIO 21 | |
| `cyd_28_nm_inv` | Same as above | ST7789 (BGR) | XPT2046 (SPI) | GPIO 21 | Colors inverted. |
| `cyd_35c` | `ESP32-3248S035C` 3.5" 480x320 | ST7796 | Capacitive (I2C) | GPIO 27 | RGB LED pins swapped (Green=17, Blue=16). |
| `cyd_35c_inv` | Same as above | ST7796 | Capacitive (I2C) | GPIO 27 | Colors inverted. |

Build with `pio run -e <env>` (or `task build:<env>`). Variant-specific values are set as `build_flags` in `platformio.ini`; shared pins live in `cyd_base`.

> [!NOTE]
> The `cyd_28c` / `cyd_28c_inv` boards cannot be flashed by the maintainer. Use `task package -- cyd_28c cyd_28c_inv` to produce firmware ZIPs for testers.

### Variant-dependent code

| Concern | Where | Selected by |
| :--- | :--- | :--- |
| Display driver / resolution | `platformio.ini` | `*_DRIVER`, `TFT_WIDTH`, `TFT_HEIGHT` |
| Touch controller (resistive vs capacitive) | `src/touch_manager.cpp` | `HAS_CAPACITIVE_TOUCH` |
| Touch coordinate mapping | `src/display.cpp` (`mapTouchCoordinates`) | `TouchManager::isCapacitive()`, orientation |
| RGB status LED pins | `config/config.h` (`CYD_LED_*`) | Overridden per env in `platformio.ini` |
| Backlight pin | `platformio.ini` | `TFT_BL` |

---

## Known Variant Issues & Diagnostics

### 1. Partial Screen Rendering / 80px Noise Band (1/4 Screen Glitch)

#### Symptoms
* The device displays the UI across the left **240 columns**, while the remaining **80 columns** on the right show static / uninitialized RAM noise.
* UI is compressed into portrait dimensions even when held in landscape orientation.

#### Root Cause
1. **ST7789 Display Controller**: The board uses an ST7789 rather than the standard ILI9341. When initialized with `ILI9341_DRIVER`, addressing commands only cycle through the controller's 240-pixel base window.
2. **ILI9341 Clone Controller**: Several manufacturers use clone ILI9341 chips with non-standard initialization timing (see Bodmer's `TFT_eSPI` [Issue #1172](https://github.com/Bodmer/TFT_eSPI/issues/1172)).

#### Solutions
* Use the pre-configured `cyd_28_nm` / `cyd_28_nm_inv` environments (ST7789), or
* Try `-D ILI9341_2_DRIVER=1` with `-D TFT_WIDTH=240 -D TFT_HEIGHT=320`.

---

### 2. Inverted Colors

#### Symptoms
* Backgrounds that should be dark appear washed out or white; Catppuccin Mocha looks light and Latte looks dark.

#### Solution
* Flash the corresponding `_inv` binary (e.g. `cyd_28r_inv`, `cyd_28c_inv`, `cyd_35c_inv`), or add `-D TFT_INVERSION_ON=1` to the build flags.

---

### 3. Red & Blue Color Swapping (RGB vs BGR)

#### Symptoms
* Warm colors appear blue or cyan, while blue elements appear red or orange.

#### Solution
* Add `-D TFT_RGB_ORDER=TFT_BGR` (or toggle to `TFT_RGB`) in `platformio.ini`.

---

### 4. Wrong RGB LED Colors

#### Symptoms
* Status LED shows green when it should be blue (scanning) or vice versa.

#### Solution
* The 3.5" board swaps the green/blue pins. Override with `-D CYD_LED_GREEN=<pin> -D CYD_LED_BLUE=<pin>` (the `cyd_35c` envs already set Green=17, Blue=16).

---

## References

* Reference implementation: [witnessmenow/ESP32-Cheap-Yellow-Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
* Alternative driver discussion: [Bodmer/TFT_eSPI #1172](https://github.com/Bodmer/TFT_eSPI/issues/1172)
* Bruce Devices board configurations: [BruceDevices/firmware](https://github.com/pr3y/Bruce)
* Sibling project: [cyd-weather-station hardware variants](https://github.com/nicholaswilde/cyd-weather-station/blob/main/docs/hardware-variants.md)
