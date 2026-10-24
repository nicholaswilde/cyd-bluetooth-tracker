#include <unity.h>
#include <cstring>
#include "Arduino.h"
#include "../mocks/mocks.cpp"
#include "../../src/settings_manager.cpp"
#include "../../src/oui.cpp"
#include "../../src/bluetooth_tracker.cpp"
#include "oui.h"
#include "bluetooth_tracker.h"

void setUp(void) {
    bleTracker.clearDevices();
}

void tearDown(void) {
    bleTracker.clearDevices();
}

void test_oui_table_integrity_and_sorting(void) {
    size_t count = getOuiTableSize();
    TEST_ASSERT_GREATER_THAN(400, count);

    const OuiEntry* table = getOuiTable();
    TEST_ASSERT_NOT_NULL(table);

    // Verify table is strictly sorted ascending and all entries can be retrieved
    for (size_t i = 0; i < count; i++) {
        if (i > 0) {
            TEST_ASSERT_GREATER_THAN(table[i - 1].prefix, table[i].prefix);
        }
        TEST_ASSERT_NOT_NULL(table[i].vendor);
        TEST_ASSERT_GREATER_THAN(0, strlen(table[i].vendor));

        // Binary search lookup for each prefix in table must succeed
        const char* vendor = getOuiVendor(table[i].prefix);
        TEST_ASSERT_EQUAL_STRING(table[i].vendor, vendor);
    }
}

void test_bt_company_table_integrity_and_sorting(void) {
    size_t count = getBtCompanyTableSize();
    TEST_ASSERT_GREATER_THAN(50, count);

    const BtCompanyEntry* table = getBtCompanyTable();
    TEST_ASSERT_NOT_NULL(table);

    // Verify table is strictly sorted ascending and all entries can be retrieved
    for (size_t i = 0; i < count; i++) {
        if (i > 0) {
            TEST_ASSERT_GREATER_THAN(table[i - 1].id, table[i].id);
        }
        TEST_ASSERT_NOT_NULL(table[i].name);
        TEST_ASSERT_GREATER_THAN(0, strlen(table[i].name));

        // Binary search lookup for each company ID must succeed
        const char* name = getBtCompanyName(table[i].id);
        TEST_ASSERT_EQUAL_STRING(table[i].name, name);
    }
}

void test_bluetooth_sig_company_lookup(void) {
    // Top consumer & IoT Bluetooth vendors
    TEST_ASSERT_EQUAL_STRING("Apple", getBtCompanyName(0x004C));
    TEST_ASSERT_EQUAL_STRING("Samsung", getBtCompanyName(0x0075));
    TEST_ASSERT_EQUAL_STRING("Microsoft", getBtCompanyName(0x0006));
    TEST_ASSERT_EQUAL_STRING("Sony", getBtCompanyName(0x012D));
    TEST_ASSERT_EQUAL_STRING("Bose", getBtCompanyName(0x009E));
    TEST_ASSERT_EQUAL_STRING("Espressif", getBtCompanyName(0x02E5));
    TEST_ASSERT_EQUAL_STRING("Nordic Semi", getBtCompanyName(0x0059));
    TEST_ASSERT_EQUAL_STRING("Garmin", getBtCompanyName(0x0087));
    TEST_ASSERT_EQUAL_STRING("Google", getBtCompanyName(0x00E0));
    TEST_ASSERT_EQUAL_STRING("Xiaomi", getBtCompanyName(0x038F));
    TEST_ASSERT_EQUAL_STRING("Tile", getBtCompanyName(0x067C));
    TEST_ASSERT_EQUAL_STRING("Amazon", getBtCompanyName(0x0171));
    TEST_ASSERT_EQUAL_STRING("Logitech", getBtCompanyName(0x01DA));
    TEST_ASSERT_EQUAL_STRING("Intel", getBtCompanyName(0x0002));
    TEST_ASSERT_EQUAL_STRING("Texas Instruments", getBtCompanyName(0x000D));

    // Unmapped / unknown company IDs
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getBtCompanyName(0x0000));
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getBtCompanyName(0xBEEF));
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getBtCompanyName(0xFFFF));
}

