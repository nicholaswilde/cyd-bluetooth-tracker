#ifndef BLUETOOTH_TRACKER_H
#define BLUETOOTH_TRACKER_H

#include <stdint.h>
#include <string>
#include <vector>
#include <cmath>
#include "config/config.h"

enum BleDeviceType {
    DEVICE_TYPE_GENERIC,
    DEVICE_TYPE_APPLE,
    DEVICE_TYPE_TILE,
    DEVICE_TYPE_SAMSUNG,
    DEVICE_TYPE_FAST_PAIR,
    DEVICE_TYPE_IBEACON
};

struct BleDevice {
    std::string mac;
    std::string name;
    int8_t rssi;
    int8_t txPower;
    uint16_t manufacturerId;
    std::string manufacturerDataHex;
    std::string serviceUuid;
    unsigned long lastSeen;
    bool isPinned;
    BleDeviceType type;

    float getEstimatedDistance() const {
        if (rssi == 0) return -1.0f;
        int8_t refPower = (txPower != 0) ? txPower : -59;
        float ratio = (float)(refPower - rssi) / (10.0f * 2.2f);
        return powf(10.0f, ratio);
    }
};

class BluetoothTracker {
public:
    BluetoothTracker();

    void begin();
    void triggerScan();
    bool update(unsigned long currentMillis);

    const std::vector<BleDevice>& getDevices() const { return _devices; }
    std::vector<BleDevice> getFilteredDevices(int sortMode, int filterMode, int minRssi) const;

    // Target tracking
    void setTrackedTarget(const std::string& mac);
    void clearTrackedTarget();
    std::string getTrackedTarget() const { return _trackedMac; }
    bool isTracking() const { return !_trackedMac.empty(); }
    const BleDevice* getTrackedDevice() const;

    // Proximity metrics
    int8_t getSmoothedRssi() const;
    void getRssiHistory(int8_t* outHistory, size_t count) const;
    size_t getDiscoveredCount() const { return _devices.size(); }
    bool isScanning() const { return _isScanning; }

    // Mock helper for tests
    void addOrUpdateDevice(const BleDevice& dev);
    void clearDevices();

private:
    void processFoundDevice(const std::string& mac, const std::string& name,
                            int8_t rssi, int8_t txPower,
                            uint16_t mfgId, const std::string& mfgHex,
                            const std::string& serviceUuid, unsigned long now);

    std::vector<BleDevice> _devices;
    std::string _trackedMac;

    static constexpr size_t HISTORY_SIZE = 8;
    int8_t _rssiHistory[HISTORY_SIZE];
    size_t _historyIndex;
    size_t _historyCount;

    bool _isScanning;
    unsigned long _lastScanStartTime;
    unsigned long _lastPruneTime;

    friend class ScanCallbackHandler;
};

extern BluetoothTracker bleTracker;

#endif // BLUETOOTH_TRACKER_H
