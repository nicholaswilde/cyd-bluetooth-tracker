#ifndef OUI_H
#define OUI_H

#include <stdint.h>
#include <stddef.h>
#include <string>

#ifdef NATIVE_TEST
#include "Arduino.h"
#else
#include <Arduino.h>
#endif

#ifndef PROGMEM
#define PROGMEM
#endif

struct OuiEntry {
    uint32_t prefix;
    const char* vendor;
};

struct BtCompanyEntry {
    uint16_t id;
    const char* name;
};

// Forward declaration
struct BleDevice;

// Returns Bluetooth SIG company name (e.g., "Apple", "Samsung", "Nordic Semi", etc.)
// or "Unknown Vendor" if not found.
const char* getBtCompanyName(uint16_t companyId);

// Returns vendor name from 24-bit OUI prefix (e.g., "Apple", "Espressif", etc.)
// or "Unknown Vendor" if not found.
const char* getOuiVendor(uint32_t prefix);
const char* getOuiVendor(const uint8_t* mac);
const char* getOuiVendor(const char* macStr);
const char* getOuiVendor(const std::string& macStr);
#ifndef NATIVE_TEST
const char* getOuiVendor(const String& macStr);
#endif

// Parse MAC address string (e.g., "AA:BB:CC:DD:EE:FF", "aa-bb-cc-dd-ee-ff", or "aabbcc") into 24-bit OUI prefix
bool parseMacPrefix(const char* macStr, uint32_t* prefixOut);

// Combined device vendor resolution:
// Resolves manufacturer/vendor name by checking:
// 1. Bluetooth SIG Company ID (if manufacturerId != 0)
// 2. Fall back to IEEE 24-bit OUI lookup from MAC address
// Returns resolved vendor string, or "Unknown Vendor" if neither matches.
const char* getDeviceVendor(uint16_t manufacturerId, const std::string& mac);
const char* getDeviceVendor(const BleDevice& dev);

// Table introspection for tests and stats
size_t getOuiTableSize();
const OuiEntry* getOuiTable();
size_t getBtCompanyTableSize();
const BtCompanyEntry* getBtCompanyTable();

#endif // OUI_H