void test_ieee_oui_lookup_mac_formats(void) {
    // 1. Numeric 24-bit prefix
    TEST_ASSERT_EQUAL_STRING("Espressif", getOuiVendor((uint32_t)0x240AC4));
    TEST_ASSERT_EQUAL_STRING("Apple", getOuiVendor((uint32_t)0x0017F2));
    TEST_ASSERT_EQUAL_STRING("Sony", getOuiVendor((uint32_t)0x0013A9));
    TEST_ASSERT_EQUAL_STRING("Bose", getOuiVendor((uint32_t)0x000C8A));
    TEST_ASSERT_EQUAL_STRING("Garmin", getOuiVendor((uint32_t)0x00054F));
    TEST_ASSERT_EQUAL_STRING("Tile", getOuiVendor((uint32_t)0x0000A5));

    // 2. Byte array (6-byte MAC)
    uint8_t mac_esp[6] = {0x24, 0x0A, 0xC4, 0x01, 0x02, 0x03};
    TEST_ASSERT_EQUAL_STRING("Espressif", getOuiVendor(mac_esp));
    uint8_t mac_sony[6] = {0x00, 0x13, 0xA9, 0xAA, 0xBB, 0xCC};
    TEST_ASSERT_EQUAL_STRING("Sony", getOuiVendor(mac_sony));
    uint8_t mac_bose[6] = {0x00, 0x0C, 0x8A, 0x11, 0x22, 0x33};
    TEST_ASSERT_EQUAL_STRING("Bose", getOuiVendor(mac_bose));

    // 3. String formats
    TEST_ASSERT_EQUAL_STRING("Espressif", getOuiVendor("24:0A:C4:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("Espressif", getOuiVendor("24:0a:c4:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("Espressif", getOuiVendor("24-0A-C4-01-02-03"));
    TEST_ASSERT_EQUAL_STRING("Espressif", getOuiVendor("240AC4010203"));
    TEST_ASSERT_EQUAL_STRING("Espressif", getOuiVendor("   24:0A:C4:01:02:03   "));

    // std::string overload
    std::string sMac = "00:17:F2:AA:BB:CC";
    TEST_ASSERT_EQUAL_STRING("Apple", getOuiVendor(sMac));

    // 4. Invalid / unmapped inputs
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor((const uint8_t*)nullptr));
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor((const char*)nullptr));
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor(""));
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor("invalid-mac"));
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor("DE:AD:BE:EF:00:01"));
}

void test_device_vendor_resolution_and_fallback(void) {
    // 1. Manufacturer ID takes precedence when present
    TEST_ASSERT_EQUAL_STRING("Nordic Semi", getDeviceVendor(0x0059, "11:22:33:44:55:66"));
    TEST_ASSERT_EQUAL_STRING("Microsoft", getDeviceVendor(0x0006, "11:22:33:44:55:66"));
    TEST_ASSERT_EQUAL_STRING("Apple", getDeviceVendor(0x004C, "11:22:33:44:55:66"));

    // 2. Fall back to IEEE OUI when manufacturer ID is 0
    TEST_ASSERT_EQUAL_STRING("Espressif", getDeviceVendor(0x0000, "24:0A:C4:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("Sony", getDeviceVendor(0x0000, "00:13:A9:AA:BB:CC"));
    TEST_ASSERT_EQUAL_STRING("Bose", getDeviceVendor(0x0000, "00:0C:8A:11:22:33"));

    // 3. Fall back to IEEE OUI when manufacturer ID is unmapped
    TEST_ASSERT_EQUAL_STRING("Espressif", getDeviceVendor(0xEEEE, "24:0A:C4:01:02:03"));

    // 4. Neither matches
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getDeviceVendor(0x0000, "DE:AD:BE:EF:00:01"));
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getDeviceVendor(0xEEEE, "DE:AD:BE:EF:00:01"));
}

