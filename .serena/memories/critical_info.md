# CYD Bluetooth Tracker - Project Critical Information

## Core Architecture
- ESP32 Cheap Yellow Display (CYD) BLE tracker & proximity locator running LVGL 8.4 and Arduino framework.
- Main display and touch management: `src/display.cpp`, `src/touch_manager.cpp`, `src/ui.cpp`.
- Hardware configuration and pinouts: `include/config/config.h`, `platformio.ini`.
- BLE tracking and scanner: `src/bluetooth_tracker.cpp`, `include/bluetooth_tracker.h`.
- Peripheral management (backlight, LED, audio): `src/backlight_manager.cpp`, `src/led_manager.cpp`, `src/audio_manager.cpp`.
- Preferences and NVS storage: `src/settings_manager.cpp`, `include/settings_manager.h`.

## Hardware & Environment Invariants
- `cyd_28r`: Resistive touch 2.8" ILI9341 + XPT2046. Primary dev board.
- `cyd_28c`: Capacitive touch 2.8" JC2432W328C. User has NO physical hardware. Never attempt direct flashing; package binaries via `.agents/skills/package-binaries/package_binaries.sh cyd_28c`.
- `cyd_35c`: Capacitive touch 3.5" ESP32-3248S035C.

## Essential Constraints
- Adding settings: Follow the 5-point checklist in `AGENTS.md` and `mem:conventions`.
- Git/GH commands: Always prefix with `rtk` (e.g. `rtk git status`, `rtk gh issue list | cat`).
- Clangd LSP: Requires `compile_commands.json` (`pio run -t compiledb`).