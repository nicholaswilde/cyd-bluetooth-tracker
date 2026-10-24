#include <unity.h>
#include "Arduino.h"
#include "../mocks/mocks.cpp"
#include "screenshot_manager.h"
#include "sd_card_manager.h"
#include "ui.h"

// Stubs for ui.h functions invoked by ScreenshotManager::handleCommand
static int mock_last_tab = -1;
static bool mock_modal_opened = false;
static bool mock_modal_closed = false;
static bool mock_tracker_opened = false;
static bool mock_tracker_closed = false;

void setTab(uint16_t index) { mock_last_tab = (int)index; }
uint16_t getTab() { return (uint16_t)mock_last_tab; }
void openDetailModal() { mock_modal_opened = true; }
void openDetailModal(const BleDevice& dev) { mock_modal_opened = true; }
void closeDetailModal() { mock_modal_closed = true; }
void openTrackerDemo() { mock_tracker_opened = true; }
void closeTrackerView() { mock_tracker_closed = true; }
void initUI() {}
void updateUI() {}
void refreshThemeUI() {}
void showUIStatusMessage(const char* message) {}

#include "../../src/sd_card_manager.cpp"
#include "../../src/screenshot_manager.cpp"

void setUp(void) {
    mock_millis_val = 0;
    mock_sd_card_present = true;
    mock_sd_card_mounted = false;
    mock_files.clear();
    mock_last_tab = -1;
    mock_modal_opened = false;
    mock_modal_closed = false;
    mock_tracker_opened = false;
    mock_tracker_closed = false;
    ScreenshotManager::init();
}

void tearDown(void) {
    ScreenshotManager::init();
    mock_files.clear();
}

void test_screenshot_rle16_compress_empty_and_null(void) {
    uint16_t pixels[10] = {0};
    uint8_t outBuf[32];

    // Null pixels pointer
    TEST_ASSERT_EQUAL(0, ScreenshotManager::compressRLE16(nullptr, 10, outBuf, sizeof(outBuf)));

    // Zero count
    TEST_ASSERT_EQUAL(0, ScreenshotManager::compressRLE16(pixels, 0, outBuf, sizeof(outBuf)));

    // Null output buffer
    TEST_ASSERT_EQUAL(0, ScreenshotManager::compressRLE16(pixels, 10, nullptr, sizeof(outBuf)));

    // Output buffer too small (< 3 bytes for single run)
    TEST_ASSERT_EQUAL(0, ScreenshotManager::compressRLE16(pixels, 10, outBuf, 2));
}

void test_screenshot_rle16_compress_single_run(void) {
    uint16_t pixels[10];
    for (int i = 0; i < 10; i++) {
        pixels[i] = 0xABCD;
    }
    uint8_t outBuf[32];

    size_t outLen = ScreenshotManager::compressRLE16(pixels, 10, outBuf, sizeof(outBuf));
    TEST_ASSERT_EQUAL(3, outLen);
    TEST_ASSERT_EQUAL_UINT8(10, outBuf[0]);
    TEST_ASSERT_EQUAL_UINT8(0xCD, outBuf[1]);
    TEST_ASSERT_EQUAL_UINT8(0xAB, outBuf[2]);
}

void test_screenshot_rle16_compress_run_exceeding_255(void) {
    // 300 identical pixels: max run is 255, so it should split into 255 + 45
    uint16_t pixels[300];
    for (int i = 0; i < 300; i++) {
        pixels[i] = 0x1234;
    }
    uint8_t outBuf[32];

    size_t outLen = ScreenshotManager::compressRLE16(pixels, 300, outBuf, sizeof(outBuf));
    TEST_ASSERT_EQUAL(6, outLen);

    // Chunk 1: 255 pixels
    TEST_ASSERT_EQUAL_UINT8(255, outBuf[0]);
    TEST_ASSERT_EQUAL_UINT8(0x34, outBuf[1]);
    TEST_ASSERT_EQUAL_UINT8(0x12, outBuf[2]);

    // Chunk 2: 45 pixels
    TEST_ASSERT_EQUAL_UINT8(45, outBuf[3]);
    TEST_ASSERT_EQUAL_UINT8(0x34, outBuf[4]);
    TEST_ASSERT_EQUAL_UINT8(0x12, outBuf[5]);
}

