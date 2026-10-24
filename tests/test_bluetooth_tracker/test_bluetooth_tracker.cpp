#include <unity.h>
#include "Arduino.h"
#include "../mocks/mocks.cpp"
#include "../../src/settings_manager.cpp"
#include "../../src/oui.cpp"
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

    // Update existing device with new RSSI, txPower, serviceUuid, mfgHex
    d1.rssi = -52;
    d1.txPower = -50;
    d1.serviceUuid = "180D";
    d1.manufacturerDataHex = "000002151122";
    bleTracker.addOrUpdateDevice(d1);
    TEST_ASSERT_EQUAL(1, bleTracker.getDiscoveredCount());
    TEST_ASSERT_EQUAL(-52, bleTracker.getDevices()[0].rssi);

    // Samsung beacon
    BleDevice dSamsung;
    dSamsung.mac = "11:22:33:44:55:01";
    dSamsung.manufacturerId = 0x0075;
    bleTracker.addOrUpdateDevice(dSamsung);
    TEST_ASSERT_EQUAL(DEVICE_TYPE_SAMSUNG, bleTracker.getDevices().back().type);

    // Tile beacon
    BleDevice dTile;
    dTile.mac = "11:22:33:44:55:02";
    dTile.serviceUuid = "feed";
    bleTracker.addOrUpdateDevice(dTile);
    TEST_ASSERT_EQUAL(DEVICE_TYPE_TILE, bleTracker.getDevices().back().type);

    // Fast Pair beacon
    BleDevice dFastPair;
    dFastPair.mac = "11:22:33:44:55:03";
    dFastPair.serviceUuid = "fe2c";
    bleTracker.addOrUpdateDevice(dFastPair);
    TEST_ASSERT_EQUAL(DEVICE_TYPE_FAST_PAIR, bleTracker.getDevices().back().type);
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

    // RSSI 0 returns -1.0
    BleDevice dZero;
    dZero.rssi = 0;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.0f, dZero.getEstimatedDistance());

    // txPower 0 defaults to -59
    BleDevice dNoTx;
    dNoTx.rssi = -59;
    dNoTx.txPower = 0;
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, dNoTx.getEstimatedDistance());
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

    // Sort by Last Seen (sortMode 2)
    auto sortedLastSeen = bleTracker.getFilteredDevices(2, 0, -95);
    TEST_ASSERT_EQUAL_STRING("02", sortedLastSeen[0].mac.c_str()); // 300
    TEST_ASSERT_EQUAL_STRING("03", sortedLastSeen[1].mac.c_str()); // 200
    TEST_ASSERT_EQUAL_STRING("01", sortedLastSeen[2].mac.c_str()); // 100

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

void test_tracker_catalog_versioning(void) {
    bleTracker.clearDevices();
    uint32_t baseVer = bleTracker.getCatalogVersion();

    BleDevice d1;
    d1.mac = "11:22:33:44:55:66";
    d1.name = "Unknown";
    d1.rssi = -70;
    d1.lastSeen = 1000;
    d1.isPinned = false;
    bleTracker.addOrUpdateDevice(d1);

    // Initial add increments catalog version
    TEST_ASSERT_EQUAL_UINT32(baseVer + 1, bleTracker.getCatalogVersion());

    // RSSI update only should NOT increment catalog version
    d1.rssi = -65;
    bleTracker.addOrUpdateDevice(d1);
    TEST_ASSERT_EQUAL_UINT32(baseVer + 1, bleTracker.getCatalogVersion());

    // Name update from Unknown to real name increments catalog version
    d1.name = "AirTag";
    bleTracker.addOrUpdateDevice(d1);
    TEST_ASSERT_EQUAL_UINT32(baseVer + 2, bleTracker.getCatalogVersion());

    // Pinning device increments catalog version
    d1.isPinned = true;
    bleTracker.addOrUpdateDevice(d1);
    TEST_ASSERT_EQUAL_UINT32(baseVer + 3, bleTracker.getCatalogVersion());

    // Clear devices increments catalog version
    bleTracker.clearDevices();
    TEST_ASSERT_EQUAL_UINT32(baseVer + 4, bleTracker.getCatalogVersion());
}

