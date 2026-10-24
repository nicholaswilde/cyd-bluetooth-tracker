#include <unity.h>
#include "Arduino.h"
#include "../mocks/mocks.cpp"
#include "../../src/settings_manager.cpp"
#include "settings_manager.h"

void setUp(void) {}
void tearDown(void) {}

void test_settings_initial_defaults(void) {
    Preferences prefs;
    prefs.begin("ble_tracker", false);
    prefs.clear();
    prefs.end();

    SettingsManager sm;
    sm.begin();

    TEST_ASSERT_EQUAL(DEFAULT_THEME_FLAVOR, sm.getThemeFlavor());
    TEST_ASSERT_EQUAL(100, sm.getBrightness());
    TEST_ASSERT_FALSE(sm.getAutoBrightness());
    TEST_ASSERT_EQUAL_UINT32(DEFAULT_SCAN_INTERVAL_MS, sm.getScanIntervalMs());
    TEST_ASSERT_EQUAL(DEFAULT_MIN_RSSI, sm.getMinRssi());
    TEST_ASSERT_TRUE(sm.getAudioGeiger());
    TEST_ASSERT_EQUAL(DEFAULT_AUDIO_VOLUME, sm.getAudioVolume());
    TEST_ASSERT_EQUAL(DEFAULT_SCREEN_ORIENTATION, sm.getScreenOrientation());
    TEST_ASSERT_TRUE(sm.getLedEnabled());
    TEST_ASSERT_EQUAL(DEFAULT_LED_BRIGHTNESS, sm.getLedBrightness());
    TEST_ASSERT_EQUAL(DEFAULT_DEVICE_SORT_MODE, sm.getDeviceSortMode());
    TEST_ASSERT_EQUAL(DEFAULT_DEVICE_FILTER_MODE, sm.getDeviceFilterMode());
    TEST_ASSERT_EQUAL_STRING("", sm.getPinnedMac().c_str());
    TEST_ASSERT_EQUAL(0, sm.getPinnedCount());
    TEST_ASSERT_TRUE(sm.getPinnedMacs().empty());
}