void test_screenshot_rle16_compress_alternating_pixels(void) {
    uint16_t pixels[3] = {0x0001, 0x0002, 0x0003};
    uint8_t outBuf[32];

    size_t outLen = ScreenshotManager::compressRLE16(pixels, 3, outBuf, sizeof(outBuf));
    TEST_ASSERT_EQUAL(9, outLen);

    // Run 1
    TEST_ASSERT_EQUAL_UINT8(1, outBuf[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01, outBuf[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00, outBuf[2]);

    // Run 2
    TEST_ASSERT_EQUAL_UINT8(1, outBuf[3]);
    TEST_ASSERT_EQUAL_UINT8(0x02, outBuf[4]);
    TEST_ASSERT_EQUAL_UINT8(0x00, outBuf[5]);

    // Run 3
    TEST_ASSERT_EQUAL_UINT8(1, outBuf[6]);
    TEST_ASSERT_EQUAL_UINT8(0x03, outBuf[7]);
    TEST_ASSERT_EQUAL_UINT8(0x00, outBuf[8]);
}

void test_screenshot_rle16_compress_buffer_limit(void) {
    uint16_t pixels[5] = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555};
    uint8_t outBuf[32];

    // With maxOut = 7, only 2 complete 3-byte runs fit (6 bytes)
    size_t outLen = ScreenshotManager::compressRLE16(pixels, 5, outBuf, 7);
    TEST_ASSERT_EQUAL(6, outLen);
}

void test_screenshot_rle16_decompress_and_roundtrip(void) {
    // Error cases
    uint16_t decPixels[600];
    uint8_t validIn[3] = {5, 0x11, 0x22};

    TEST_ASSERT_EQUAL(0, ScreenshotManager::decompressRLE16(nullptr, 3, decPixels, 600));
    TEST_ASSERT_EQUAL(0, ScreenshotManager::decompressRLE16(validIn, 2, decPixels, 600));
    TEST_ASSERT_EQUAL(0, ScreenshotManager::decompressRLE16(validIn, 3, nullptr, 600));
    TEST_ASSERT_EQUAL(0, ScreenshotManager::decompressRLE16(validIn, 3, decPixels, 0));

    // Construct a diverse array of 500 pixels
    uint16_t origPixels[500];
    size_t idx = 0;
    for (int i = 0; i < 50; i++) origPixels[idx++] = 0xF800;   // Red run (50)
    for (int i = 0; i < 280; i++) origPixels[idx++] = 0x07E0;  // Green run > 255 (280)
    for (int i = 0; i < 10; i++) origPixels[idx++] = (uint16_t)(0x1000 + i); // Alternating
    for (int i = 0; i < 160; i++) origPixels[idx++] = 0x001F;  // Blue run (160)
    TEST_ASSERT_EQUAL(500, idx);

    uint8_t compBuf[1024];
    size_t compBytes = ScreenshotManager::compressRLE16(origPixels, 500, compBuf, sizeof(compBuf));
    TEST_ASSERT_TRUE(compBytes > 0);
    TEST_ASSERT_TRUE(compBytes < 500 * sizeof(uint16_t)); // Verified compression reduction

    size_t decCount = ScreenshotManager::decompressRLE16(compBuf, compBytes, decPixels, 600);
    TEST_ASSERT_EQUAL(500, decCount);

    for (size_t i = 0; i < 500; i++) {
        TEST_ASSERT_EQUAL_HEX16(origPixels[i], decPixels[i]);
    }
}

void test_screenshot_bmp_header_calculation_320x240(void) {
    BmpHeader hdr = ScreenshotManager::createHeader(320, 240);

    // Magic number 'BM'
    TEST_ASSERT_EQUAL_UINT8('B', hdr.bytes[0]);
    TEST_ASSERT_EQUAL_UINT8('M', hdr.bytes[1]);

    // Total file size: 54 + (320 * 240 * 3) = 230454 bytes
    uint32_t fileSize = (uint32_t)hdr.bytes[2] |
                        ((uint32_t)hdr.bytes[3] << 8) |
                        ((uint32_t)hdr.bytes[4] << 16) |
                        ((uint32_t)hdr.bytes[5] << 24);
    TEST_ASSERT_EQUAL_UINT32(230454, fileSize);

    // Reserved fields must be 0
    TEST_ASSERT_EQUAL_UINT8(0, hdr.bytes[6]);
    TEST_ASSERT_EQUAL_UINT8(0, hdr.bytes[7]);
    TEST_ASSERT_EQUAL_UINT8(0, hdr.bytes[8]);
    TEST_ASSERT_EQUAL_UINT8(0, hdr.bytes[9]);

    // Pixel offset: 54
    uint32_t offset = (uint32_t)hdr.bytes[10] |
                      ((uint32_t)hdr.bytes[11] << 8) |
                      ((uint32_t)hdr.bytes[12] << 16) |
                      ((uint32_t)hdr.bytes[13] << 24);
    TEST_ASSERT_EQUAL_UINT32(54, offset);

    // DIB header size: 40 bytes (BITMAPINFOHEADER)
    uint32_t dibSize = (uint32_t)hdr.bytes[14] |
                       ((uint32_t)hdr.bytes[15] << 8) |
                       ((uint32_t)hdr.bytes[16] << 16) |
                       ((uint32_t)hdr.bytes[17] << 24);
    TEST_ASSERT_EQUAL_UINT32(40, dibSize);

    // Width: 320
    uint32_t width = (uint32_t)hdr.bytes[18] |
                     ((uint32_t)hdr.bytes[19] << 8) |
                     ((uint32_t)hdr.bytes[20] << 16) |
                     ((uint32_t)hdr.bytes[21] << 24);
    TEST_ASSERT_EQUAL_UINT32(320, width);

    // Height: -240 (negative indicates top-down row order)
    int32_t height = (int32_t)((uint32_t)hdr.bytes[22] |
                               ((uint32_t)hdr.bytes[23] << 8) |
                               ((uint32_t)hdr.bytes[24] << 16) |
                               ((uint32_t)hdr.bytes[25] << 24));
    TEST_ASSERT_EQUAL_INT32(-240, height);

    // Planes: 1
    uint16_t planes = (uint16_t)hdr.bytes[26] | ((uint16_t)hdr.bytes[27] << 8);
    TEST_ASSERT_EQUAL_UINT16(1, planes);

    // Bits per pixel: 24
    uint16_t bpp = (uint16_t)hdr.bytes[28] | ((uint16_t)hdr.bytes[29] << 8);
    TEST_ASSERT_EQUAL_UINT16(24, bpp);

    // Compression: 0 (BI_RGB)
    uint32_t compression = (uint32_t)hdr.bytes[30] |
                           ((uint32_t)hdr.bytes[31] << 8) |
                           ((uint32_t)hdr.bytes[32] << 16) |
                           ((uint32_t)hdr.bytes[33] << 24);
    TEST_ASSERT_EQUAL_UINT32(0, compression);

    // Image size: 320 * 240 * 3 = 230400 bytes
    uint32_t imageSize = (uint32_t)hdr.bytes[34] |
                         ((uint32_t)hdr.bytes[35] << 8) |
                         ((uint32_t)hdr.bytes[36] << 16) |
                         ((uint32_t)hdr.bytes[37] << 24);
    TEST_ASSERT_EQUAL_UINT32(230400, imageSize);
}

void test_screenshot_bmp_header_calculation_varied_dimensions(void) {
    // 3.5" CYD resolution: 480x320 landscape
    BmpHeader hdr35 = ScreenshotManager::createHeader(480, 320);

    uint32_t fileSize35 = (uint32_t)hdr35.bytes[2] |
                          ((uint32_t)hdr35.bytes[3] << 8) |
                          ((uint32_t)hdr35.bytes[4] << 16) |
                          ((uint32_t)hdr35.bytes[5] << 24);
    TEST_ASSERT_EQUAL_UINT32(54 + (480 * 320 * 3), fileSize35);

    uint32_t width35 = (uint32_t)hdr35.bytes[18] |
                       ((uint32_t)hdr35.bytes[19] << 8) |
                       ((uint32_t)hdr35.bytes[20] << 16) |
                       ((uint32_t)hdr35.bytes[21] << 24);
    TEST_ASSERT_EQUAL_UINT32(480, width35);

    int32_t height35 = (int32_t)((uint32_t)hdr35.bytes[22] |
                                 ((uint32_t)hdr35.bytes[23] << 8) |
                                 ((uint32_t)hdr35.bytes[24] << 16) |
                                 ((uint32_t)hdr35.bytes[25] << 24));
    TEST_ASSERT_EQUAL_INT32(-320, height35);

    // Portrait 240x320
    BmpHeader hdrPortrait = ScreenshotManager::createHeader(240, 320);
    uint32_t widthP = (uint32_t)hdrPortrait.bytes[18] |
                      ((uint32_t)hdrPortrait.bytes[19] << 8) |
                      ((uint32_t)hdrPortrait.bytes[20] << 16) |
                      ((uint32_t)hdrPortrait.bytes[21] << 24);
    TEST_ASSERT_EQUAL_UINT32(240, widthP);

    int32_t heightP = (int32_t)((uint32_t)hdrPortrait.bytes[22] |
                                ((uint32_t)hdrPortrait.bytes[23] << 8) |
                                ((uint32_t)hdrPortrait.bytes[24] << 16) |
                                ((uint32_t)hdrPortrait.bytes[25] << 24));
    TEST_ASSERT_EQUAL_INT32(-320, heightP);
}

void test_screenshot_color_conversion(void) {
    uint8_t r, g, b;

    // Black (0x0000)
    ScreenshotManager::convertRGB565ToBGR24(0x0000, r, g, b);
    TEST_ASSERT_EQUAL_UINT8(0, r);
    TEST_ASSERT_EQUAL_UINT8(0, g);
    TEST_ASSERT_EQUAL_UINT8(0, b);
    TEST_ASSERT_EQUAL_HEX16(0x0000, ScreenshotManager::convertBGR24ToRGB565(r, g, b));

    // White (0xFFFF)
    ScreenshotManager::convertRGB565ToBGR24(0xFFFF, r, g, b);
    TEST_ASSERT_EQUAL_UINT8(255, r);
    TEST_ASSERT_EQUAL_UINT8(255, g);
    TEST_ASSERT_EQUAL_UINT8(255, b);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, ScreenshotManager::convertBGR24ToRGB565(r, g, b));

    // Pure Red (0xF800)
    ScreenshotManager::convertRGB565ToBGR24(0xF800, r, g, b);
    TEST_ASSERT_EQUAL_UINT8(255, r);
    TEST_ASSERT_EQUAL_UINT8(0, g);
    TEST_ASSERT_EQUAL_UINT8(0, b);
    TEST_ASSERT_EQUAL_HEX16(0xF800, ScreenshotManager::convertBGR24ToRGB565(r, g, b));

    // Pure Green (0x07E0)
    ScreenshotManager::convertRGB565ToBGR24(0x07E0, r, g, b);
    TEST_ASSERT_EQUAL_UINT8(0, r);
    TEST_ASSERT_EQUAL_UINT8(255, g);
    TEST_ASSERT_EQUAL_UINT8(0, b);
    TEST_ASSERT_EQUAL_HEX16(0x07E0, ScreenshotManager::convertBGR24ToRGB565(r, g, b));

    // Pure Blue (0x001F)
    ScreenshotManager::convertRGB565ToBGR24(0x001F, r, g, b);
    TEST_ASSERT_EQUAL_UINT8(0, r);
    TEST_ASSERT_EQUAL_UINT8(0, g);
    TEST_ASSERT_EQUAL_UINT8(255, b);
    TEST_ASSERT_EQUAL_HEX16(0x001F, ScreenshotManager::convertBGR24ToRGB565(r, g, b));
}