void test_tracker_pruning_version_increment(void) {
    bleTracker.clearDevices();
    uint32_t baseVer = bleTracker.getCatalogVersion();

    BleDevice d;
    d.mac = "22:33:44:55:66:77";
    d.name = "Ephemeral";
    d.rssi = -80;
    d.lastSeen = 1000;
    d.isPinned = false;
    bleTracker.addOrUpdateDevice(d);
    TEST_ASSERT_EQUAL_UINT32(baseVer + 1, bleTracker.getCatalogVersion());

    // Before 30s timeout, update() should not prune
    bool pruned = bleTracker.update(10000);
    TEST_ASSERT_FALSE(pruned);
    TEST_ASSERT_EQUAL_UINT32(baseVer + 1, bleTracker.getCatalogVersion());

    // After 35s (> 30s timeout), update() should prune and increment version
    pruned = bleTracker.update(45000);
    TEST_ASSERT_TRUE(pruned);
    TEST_ASSERT_EQUAL_UINT32(baseVer + 2, bleTracker.getCatalogVersion());
    TEST_ASSERT_EQUAL(0, bleTracker.getDiscoveredCount());
}

void test_tracker_scan_interval_and_window(void) {
    // Default interval should match DEFAULT_SCAN_INTERVAL_MS (2000ms)
    TEST_ASSERT_EQUAL_UINT32(DEFAULT_SCAN_INTERVAL_MS, bleTracker.getScanInterval());
    TEST_ASSERT_EQUAL_UINT32(DEFAULT_SCAN_INTERVAL_MS * 9 / 10, bleTracker.getScanWindow());

    // Setting 1000ms -> 900ms window
    bleTracker.setScanInterval(1000);
    TEST_ASSERT_EQUAL_UINT32(1000, bleTracker.getScanInterval());
    TEST_ASSERT_EQUAL_UINT32(900, bleTracker.getScanWindow());

    // Setting 5000ms -> 4500ms window
    bleTracker.setScanInterval(5000);
    TEST_ASSERT_EQUAL_UINT32(5000, bleTracker.getScanInterval());
    TEST_ASSERT_EQUAL_UINT32(4500, bleTracker.getScanWindow());

    // Clamping lower bound (< 100ms clamps to 100ms)
    bleTracker.setScanInterval(50);
    TEST_ASSERT_EQUAL_UINT32(100, bleTracker.getScanInterval());
    TEST_ASSERT_EQUAL_UINT32(99, bleTracker.getScanWindow());

    // begin() adopts settings.getScanIntervalMs()
    settings.setScanIntervalMs(3000);
    bleTracker.begin();
    TEST_ASSERT_EQUAL_UINT32(3000, bleTracker.getScanInterval());
    TEST_ASSERT_EQUAL_UINT32(2700, bleTracker.getScanWindow());
}

// Regression tests for issue #4: proximity trend indicator
static void trackWithRssiSequence(const int8_t* seq, size_t n) {
    BleDevice d;
    d.mac = "AA:BB:CC:44:44:44";
    d.name = "TrendTarget";
    d.rssi = seq[0];
    d.lastSeen = 500;
    bleTracker.addOrUpdateDevice(d);
    bleTracker.setTrackedTarget(d.mac);  // seeds history with seq[0]
    for (size_t i = 1; i < n; ++i) {
        d.rssi = seq[i];
        bleTracker.addOrUpdateDevice(d);
    }
}

void test_tracker_trend_insufficient_samples_is_stable(void) {
    const int8_t seq[] = {-90, -60};
    trackWithRssiSequence(seq, 2);
    TEST_ASSERT_EQUAL(TREND_STABLE, bleTracker.getTrend());
    bleTracker.clearTrackedTarget();
    TEST_ASSERT_EQUAL(TREND_STABLE, bleTracker.getTrend());
}

