#include "bluetooth_tracker.h"
#include "settings_manager.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

#ifndef NATIVE_TEST
#include <Arduino.h>
#include <NimBLEDevice.h>
#else
#include "Arduino.h"
#endif

BluetoothTracker bleTracker;

#ifndef NATIVE_TEST
class ScanCallbackHandler : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* dev) override {
        std::string mac = dev->getAddress().toString();
        std::string name = dev->getName();
        int8_t rssi = dev->getRSSI();
        int8_t txPower = dev->haveTXPower() ? dev->getTXPower() : 0;

        uint16_t mfgId = 0;
        std::string mfgHex = "";
        if (dev->haveManufacturerData()) {
            std::string rawMfg = dev->getManufacturerData();
            if (rawMfg.size() >= 2) {
                mfgId = (uint8_t)rawMfg[0] | ((uint8_t)rawMfg[1] << 8);
            }
            std::ostringstream oss;
            for (unsigned char c : rawMfg) {
                oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
            }
            mfgHex = oss.str();
        }

        std::string serviceUuid = "";
        if (dev->haveServiceUUID()) {
            serviceUuid = dev->getServiceUUID().toString();
        }

        bleTracker.processFoundDevice(mac, name, rssi, txPower, mfgId, mfgHex, serviceUuid, millis());
    }
};

static ScanCallbackHandler scanCallbacks;
#endif

constexpr size_t BluetoothTracker::HISTORY_SIZE;

BluetoothTracker::BluetoothTracker()
    : _isScanning(false),
      _lastScanStartTime(0),
      _lastPruneTime(0),
      _historyIndex(0),
      _historyCount(0),
      _catalogVersion(0),
      _scanIntervalMs(DEFAULT_SCAN_INTERVAL_MS),
      _scanWindowMs(DEFAULT_SCAN_INTERVAL_MS * 9 / 10) {
    for (size_t i = 0; i < HISTORY_SIZE; ++i) {
        _rssiHistory[i] = RSSI_FLOOR;
    }
}

void BluetoothTracker::begin() {
    uint32_t interval = settings.getScanIntervalMs();
    if (interval == 0) interval = DEFAULT_SCAN_INTERVAL_MS;

#ifndef NATIVE_TEST
    NimBLEDevice::init("CYD-BLE-Tracker");
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&scanCallbacks, true);
    pScan->setActiveScan(true);
#endif

    setScanInterval(interval);

    // Restore pinned target from settings if present
    std::string pinned = settings.getPinnedMac();
    if (!pinned.empty()) {
        _trackedMac = pinned;
    }
}

void BluetoothTracker::setScanInterval(uint32_t intervalMs) {
    if (intervalMs < 100) intervalMs = 100;
    _scanIntervalMs = intervalMs;
    _scanWindowMs = (intervalMs <= 200) ? (intervalMs - 1) : (intervalMs * 9 / 10);

#ifndef NATIVE_TEST
    NimBLEScan* pScan = NimBLEDevice::getScan();
    if (pScan) {
        bool wasScanning = _isScanning;
        if (wasScanning) {
            pScan->stop();
        }
        uint16_t itvl = (_scanIntervalMs > 65535) ? 65535 : (uint16_t)_scanIntervalMs;
        uint16_t win = (_scanWindowMs > 65535) ? 65535 : (uint16_t)_scanWindowMs;
        pScan->setInterval(itvl);
        pScan->setWindow(win);
        if (wasScanning) {
            pScan->start(0, nullptr, false);
        }
    }
#endif
}

void BluetoothTracker::triggerScan() {
#ifndef NATIVE_TEST
    NimBLEScan* pScan = NimBLEDevice::getScan();
    if (pScan && !_isScanning) {
        pScan->start(0, nullptr, false); // Continuous asynchronous scan
        _isScanning = true;
        _lastScanStartTime = millis();
    }
#else
    _isScanning = true;
    _lastScanStartTime = millis();
#endif
}