void test_screenshot_filename_generation(void) {
    // Valid RTC timeinfo
    struct tm t;
    memset(&t, 0, sizeof(t));
    t.tm_year = 2026 - 1900;
    t.tm_mon  = 9;  // October (0 = January)
    t.tm_mday = 24;
    t.tm_hour = 14;
    t.tm_min  = 30;
    t.tm_sec  = 45;

    std::string fn = ScreenshotManager::generateFilename(&t, 99999);
    TEST_ASSERT_EQUAL_STRING("/screenshot_20261024_143045.bmp", fn.c_str());

    // Nullptr timeinfo uses millis fallback
    std::string fnFallback = ScreenshotManager::generateFilename(nullptr, 123456);
    TEST_ASSERT_EQUAL_STRING("/screenshot_123456.bmp", fnFallback.c_str());

    std::string fnZero = ScreenshotManager::generateFilename(nullptr, 0);
    TEST_ASSERT_EQUAL_STRING("/screenshot_0.bmp", fnZero.c_str());
}

void test_screenshot_sd_capture_flow(void) {
    mock_sd_card_present = true;
    mock_sd_card_mounted = false;
    mock_files.clear();

    TEST_ASSERT_FALSE(ScreenshotManager::isCaptureInProgress());

    bool success = ScreenshotManager::captureToSD("/test_capture.bmp");
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_FALSE(ScreenshotManager::isCaptureInProgress());

    // Check file exists in mock files
    TEST_ASSERT_TRUE(mock_files.find("/test_capture.bmp") != mock_files.end());
    const std::string& content = mock_files["/test_capture.bmp"];

    // File size must be exactly 54 header + 320*240*3 pixel bytes
    TEST_ASSERT_EQUAL_UINT32(230454, content.size());

    // BMP magic
    TEST_ASSERT_EQUAL('B', content[0]);
    TEST_ASSERT_EQUAL('M', content[1]);

    // Test failure when SD card is not present
    mock_sd_card_present = false;
    bool failSuccess = ScreenshotManager::captureToSD("/fail.bmp");
    TEST_ASSERT_FALSE(failSuccess);
    TEST_ASSERT_FALSE(ScreenshotManager::isCaptureInProgress());
}

