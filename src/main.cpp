#include <Arduino.h>
#include <lvgl.h>
#include "config/config.h"
#include "settings_manager.h"
#include "display.h"
#include "backlight_manager.h"
#include "bluetooth_tracker.h"
#include "led_manager.h"
#include "audio_manager.h"
#include "screenshot_manager.h"
#include "sd_card_manager.h"
#include "button_manager.h"
#include "ui.h"

BacklightManager backlight(TFT_BL, 0, 10.0f);
ButtonManager buttonManager(BOOT_BUTTON_PIN);

static unsigned long lastBacklightUpdate = 0;
static unsigned long lastUiRefresh = 0;

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n--- ESP32 CYD Bluetooth Tracker Starting ---");

    // Initialize Settings from NVS
    settings.begin();

    // Initialize Screenshot Manager (USB Serial frame dump listener)
    ScreenshotManager::init();

    // Initialize RGB Status LED
    led.begin();
    led.setEnabled(settings.getLedEnabled());
    led.setBrightness(settings.getLedBrightness());
    led.setState(LedManager::STATE_SCANNING);

    // Initialize Audio Manager (GPIO 26 buzzer / Geiger counter)
    audio.begin();
    audio.setEnabled(settings.getAudioGeiger());
    audio.setVolume(settings.getAudioVolume());

    // Initialize physical BOOT button (GPIO 0)
    buttonManager.begin();

    // Initialize MicroSD Card Manager
    SdCardManager::begin();

    // Initialize Display and Touch (must run before backlight so tft.init does not clobber LEDC PWM pin)
    initDisplayAndTouch();

    // Initialize Backlight (configured after display init so LEDC controls TFT_BL)
    backlight.begin();
    if (settings.getAutoBrightness()) {
        uint16_t ldrVal = analogRead(LDR_PIN);
        backlight.update(ldrVal);
    } else {
        backlight.setManualBrightness(settings.getBrightness());
    }

    // Initialize LVGL core & drivers
    initLVGL();

    // Initialize LVGL UI widgets & tabs
    initUI();

    // Initialize BLE Tracker engine
    bleTracker.begin();
    bleTracker.triggerScan();

    Serial.println("Initialization complete, starting BLE tracker loop.");
}

void loop() {
    unsigned long now = millis();

    // Check USB Serial for screenshot capture or tab navigation commands
    ScreenshotManager::checkSerial();

    // Check physical BOOT button actions
    ButtonAction action = buttonManager.update(now);
    if (action == ButtonAction::SINGLE_PRESS) {
        Serial.println("[System] BOOT button single press: Triggering BLE rescan...");
        showUIStatusMessage("Scanning BLE...");
        bleTracker.triggerScan();
        led.setState(LedManager::STATE_SCANNING);
    } else if (action == ButtonAction::LONG_PRESS) {
        Serial.println("[System] BOOT button long press: Capturing screenshot to SD...");
        std::string filename = ScreenshotManager::generateFilename(nullptr, now);
        bool ok = ScreenshotManager::captureToSD(filename.c_str());
        if (ok) {
            showUIStatusMessage("Screenshot saved!");
            led.setState(LedManager::STATE_PULSE_WHITE);
        } else {
            showUIStatusMessage("SD Card Missing!");
            led.setState(LedManager::STATE_ALERT_RED);
        }
    }

    // Non-blocking RGB status LED updates
    led.update(now);

    // Audio Geiger counter beeper updates
    audio.update(now);

    // Let LVGL process UI events and render dirty areas
    lv_timer_handler();

    // Check BLE scan status & prune stale devices
    bleTracker.update(now);

    // Periodic UI refresh (update proximity meter & list every 250ms)
    if (now - lastUiRefresh >= 250) {
        lastUiRefresh = now;
        updateUI();
    }

    // Periodically update backlight from ambient light sensor (LDR) if auto-brightness is enabled
    if (settings.getAutoBrightness()) {
        if (now - lastBacklightUpdate >= 500) {
            lastBacklightUpdate = now;
            uint16_t ldrVal = analogRead(LDR_PIN);
            backlight.update(ldrVal);
        }
    }

    delay(5);
}
