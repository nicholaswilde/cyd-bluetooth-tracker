# Code Conventions & Invariants

## Adding Settings (5-Point Checklist)
Every new setting or configuration option MUST be implemented across all 5 integration points:
1. `include/config/config.h`: Add default constant definitions.
2. `include/settings_manager.h` & `src/settings_manager.cpp`: Add getters, setters, NVS keys, and default fallbacks.
3. `src/ui.cpp` (Tab 4 Settings): Add UI control (toggle switch, slider, dropdown) and synchronize in `initUI()` and `refreshThemeUI()`.
4. Consumer Subsystems: Wire setting into destination subsystem (`backlight_manager`, `bluetooth_tracker`, `audio_manager`, `led_manager`, `display`, etc.).
5. `tests/test_settings/test_settings.cpp`: Add unit tests for defaults, bounds clamping, reboot persistence simulation, and factory reset.

## UI & Hardware Guidelines
- **UI Resolution**: Fixed 320x240 landscape (CYD 2.8" default; 480x320 for 3.5").
- **Touch Targets**: Minimum interactive touch area >= 32x32 px (preferably >= 40x40 px) for reliable resistive/capacitive response.
- **Theming**: Use Catppuccin RGB565 tokens exclusively. Support 4 flavors (`Mocha`, `Macchiato`, `Frappe`, `Latte`).
- **Non-blocking BLE**: Asynchronous background scanning via NimBLE-Arduino; never block LVGL render loops or touch handling.
- **Capacitive Board Testing (`cyd_28c`)**: User does not possess physical hardware; do not attempt direct flash. Always package binaries via `.agents/skills/package-binaries/package_binaries.sh cyd_28c`.
- **Regression Protection**: Every bugfix or feature issue resolution requires companion regression tests under `tests/`.