void test_tracker_trend_closing_in(void) {
    const int8_t seq[] = {-80, -75, -70, -64, -60, -55};
    trackWithRssiSequence(seq, 6);
    TEST_ASSERT_TRUE(bleTracker.getRssiSlope() >= RSSI_TREND_SLOPE_THRESHOLD);
    TEST_ASSERT_EQUAL(TREND_CLOSING, bleTracker.getTrend());
    bleTracker.clearTrackedTarget();
}

void test_tracker_trend_drifting(void) {
    const int8_t seq[] = {-50, -56, -61, -67, -72, -78};
    trackWithRssiSequence(seq, 6);
    TEST_ASSERT_TRUE(bleTracker.getRssiSlope() <= -RSSI_TREND_SLOPE_THRESHOLD);
    TEST_ASSERT_EQUAL(TREND_DRIFTING, bleTracker.getTrend());
    bleTracker.clearTrackedTarget();
}

void test_tracker_trend_stable_with_noise(void) {
    const int8_t seq[] = {-65, -66, -64, -65, -66, -65};
    trackWithRssiSequence(seq, 6);
    TEST_ASSERT_EQUAL(TREND_STABLE, bleTracker.getTrend());
    bleTracker.clearTrackedTarget();
}

void test_tracker_trend_after_history_wraps(void) {
    // 12 samples > HISTORY_SIZE (8): only the most recent 8 (improving) count
    const int8_t seq[] = {-40, -45, -50, -55, -90, -85, -80, -75, -70, -65, -60, -55};
    trackWithRssiSequence(seq, 12);
    TEST_ASSERT_EQUAL(TREND_CLOSING, bleTracker.getTrend());
    bleTracker.clearTrackedTarget();
}

void test_tracker_begin_and_scan_trigger(void) {
    settings.setScanIntervalMs(2500);
    settings.setPinnedMac("AA:BB:CC:11:22:33");
    bleTracker.begin();
    TEST_ASSERT_EQUAL_UINT32(2500, bleTracker.getScanInterval());
    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:11:22:33", bleTracker.getTrackedTarget().c_str());

    // When scan interval is 0 in settings, begin defaults to DEFAULT_SCAN_INTERVAL_MS
    settings.setScanIntervalMs(500); // minimum allowed is 500 in settings
    bleTracker.begin();
    TEST_ASSERT_EQUAL_UINT32(500, bleTracker.getScanInterval());

    bleTracker.triggerScan();
    TEST_ASSERT_TRUE(bleTracker.isScanning());
}

void test_tracker_clear_devices(void) {
    BleDevice d;
    d.mac = "12:34:56:78:9A:BC";
    d.name = "ClearMe";
    d.rssi = -65;
    d.lastSeen = 1000;
    bleTracker.addOrUpdateDevice(d);
    bleTracker.setTrackedTarget(d.mac);

    TEST_ASSERT_TRUE(bleTracker.getDevices().size() > 0);
    TEST_ASSERT_EQUAL_STRING(d.mac.c_str(), bleTracker.getTrackedTarget().c_str());

    bleTracker.clearDevices();
    TEST_ASSERT_EQUAL(0, bleTracker.getDevices().size());
    TEST_ASSERT_EQUAL_STRING("", bleTracker.getTrackedTarget().c_str());
}

void test_tracker_rssi_history_and_slope_edge_cases(void) {
    bleTracker.clearTrackedTarget();
    TEST_ASSERT_EQUAL_INT8(RSSI_FLOOR, bleTracker.getSmoothedRssi());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, bleTracker.getRssiSlope());

    // Null and zero count bounds safety
    bleTracker.getRssiHistory(nullptr, 0);
    int8_t hist[4];
    bleTracker.getRssiHistory(hist, 0);
}

void test_tracker_add_or_update_existing_and_vendor_override(void) {
    BleDevice dev;
    dev.mac = "99:88:77:66:55:44";
    dev.name = "Initial";
    dev.rssi = -60;
    dev.lastSeen = 1000;
    dev.isPinned = false;
    bleTracker.addOrUpdateDevice(dev);

    // Update with new pinned status and custom vendor
    dev.name = "Renamed";
    dev.isPinned = true;
    dev.vendor = "Custom Corp";
    bleTracker.addOrUpdateDevice(dev);

    const BleDevice* updated = nullptr;
    for (const auto& d : bleTracker.getDevices()) {
        if (d.mac == "99:88:77:66:55:44") {
            updated = &d;
            break;
        }
    }
    TEST_ASSERT_NOT_NULL(updated);
    TEST_ASSERT_EQUAL_STRING("Renamed", updated->name.c_str());
    TEST_ASSERT_TRUE(updated->isPinned);
    TEST_ASSERT_EQUAL_STRING("Custom Corp", updated->getVendor());
}

