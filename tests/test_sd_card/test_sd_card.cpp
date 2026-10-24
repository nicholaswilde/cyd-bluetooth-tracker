#include <unity.h>
#include "Arduino.h"
#include "../mocks/mocks.cpp"
#include "sd_card_manager.h"
#include "../../src/sd_card_manager.cpp"

void setUp(void) {
    mock_sd_card_present = true;
    mock_sd_card_mounted = false;
}

void tearDown(void) {
    SdCardManager::end();
    mock_sd_card_present = true;
}

void test_sd_card_begin_mounted_and_card_present(void) {
    mock_sd_card_present = true;
    mock_sd_card_mounted = false;

    TEST_ASSERT_TRUE(SdCardManager::begin());
    TEST_ASSERT_TRUE(SdCardManager::isMounted());
    TEST_ASSERT_TRUE(SdCardManager::isCardPresent());

    // Calling begin() again while already mounted should succeed
    TEST_ASSERT_TRUE(SdCardManager::begin());
}

void test_sd_card_end_unmounts(void) {
    mock_sd_card_present = true;
    SdCardManager::begin();
    TEST_ASSERT_TRUE(SdCardManager::isMounted());

    SdCardManager::end();
    TEST_ASSERT_FALSE(SdCardManager::isMounted());
}

void test_sd_card_begin_fails_when_absent(void) {
    mock_sd_card_present = false;
    mock_sd_card_mounted = false;

    TEST_ASSERT_FALSE(SdCardManager::begin());
    TEST_ASSERT_FALSE(SdCardManager::isMounted());
    TEST_ASSERT_FALSE(SdCardManager::isCardPresent());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_sd_card_begin_mounted_and_card_present);
    RUN_TEST(test_sd_card_end_unmounts);
    RUN_TEST(test_sd_card_begin_fails_when_absent);
    return UNITY_END();
}
