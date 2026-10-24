#include <unity.h>
#include "../mocks/mocks.cpp"
#include "display.h"
#include "screenshot_manager.h"
#include "../../src/settings_manager.cpp"

// Mock ScreenshotManager functions used by display.cpp
bool ScreenshotManager::isCaptureInProgress() { return false; }
void ScreenshotManager::onFlushTile(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t* pixels) {}

#include "../../src/touch_manager.cpp"
#include "../../src/display.cpp"

void setUp(void) {
    setMockTouch(false, 0, 0);
    setMockCapacitive(false);
}

void tearDown(void) {
    setMockTouch(false, 0, 0);
    setMockCapacitive(false);
}

void test_coordinate_mapping_resistive_landscape(void) {
    setMockCapacitive(false);
    int16_t out_x, out_y;

    // Rotation 1 (Landscape, 320x240)
    mapTouchCoordinates(200, 200, out_x, out_y, 1);
    TEST_ASSERT_EQUAL(0, out_x);
    TEST_ASSERT_EQUAL(0, out_y);

    mapTouchCoordinates(3700, 3900, out_x, out_y, 1);
    TEST_ASSERT_EQUAL(319, out_x);
    TEST_ASSERT_EQUAL(239, out_y);
}

void test_coordinate_mapping_resistive_landscape_rev(void) {
    setMockCapacitive(false);
    int16_t out_x, out_y;

    // Rotation 3 (Landscape Rev, 320x240)
    mapTouchCoordinates(200, 200, out_x, out_y, 3);
    TEST_ASSERT_EQUAL(319, out_x);
    TEST_ASSERT_EQUAL(239, out_y);

    mapTouchCoordinates(3700, 3900, out_x, out_y, 3);
    TEST_ASSERT_EQUAL(0, out_x);
    TEST_ASSERT_EQUAL(0, out_y);
}

void test_coordinate_mapping_resistive_portrait(void) {
    setMockCapacitive(false);
    int16_t out_x, out_y;

    // Rotation 0 (Portrait Rev, 240x320)
    mapTouchCoordinates(200, 200, out_x, out_y, 0);
    TEST_ASSERT_EQUAL(239, out_x);
    TEST_ASSERT_EQUAL(0, out_y);

    mapTouchCoordinates(3700, 3900, out_x, out_y, 0);
    TEST_ASSERT_EQUAL(0, out_x);
    TEST_ASSERT_EQUAL(319, out_y);

    // Rotation 2 (Portrait, 240x320)
    mapTouchCoordinates(200, 200, out_x, out_y, 2);
    TEST_ASSERT_EQUAL(0, out_x);
    TEST_ASSERT_EQUAL(319, out_y);

    mapTouchCoordinates(3700, 3900, out_x, out_y, 2);
    TEST_ASSERT_EQUAL(239, out_x);
    TEST_ASSERT_EQUAL(0, out_y);
}

void test_coordinate_mapping_capacitive_variants(void) {
    setMockCapacitive(true);
    int16_t out_x, out_y;

    // Raw portrait touch (e.g. CST816 / GT911 raw_x <= 240, raw_y <= 320)
    // In landscape (orientation 1): lx = raw_y, ly = 240 - raw_x
    mapTouchCoordinates(50, 100, out_x, out_y, 1);
    TEST_ASSERT_EQUAL(100, out_x);
    TEST_ASSERT_EQUAL(240 - 50, out_y);

    // In landscape rev (orientation 3): lx = 320 - raw_y, ly = raw_x
    mapTouchCoordinates(50, 100, out_x, out_y, 3);
    TEST_ASSERT_EQUAL(320 - 100, out_x);
    TEST_ASSERT_EQUAL(50, out_y);

    // In native portrait (orientation 0): lx = raw_x, ly = raw_y
    mapTouchCoordinates(50, 100, out_x, out_y, 0);
    TEST_ASSERT_EQUAL(50, out_x);
    TEST_ASSERT_EQUAL(100, out_y);

    // Raw landscape touch (raw_x > 240)
    // In landscape (orientation 1): lx = raw_x, ly = raw_y
    mapTouchCoordinates(260, 150, out_x, out_y, 1);
    TEST_ASSERT_EQUAL(260, out_x);
    TEST_ASSERT_EQUAL(150, out_y);
}

void test_coordinate_clamping(void) {
    setMockCapacitive(false);
    int16_t out_x, out_y;

    // Out of bounds low: negative raw coordinates
    mapTouchCoordinates(-100, -100, out_x, out_y, 1);
    TEST_ASSERT_EQUAL(0, out_x);
    TEST_ASSERT_EQUAL(0, out_y);

    // Out of bounds high: very high raw coordinates
    mapTouchCoordinates(5000, 5000, out_x, out_y, 1);
    TEST_ASSERT_EQUAL(319, out_x);
    TEST_ASSERT_EQUAL(239, out_y);
}

void test_touch_manager_mock_and_callback(void) {
    TouchManager::begin();

    setMockCapacitive(false);
    TEST_ASSERT_FALSE(TouchManager::isCapacitive());

    setMockCapacitive(true);
    TEST_ASSERT_TRUE(TouchManager::isCapacitive());

    setMockTouch(false, 0, 0);
    TEST_ASSERT_FALSE(TouchManager::isTouched());

    int tx = 0, ty = 0;
    TEST_ASSERT_FALSE(TouchManager::getTouchPoint(tx, ty));

    setMockTouch(true, 1500, 2000);
    TEST_ASSERT_TRUE(TouchManager::isTouched());
    TEST_ASSERT_TRUE(TouchManager::getTouchPoint(tx, ty));
    TEST_ASSERT_EQUAL(1500, tx);
    TEST_ASSERT_EQUAL(2000, ty);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_coordinate_mapping_resistive_landscape);
    RUN_TEST(test_coordinate_mapping_resistive_landscape_rev);
    RUN_TEST(test_coordinate_mapping_resistive_portrait);
    RUN_TEST(test_coordinate_mapping_capacitive_variants);
    RUN_TEST(test_coordinate_clamping);
    RUN_TEST(test_touch_manager_mock_and_callback);
    return UNITY_END();
}
