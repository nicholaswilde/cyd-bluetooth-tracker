#include "settings_manager.h"
#include <sstream>
#include <algorithm>
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
      _pinnedMacs() {}

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

        _pinnedMacs.clear();
        String macs = prefs.getString("pinned_macs", "");
        if (macs.length() == 0) {
            String legacy = prefs.getString("pinned_mac", "");
            if (legacy.length() > 0) {
                _pinnedMacs.push_back(legacy.c_str());
            }
        } else {
            std::string s = macs.c_str();
            std::stringstream ss(s);
            std::string item;
            while (std::getline(ss, item, ',')) {
                size_t first = item.find_first_not_of(" \t\r\n");
                if (first == std::string::npos) continue;
                size_t last = item.find_last_not_of(" \t\r\n");
                std::string trimmed = item.substr(first, (last - first + 1));
                if (!trimmed.empty() && _pinnedMacs.size() < MAX_PINNED_DEVICES) {
                    _pinnedMacs.push_back(trimmed);
                }
            }
        }
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

bool SettingsManager::isMacPinned(const std::string& mac) const {
    if (mac.empty()) return false;
    for (const auto& m : _pinnedMacs) {
        if (m == mac) return true;
    }
    return false;
}

bool SettingsManager::addPinnedMac(const std::string& mac) {
    if (mac.empty()) return false;
    if (isMacPinned(mac)) return false;
    if (_pinnedMacs.size() >= MAX_PINNED_DEVICES) return false;
    _pinnedMacs.push_back(mac);
    savePinnedMacs();
    return true;
}

bool SettingsManager::removePinnedMac(const std::string& mac) {
    if (mac.empty()) return false;
    for (auto it = _pinnedMacs.begin(); it != _pinnedMacs.end(); ++it) {
        if (*it == mac) {
            _pinnedMacs.erase(it);
            savePinnedMacs();
            return true;
        }
    }
    return false;
}

bool SettingsManager::togglePinnedMac(const std::string& mac) {
    if (isMacPinned(mac)) {
        return removePinnedMac(mac);
    } else {
        return addPinnedMac(mac);
    }
}

void SettingsManager::clearPinnedMacs() {
    _pinnedMacs.clear();
    savePinnedMacs();
}

std::string SettingsManager::getPinnedMac() const {
    return _pinnedMacs.empty() ? "" : _pinnedMacs[0];
}

void SettingsManager::setPinnedMac(const std::string& mac) {
    _pinnedMacs.clear();
    if (!mac.empty()) {
        _pinnedMacs.push_back(mac);
    }
    savePinnedMacs();
}

void SettingsManager::savePinnedMacs() {
    Preferences prefs;
    if (prefs.begin("ble_tracker", false)) {
        std::string joined = "";
        for (size_t i = 0; i < _pinnedMacs.size(); ++i) {
            if (i > 0) joined += ",";
            joined += _pinnedMacs[i];
        }
        prefs.putString("pinned_macs", joined.c_str());
        prefs.putString("pinned_mac", getPinnedMac().c_str());
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
    clearPinnedMacs();

    Preferences prefs;
    if (prefs.begin("ble_tracker", false)) {
        prefs.clear();
        prefs.end();
    }
}
