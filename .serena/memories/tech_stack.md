# Tech Stack

- **Languages**: C/C++ (C++14/C++17 for Arduino/ESP32), Python 3.11+ (build & utility scripts).
- **Framework & Build System**: Arduino Core on ESP32 (`platform = espressif32@6.9.0`), PlatformIO Core (`pio`), `go-task` (`Taskfile.yml`).
- **Target Boards (CYD Family)**:
  - `cyd_28r` / `cyd_28r_inv`: ESP32-2432S028R 2.8" Resistive (ILI9341 + XPT2046).
  - `cyd_28c` / `cyd_28c_inv`: JC2432W328C 2.8" Capacitive (no physical access for user; test via packaged binaries).
  - `cyd_28_nm` / `cyd_28_nm_inv`: CYD NodeMiner 2.8" (ST7789 + XPT2046).
  - `cyd_35c` / `cyd_35c_inv`: ESP32-3248S035C 3.5" Capacitive (ST7796 + GT911).
- **Core Libraries**:
  - `lvgl@8.4.0`: Embedded GUI engine (landscape 320x240 default, 480x320 for 3.5").
  - `catppuccin-rgb565`: Color token palette (Mocha, Macchiato, Frappe, Latte).
  - `TFT_eSPI@2.5.43`: Display driver.
  - `XPT2046_Touchscreen`: Resistive touch driver.
  - `h2zero/NimBLE-Arduino@^1.4.2`: Asynchronous BLE scan and tracking stack.
  - `ArduinoJson@6.21.6`: JSON serialization/deserialization.
  - ESP32 `Preferences`: Non-volatile storage (NVS) persistence.
- **Testing & Quality**: Unity runner via `pio test -e native`, `lcov` coverage, `designmd` for design specification linting.