void test_ble_device_get_vendor_fallbacks(void) {
    // When vendor is empty, fall back to OUI resolution
    BleDevice d1;
    d1.vendor = "";
    d1.mac = "00:03:93:11:22:33"; // Apple OUI
    TEST_ASSERT_EQUAL_STRING("Apple", d1.getVendor());

    // When vendor is "Unknown Vendor", fall back to OUI resolution
    BleDevice d2;
    d2.vendor = "Unknown Vendor";
    d2.mac = "00:03:93:11:22:33";
    TEST_ASSERT_EQUAL_STRING("Apple", d2.getVendor());
}

void test_tracker_multi_device_pinning(void) {
    bleTracker.clearDevices();
    settings.clearPinnedMacs();

    // Add 3 devices
    BleDevice d1; d1.mac = "10:00:00:00:00:01"; d1.name = "Dev1"; d1.rssi = -60; d1.lastSeen = 1000;
    BleDevice d2; d2.mac = "10:00:00:00:00:02"; d2.name = "Dev2"; d2.rssi = -70; d2.lastSeen = 1000;
    BleDevice d3; d3.mac = "10:00:00:00:00:03"; d3.name = "Dev3"; d3.rssi = -80; d3.lastSeen = 1000;

    bleTracker.addOrUpdateDevice(d1);
    bleTracker.addOrUpdateDevice(d2);
    bleTracker.addOrUpdateDevice(d3);

    // None pinned initially
    auto pinnedOnly = bleTracker.getFilteredDevices(0, 2, -95);
    TEST_ASSERT_EQUAL(0, pinnedOnly.size());

    // Pin Dev1 and Dev3
    settings.addPinnedMac("10:00:00:00:00:01");
    settings.addPinnedMac("10:00:00:00:00:03");
    bleTracker.updatePinnedDevices();

    pinnedOnly = bleTracker.getFilteredDevices(0, 2, -95);
    TEST_ASSERT_EQUAL(2, pinnedOnly.size());

    // Unpin Dev1
    settings.removePinnedMac("10:00:00:00:00:01");
    bleTracker.updatePinnedDevices();

    pinnedOnly = bleTracker.getFilteredDevices(0, 2, -95);
    TEST_ASSERT_EQUAL(1, pinnedOnly.size());
    TEST_ASSERT_EQUAL_STRING("10:00:00:00:00:03", pinnedOnly[0].mac.c_str());

    settings.clearPinnedMacs();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_tracker_add_and_get_devices);
    RUN_TEST(test_tracker_distance_estimation);
    RUN_TEST(test_tracker_sorting_and_filtering);
    RUN_TEST(test_tracker_target_tracking_and_history);
    RUN_TEST(test_tracker_catalog_versioning);
    RUN_TEST(test_tracker_pruning_version_increment);
    RUN_TEST(test_tracker_scan_interval_and_window);
    RUN_TEST(test_tracker_trend_insufficient_samples_is_stable);
    RUN_TEST(test_tracker_trend_closing_in);
    RUN_TEST(test_tracker_trend_drifting);
    RUN_TEST(test_tracker_trend_stable_with_noise);
    RUN_TEST(test_tracker_trend_after_history_wraps);
    RUN_TEST(test_tracker_begin_and_scan_trigger);
    RUN_TEST(test_tracker_clear_devices);
    RUN_TEST(test_tracker_rssi_history_and_slope_edge_cases);
    RUN_TEST(test_tracker_add_or_update_existing_and_vendor_override);
    RUN_TEST(test_ble_device_get_vendor_fallbacks);
    RUN_TEST(test_tracker_multi_device_pinning);
    return UNITY_END();
}
