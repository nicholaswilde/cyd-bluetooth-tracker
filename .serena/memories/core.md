# CYD Bluetooth Tracker - Core Architecture & Source Map

The ESP32 CYD Bluetooth Tracker is a touch-enabled BLE locator and scanner built for the ESP32 Cheap Yellow Display hardware family running LVGL v8 and styled with Catppuccin RGB565.

## Reference Graph
- **Technology Stack & Dependencies**: `mem:tech_stack` covers MCU platforms, framework versions, display/touch drivers, and BLE stack.
- **Workflow & Commands**: `mem:suggested_commands` covers build, test, compilation database, flashing, and `rtk` tool usage.
- **Design & Coding Conventions**: `mem:conventions` details the 5-point settings checklist, touch targets, and non-blocking rules.
- **Verification & Gates**: `mem:task_completion` provides the exact commands required before declaring tasks complete.
- **Critical Constraints**: `mem:critical_info` contains hardware caveats and safety rules (e.g. no physical device for `cyd_28c`).

## Source Map
- `src/main.cpp`: System entry point; initializes subsystems, runs LVGL ticker and async background tasks.
- `src/bluetooth_tracker.cpp` / `include/bluetooth_tracker.h`: NimBLE scan loop, advertisement parser, device catalog, RSSI proximity tracker, target lock.
- `src/ui.cpp` / `include/ui.h`: LVGL tabview interface (Tracker, Devices, Details, Settings tabs) and Catppuccin color bindings.
- `src/settings_manager.cpp` / `include/settings_manager.h`: NVS persistence via ESP32 `Preferences` for brightness, themes, audio, scan intervals, pins.
- `src/display.cpp` & `src/touch_manager.cpp`: TFT_eSPI display and XPT2046 resistive / I2C capacitive touch driver integration.
- `src/backlight_manager.cpp`: PWM LEDC backlight control and LDR auto-dimming.
- `src/audio_manager.cpp` & `src/led_manager.cpp`: Acoustic Geiger counter beeps and active-low RGB status LED feedback.
- `include/config/config.h`: Board pinouts, default scan parameters, hardware pin definitions.
- `tests/`: Host-native Unity test suites executed under PlatformIO native environment.