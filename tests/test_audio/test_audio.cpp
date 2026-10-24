#include <unity.h>
#include "Arduino.h"
#include "../mocks/mocks.cpp"
#include "audio_manager.h"
#include "../../src/audio_manager.cpp"

void setUp(void) {
    mock_millis_val = 0;
    audio.begin();
    audio.setEnabled(true);
    audio.setVolume(DEFAULT_AUDIO_VOLUME);
    audio.clearTarget();
}

void tearDown(void) {
    audio.clearTarget();
}

void test_audio_initial_state(void) {
    AudioManager am(26, 1);
    am.begin();

    TEST_ASSERT_TRUE(am.isEnabled());
    TEST_ASSERT_EQUAL(DEFAULT_AUDIO_VOLUME, am.getVolume());
    TEST_ASSERT_FALSE(am.hasTarget());
    TEST_ASSERT_FALSE(am.isBeeping());
    TEST_ASSERT_EQUAL_UINT32(0, am.getBeepIntervalMs());
    TEST_ASSERT_EQUAL_UINT16(0, am.getLastFrequency());
}

void test_audio_volume_clamping_and_mute(void) {
    audio.setVolume(50);
    TEST_ASSERT_EQUAL(50, audio.getVolume());

    // Clamping above 100
    audio.setVolume(150);
    TEST_ASSERT_EQUAL(100, audio.getVolume());

    // Mute at 0
    audio.setVolume(0);
    TEST_ASSERT_EQUAL(0, audio.getVolume());

    // Play tone while muted must not beep
    audio.playTone(1000, 50);
    TEST_ASSERT_FALSE(audio.isBeeping());

    // Update with target while muted must not beep
    audio.setTargetRssi(-50);
    mock_millis_val = 1000;
    audio.update(1000);
    TEST_ASSERT_FALSE(audio.isBeeping());

    // Muting while a tone is active stops it immediately
    audio.setVolume(80);
    audio.playTone(1000, 100);
    TEST_ASSERT_TRUE(audio.isBeeping());

    audio.setVolume(0);
    TEST_ASSERT_FALSE(audio.isBeeping());
}

void test_audio_enabled_and_disabled(void) {
    audio.setEnabled(false);
    TEST_ASSERT_FALSE(audio.isEnabled());

    // Play tone while disabled must not beep
    audio.playTone(1200, 50);
    TEST_ASSERT_FALSE(audio.isBeeping());

    // Update with target while disabled must not beep
    audio.setTargetRssi(-50);
    mock_millis_val = 2000;
    audio.update(2000);
    TEST_ASSERT_FALSE(audio.isBeeping());

    // Disabling while a tone is active stops it immediately
    audio.setEnabled(true);
    audio.playTone(1500, 100);
    TEST_ASSERT_TRUE(audio.isBeeping());

    audio.setEnabled(false);
    TEST_ASSERT_FALSE(audio.isBeeping());
}

void test_audio_rssi_to_frequency_mapping(void) {
    // RSSI Floor (-100 dBm) maps to 900 Hz
    TEST_ASSERT_EQUAL_UINT16(900, AudioManager::rssiToFrequency(RSSI_FLOOR));

    // Values below floor clamp to 900 Hz
    TEST_ASSERT_EQUAL_UINT16(900, AudioManager::rssiToFrequency(-115));
    TEST_ASSERT_EQUAL_UINT16(900, AudioManager::rssiToFrequency(-128));

    // RSSI Ceiling (-30 dBm) maps to 2600 Hz
    TEST_ASSERT_EQUAL_UINT16(2600, AudioManager::rssiToFrequency(RSSI_CEILING));

    // Values above ceiling clamp to 2600 Hz
    TEST_ASSERT_EQUAL_UINT16(2600, AudioManager::rssiToFrequency(-20));
    TEST_ASSERT_EQUAL_UINT16(2600, AudioManager::rssiToFrequency(0));

    // Midpoint between -100 and -30 is -65 dBm (ratio = 0.5)
    // 900 + 0.5 * 1700 = 1750 Hz
    TEST_ASSERT_EQUAL_UINT16(1750, AudioManager::rssiToFrequency(-65));

    // Zone boundaries
    // Zone Immediate (-45 dBm): ratio = 55/70 = 0.785714 -> 900 + 1335 = 2235 Hz
    TEST_ASSERT_EQUAL_UINT16(2235, AudioManager::rssiToFrequency(RSSI_ZONE_IMMEDIATE));

    // Zone Mid (-80 dBm): ratio = 20/70 = 0.285714 -> 900 + 485 = 1385 Hz
    TEST_ASSERT_EQUAL_UINT16(1385, AudioManager::rssiToFrequency(RSSI_ZONE_MID));

    // Zone Far (-95 dBm): ratio = 5/70 = 0.071428 -> 900 + 121 = 1021 Hz
    TEST_ASSERT_EQUAL_UINT16(1021, AudioManager::rssiToFrequency(RSSI_ZONE_FAR));
}