void test_settings_setters_and_persistence(void) {
    Preferences prefs;
    prefs.begin("ble_tracker", false);
    prefs.clear();
    prefs.end();

    SettingsManager sm;
    sm.begin();

    sm.setThemeFlavor(CATPPUCCIN_LATTE);
    TEST_ASSERT_EQUAL(CATPPUCCIN_LATTE, sm.getThemeFlavor());

    sm.setBrightness(75);
    TEST_ASSERT_EQUAL(75, sm.getBrightness());

    // Test brightness clamping (10 - 100)
    sm.setBrightness(5);
    TEST_ASSERT_EQUAL(10, sm.getBrightness());
    sm.setBrightness(120);
    TEST_ASSERT_EQUAL(100, sm.getBrightness());

    sm.setAutoBrightness(true);
    TEST_ASSERT_TRUE(sm.getAutoBrightness());

    sm.setScanIntervalMs(3000);
    TEST_ASSERT_EQUAL_UINT32(3000, sm.getScanIntervalMs());
    sm.setScanIntervalMs(200); // below min 500
    TEST_ASSERT_EQUAL_UINT32(500, sm.getScanIntervalMs());

    sm.setMinRssi(-70);
    TEST_ASSERT_EQUAL(-70, sm.getMinRssi());
    sm.setMinRssi(-110); // below min -95
    TEST_ASSERT_EQUAL(-95, sm.getMinRssi());
    sm.setMinRssi(-20);  // above max -40
    TEST_ASSERT_EQUAL(-40, sm.getMinRssi());

    sm.setAudioGeiger(false);
    TEST_ASSERT_FALSE(sm.getAudioGeiger());

    sm.setAudioVolume(50);
    TEST_ASSERT_EQUAL(50, sm.getAudioVolume());
    sm.setAudioVolume(150);
    TEST_ASSERT_EQUAL(100, sm.getAudioVolume());

    sm.setScreenOrientation(3);
    TEST_ASSERT_EQUAL(3, sm.getScreenOrientation());

    sm.setLedEnabled(false);
    TEST_ASSERT_FALSE(sm.getLedEnabled());

    sm.setLedBrightness(180);
    TEST_ASSERT_EQUAL(180, sm.getLedBrightness());
    sm.setLedBrightness(-20);
    TEST_ASSERT_EQUAL(0, sm.getLedBrightness());
    sm.setLedBrightness(300);
    TEST_ASSERT_EQUAL(255, sm.getLedBrightness());

    sm.setDeviceSortMode(1);
    TEST_ASSERT_EQUAL(1, sm.getDeviceSortMode());

    sm.setDeviceFilterMode(2);
    TEST_ASSERT_EQUAL(2, sm.getDeviceFilterMode());

    sm.setPinnedMac("AA:BB:CC:DD:EE:FF");
    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:FF", sm.getPinnedMac().c_str());

    // Reboot simulation
    SettingsManager rebooted;
    rebooted.begin();

    TEST_ASSERT_EQUAL(CATPPUCCIN_LATTE, rebooted.getThemeFlavor());
    TEST_ASSERT_EQUAL(100, rebooted.getBrightness());
    TEST_ASSERT_TRUE(rebooted.getAutoBrightness());
    TEST_ASSERT_EQUAL_UINT32(500, rebooted.getScanIntervalMs());
    TEST_ASSERT_EQUAL(-40, rebooted.getMinRssi());
    TEST_ASSERT_FALSE(rebooted.getAudioGeiger());
    TEST_ASSERT_EQUAL(100, rebooted.getAudioVolume());
    TEST_ASSERT_EQUAL(3, rebooted.getScreenOrientation());
    TEST_ASSERT_FALSE(rebooted.getLedEnabled());
    TEST_ASSERT_EQUAL(255, rebooted.getLedBrightness());
    TEST_ASSERT_EQUAL(1, rebooted.getDeviceSortMode());
    TEST_ASSERT_EQUAL(2, rebooted.getDeviceFilterMode());
    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:FF", rebooted.getPinnedMac().c_str());
}

