#include "settings_manager.h"
#ifndef NATIVE_TEST
#include <Preferences.h>
#else
#include "Preferences.h"
#endif

SettingsManager settings;

SettingsManager::SettingsManager()
    : _themeFlavor(DEFAULT_THEME_FLAVOR),
      _brightness(DEFAULT_BRIGHTNESS),
      _autoBrightness(DEFAULT_AUTO_BRIGHTNESS),
      _scanIntervalMs(DEFAULT_SCAN_INTERVAL_MS),
      _minRssi(DEFAULT_MIN_RSSI),
      _audioGeiger(DEFAULT_AUDIO_GEIGER),
      _audioVolume(DEFAULT_AUDIO_VOLUME),
      _screenOrientation(DEFAULT_SCREEN_ORIENTATION),
      _ledEnabled(DEFAULT_LED_ENABLED),
      _ledBrightness(DEFAULT_LED_BRIGHTNESS),
      _deviceSortMode(DEFAULT_DEVICE_SORT_MODE),
      _deviceFilterMode(DEFAULT_DEVICE_FILTER_MODE),
      _pinnedMac("") {}

void SettingsManager::begin() {
    Preferences prefs;
    if (prefs.begin("ble_tracker", true)) {
        _themeFlavor = prefs.getInt("theme", DEFAULT_THEME_FLAVOR);
        _brightness = prefs.getInt("brightness", DEFAULT_BRIGHTNESS);
        _autoBrightness = prefs.getBool("auto_bright", DEFAULT_AUTO_BRIGHTNESS);
        _scanIntervalMs = prefs.getUInt("scan_interval", DEFAULT_SCAN_INTERVAL_MS);
        _minRssi = prefs.getInt("min_rssi", DEFAULT_MIN_RSSI);
        _audioGeiger = prefs.getBool("audio_geiger", DEFAULT_AUDIO_GEIGER);
        _audioVolume = prefs.getInt("audio_vol", DEFAULT_AUDIO_VOLUME);
        _screenOrientation = prefs.getInt("orientation", DEFAULT_SCREEN_ORIENTATION);
        _ledEnabled = prefs.getBool("led_en", DEFAULT_LED_ENABLED);
        _ledBrightness = prefs.getInt("led_bright", DEFAULT_LED_BRIGHTNESS);
        _deviceSortMode = prefs.getInt("dev_sort", DEFAULT_DEVICE_SORT_MODE);
        _deviceFilterMode = prefs.getInt("dev_filter", DEFAULT_DEVICE_FILTER_MODE);
        String mac = prefs.getString("pinned_mac", "");
        _pinnedMac = mac.c_str();
        prefs.end();
    }
}

void SettingsManager::setThemeFlavor(int flavor) {
    _themeFlavor = flavor;
    Preferences prefs;
    if (prefs.begin("ble_tracker", false)) {
        prefs.putInt("theme", _themeFlavor);
        prefs.end();
    }
}

void SettingsManager::setBrightness(int brightness) {
    if (brightness < 10) brightness = 10;
    if (brightness > 100) brightness = 100;
    _brightness = brightness;
    Preferences prefs;
    if (prefs.begin("ble_tracker", false)) {
        prefs.putInt("brightness", _brightness);
        prefs.end();
    }
}

void SettingsManager::setAutoBrightness(bool enabled) {
    _autoBrightness = enabled;
    Preferences prefs;
    if (prefs.begin("ble_tracker", false)) {
        prefs.putBool("auto_bright", _autoBrightness);
        prefs.end();
    }
}

void SettingsManager::setScanIntervalMs(uint32_t ms) {
    if (ms < 500) ms = 500;
    _scanIntervalMs = ms;
    Preferences prefs;
    if (prefs.begin("ble_tracker", false)) {
        prefs.putUInt("scan_interval", _scanIntervalMs);
        prefs.end();
    }
}

void SettingsManager::setMinRssi(int rssi) {
    if (rssi < -95) rssi = -95;
    if (rssi > -40) rssi = -40;
    _minRssi = rssi;
    Preferences prefs;
    if (prefs.begin("ble_tracker", false)) {
        prefs.putInt("min_rssi", _minRssi);
        prefs.end();
    }
}

void SettingsManager::setAudioGeiger(bool enabled) {
    _audioGeiger = enabled;
    Preferences prefs;
    if (prefs.begin("ble_tracker", false)) {
        prefs.putBool("audio_geiger", _audioGeiger);
        prefs.end();
    }
}

void SettingsManager::setAudioVolume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    _audioVolume = volume;
    Preferences prefs;
    if (prefs.begin("ble_tracker", false)) {
        prefs.putInt("audio_vol", _audioVolume);
        prefs.end();
    }
}

void SettingsManager::setScreenOrientation(int orientation) {
    _screenOrientation = orientation;
    Preferences prefs;
    if (prefs.begin("ble_tracker", false)) {
        prefs.putInt("orientation", _screenOrientation);
        prefs.end();
    }
}

void SettingsManager::setLedEnabled(bool enabled) {
    _ledEnabled = enabled;
    Preferences prefs;
    if (prefs.begin("ble_tracker", false)) {
        prefs.putBool("led_en", _ledEnabled);
        prefs.end();
    }
}

void SettingsManager::setLedBrightness(int brightness) {
    if (brightness < 0) brightness = 0;
    if (brightness > 255) brightness = 255;
    _ledBrightness = brightness;
    Preferences prefs;
    if (prefs.begin("ble_tracker", false)) {
        prefs.putInt("led_bright", _ledBrightness);
        prefs.end();
    }
}

void SettingsManager::setDeviceSortMode(int mode) {
    if (mode < 0) mode = 0;
    if (mode > 2) mode = 2;
    _deviceSortMode = mode;
    Preferences prefs;
    if (prefs.begin("ble_tracker", false)) {
        prefs.putInt("dev_sort", _deviceSortMode);
        prefs.end();
    }
}

void SettingsManager::setDeviceFilterMode(int mode) {
    if (mode < 0) mode = 0;
    if (mode > 2) mode = 2;
    _deviceFilterMode = mode;
    Preferences prefs;
    if (prefs.begin("ble_tracker", false)) {
        prefs.putInt("dev_filter", _deviceFilterMode);
        prefs.end();
    }
}

void SettingsManager::setPinnedMac(const std::string& mac) {
    _pinnedMac = mac;
    Preferences prefs;
    if (prefs.begin("ble_tracker", false)) {
        prefs.putString("pinned_mac", _pinnedMac.c_str());
        prefs.end();
    }
}

void SettingsManager::resetToDefaults() {
    _themeFlavor = DEFAULT_THEME_FLAVOR;
    _brightness = DEFAULT_BRIGHTNESS;
    _autoBrightness = DEFAULT_AUTO_BRIGHTNESS;
    _scanIntervalMs = DEFAULT_SCAN_INTERVAL_MS;
    _minRssi = DEFAULT_MIN_RSSI;
    _audioGeiger = DEFAULT_AUDIO_GEIGER;
    _audioVolume = DEFAULT_AUDIO_VOLUME;
    _screenOrientation = DEFAULT_SCREEN_ORIENTATION;
    _ledEnabled = DEFAULT_LED_ENABLED;
    _ledBrightness = DEFAULT_LED_BRIGHTNESS;
    _deviceSortMode = DEFAULT_DEVICE_SORT_MODE;
    _deviceFilterMode = DEFAULT_DEVICE_FILTER_MODE;
    _pinnedMac = "";

    Preferences prefs;
    if (prefs.begin("ble_tracker", false)) {
        prefs.clear();
        prefs.end();
    }
}