void test_screenshot_handle_command(void) {
    ScreenshotManager::init();

    // Tab switching commands
    ScreenshotManager::handleCommand("tab 2");
    TEST_ASSERT_EQUAL(2, mock_last_tab);

    ScreenshotManager::handleCommand("TAB 0");
    TEST_ASSERT_EQUAL(0, mock_last_tab);

    // Modal commands
    ScreenshotManager::handleCommand("modal");
    TEST_ASSERT_TRUE(mock_modal_opened);

    ScreenshotManager::handleCommand("modal close");
    TEST_ASSERT_TRUE(mock_modal_closed);

    // Tracker view commands
    ScreenshotManager::handleCommand("tracker");
    TEST_ASSERT_TRUE(mock_tracker_opened);

    ScreenshotManager::handleCommand("tracker close");
    TEST_ASSERT_TRUE(mock_tracker_closed);

    // Empty command handling
    ScreenshotManager::handleCommand("   ");
    TEST_ASSERT_FALSE(ScreenshotManager::isCaptureInProgress());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_screenshot_rle16_compress_empty_and_null);
    RUN_TEST(test_screenshot_rle16_compress_single_run);
    RUN_TEST(test_screenshot_rle16_compress_run_exceeding_255);
    RUN_TEST(test_screenshot_rle16_compress_alternating_pixels);
    RUN_TEST(test_screenshot_rle16_compress_buffer_limit);
    RUN_TEST(test_screenshot_rle16_decompress_and_roundtrip);
    RUN_TEST(test_screenshot_bmp_header_calculation_320x240);
    RUN_TEST(test_screenshot_bmp_header_calculation_varied_dimensions);
    RUN_TEST(test_screenshot_color_conversion);
    RUN_TEST(test_screenshot_filename_generation);
    RUN_TEST(test_screenshot_sd_capture_flow);
    RUN_TEST(test_screenshot_handle_command);
    return UNITY_END();
}
