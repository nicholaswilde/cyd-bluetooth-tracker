#include <unity.h>
#include "Arduino.h"
#include "../mocks/mocks.cpp"
#include "../../src/settings_manager.cpp"
#include "../../src/bluetooth_tracker.cpp"
#include "bluetooth_tracker.h"

void setUp(void) {
    bleTracker.clearDevices();
}

void tearDown(void) {
    bleTracker.clearDevices();
}

void test_tracker_add_and_get_devices(void) {
    BleDevice d1;
    d1.mac = "AA:BB:CC:DD:EE:01";
    d1.name = "MyBeacon";
    d1.rssi = -60;
    d1.txPower = -59;
    d1.manufacturerId = 0x004C; // Apple
    d1.manufacturerDataHex = "4c000215abcd";
    d1.serviceUuid = "";
    d1.lastSeen = 1000;
    d1.isPinned = false;
    d1.type = DEVICE_TYPE_IBEACON;

    bleTracker.addOrUpdateDevice(d1);

    TEST_ASSERT_EQUAL(1, bleTracker.getDiscoveredCount());
    const auto& list = bleTracker.getDevices();
    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:01", list[0].mac.c_str());
    TEST_ASSERT_EQUAL_STRING("MyBeacon", list[0].name.c_str());
    TEST_ASSERT_EQUAL(-60, list[0].rssi);
    TEST_ASSERT_EQUAL(DEVICE_TYPE_IBEACON, list[0].type);

    // Update existing device with new RSSI
    d1.rssi = -52;
    bleTracker.addOrUpdateDevice(d1);
    TEST_ASSERT_EQUAL(1, bleTracker.getDiscoveredCount());
    TEST_ASSERT_EQUAL(-52, bleTracker.getDevices()[0].rssi);
}

void test_tracker_distance_estimation(void) {
    BleDevice d;
    d.rssi = -59;
    d.txPower = -59;
    // At RSSI == txPower, estimated distance is 10^0 = 1.0 meter
    float dist = d.getEstimatedDistance();
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, dist);

    // Distant beacon
    d.rssi = -81;
    d.txPower = -59;
    // ( -59 - (-81) ) / (10 * 2.2) = 22 / 22 = 1.0 -> 10^1 = 10.0 meters
    float distFar = d.getEstimatedDistance();
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 10.0f, distFar);
}

void test_tracker_sorting_and_filtering(void) {
    BleDevice d1; d1.mac = "01"; d1.name = "Zulu"; d1.rssi = -80; d1.lastSeen = 100; d1.isPinned = false;
    BleDevice d2; d2.mac = "02"; d2.name = "Alpha"; d2.rssi = -50; d2.lastSeen = 300; d2.isPinned = true;
    BleDevice d3; d3.mac = "03"; d3.name = "Unknown"; d3.rssi = -92; d3.lastSeen = 200; d3.isPinned = false;

    bleTracker.addOrUpdateDevice(d1);
    bleTracker.addOrUpdateDevice(d2);
    bleTracker.addOrUpdateDevice(d3);

    // Sort by RSSI strongest (sortMode 0)
    auto sortedRssi = bleTracker.getFilteredDevices(0, 0, -95);
    TEST_ASSERT_EQUAL(3, sortedRssi.size());
    TEST_ASSERT_EQUAL_STRING("02", sortedRssi[0].mac.c_str());
    TEST_ASSERT_EQUAL_STRING("01", sortedRssi[1].mac.c_str());
    TEST_ASSERT_EQUAL_STRING("03", sortedRssi[2].mac.c_str());

    // Sort by Name (sortMode 1)
    auto sortedName = bleTracker.getFilteredDevices(1, 0, -95);
    TEST_ASSERT_EQUAL_STRING("02", sortedName[0].mac.c_str()); // Alpha
    TEST_ASSERT_EQUAL_STRING("03", sortedName[1].mac.c_str()); // Unknown
    TEST_ASSERT_EQUAL_STRING("01", sortedName[2].mac.c_str()); // Zulu

    // Filter by Min RSSI (-70)
    auto filteredMinRssi = bleTracker.getFilteredDevices(0, 0, -70);
    TEST_ASSERT_EQUAL(1, filteredMinRssi.size());
    TEST_ASSERT_EQUAL_STRING("02", filteredMinRssi[0].mac.c_str());

    // Filter named only (filterMode 1)
    auto filteredNamed = bleTracker.getFilteredDevices(0, 1, -95);
    TEST_ASSERT_EQUAL(2, filteredNamed.size());

    // Filter pinned only (filterMode 2)
    auto filteredPinned = bleTracker.getFilteredDevices(0, 2, -95);
    TEST_ASSERT_EQUAL(1, filteredPinned.size());
    TEST_ASSERT_EQUAL_STRING("02", filteredPinned[0].mac.c_str());
}

void test_tracker_target_tracking_and_history(void) {
    BleDevice d;
    d.mac = "AA:BB:CC:11:22:33";
    d.name = "MyKeys";
    d.rssi = -70;
    d.lastSeen = 500;
    bleTracker.addOrUpdateDevice(d);

    bleTracker.setTrackedTarget("AA:BB:CC:11:22:33");
    TEST_ASSERT_TRUE(bleTracker.isTracking());
    TEST_ASSERT_NOT_NULL(bleTracker.getTrackedDevice());

    // Initial smoothed RSSI
    TEST_ASSERT_EQUAL(-70, bleTracker.getSmoothedRssi());

    // Add updates
    d.rssi = -60;
    bleTracker.addOrUpdateDevice(d);
    d.rssi = -50;
    bleTracker.addOrUpdateDevice(d);

    // Smoothed RSSI is average of (-70, -60, -50) = -60
    TEST_ASSERT_EQUAL(-60, bleTracker.getSmoothedRssi());

    int8_t hist[8];
    bleTracker.getRssiHistory(hist, 8);
    TEST_ASSERT_EQUAL(-70, hist[0]);
    TEST_ASSERT_EQUAL(-60, hist[1]);
    TEST_ASSERT_EQUAL(-50, hist[2]);

    bleTracker.clearTrackedTarget();
    TEST_ASSERT_FALSE(bleTracker.isTracking());
    TEST_ASSERT_NULL(bleTracker.getTrackedDevice());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_tracker_add_and_get_devices);
    RUN_TEST(test_tracker_distance_estimation);
    RUN_TEST(test_tracker_sorting_and_filtering);
    RUN_TEST(test_tracker_target_tracking_and_history);
    return UNITY_END();
}
