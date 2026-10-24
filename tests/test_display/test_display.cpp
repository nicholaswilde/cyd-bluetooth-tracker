#include <unity.h>
#include "../mocks/mocks.cpp"
#include "display.h"
#include "screenshot_manager.h"
#include "../../src/settings_manager.cpp"

// Mock ScreenshotManager functions used by display.cpp
static bool mock_screenshot_capture_in_progress = false;
static bool mock_tile_flushed = false;
bool ScreenshotManager::isCaptureInProgress() { return mock_screenshot_capture_in_progress; }
void ScreenshotManager::onFlushTile(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t* pixels) {
    mock_tile_flushed = true;
}

#include "../../src/touch_manager.cpp"
#include "../../src/display.cpp"

void setUp(void) {
    setMockTouch(false, 0, 0);
    setMockCapacitive(false);
    mock_screenshot_capture_in_progress = false;
    mock_tile_flushed = false;
}

void tearDown(void) {
    setMockTouch(false, 0, 0);
    setMockCapacitive(false);
    mock_screenshot_capture_in_progress = false;
    mock_tile_flushed = false;
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

    // Default fallback rotation
    mapTouchCoordinates(200, 200, out_x, out_y, 99);
    TEST_ASSERT_EQUAL(0, out_x);
    TEST_ASSERT_EQUAL(0, out_y);
}

void test_coordinate_mapping_capacitive_variants(void) {
    setMockCapacitive(true);
    int16_t out_x, out_y;

    // Raw portrait touch (raw_x <= 240, raw_y <= 320)
    // Orientation 0
    mapTouchCoordinates(50, 100, out_x, out_y, 0);
    TEST_ASSERT_EQUAL(50, out_x);
    TEST_ASSERT_EQUAL(100, out_y);

    // Orientation 1
    mapTouchCoordinates(50, 100, out_x, out_y, 1);
    TEST_ASSERT_EQUAL(100, out_x);
    TEST_ASSERT_EQUAL(240 - 50, out_y);

    // Orientation 2
    mapTouchCoordinates(50, 100, out_x, out_y, 2);
    TEST_ASSERT_EQUAL(240 - 50, out_x);
    TEST_ASSERT_EQUAL(320 - 100, out_y);

    // Orientation 3
    mapTouchCoordinates(50, 100, out_x, out_y, 3);
    TEST_ASSERT_EQUAL(320 - 100, out_x);
    TEST_ASSERT_EQUAL(50, out_y);

    // Orientation 99 (default fallback)
    mapTouchCoordinates(50, 100, out_x, out_y, 99);
    TEST_ASSERT_EQUAL(100, out_x);
    TEST_ASSERT_EQUAL(50, out_y);

    // Raw landscape touch (raw_x > 240)
    // Orientation 0
    mapTouchCoordinates(260, 150, out_x, out_y, 0);
    TEST_ASSERT_EQUAL(240 - 150, out_x);
    TEST_ASSERT_EQUAL(320 - 260, out_y);

    // Orientation 1
    mapTouchCoordinates(260, 150, out_x, out_y, 1);
    TEST_ASSERT_EQUAL(260, out_x);
    TEST_ASSERT_EQUAL(150, out_y);

    // Orientation 2
    mapTouchCoordinates(260, 150, out_x, out_y, 2);
    TEST_ASSERT_EQUAL(150, out_x);
    TEST_ASSERT_EQUAL(260, out_y);

    // Orientation 3
    mapTouchCoordinates(260, 150, out_x, out_y, 3);
    TEST_ASSERT_EQUAL(320 - 260, out_x);
    TEST_ASSERT_EQUAL(240 - 150, out_y);

    // Orientation 99 (default fallback)
    mapTouchCoordinates(260, 150, out_x, out_y, 99);
    TEST_ASSERT_EQUAL(239, out_x);
    TEST_ASSERT_EQUAL(150, out_y);
}

void test_coordinate_clamping(void) {
    setMockCapacitive(false);
    int16_t out_x, out_y;

    // Landscape bounds
    mapTouchCoordinates(-100, -100, out_x, out_y, 1);
    TEST_ASSERT_EQUAL(0, out_x);
    TEST_ASSERT_EQUAL(0, out_y);

    mapTouchCoordinates(5000, 5000, out_x, out_y, 1);
    TEST_ASSERT_EQUAL(319, out_x);
    TEST_ASSERT_EQUAL(239, out_y);

    // Portrait bounds (orientation 0 is Portrait Rev: out_x = 240 - ly, out_y = lx)
    mapTouchCoordinates(-100, -100, out_x, out_y, 0);
    TEST_ASSERT_EQUAL(239, out_x);
    TEST_ASSERT_EQUAL(0, out_y);

    mapTouchCoordinates(5000, 5000, out_x, out_y, 0);
    TEST_ASSERT_EQUAL(0, out_x);
    TEST_ASSERT_EQUAL(319, out_y);
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

void test_display_flush_callback(void) {
    lv_disp_drv_t drv;
    lv_area_t area = {10, 10, 19, 19}; // 10x10 tile
    lv_color_t colorBuf[100] = {0};

    // 1. Without screenshot capture active
    mock_screenshot_capture_in_progress = false;
    mock_tile_flushed = false;
    my_disp_flush(&drv, &area, colorBuf);
    TEST_ASSERT_FALSE(mock_tile_flushed);

    // 2. With screenshot capture active
    mock_screenshot_capture_in_progress = true;
    mock_tile_flushed = false;
    my_disp_flush(&drv, &area, colorBuf);
    TEST_ASSERT_TRUE(mock_tile_flushed);
}

void test_display_touchpad_read(void) {
    lv_indev_drv_t indev;
    lv_indev_data_t data;

    // Released state
    setMockTouch(false, 0, 0);
    my_touchpad_read(&indev, &data);
    TEST_ASSERT_EQUAL(LV_INDEV_STATE_REL, data.state);

    // Pressed state
    setMockTouch(true, 1500, 2000);
    settings.setScreenOrientation(1);
    my_touchpad_read(&indev, &data);
    TEST_ASSERT_EQUAL(LV_INDEV_STATE_PR, data.state);
}

void test_display_initialization_functions(void) {
    settings.setScreenOrientation(1);
    initDisplayAndTouch();

    settings.setScreenOrientation(0);
    initDisplayAndTouch();

    settings.setScreenOrientation(1); // Landscape
    initLVGL();

    settings.setScreenOrientation(0); // Portrait
    initLVGL();
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_coordinate_mapping_resistive_landscape);
    RUN_TEST(test_coordinate_mapping_resistive_landscape_rev);
    RUN_TEST(test_coordinate_mapping_resistive_portrait);
    RUN_TEST(test_coordinate_mapping_capacitive_variants);
    RUN_TEST(test_coordinate_clamping);
    RUN_TEST(test_touch_manager_mock_and_callback);
    RUN_TEST(test_display_flush_callback);
    RUN_TEST(test_display_touchpad_read);
    RUN_TEST(test_display_initialization_functions);
    return UNITY_END();
}
