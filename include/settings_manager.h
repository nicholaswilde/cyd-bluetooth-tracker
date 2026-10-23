#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <stdint.h>
#include <string>
#include "config/config.h"

#ifdef NATIVE_TEST
#include "Arduino.h"
#else
#include <Arduino.h>
#endif

class SettingsManager {
public:
    SettingsManager();
    void begin();

    int getThemeFlavor() const { return _themeFlavor; }
    void setThemeFlavor(int flavor);

    int getBrightness() const { return _brightness; }
    void setBrightness(int brightness);

    bool getAutoBrightness() const { return _autoBrightness; }
    void setAutoBrightness(bool enabled);

    uint32_t getScanIntervalMs() const { return _scanIntervalMs; }
    void setScanIntervalMs(uint32_t ms);

    int getMinRssi() const { return _minRssi; }
    void setMinRssi(int rssi);

    bool getAudioGeiger() const { return _audioGeiger; }
    void setAudioGeiger(bool enabled);

    int getAudioVolume() const { return _audioVolume; }
    void setAudioVolume(int volume);

    int getScreenOrientation() const { return _screenOrientation; }
    void setScreenOrientation(int orientation);

    bool getLedEnabled() const { return _ledEnabled; }
    void setLedEnabled(bool enabled);

    int getLedBrightness() const { return _ledBrightness; }
    void setLedBrightness(int brightness);

    int getDeviceSortMode() const { return _deviceSortMode; }
    void setDeviceSortMode(int mode);

    int getDeviceFilterMode() const { return _deviceFilterMode; }
    void setDeviceFilterMode(int mode);

    std::string getPinnedMac() const { return _pinnedMac; }
    void setPinnedMac(const std::string& mac);

    void resetToDefaults();

private:
    int _themeFlavor;
    int _brightness;
    bool _autoBrightness;
    uint32_t _scanIntervalMs;
    int _minRssi;
    bool _audioGeiger;
    int _audioVolume;
    int _screenOrientation;
    bool _ledEnabled;
    int _ledBrightness;
    int _deviceSortMode;
    int _deviceFilterMode;
    std::string _pinnedMac;
};

extern SettingsManager settings;

#endif // SETTINGS_MANAGER_H
