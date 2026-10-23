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
      _catalogVersion(0) {
    for (size_t i = 0; i < HISTORY_SIZE; ++i) {
        _rssiHistory[i] = RSSI_FLOOR;
    }
}

void BluetoothTracker::begin() {
#ifndef NATIVE_TEST
    NimBLEDevice::init("CYD-BLE-Tracker");
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&scanCallbacks, true);
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(99);
#endif
    // Restore pinned target from settings if present
    std::string pinned = settings.getPinnedMac();
    if (!pinned.empty()) {
        _trackedMac = pinned;
    }
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

    bool isPinned = (mac == settings.getPinnedMac());

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
            break;
        }
    }
}

void BluetoothTracker::clearDevices() {
    _devices.clear();
    clearTrackedTarget();
    _catalogVersion++;
}
