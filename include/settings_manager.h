#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <stdint.h>
#include <string>
#include <vector>
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

    // Multi-device favorites / pinning
    const std::vector<std::string>& getPinnedMacs() const { return _pinnedMacs; }
    bool isMacPinned(const std::string& mac) const;
    bool addPinnedMac(const std::string& mac);
    bool removePinnedMac(const std::string& mac);
    bool togglePinnedMac(const std::string& mac);
    void clearPinnedMacs();
    size_t getPinnedCount() const { return _pinnedMacs.size(); }

    // Legacy single-target compatibility
    std::string getPinnedMac() const;
    void setPinnedMac(const std::string& mac);

    void resetToDefaults();

private:
    void savePinnedMacs();

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
    std::vector<std::string> _pinnedMacs;
};

extern SettingsManager settings;

#endif // SETTINGS_MANAGER_H