void test_ble_device_struct_vendor_integration(void) {
    // Case 1: Device with known Company ID
    BleDevice d1;
    d1.mac = "AA:BB:CC:DD:EE:01";
    d1.name = "Unknown";
    d1.rssi = -60;
    d1.txPower = 0;
    d1.manufacturerId = 0x0059; // Nordic Semi
    d1.type = DEVICE_TYPE_GENERIC;
    bleTracker.addOrUpdateDevice(d1);

    const auto& devs = bleTracker.getDevices();
    TEST_ASSERT_EQUAL(1, devs.size());
    TEST_ASSERT_EQUAL_STRING("Nordic Semi", devs[0].getVendor());
    TEST_ASSERT_EQUAL_STRING("Nordic Semi", devs[0].vendor.c_str());

    // Case 2: Device with type Apple / iBeacon
    BleDevice d2;
    d2.mac = "AA:BB:CC:DD:EE:02";
    d2.name = "Unknown";
    d2.rssi = -70;
    d2.manufacturerId = 0x004C;
    d2.type = DEVICE_TYPE_APPLE;
    bleTracker.addOrUpdateDevice(d2);

    TEST_ASSERT_EQUAL(2, bleTracker.getDevices().size());
    TEST_ASSERT_EQUAL_STRING("Apple", bleTracker.getDevices()[1].getVendor());

    // Case 3: Device with no manufacturer data (mfgId 0) but Espressif OUI in MAC
    BleDevice d3;
    d3.mac = "24:0A:C4:55:66:77";
    d3.name = "Unknown";
    d3.rssi = -80;
    d3.manufacturerId = 0;
    d3.type = DEVICE_TYPE_GENERIC;
    bleTracker.addOrUpdateDevice(d3);

    TEST_ASSERT_EQUAL(3, bleTracker.getDevices().size());
    TEST_ASSERT_EQUAL_STRING("Espressif", bleTracker.getDevices()[2].getVendor());

    // Case 4: Completely unknown device
    BleDevice d4;
    d4.mac = "DE:AD:BE:EF:01:02";
    d4.name = "Unknown";
    d4.rssi = -85;
    d4.manufacturerId = 0;
    d4.type = DEVICE_TYPE_GENERIC;
    bleTracker.addOrUpdateDevice(d4);

    TEST_ASSERT_EQUAL(4, bleTracker.getDevices().size());
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", bleTracker.getDevices()[3].getVendor());
}

void test_oui_vendor_null_and_specialized_types(void) {
    // Null pointers safety
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor((const uint8_t*)nullptr));
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor((const char*)nullptr));

    // parseMacPrefix edge cases
    uint32_t prefix = 0;
    TEST_ASSERT_FALSE(parseMacPrefix(nullptr, &prefix));
    TEST_ASSERT_FALSE(parseMacPrefix("00:11:22", nullptr));
    TEST_ASSERT_FALSE(parseMacPrefix("invalid", &prefix));
    TEST_ASSERT_FALSE(parseMacPrefix("00", &prefix));
    TEST_ASSERT_TRUE(parseMacPrefix("  24:0A:C4  ", &prefix));
    TEST_ASSERT_EQUAL_HEX32(0x240AC4, prefix);

    // Unmatched vendor Lookups
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor((uint32_t)0x000000));
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getBtCompanyName((uint16_t)0xFFFF));

    // getDeviceVendor(mfgId, mac)
    TEST_ASSERT_EQUAL_STRING("Apple", getDeviceVendor(0x004C, "00:00:00:00:00:00"));
    TEST_ASSERT_EQUAL_STRING("Espressif", getDeviceVendor(0xFFFF, "24:0A:C4:11:22:33"));
    TEST_ASSERT_EQUAL_STRING("Espressif", getDeviceVendor(0, "24:0A:C4:11:22:33"));

    // Specialized BleDevice types
    BleDevice dev;
    dev.type = DEVICE_TYPE_SAMSUNG;
    TEST_ASSERT_EQUAL_STRING("Samsung", getDeviceVendor(dev));

    dev.type = DEVICE_TYPE_TILE;
    TEST_ASSERT_EQUAL_STRING("Tile", getDeviceVendor(dev));

    dev.type = DEVICE_TYPE_FAST_PAIR;
    TEST_ASSERT_EQUAL_STRING("Google", getDeviceVendor(dev));

    dev.type = DEVICE_TYPE_GENERIC;
    dev.manufacturerId = 0x0006; // Microsoft
    TEST_ASSERT_EQUAL_STRING("Microsoft", getDeviceVendor(dev));
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_oui_table_integrity_and_sorting);
    RUN_TEST(test_bt_company_table_integrity_and_sorting);
    RUN_TEST(test_bluetooth_sig_company_lookup);
    RUN_TEST(test_ieee_oui_lookup_mac_formats);
    RUN_TEST(test_device_vendor_resolution_and_fallback);
    RUN_TEST(test_ble_device_struct_vendor_integration);
    RUN_TEST(test_oui_vendor_null_and_specialized_types);
    return UNITY_END();
}