void test_audio_rssi_to_interval_cadence_mapping(void) {
    // RSSI Floor (-100 dBm) maps to 1200 ms
    TEST_ASSERT_EQUAL_UINT32(1200, AudioManager::rssiToIntervalMs(RSSI_FLOOR));

    // Values below floor clamp to 1200 ms
    TEST_ASSERT_EQUAL_UINT32(1200, AudioManager::rssiToIntervalMs(-110));
    TEST_ASSERT_EQUAL_UINT32(1200, AudioManager::rssiToIntervalMs(-125));

    // RSSI Ceiling (-30 dBm) maps to 70 ms
    TEST_ASSERT_EQUAL_UINT32(70, AudioManager::rssiToIntervalMs(RSSI_CEILING));

    // Values above ceiling clamp to 70 ms
    TEST_ASSERT_EQUAL_UINT32(70, AudioManager::rssiToIntervalMs(-15));
    TEST_ASSERT_EQUAL_UINT32(70, AudioManager::rssiToIntervalMs(0));

    // Midpoint between -100 and -30 is -65 dBm (ratio = 0.5)
    // 1200 - 0.5 * 1130 = 635 ms
    TEST_ASSERT_EQUAL_UINT32(635, AudioManager::rssiToIntervalMs(-65));

    // Zone boundaries
    // Zone Immediate (-45 dBm): 1200 - (55/70) * 1130 = 312 ms
    TEST_ASSERT_EQUAL_UINT32(312, AudioManager::rssiToIntervalMs(RSSI_ZONE_IMMEDIATE));

    // Zone Mid (-80 dBm): 1200 - (20/70) * 1130 = 877 ms
    TEST_ASSERT_EQUAL_UINT32(877, AudioManager::rssiToIntervalMs(RSSI_ZONE_MID));

    // Zone Far (-95 dBm): 1200 - (5/70) * 1130 = 1119 ms
    TEST_ASSERT_EQUAL_UINT32(1119, AudioManager::rssiToIntervalMs(RSSI_ZONE_FAR));
}

void test_audio_tone_duration_and_timing(void) {
    mock_millis_val = 100;
    audio.playTone(1800, 40);

    TEST_ASSERT_TRUE(audio.isBeeping());
    TEST_ASSERT_EQUAL_UINT16(1800, audio.getLastFrequency());

    // At t=139: 39 ms elapsed (less than 40 ms duration) -> still beeping
    mock_millis_val = 139;
    audio.update(139);
    TEST_ASSERT_TRUE(audio.isBeeping());

    // At t=140: 40 ms elapsed -> tone stops
    mock_millis_val = 140;
    audio.update(140);
    TEST_ASSERT_FALSE(audio.isBeeping());

    // Test playBeep helper
    audio.playBeep(2100, 30);
    TEST_ASSERT_TRUE(audio.isBeeping());
    TEST_ASSERT_EQUAL_UINT16(2100, audio.getLastFrequency());

    mock_millis_val = 170;
    audio.update(170);
    TEST_ASSERT_FALSE(audio.isBeeping());
}

void test_audio_target_rssi_cadence_and_state_transitions(void) {
    audio.clearTarget();
    TEST_ASSERT_FALSE(audio.hasTarget());

    // Set target to RSSI ceiling (-30 dBm) -> 70 ms cadence, 2600 Hz pitch
    audio.setTargetRssi(RSSI_CEILING);
    TEST_ASSERT_TRUE(audio.hasTarget());
    TEST_ASSERT_EQUAL(RSSI_CEILING, audio.getTargetRssi());

    // At t=70: First cadence interval triggers beep
    mock_millis_val = 70;
    audio.update(70);

    TEST_ASSERT_EQUAL_UINT32(70, audio.getBeepIntervalMs());
    TEST_ASSERT_EQUAL_UINT16(2600, audio.getLastFrequency());
    TEST_ASSERT_TRUE(audio.isBeeping());

    // Tone duration is 25 ms. At t=95, tone expires.
    mock_millis_val = 95;
    audio.update(95);
    TEST_ASSERT_FALSE(audio.isBeeping());

    // At t=139 (69 ms after last beep): Interval not yet reached
    mock_millis_val = 139;
    audio.update(139);
    TEST_ASSERT_FALSE(audio.isBeeping());

    // At t=140 (70 ms after last beep): Next beep fires
    mock_millis_val = 140;
    audio.update(140);
    TEST_ASSERT_TRUE(audio.isBeeping());

    // Clearing target stops active tone and resets target state
    audio.clearTarget();
    TEST_ASSERT_FALSE(audio.hasTarget());
    TEST_ASSERT_FALSE(audio.isBeeping());

    // Subsequent updates without target must never beep
    mock_millis_val = 210;
    audio.update(210);
    TEST_ASSERT_FALSE(audio.isBeeping());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_audio_initial_state);
    RUN_TEST(test_audio_volume_clamping_and_mute);
    RUN_TEST(test_audio_enabled_and_disabled);
    RUN_TEST(test_audio_rssi_to_frequency_mapping);
    RUN_TEST(test_audio_rssi_to_interval_cadence_mapping);
    RUN_TEST(test_audio_tone_duration_and_timing);
    RUN_TEST(test_audio_target_rssi_cadence_and_state_transitions);
    return UNITY_END();
}