void test_settings_multi_pinned_favorites(void) {
    Preferences prefs;
    prefs.begin("ble_tracker", false);
    prefs.clear();
    prefs.end();

    SettingsManager sm;
    sm.begin();

    TEST_ASSERT_FALSE(sm.isMacPinned(""));
    TEST_ASSERT_FALSE(sm.isMacPinned("00:11:22:33:44:55"));

    // Add first device
    TEST_ASSERT_TRUE(sm.addPinnedMac("00:11:22:33:44:01"));
    TEST_ASSERT_TRUE(sm.isMacPinned("00:11:22:33:44:01"));
    TEST_ASSERT_EQUAL(1, sm.getPinnedCount());
    TEST_ASSERT_EQUAL_STRING("00:11:22:33:44:01", sm.getPinnedMac().c_str());

    // Duplicate add should return false
    TEST_ASSERT_FALSE(sm.addPinnedMac("00:11:22:33:44:01"));
    TEST_ASSERT_EQUAL(1, sm.getPinnedCount());

    // Empty add should return false
    TEST_ASSERT_FALSE(sm.addPinnedMac(""));

    // Add up to MAX_PINNED_DEVICES (8)
    for (int i = 2; i <= 8; i++) {
        char mac[32];
        snprintf(mac, sizeof(mac), "00:11:22:33:44:%02d", i);
        TEST_ASSERT_TRUE(sm.addPinnedMac(mac));
    }
    TEST_ASSERT_EQUAL(8, sm.getPinnedCount());

    // 9th device must be rejected due to capacity limit
    TEST_ASSERT_FALSE(sm.addPinnedMac("00:11:22:33:44:09"));
    TEST_ASSERT_EQUAL(8, sm.getPinnedCount());

    // Remove a device
    TEST_ASSERT_TRUE(sm.removePinnedMac("00:11:22:33:44:01"));
    TEST_ASSERT_FALSE(sm.isMacPinned("00:11:22:33:44:01"));
    TEST_ASSERT_EQUAL(7, sm.getPinnedCount());
    TEST_ASSERT_EQUAL_STRING("00:11:22:33:44:02", sm.getPinnedMac().c_str());

    // Remove non-existent device returns false
    TEST_ASSERT_FALSE(sm.removePinnedMac("FF:FF:FF:FF:FF:FF"));
    TEST_ASSERT_FALSE(sm.removePinnedMac(""));

    // Toggle pinned device
    // Pinned -> Unpin
    TEST_ASSERT_TRUE(sm.togglePinnedMac("00:11:22:33:44:02"));
    TEST_ASSERT_FALSE(sm.isMacPinned("00:11:22:33:44:02"));
    TEST_ASSERT_EQUAL(6, sm.getPinnedCount());

    // Unpinned -> Pin
    TEST_ASSERT_TRUE(sm.togglePinnedMac("00:11:22:33:44:02"));
    TEST_ASSERT_TRUE(sm.isMacPinned("00:11:22:33:44:02"));
    TEST_ASSERT_EQUAL(7, sm.getPinnedCount());

    // Simulated reboot persistence of multi-pinned list
    SettingsManager rebooted;
    rebooted.begin();
    TEST_ASSERT_EQUAL(7, rebooted.getPinnedCount());
    TEST_ASSERT_TRUE(rebooted.isMacPinned("00:11:22:33:44:02"));
    TEST_ASSERT_TRUE(rebooted.isMacPinned("00:11:22:33:44:08"));
    TEST_ASSERT_FALSE(rebooted.isMacPinned("00:11:22:33:44:01"));

    // Clear all pinned
    rebooted.clearPinnedMacs();
    TEST_ASSERT_EQUAL(0, rebooted.getPinnedCount());
    TEST_ASSERT_EQUAL_STRING("", rebooted.getPinnedMac().c_str());
}

void test_settings_reset_to_defaults(void) {
    SettingsManager sm;
    sm.setThemeFlavor(CATPPUCCIN_FRAPPE);
    sm.setBrightness(40);
    sm.setAutoBrightness(true);
    sm.setMinRssi(-60);
    sm.setAudioGeiger(false);
    sm.addPinnedMac("11:22:33:44:55:66");
    sm.addPinnedMac("22:33:44:55:66:77");

    TEST_ASSERT_EQUAL(2, sm.getPinnedCount());

    sm.resetToDefaults();

    TEST_ASSERT_EQUAL(DEFAULT_THEME_FLAVOR, sm.getThemeFlavor());
    TEST_ASSERT_EQUAL(100, sm.getBrightness());
    TEST_ASSERT_FALSE(sm.getAutoBrightness());
    TEST_ASSERT_EQUAL(DEFAULT_MIN_RSSI, sm.getMinRssi());
    TEST_ASSERT_TRUE(sm.getAudioGeiger());
    TEST_ASSERT_EQUAL_STRING("", sm.getPinnedMac().c_str());
    TEST_ASSERT_EQUAL(0, sm.getPinnedCount());

    SettingsManager rebooted;
    rebooted.begin();
    TEST_ASSERT_EQUAL(DEFAULT_THEME_FLAVOR, rebooted.getThemeFlavor());
    TEST_ASSERT_EQUAL(100, rebooted.getBrightness());
    TEST_ASSERT_EQUAL_STRING("", rebooted.getPinnedMac().c_str());
    TEST_ASSERT_EQUAL(0, rebooted.getPinnedCount());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_settings_initial_defaults);
    RUN_TEST(test_settings_setters_and_persistence);
    RUN_TEST(test_settings_multi_pinned_favorites);
    RUN_TEST(test_settings_reset_to_defaults);
    return UNITY_END();
}