void BluetoothTracker::processFoundDevice(const std::string& mac, const std::string& name,
                                         int8_t rssi, int8_t txPower,
                                         uint16_t mfgId, const std::string& mfgHex,
                                         const std::string& serviceUuid, unsigned long now) {
    BleDeviceType type = DEVICE_TYPE_GENERIC;
    if (mfgId == 0x004C) {
        if (mfgHex.find("0215") == 4) { // iBeacon prefix
            type = DEVICE_TYPE_IBEACON;
        } else {
            type = DEVICE_TYPE_APPLE;
        }
    } else if (mfgId == 0x0075) {
        type = DEVICE_TYPE_SAMSUNG;
    } else if (serviceUuid.find("feed") != std::string::npos) {
        type = DEVICE_TYPE_TILE;
    } else if (serviceUuid.find("fe2c") != std::string::npos) {
        type = DEVICE_TYPE_FAST_PAIR;
    }

    bool isPinned = settings.isMacPinned(mac);

    // Update existing device or append new one
    bool found = false;
    for (auto& d : _devices) {
        if (d.mac == mac) {
            d.rssi = rssi;
            if (!name.empty() && d.name != name) {
                d.name = name;
                _catalogVersion++;
            }
            if (txPower != 0) d.txPower = txPower;
            if (mfgId != 0) d.manufacturerId = mfgId;
            if (!mfgHex.empty()) d.manufacturerDataHex = mfgHex;
            if (!serviceUuid.empty()) d.serviceUuid = serviceUuid;
            d.lastSeen = now;
            if (d.type != type) {
                d.type = type;
                _catalogVersion++;
            }
            if (d.isPinned != isPinned) {
                d.isPinned = isPinned;
                _catalogVersion++;
            }
            if (d.vendor.empty() || d.vendor == "Unknown Vendor") {
                const char* resolved = getDeviceVendor(d);
                if (strcmp(resolved, "Unknown Vendor") != 0 && d.vendor != resolved) {
                    d.vendor = resolved;
                    _catalogVersion++;
                }
            }
            found = true;
            break;
        }
    }

    if (!found) {
        BleDevice newDev;
        newDev.mac = mac;
        newDev.name = name.empty() ? "Unknown" : name;
        newDev.rssi = rssi;
        newDev.txPower = txPower;
        newDev.manufacturerId = mfgId;
        newDev.manufacturerDataHex = mfgHex;
        newDev.serviceUuid = serviceUuid;
        newDev.lastSeen = now;
        newDev.isPinned = isPinned;
        newDev.type = type;
        newDev.vendor = getDeviceVendor(newDev);
        _devices.push_back(newDev);
        _catalogVersion++;
    }

    // If this is the currently tracked target, record in rolling RSSI history
    if (!_trackedMac.empty() && mac == _trackedMac) {
        _rssiHistory[_historyIndex] = rssi;
        _historyIndex = (_historyIndex + 1) % HISTORY_SIZE;
        if (_historyCount < HISTORY_SIZE) {
            _historyCount++;
        }
    }
}

bool BluetoothTracker::update(unsigned long currentMillis) {
    bool stateChanged = false;

    // Prune devices older than 30 seconds
    if (currentMillis - _lastPruneTime >= 5000) {
        _lastPruneTime = currentMillis;
        size_t initialCount = _devices.size();
        _devices.erase(
            std::remove_if(_devices.begin(), _devices.end(),
                           [currentMillis](const BleDevice& d) {
                               return (currentMillis - d.lastSeen) > 30000 && !d.isPinned;
                           }),
            _devices.end()
        );
        if (_devices.size() != initialCount) {
            _catalogVersion++;
            stateChanged = true;
        }
    }

    return stateChanged;
}

std::vector<BleDevice> BluetoothTracker::getFilteredDevices(int sortMode, int filterMode, int minRssi) const {
    std::vector<BleDevice> result;
    result.reserve(_devices.size());

    for (const auto& d : _devices) {
        if (d.rssi < minRssi) continue;
        if (filterMode == 1 && (d.name.empty() || d.name == "Unknown")) continue;
        if (filterMode == 2 && !d.isPinned) continue;
        result.push_back(d);
    }

    if (sortMode == 0) {
        // RSSI strongest first
        std::sort(result.begin(), result.end(), [](const BleDevice& a, const BleDevice& b) {
            return a.rssi > b.rssi;
        });
    } else if (sortMode == 1) {
        // Name A-Z
        std::sort(result.begin(), result.end(), [](const BleDevice& a, const BleDevice& b) {
            return a.name < b.name;
        });
    } else if (sortMode == 2) {
        // Last Seen most recent first
        std::sort(result.begin(), result.end(), [](const BleDevice& a, const BleDevice& b) {
            return a.lastSeen > b.lastSeen;
        });
    }

    return result;
}

void BluetoothTracker::setTrackedTarget(const std::string& mac) {
    _trackedMac = mac;
    _historyIndex = 0;
    _historyCount = 0;
    for (size_t i = 0; i < HISTORY_SIZE; ++i) {
        _rssiHistory[i] = RSSI_FLOOR;
    }

    // Populate current RSSI if device is already in catalog
    const BleDevice* dev = getTrackedDevice();
    if (dev) {
        _rssiHistory[0] = dev->rssi;
        _historyIndex = 1;
        _historyCount = 1;
    }
}

void BluetoothTracker::clearTrackedTarget() {
    _trackedMac.clear();
    _historyIndex = 0;
    _historyCount = 0;
}

const BleDevice* BluetoothTracker::getTrackedDevice() const {
    if (_trackedMac.empty()) return nullptr;
    for (const auto& d : _devices) {
        if (d.mac == _trackedMac) {
            return &d;
        }
    }
    return nullptr;
}

int8_t BluetoothTracker::getSmoothedRssi() const {
    if (_historyCount == 0) return RSSI_FLOOR;
    int32_t sum = 0;
    for (size_t i = 0; i < _historyCount; ++i) {
        sum += _rssiHistory[i];
    }
    return (int8_t)(sum / (int32_t)_historyCount);
}

void BluetoothTracker::getRssiHistory(int8_t* outHistory, size_t count) const {
    if (!outHistory || count == 0) return;
    size_t copyCount = std::min(count, HISTORY_SIZE);
    for (size_t i = 0; i < copyCount; ++i) {
        // Return in chronological order
        size_t idx = (_historyIndex + HISTORY_SIZE - _historyCount + i) % HISTORY_SIZE;
        outHistory[i] = _rssiHistory[idx];
    }
}

float BluetoothTracker::getRssiSlope() const {
    if (_historyCount < 2) return 0.0f;

    // Least-squares slope over chronological samples (x = 0..n-1), in dB per sample.
    const float n = (float)_historyCount;
    float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumXX = 0.0f;
    for (size_t i = 0; i < _historyCount; ++i) {
        size_t idx = (_historyIndex + HISTORY_SIZE - _historyCount + i) % HISTORY_SIZE;
        const float x = (float)i;
        const float y = (float)_rssiHistory[idx];
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumXX += x * x;
    }
    const float denom = n * sumXX - sumX * sumX;
    if (denom == 0.0f) return 0.0f;
    return (n * sumXY - sumX * sumY) / denom;
}

ProximityTrend BluetoothTracker::getTrend() const {
    if (_historyCount < RSSI_TREND_MIN_SAMPLES) return TREND_STABLE;
    const float slope = getRssiSlope();
    if (slope >= RSSI_TREND_SLOPE_THRESHOLD) return TREND_CLOSING;
    if (slope <= -RSSI_TREND_SLOPE_THRESHOLD) return TREND_DRIFTING;
    return TREND_STABLE;
}

void BluetoothTracker::addOrUpdateDevice(const BleDevice& dev) {
    processFoundDevice(dev.mac, dev.name, dev.rssi, dev.txPower,
                       dev.manufacturerId, dev.manufacturerDataHex,
                       dev.serviceUuid, dev.lastSeen);
    for (auto& d : _devices) {
        if (d.mac == dev.mac) {
            if (d.isPinned != dev.isPinned) {
                d.isPinned = dev.isPinned;
                _catalogVersion++;
            }
            if (!dev.vendor.empty()) {
                d.vendor = dev.vendor;
            }
            break;
        }
    }
}

void BluetoothTracker::updatePinnedDevices() {
    bool changed = false;
    for (auto& d : _devices) {
        bool shouldPin = settings.isMacPinned(d.mac);
        if (d.isPinned != shouldPin) {
            d.isPinned = shouldPin;
            changed = true;
        }
    }
    if (changed) {
        _catalogVersion++;
    }
}

void BluetoothTracker::clearDevices() {
    _devices.clear();
    clearTrackedTarget();
    _catalogVersion++;
}

const char* BleDevice::getVendor() const {
    if (!vendor.empty() && vendor != "Unknown Vendor") {
        return vendor.c_str();
    }
    return getDeviceVendor(*this);
}
