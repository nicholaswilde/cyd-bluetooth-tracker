#include "ui.h"
#include "config/config.h"
#include "catppuccin.h"
#include "backlight_manager.h"
#include "settings_manager.h"
#include "led_manager.h"
#include "audio_manager.h"
#include "display.h"
#include <cstdio>
#include <cstring>
#include <vector>

extern BacklightManager backlight;

static lv_obj_t* s_tabview = nullptr;
static lv_obj_t* s_tab_tracker = nullptr;
static lv_obj_t* s_tab_devices = nullptr;
static lv_obj_t* s_tab_settings = nullptr;

// Tracker Tab Widgets
static lv_obj_t* s_tracker_lbl_title = nullptr;
static lv_obj_t* s_tracker_lbl_details = nullptr;
static lv_obj_t* s_tracker_arc = nullptr;
static lv_obj_t* s_tracker_lbl_rssi = nullptr;
static lv_obj_t* s_tracker_lbl_proximity = nullptr;
static lv_obj_t* s_tracker_lbl_trend = nullptr;
static lv_obj_t* s_tracker_chart = nullptr;
static lv_chart_series_t* s_tracker_ser = nullptr;
static lv_obj_t* s_tracker_btn_audio = nullptr;
static lv_obj_t* s_tracker_lbl_audio = nullptr;
static lv_obj_t* s_tracker_btn_clear = nullptr;
static lv_obj_t* s_tracker_lbl_stats = nullptr;

// Devices Tab Widgets
static lv_obj_t* s_dropdown_dev_sort = nullptr;
static lv_obj_t* s_dropdown_dev_filter = nullptr;
static lv_obj_t* s_list_devices = nullptr;

// Settings Tab Labels
static lv_obj_t* s_lbl_theme = nullptr;
static lv_obj_t* s_lbl_bright = nullptr;
static lv_obj_t* s_lbl_auto_bright = nullptr;
static lv_obj_t* s_lbl_interval = nullptr;
static lv_obj_t* s_lbl_min_rssi = nullptr;
static lv_obj_t* s_lbl_audio = nullptr;
static lv_obj_t* s_lbl_audio_vol = nullptr;
static lv_obj_t* s_lbl_rotation = nullptr;
static lv_obj_t* s_lbl_led = nullptr;
static lv_obj_t* s_lbl_led_bright = nullptr;
static lv_obj_t* s_lbl_reset_text = nullptr;
static lv_obj_t* s_lbl_info = nullptr;

// Settings Tab Controls
static lv_obj_t* s_dropdown_theme = nullptr;
static lv_obj_t* s_slider_brightness = nullptr;
static lv_obj_t* s_switch_auto_bright = nullptr;
static lv_obj_t* s_dropdown_interval = nullptr;
static lv_obj_t* s_slider_min_rssi = nullptr;
static lv_obj_t* s_switch_audio = nullptr;
static lv_obj_t* s_slider_audio_vol = nullptr;
static lv_obj_t* s_dropdown_rotation = nullptr;
static lv_obj_t* s_switch_led = nullptr;
static lv_obj_t* s_slider_led_bright = nullptr;
static lv_obj_t* s_btn_reset = nullptr;

// Detail Modal Widgets
static lv_obj_t* s_detail_modal = nullptr;
static BleDevice s_modal_dev;
static lv_obj_t* s_modal_lbl_name = nullptr;
static lv_obj_t* s_modal_lbl_mac = nullptr;
static lv_obj_t* s_modal_lbl_signal = nullptr;
static lv_obj_t* s_modal_bar_signal = nullptr;
static lv_obj_t* s_modal_lbl_mfg = nullptr;
static lv_obj_t* s_modal_lbl_uuid = nullptr;
static lv_obj_t* s_modal_btn_track = nullptr;
static lv_obj_t* s_modal_btn_pin = nullptr;
static lv_obj_t* s_modal_lbl_pin = nullptr;
static lv_obj_t* s_modal_btn_close = nullptr;

// Toast / status banner
static lv_obj_t* s_toast_msg = nullptr;

static void updateDevicesList(bool force = false);

static void tabview_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        if (getTab() == 1) {
            updateDevicesList(true);
        }
    }
}

static void tracker_audio_btn_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        bool en = !audio.isEnabled();
        audio.setEnabled(en);
        settings.setAudioGeiger(en);
        if (s_tracker_lbl_audio) {
            lv_label_set_text(s_tracker_lbl_audio, en ? LV_SYMBOL_VOLUME_MAX " Mute" : LV_SYMBOL_MUTE " Unmute");
        }
        if (s_switch_audio) {
            if (en) lv_obj_add_state(s_switch_audio, LV_STATE_CHECKED);
            else lv_obj_clear_state(s_switch_audio, LV_STATE_CHECKED);
        }
        if (s_slider_audio_vol) {
            if (en) lv_obj_clear_state(s_slider_audio_vol, LV_STATE_DISABLED);
            else lv_obj_add_state(s_slider_audio_vol, LV_STATE_DISABLED);
        }
    }
}

static void tracker_clear_btn_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        bleTracker.clearTrackedTarget();
        audio.clearTarget();
        led.setState(LedManager::STATE_SCANNING);
        updateUI();
    }
}

static void device_item_click_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        const char* macStr = (const char*)lv_event_get_user_data(e);
        if (macStr) {
            for (const auto& d : bleTracker.getDevices()) {
                if (d.mac == macStr) {
                    openDetailModal(d);
                    break;
                }
            }
        }
    }
}

static void modal_track_btn_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        bleTracker.setTrackedTarget(s_modal_dev.mac);
        audio.setTargetRssi(s_modal_dev.rssi);
        closeDetailModal();
        setTab(0); // Switch to Tracker tab
        updateUI();
    }
}

static void modal_pin_btn_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        std::string currentPinned = settings.getPinnedMac();
        if (currentPinned == s_modal_dev.mac) {
            settings.setPinnedMac("");
            showUIStatusMessage("Unpinned Target");
            if (s_modal_lbl_pin) lv_label_set_text(s_modal_lbl_pin, LV_SYMBOL_SAVE " Pin");
        } else {
            settings.setPinnedMac(s_modal_dev.mac);
            showUIStatusMessage("Pinned Target");
            if (s_modal_lbl_pin) lv_label_set_text(s_modal_lbl_pin, LV_SYMBOL_OK " Unpin");
        }
    }
}

static void modal_close_btn_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        closeDetailModal();
    }
}

void openDetailModal();

void openDetailModal() {
    BleDevice demo;
    demo.mac = "AA:BB:CC:DD:EE:01";
    demo.name = "Demo AirTag";
    demo.rssi = -55;
    demo.txPower = -59;
    demo.manufacturerId = 0x004C;
    demo.serviceUuid = "";
    demo.type = DEVICE_TYPE_APPLE;
    demo.lastSeen = 0;
    demo.isPinned = true;
    openDetailModal(demo);
}

void openTrackerDemo() {
    BleDevice demo;
    demo.mac = "AA:BB:CC:DD:EE:01";
    demo.name = "Demo AirTag";
    demo.rssi = -55;
    demo.txPower = -59;
    demo.type = DEVICE_TYPE_APPLE;
    demo.lastSeen = 0;
    demo.isPinned = true;
    bleTracker.addOrUpdateDevice(demo);
    bleTracker.setTrackedTarget(demo.mac);
    setTab(0);
    updateUI();
}

void closeTrackerView() {
    bleTracker.clearTrackedTarget();
    updateUI();
}

void openDetailModal(const BleDevice& dev) {
    if (s_detail_modal) {
        closeDetailModal();
    }

    s_modal_dev = dev;
    const auto& p = getCatppuccinFlavor(getCurrentThemeFlavor());

    s_detail_modal = lv_obj_create(lv_scr_act());
    lv_obj_set_size(s_detail_modal, 290, 215);
    lv_obj_center(s_detail_modal);
    lv_obj_clear_flag(s_detail_modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_detail_modal, TO_LV_COLOR(p.mantle), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_detail_modal, TO_LV_COLOR(p.surface2), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_detail_modal, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(s_detail_modal, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_detail_modal, 8, LV_PART_MAIN);

    // Title / Name
    s_modal_lbl_name = lv_label_create(s_detail_modal);
    lv_label_set_text(s_modal_lbl_name, dev.name.c_str());
    lv_obj_set_style_text_color(s_modal_lbl_name, TO_LV_COLOR(p.mauve), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_modal_lbl_name, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(s_modal_lbl_name, LV_ALIGN_TOP_LEFT, 0, 0);

    // MAC
    s_modal_lbl_mac = lv_label_create(s_detail_modal);
    lv_label_set_text(s_modal_lbl_mac, dev.mac.c_str());
    lv_obj_set_style_text_color(s_modal_lbl_mac, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_modal_lbl_mac, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(s_modal_lbl_mac, LV_ALIGN_TOP_LEFT, 0, 18);

    // Signal / Distance
    s_modal_lbl_signal = lv_label_create(s_detail_modal);
    char sigBuf[64];
    float dist = dev.getEstimatedDistance();
    if (dist > 0.0f) {
        snprintf(sigBuf, sizeof(sigBuf), "%d dBm  (~%.1fm)", (int)dev.rssi, dist);
    } else {
        snprintf(sigBuf, sizeof(sigBuf), "%d dBm", (int)dev.rssi);
    }
    lv_label_set_text(s_modal_lbl_signal, sigBuf);
    lv_obj_set_style_text_color(s_modal_lbl_signal, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_modal_lbl_signal, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(s_modal_lbl_signal, LV_ALIGN_TOP_LEFT, 0, 36);

    // Signal Bar
    s_modal_bar_signal = lv_bar_create(s_detail_modal);
    lv_obj_set_size(s_modal_bar_signal, 270, 8);
    lv_obj_align(s_modal_bar_signal, LV_ALIGN_TOP_LEFT, 0, 56);
    lv_bar_set_range(s_modal_bar_signal, 0, 100);
    int q = (int)((float)(dev.rssi - RSSI_FLOOR) / (float)(RSSI_CEILING - RSSI_FLOOR) * 100.0f);
    if (q < 0) q = 0; if (q > 100) q = 100;
    lv_bar_set_value(s_modal_bar_signal, q, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_modal_bar_signal, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_modal_bar_signal, getZoneLvColor(dev.rssi, getCurrentThemeFlavor()), LV_PART_INDICATOR);

    // Manufacturer
    s_modal_lbl_mfg = lv_label_create(s_detail_modal);
    std::string mfgText = "Mfg: ";
    if (dev.type == DEVICE_TYPE_APPLE) mfgText += "Apple (AirTag/FindMy)";
    else if (dev.type == DEVICE_TYPE_IBEACON) mfgText += "Apple (iBeacon)";
    else if (dev.type == DEVICE_TYPE_SAMSUNG) mfgText += "Samsung (SmartTag)";
    else if (dev.type == DEVICE_TYPE_TILE) mfgText += "Tile Tag";
    else if (dev.type == DEVICE_TYPE_FAST_PAIR) mfgText += "Google Fast Pair";
    else if (dev.manufacturerId != 0) {
        char mBuf[32]; snprintf(mBuf, sizeof(mBuf), "ID 0x%04X", dev.manufacturerId);
        mfgText += mBuf;
    } else {
        mfgText += "Generic BLE";
    }
    lv_label_set_text(s_modal_lbl_mfg, mfgText.c_str());
    lv_obj_set_style_text_color(s_modal_lbl_mfg, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_modal_lbl_mfg, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(s_modal_lbl_mfg, LV_ALIGN_TOP_LEFT, 0, 72);

    // UUID
    s_modal_lbl_uuid = lv_label_create(s_detail_modal);
    std::string uuidText = "Service: " + (dev.serviceUuid.empty() ? "None" : dev.serviceUuid);
    lv_label_set_text(s_modal_lbl_uuid, uuidText.c_str());
    lv_obj_set_style_text_color(s_modal_lbl_uuid, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_modal_lbl_uuid, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(s_modal_lbl_uuid, LV_ALIGN_TOP_LEFT, 0, 88);

    // Buttons: Track, Pin, Close
    s_modal_btn_track = lv_btn_create(s_detail_modal);
    lv_obj_set_size(s_modal_btn_track, 85, 34);
    lv_obj_align(s_modal_btn_track, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(s_modal_btn_track, TO_LV_COLOR(p.blue), LV_PART_MAIN);
    lv_obj_add_event_cb(s_modal_btn_track, modal_track_btn_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_track = lv_label_create(s_modal_btn_track);
    lv_label_set_text(lbl_track, LV_SYMBOL_GPS " Track");
    lv_obj_set_style_text_font(lbl_track, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_center(lbl_track);

    s_modal_btn_pin = lv_btn_create(s_detail_modal);
    lv_obj_set_size(s_modal_btn_pin, 85, 34);
    lv_obj_align(s_modal_btn_pin, LV_ALIGN_BOTTOM_LEFT, 92, 0);
    lv_obj_set_style_bg_color(s_modal_btn_pin, TO_LV_COLOR(p.surface1), LV_PART_MAIN);
    lv_obj_add_event_cb(s_modal_btn_pin, modal_pin_btn_cb, LV_EVENT_CLICKED, nullptr);
    s_modal_lbl_pin = lv_label_create(s_modal_btn_pin);
    bool isPinned = (settings.getPinnedMac() == dev.mac);
    lv_label_set_text(s_modal_lbl_pin, isPinned ? LV_SYMBOL_OK " Unpin" : LV_SYMBOL_SAVE " Pin");
    lv_obj_set_style_text_font(s_modal_lbl_pin, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_center(s_modal_lbl_pin);

    s_modal_btn_close = lv_btn_create(s_detail_modal);
    lv_obj_set_size(s_modal_btn_close, 85, 34);
    lv_obj_align(s_modal_btn_close, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(s_modal_btn_close, TO_LV_COLOR(p.red), LV_PART_MAIN);
    lv_obj_add_event_cb(s_modal_btn_close, modal_close_btn_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_close = lv_label_create(s_modal_btn_close);
    lv_label_set_text(lbl_close, LV_SYMBOL_CLOSE " Close");
    lv_obj_set_style_text_font(lbl_close, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_center(lbl_close);
}

void closeDetailModal() {
    if (s_detail_modal) {
        lv_obj_del(s_detail_modal);
        s_detail_modal = nullptr;
    }
}

void showUIStatusMessage(const char* message) {
    if (!s_toast_msg) {
        s_toast_msg = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(s_toast_msg, &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_align(s_toast_msg, LV_ALIGN_BOTTOM_MID, 0, -8);
    }
    const auto& p = getCatppuccinFlavor(getCurrentThemeFlavor());
    lv_label_set_text(s_toast_msg, message);
    lv_obj_set_style_text_color(s_toast_msg, TO_LV_COLOR(p.peach), LV_PART_MAIN);
}

static uint16_t scanIntervalToDropdownIdx(uint32_t ms) {
    if (ms <= 1000) return 0;
    if (ms <= 2000) return 1;
    if (ms <= 3000) return 2;
    return 3;
}

static uint32_t dropdownIdxToScanInterval(uint16_t idx) {
    switch (idx) {
        case 0: return 1000;
        case 1: return 2000;
        case 2: return 3000;
        case 3: return 5000;
        default: return 2000;
    }
}

static uint16_t orientationToDropdownIdx(int ori) {
    return (ori == 3) ? 1 : 0;
}

static int dropdownIdxToOrientation(uint16_t idx) {
    return (idx == 1) ? 3 : 1;
}

static void theme_dropdown_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* dropdown = lv_event_get_target(e);
        uint16_t opt = lv_dropdown_get_selected(dropdown);
        int flavor = CATPPUCCIN_MOCHA;
        if (opt == 1) flavor = CATPPUCCIN_MACCHIATO;
        else if (opt == 2) flavor = CATPPUCCIN_FRAPPE;
        else if (opt == 3) flavor = CATPPUCCIN_LATTE;

        settings.setThemeFlavor(flavor);
        setCurrentThemeFlavor(flavor);
        refreshThemeUI();
    }
}

static void brightness_slider_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* slider = lv_event_get_target(e);
        int val = lv_slider_get_value(slider);
        settings.setBrightness(val);
        if (!settings.getAutoBrightness()) {
            backlight.setManualBrightness((uint8_t)val);
        }
    }
}

static void auto_bright_switch_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* sw = lv_event_get_target(e);
        bool en = lv_obj_has_state(sw, LV_STATE_CHECKED);
        settings.setAutoBrightness(en);
        if (s_slider_brightness) {
            if (en) {
                lv_obj_add_state(s_slider_brightness, LV_STATE_DISABLED);
            } else {
                lv_obj_clear_state(s_slider_brightness, LV_STATE_DISABLED);
            }
        }
        if (en) {
#ifndef NATIVE_TEST
            uint16_t ldrVal = analogRead(LDR_PIN);
            backlight.update(ldrVal);
#endif
        } else {
            backlight.setManualBrightness((uint8_t)settings.getBrightness());
        }
    }
}

static void interval_dropdown_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* dropdown = lv_event_get_target(e);
        uint16_t opt = lv_dropdown_get_selected(dropdown);
        uint32_t interval = dropdownIdxToScanInterval(opt);
        settings.setScanIntervalMs(interval);
        bleTracker.setScanInterval(interval);
    }
}

static void min_rssi_slider_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* slider = lv_event_get_target(e);
        int val = lv_slider_get_value(slider);
        settings.setMinRssi(val);
        updateUI();
    }
}

static void audio_switch_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* sw = lv_event_get_target(e);
        bool en = lv_obj_has_state(sw, LV_STATE_CHECKED);
        settings.setAudioGeiger(en);
        audio.setEnabled(en);
        if (s_slider_audio_vol) {
            if (en) {
                lv_obj_clear_state(s_slider_audio_vol, LV_STATE_DISABLED);
            } else {
                lv_obj_add_state(s_slider_audio_vol, LV_STATE_DISABLED);
            }
        }
        if (s_tracker_lbl_audio) {
            lv_label_set_text(s_tracker_lbl_audio, en ? LV_SYMBOL_VOLUME_MAX " Mute" : LV_SYMBOL_MUTE " Unmute");
        }
    }
}

static void audio_vol_slider_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* slider = lv_event_get_target(e);
        int val = lv_slider_get_value(slider);
        settings.setAudioVolume(val);
        audio.setVolume(val);
    }
}

static void led_switch_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* sw = lv_event_get_target(e);
        bool en = lv_obj_has_state(sw, LV_STATE_CHECKED);
        settings.setLedEnabled(en);
        led.setEnabled(en);
        if (s_slider_led_bright) {
            if (en) {
                lv_obj_clear_state(s_slider_led_bright, LV_STATE_DISABLED);
            } else {
                lv_obj_add_state(s_slider_led_bright, LV_STATE_DISABLED);
            }
        }
    }
}

static void led_bright_slider_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* slider = lv_event_get_target(e);
        int val = lv_slider_get_value(slider);
        settings.setLedBrightness(val);
        led.setBrightness(val);
    }
}

static void rotation_dropdown_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t* dropdown = lv_event_get_target(e);
        uint16_t opt = lv_dropdown_get_selected(dropdown);
        int rot = dropdownIdxToOrientation(opt);
        settings.setScreenOrientation(rot);
        tft.setRotation(rot);
        if (lv_scr_act()) {
            lv_obj_invalidate(lv_scr_act());
        }
    }
}

static void reset_btn_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        settings.resetToDefaults();
        setCurrentThemeFlavor(settings.getThemeFlavor());
        backlight.setManualBrightness((uint8_t)settings.getBrightness());
        led.setEnabled(settings.getLedEnabled());
        led.setBrightness(settings.getLedBrightness());
        audio.setEnabled(settings.getAudioGeiger());
        audio.setVolume(settings.getAudioVolume());
        tft.setRotation(settings.getScreenOrientation());
        bleTracker.setScanInterval(settings.getScanIntervalMs());

        if (s_dropdown_theme) {
            lv_dropdown_set_selected(s_dropdown_theme, settings.getThemeFlavor() - 1);
        }
        if (s_slider_brightness) {
            lv_slider_set_value(s_slider_brightness, settings.getBrightness(), LV_ANIM_OFF);
            if (settings.getAutoBrightness()) {
                lv_obj_add_state(s_slider_brightness, LV_STATE_DISABLED);
            } else {
                lv_obj_clear_state(s_slider_brightness, LV_STATE_DISABLED);
            }
        }
        if (s_switch_auto_bright) {
            if (settings.getAutoBrightness()) {
                lv_obj_add_state(s_switch_auto_bright, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(s_switch_auto_bright, LV_STATE_CHECKED);
            }
        }
        if (s_dropdown_interval) {
            lv_dropdown_set_selected(s_dropdown_interval, scanIntervalToDropdownIdx(settings.getScanIntervalMs()));
        }
        if (s_slider_min_rssi) {
            lv_slider_set_value(s_slider_min_rssi, settings.getMinRssi(), LV_ANIM_OFF);
        }
        if (s_switch_audio) {
            if (settings.getAudioGeiger()) {
                lv_obj_add_state(s_switch_audio, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(s_switch_audio, LV_STATE_CHECKED);
            }
        }
        if (s_slider_audio_vol) {
            lv_slider_set_value(s_slider_audio_vol, settings.getAudioVolume(), LV_ANIM_OFF);
            if (settings.getAudioGeiger()) {
                lv_obj_clear_state(s_slider_audio_vol, LV_STATE_DISABLED);
            } else {
                lv_obj_add_state(s_slider_audio_vol, LV_STATE_DISABLED);
            }
        }
        if (s_dropdown_rotation) {
            lv_dropdown_set_selected(s_dropdown_rotation, orientationToDropdownIdx(settings.getScreenOrientation()));
        }
        if (s_switch_led) {
            if (settings.getLedEnabled()) {
                lv_obj_add_state(s_switch_led, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(s_switch_led, LV_STATE_CHECKED);
            }
        }
        if (s_slider_led_bright) {
            lv_slider_set_value(s_slider_led_bright, settings.getLedBrightness(), LV_ANIM_OFF);
            if (settings.getLedEnabled()) {
                lv_obj_clear_state(s_slider_led_bright, LV_STATE_DISABLED);
            } else {
                lv_obj_add_state(s_slider_led_bright, LV_STATE_DISABLED);
            }
        }

        refreshThemeUI();
        showUIStatusMessage("Defaults Restored!");
    }
}

static void styleSettingsControls(const Catppuccin::Palette& p, int flavor) {
    lv_color_t track_bg = (flavor == CATPPUCCIN_LATTE) ? TO_LV_COLOR(p.surface1) : TO_LV_COLOR(p.surface0);
    lv_color_t track_border = TO_LV_COLOR(p.surface2);
    lv_color_t active_bg = TO_LV_COLOR(p.blue);
    lv_color_t knob_bg = (flavor == CATPPUCCIN_LATTE) ? TO_LV_COLOR(p.base) : TO_LV_COLOR(p.text);

    lv_obj_t* dropdowns[] = {s_dropdown_theme, s_dropdown_interval, s_dropdown_rotation, s_dropdown_dev_sort, s_dropdown_dev_filter};
    for (lv_obj_t* d : dropdowns) {
        if (d) {
            lv_obj_set_style_bg_color(d, TO_LV_COLOR(p.mantle), LV_PART_MAIN);
            lv_obj_set_style_border_color(d, TO_LV_COLOR(p.surface1), LV_PART_MAIN);
            lv_obj_set_style_border_width(d, 1, LV_PART_MAIN);
            lv_obj_set_style_text_color(d, TO_LV_COLOR(p.text), LV_PART_MAIN);
        }
    }

    lv_obj_t* sliders[] = {s_slider_brightness, s_slider_min_rssi, s_slider_audio_vol, s_slider_led_bright};
    for (lv_obj_t* sl : sliders) {
        if (sl) {
            lv_obj_set_style_bg_color(sl, track_bg, LV_PART_MAIN);
            lv_obj_set_style_border_color(sl, track_border, LV_PART_MAIN);
            lv_obj_set_style_border_width(sl, 1, LV_PART_MAIN);
            lv_obj_set_style_bg_color(sl, active_bg, LV_PART_INDICATOR);
            lv_obj_set_style_bg_color(sl, active_bg, LV_PART_KNOB);
        }
    }

    lv_obj_t* switches[] = {s_switch_auto_bright, s_switch_audio, s_switch_led};
    for (lv_obj_t* sw : switches) {
        if (sw) {
            lv_obj_set_style_bg_color(sw, track_bg, LV_PART_MAIN);
            lv_obj_set_style_border_color(sw, track_border, LV_PART_MAIN);
            lv_obj_set_style_border_width(sw, 1, LV_PART_MAIN);
            lv_obj_set_style_bg_color(sw, active_bg, LV_PART_INDICATOR | LV_STATE_CHECKED);
            lv_obj_set_style_bg_color(sw, knob_bg, LV_PART_KNOB);
        }
    }

    lv_obj_t* buttons[] = {s_btn_reset};
    for (lv_obj_t* b : buttons) {
        if (b) {
            lv_obj_set_style_bg_color(b, track_bg, LV_PART_MAIN);
            lv_obj_set_style_border_color(b, track_border, LV_PART_MAIN);
            lv_obj_set_style_border_width(b, 1, LV_PART_MAIN);
        }
    }
}

void initUI() {
    const auto& p = getCatppuccinFlavor(getCurrentThemeFlavor());

    lv_obj_set_style_bg_color(lv_scr_act(), TO_LV_COLOR(p.base), LV_PART_MAIN);

    s_tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 34);
    lv_obj_set_style_bg_color(s_tabview, TO_LV_COLOR(p.base), LV_PART_MAIN);

    int flavor = getCurrentThemeFlavor();

    // Tab Header buttons
    lv_obj_t* tab_btns = lv_tabview_get_tab_btns(s_tabview);
    lv_obj_set_style_bg_color(tab_btns, TO_LV_COLOR(p.mantle), LV_PART_MAIN);
    lv_obj_set_style_text_color(tab_btns, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_set_style_bg_color(tab_btns, (flavor == CATPPUCCIN_LATTE ? TO_LV_COLOR(p.surface1) : TO_LV_COLOR(p.surface0)), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(tab_btns, TO_LV_COLOR(p.mauve), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(tab_btns, TO_LV_COLOR(p.blue), LV_PART_INDICATOR);

    s_tab_tracker = lv_tabview_add_tab(s_tabview, "Tracker");
    s_tab_devices = lv_tabview_add_tab(s_tabview, "Devices");
    s_tab_settings = lv_tabview_add_tab(s_tabview, "Settings");
    lv_obj_add_event_cb(s_tabview, tabview_event_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    // ==========================================
    // 1. Tracker Tab
    // ==========================================
    lv_obj_clear_flag(s_tab_tracker, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_tab_tracker, 6, LV_PART_MAIN);

    s_tracker_lbl_title = lv_label_create(s_tab_tracker);
    lv_label_set_text(s_tracker_lbl_title, "No Target Selected");
    lv_obj_set_style_text_color(s_tracker_lbl_title, TO_LV_COLOR(p.mauve), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_tracker_lbl_title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(s_tracker_lbl_title, LV_ALIGN_TOP_LEFT, 4, 2);

    s_tracker_lbl_details = lv_label_create(s_tab_tracker);
    lv_label_set_text(s_tracker_lbl_details, "Select a device in Devices tab");
    lv_obj_set_style_text_color(s_tracker_lbl_details, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_tracker_lbl_details, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(s_tracker_lbl_details, LV_ALIGN_TOP_LEFT, 4, 18);

    // Arc Gauge
    s_tracker_arc = lv_arc_create(s_tab_tracker);
    lv_obj_set_size(s_tracker_arc, 125, 125);
    lv_obj_align(s_tracker_arc, LV_ALIGN_TOP_LEFT, 6, 36);
    lv_arc_set_rotation(s_tracker_arc, 135);
    lv_arc_set_bg_angles(s_tracker_arc, 0, 270);
    lv_arc_set_range(s_tracker_arc, 0, 100);
    lv_arc_set_value(s_tracker_arc, 0);
    lv_obj_clear_flag(s_tracker_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_style(s_tracker_arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(s_tracker_arc, 9, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_tracker_arc, 9, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_tracker_arc, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_tracker_arc, TO_LV_COLOR(p.green), LV_PART_INDICATOR);

    // Inner Arc Readouts
    s_tracker_lbl_rssi = lv_label_create(s_tab_tracker);
    lv_label_set_text(s_tracker_lbl_rssi, "-- dBm");
    lv_obj_set_style_text_font(s_tracker_lbl_rssi, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_tracker_lbl_rssi, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align_to(s_tracker_lbl_rssi, s_tracker_arc, LV_ALIGN_CENTER, -6, -8);

    s_tracker_lbl_proximity = lv_label_create(s_tab_tracker);
    lv_label_set_text(s_tracker_lbl_proximity, "STANDBY");
    lv_obj_set_style_text_font(s_tracker_lbl_proximity, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_tracker_lbl_proximity, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_align_to(s_tracker_lbl_proximity, s_tracker_arc, LV_ALIGN_CENTER, -6, 14);

    // Sparkline Chart (Right side)
    s_tracker_chart = lv_chart_create(s_tab_tracker);
    lv_obj_set_size(s_tracker_chart, 150, 85);
    lv_obj_align(s_tracker_chart, LV_ALIGN_TOP_RIGHT, -4, 36);
    lv_chart_set_type(s_tracker_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_tracker_chart, 8);
    lv_chart_set_range(s_tracker_chart, LV_CHART_AXIS_PRIMARY_Y, -100, -30);
    lv_obj_set_style_bg_color(s_tracker_chart, TO_LV_COLOR(p.mantle), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_tracker_chart, TO_LV_COLOR(p.surface1), LV_PART_MAIN);
    s_tracker_ser = lv_chart_add_series(s_tracker_chart, TO_LV_COLOR(p.blue), LV_CHART_AXIS_PRIMARY_Y);
    for (int i = 0; i < 8; ++i) s_tracker_ser->y_points[i] = -100;

    // Trend Indicator
    s_tracker_lbl_trend = lv_label_create(s_tab_tracker);
    lv_label_set_text(s_tracker_lbl_trend, LV_SYMBOL_MINUS " STABLE");
    lv_obj_set_style_text_font(s_tracker_lbl_trend, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_tracker_lbl_trend, TO_LV_COLOR(p.yellow), LV_PART_MAIN);
    lv_obj_align(s_tracker_lbl_trend, LV_ALIGN_TOP_RIGHT, -30, 126);

    // Action buttons at bottom: Mute / Clear
    s_tracker_btn_audio = lv_btn_create(s_tab_tracker);
    lv_obj_set_size(s_tracker_btn_audio, 80, 30);
    lv_obj_align(s_tracker_btn_audio, LV_ALIGN_BOTTOM_LEFT, 6, 0);
    lv_obj_set_style_bg_color(s_tracker_btn_audio, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
    lv_obj_add_event_cb(s_tracker_btn_audio, tracker_audio_btn_event_cb, LV_EVENT_CLICKED, nullptr);
    s_tracker_lbl_audio = lv_label_create(s_tracker_btn_audio);
    lv_label_set_text(s_tracker_lbl_audio, audio.isEnabled() ? LV_SYMBOL_VOLUME_MAX " Mute" : LV_SYMBOL_MUTE " Unmute");
    lv_obj_set_style_text_font(s_tracker_lbl_audio, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_center(s_tracker_lbl_audio);

    s_tracker_btn_clear = lv_btn_create(s_tab_tracker);
    lv_obj_set_size(s_tracker_btn_clear, 70, 30);
    lv_obj_align(s_tracker_btn_clear, LV_ALIGN_BOTTOM_LEFT, 92, 0);
    lv_obj_set_style_bg_color(s_tracker_btn_clear, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
    lv_obj_add_event_cb(s_tracker_btn_clear, tracker_clear_btn_event_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl_clear = lv_label_create(s_tracker_btn_clear);
    lv_label_set_text(lbl_clear, LV_SYMBOL_TRASH " Clear");
    lv_obj_set_style_text_font(lbl_clear, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_center(lbl_clear);

    s_tracker_lbl_stats = lv_label_create(s_tab_tracker);
    lv_label_set_text(s_tracker_lbl_stats, "Devices: 0");
    lv_obj_set_style_text_font(s_tracker_lbl_stats, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_tracker_lbl_stats, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_align(s_tracker_lbl_stats, LV_ALIGN_BOTTOM_RIGHT, -8, -6);

    // ==========================================
    // 2. Devices Tab
    // ==========================================
    s_list_devices = lv_list_create(s_tab_devices);
    lv_obj_set_size(s_list_devices, LV_PCT(100), LV_PCT(100));
    lv_obj_center(s_list_devices);
    lv_obj_set_style_bg_color(s_list_devices, TO_LV_COLOR(p.base), LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_list_devices, 4, LV_PART_MAIN);

    // ==========================================
    // 3. Settings Tab
    // ==========================================
    s_lbl_theme = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_theme, "Theme Flavor:");
    lv_obj_set_style_text_color(s_lbl_theme, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_theme, LV_ALIGN_TOP_LEFT, 10, 12);

    s_dropdown_theme = lv_dropdown_create(s_tab_settings);
    lv_dropdown_set_options(s_dropdown_theme, "Mocha\nMacchiato\nFrappe\nLatte");
    lv_dropdown_set_selected(s_dropdown_theme, settings.getThemeFlavor() - 1);
    lv_obj_set_size(s_dropdown_theme, 142, LV_SIZE_CONTENT);
    lv_obj_align(s_dropdown_theme, LV_ALIGN_TOP_LEFT, 132, 6);
    lv_obj_add_event_cb(s_dropdown_theme, theme_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    s_lbl_bright = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_bright, "Brightness:");
    lv_obj_set_style_text_color(s_lbl_bright, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_bright, LV_ALIGN_TOP_LEFT, 10, 52);

    s_slider_brightness = lv_slider_create(s_tab_settings);
    lv_slider_set_range(s_slider_brightness, 10, 100);
    lv_slider_set_value(s_slider_brightness, settings.getBrightness(), LV_ANIM_OFF);
    lv_obj_set_size(s_slider_brightness, 142, 12);
    lv_obj_align(s_slider_brightness, LV_ALIGN_TOP_LEFT, 132, 54);
    lv_obj_add_event_cb(s_slider_brightness, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    if (settings.getAutoBrightness()) {
        lv_obj_add_state(s_slider_brightness, LV_STATE_DISABLED);
    }

    s_lbl_auto_bright = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_auto_bright, "Auto Bright:");
    lv_obj_set_style_text_color(s_lbl_auto_bright, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_auto_bright, LV_ALIGN_TOP_LEFT, 10, 84);

    s_switch_auto_bright = lv_switch_create(s_tab_settings);
    if (settings.getAutoBrightness()) {
        lv_obj_add_state(s_switch_auto_bright, LV_STATE_CHECKED);
    }
    lv_obj_align(s_switch_auto_bright, LV_ALIGN_TOP_LEFT, 132, 80);
    lv_obj_add_event_cb(s_switch_auto_bright, auto_bright_switch_event_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    s_lbl_interval = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_interval, "Scan Interval:");
    lv_obj_set_style_text_color(s_lbl_interval, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_interval, LV_ALIGN_TOP_LEFT, 10, 126);

    s_dropdown_interval = lv_dropdown_create(s_tab_settings);
    lv_dropdown_set_options(s_dropdown_interval, "1s\n2s\n3s\n5s");
    lv_dropdown_set_selected(s_dropdown_interval, scanIntervalToDropdownIdx(settings.getScanIntervalMs()));
    lv_obj_set_size(s_dropdown_interval, 142, LV_SIZE_CONTENT);
    lv_obj_align(s_dropdown_interval, LV_ALIGN_TOP_LEFT, 132, 120);
    lv_obj_add_event_cb(s_dropdown_interval, interval_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    s_lbl_min_rssi = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_min_rssi, "Min RSSI:");
    lv_obj_set_style_text_color(s_lbl_min_rssi, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_min_rssi, LV_ALIGN_TOP_LEFT, 10, 168);

    s_slider_min_rssi = lv_slider_create(s_tab_settings);
    lv_slider_set_range(s_slider_min_rssi, -95, -40);
    lv_slider_set_value(s_slider_min_rssi, settings.getMinRssi(), LV_ANIM_OFF);
    lv_obj_set_size(s_slider_min_rssi, 142, 12);
    lv_obj_align(s_slider_min_rssi, LV_ALIGN_TOP_LEFT, 132, 170);
    lv_obj_add_event_cb(s_slider_min_rssi, min_rssi_slider_event_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    s_lbl_audio = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_audio, "Audio Geiger:");
    lv_obj_set_style_text_color(s_lbl_audio, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_audio, LV_ALIGN_TOP_LEFT, 10, 208);

    s_switch_audio = lv_switch_create(s_tab_settings);
    if (settings.getAudioGeiger()) {
        lv_obj_add_state(s_switch_audio, LV_STATE_CHECKED);
    }
    lv_obj_align(s_switch_audio, LV_ALIGN_TOP_LEFT, 132, 204);
    lv_obj_add_event_cb(s_switch_audio, audio_switch_event_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    s_lbl_audio_vol = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_audio_vol, "Audio Volume:");
    lv_obj_set_style_text_color(s_lbl_audio_vol, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_audio_vol, LV_ALIGN_TOP_LEFT, 10, 248);

    s_slider_audio_vol = lv_slider_create(s_tab_settings);
    lv_slider_set_range(s_slider_audio_vol, 0, 100);
    lv_slider_set_value(s_slider_audio_vol, settings.getAudioVolume(), LV_ANIM_OFF);
    lv_obj_set_size(s_slider_audio_vol, 142, 12);
    lv_obj_align(s_slider_audio_vol, LV_ALIGN_TOP_LEFT, 132, 250);
    lv_obj_add_event_cb(s_slider_audio_vol, audio_vol_slider_event_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    if (!settings.getAudioGeiger()) {
        lv_obj_add_state(s_slider_audio_vol, LV_STATE_DISABLED);
    }

    s_lbl_rotation = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_rotation, "Orientation:");
    lv_obj_set_style_text_color(s_lbl_rotation, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_rotation, LV_ALIGN_TOP_LEFT, 10, 290);

    s_dropdown_rotation = lv_dropdown_create(s_tab_settings);
    lv_dropdown_set_options(s_dropdown_rotation, "Landscape (0\xC2\xB0)\nLandscape (180\xC2\xB0)");
    lv_dropdown_set_selected(s_dropdown_rotation, orientationToDropdownIdx(settings.getScreenOrientation()));
    lv_obj_set_size(s_dropdown_rotation, 142, LV_SIZE_CONTENT);
    lv_obj_align(s_dropdown_rotation, LV_ALIGN_TOP_LEFT, 132, 284);
    lv_obj_add_event_cb(s_dropdown_rotation, rotation_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    s_lbl_led = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_led, "Status LED:");
    lv_obj_set_style_text_color(s_lbl_led, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_led, LV_ALIGN_TOP_LEFT, 10, 332);

    s_switch_led = lv_switch_create(s_tab_settings);
    if (settings.getLedEnabled()) {
        lv_obj_add_state(s_switch_led, LV_STATE_CHECKED);
    }
    lv_obj_align(s_switch_led, LV_ALIGN_TOP_LEFT, 132, 328);
    lv_obj_add_event_cb(s_switch_led, led_switch_event_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    s_lbl_led_bright = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_led_bright, "LED Bright:");
    lv_obj_set_style_text_color(s_lbl_led_bright, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_led_bright, LV_ALIGN_TOP_LEFT, 10, 372);

    s_slider_led_bright = lv_slider_create(s_tab_settings);
    lv_slider_set_range(s_slider_led_bright, 10, 255);
    lv_slider_set_value(s_slider_led_bright, settings.getLedBrightness(), LV_ANIM_OFF);
    lv_obj_set_size(s_slider_led_bright, 142, 12);
    lv_obj_align(s_slider_led_bright, LV_ALIGN_TOP_LEFT, 132, 374);
    lv_obj_add_event_cb(s_slider_led_bright, led_bright_slider_event_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    if (!settings.getLedEnabled()) {
        lv_obj_add_state(s_slider_led_bright, LV_STATE_DISABLED);
    }

    s_btn_reset = lv_btn_create(s_tab_settings);
    lv_obj_set_size(s_btn_reset, 265, 34);
    lv_obj_align(s_btn_reset, LV_ALIGN_TOP_LEFT, 10, 416);
    lv_obj_add_event_cb(s_btn_reset, reset_btn_event_cb, LV_EVENT_CLICKED, nullptr);

    s_lbl_reset_text = lv_label_create(s_btn_reset);
    lv_label_set_text(s_lbl_reset_text, "Reset Settings to Default");
    lv_obj_set_style_text_color(s_lbl_reset_text, TO_LV_COLOR(p.red), LV_PART_MAIN);
    lv_obj_center(s_lbl_reset_text);

    s_lbl_info = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_info, "ESP32 CYD Bluetooth Tracker\nEngine: LVGL v8 + Catppuccin RGB565");
    lv_obj_set_style_text_color(s_lbl_info, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_align(s_lbl_info, LV_ALIGN_TOP_LEFT, 10, 460);

    styleSettingsControls(p, flavor);
}

static std::vector<std::string> s_device_list_macs;

static void updateDevicesList(bool force) {
    if (!s_list_devices) return;

    // Do not rebuild list if not currently on the Devices tab unless forced
    if (getTab() != 1 && !force) {
        return;
    }

    // Never interrupt user scrolling
    if (lv_obj_is_scrolling(s_list_devices)) {
        return;
    }

    static unsigned long s_lastDeviceListUpdate = 0;
    static uint32_t s_lastRenderedCatalogVersion = 0;
    static int s_lastSortMode = -1;
    static int s_lastFilterMode = -1;
    static int s_lastMinRssi = -999;
    unsigned long now = millis();

    bool versionChanged = (bleTracker.getCatalogVersion() != s_lastRenderedCatalogVersion);
    bool settingsChanged = (settings.getDeviceSortMode() != s_lastSortMode ||
                            settings.getDeviceFilterMode() != s_lastFilterMode ||
                            settings.getMinRssi() != s_lastMinRssi);
    bool timeElapsed = (now - s_lastDeviceListUpdate >= 2000);

    if (!force && !versionChanged && !settingsChanged && !timeElapsed) {
        return;
    }

    // Preserve scroll position across redraws
    lv_coord_t scroll_y = lv_obj_get_scroll_y(s_list_devices);

    lv_obj_clean(s_list_devices);

    s_lastDeviceListUpdate = now;
    s_lastRenderedCatalogVersion = bleTracker.getCatalogVersion();
    s_lastSortMode = settings.getDeviceSortMode();
    s_lastFilterMode = settings.getDeviceFilterMode();
    s_lastMinRssi = settings.getMinRssi();

    const auto& p = getCatppuccinFlavor(getCurrentThemeFlavor());
    auto filtered = bleTracker.getFilteredDevices(s_lastSortMode, s_lastFilterMode, s_lastMinRssi);

    s_device_list_macs.clear();

    if (filtered.empty()) {
        lv_obj_t* btn = lv_list_add_btn(s_list_devices, LV_SYMBOL_WARNING, "No devices match filter");
        lv_obj_set_style_bg_color(btn, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
        lv_obj_set_style_text_color(btn, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, TO_LV_COLOR(p.surface1), LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    } else {
        s_device_list_macs.reserve(filtered.size());
        for (const auto& d : filtered) {
            s_device_list_macs.push_back(d.mac);

            char btnText[64];
            snprintf(btnText, sizeof(btnText), "%s\n%s (%d dBm)",
                     d.name.c_str(), d.mac.c_str(), (int)d.rssi);

            const char* icon = LV_SYMBOL_BLUETOOTH;
            if (d.type == DEVICE_TYPE_APPLE) icon = LV_SYMBOL_EYE_OPEN;
            else if (d.isPinned) icon = LV_SYMBOL_SAVE;

            lv_obj_t* btn = lv_list_add_btn(s_list_devices, icon, btnText);
            lv_obj_set_style_bg_color(btn, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
            lv_obj_set_style_text_color(btn, TO_LV_COLOR(p.text), LV_PART_MAIN);
            lv_obj_set_style_border_color(btn, TO_LV_COLOR(p.surface1), LV_PART_MAIN);
            lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
            lv_obj_add_event_cb(btn, device_item_click_cb, LV_EVENT_CLICKED, (void*)s_device_list_macs.back().c_str());
        }
    }

    lv_obj_scroll_to_y(s_list_devices, scroll_y, LV_ANIM_OFF);
}

void updateUI() {
    const auto& p = getCatppuccinFlavor(getCurrentThemeFlavor());

    // Update Tracker Tab
    const BleDevice* target = bleTracker.getTrackedDevice();
    if (target) {
        lv_label_set_text(s_tracker_lbl_title, target->name.c_str());
        char detBuf[64];
        float dist = target->getEstimatedDistance();
        if (dist > 0.0f) {
            snprintf(detBuf, sizeof(detBuf), "%s | ~%.1fm", target->mac.c_str(), dist);
        } else {
            snprintf(detBuf, sizeof(detBuf), "%s", target->mac.c_str());
        }
        lv_label_set_text(s_tracker_lbl_details, detBuf);

        int8_t rssi = target->rssi;
        int q = (int)((float)(rssi - RSSI_FLOOR) / (float)(RSSI_CEILING - RSSI_FLOOR) * 100.0f);
        if (q < 0) q = 0; if (q > 100) q = 100;

        lv_arc_set_value(s_tracker_arc, q);
        lv_color_t zoneColor = getZoneLvColor(rssi, getCurrentThemeFlavor());
        lv_obj_set_style_arc_color(s_tracker_arc, zoneColor, LV_PART_INDICATOR);

        char rBuf[32];
        snprintf(rBuf, sizeof(rBuf), "%d dBm", (int)rssi);
        lv_label_set_text(s_tracker_lbl_rssi, rBuf);
        lv_obj_align_to(s_tracker_lbl_rssi, s_tracker_arc, LV_ALIGN_CENTER, -6, -8);

        if (rssi >= RSSI_ZONE_IMMEDIATE) {
            lv_label_set_text(s_tracker_lbl_proximity, "IMMEDIATE");
            led.setState(LedManager::STATE_CLEAN);
        } else if (rssi >= RSSI_ZONE_NEAR) {
            lv_label_set_text(s_tracker_lbl_proximity, "NEAR");
            led.setState(LedManager::STATE_CONGESTED_YELLOW);
        } else if (rssi >= RSSI_ZONE_MID) {
            lv_label_set_text(s_tracker_lbl_proximity, "MID-RANGE");
            led.setState(LedManager::STATE_PULSE_YELLOW);
        } else {
            lv_label_set_text(s_tracker_lbl_proximity, "FAR / WEAK");
            led.setState(LedManager::STATE_CONGESTED_RED);
        }
        lv_obj_align_to(s_tracker_lbl_proximity, s_tracker_arc, LV_ALIGN_CENTER, -6, 14);

        // Update history chart
        int8_t hist[8];
        bleTracker.getRssiHistory(hist, 8);
        for (int i = 0; i < 8; ++i) {
            s_tracker_ser->y_points[i] = (lv_coord_t)hist[i];
        }
        lv_chart_refresh(s_tracker_chart);

        // Update proximity trend indicator
        switch (bleTracker.getTrend()) {
            case TREND_CLOSING:
                lv_label_set_text(s_tracker_lbl_trend, LV_SYMBOL_UP " CLOSING IN");
                lv_obj_set_style_text_color(s_tracker_lbl_trend, TO_LV_COLOR(p.green), LV_PART_MAIN);
                break;
            case TREND_DRIFTING:
                lv_label_set_text(s_tracker_lbl_trend, LV_SYMBOL_DOWN " DRIFTING");
                lv_obj_set_style_text_color(s_tracker_lbl_trend, TO_LV_COLOR(p.red), LV_PART_MAIN);
                break;
            default:
                lv_label_set_text(s_tracker_lbl_trend, LV_SYMBOL_MINUS " STABLE");
                lv_obj_set_style_text_color(s_tracker_lbl_trend, TO_LV_COLOR(p.yellow), LV_PART_MAIN);
                break;
        }

        // Update audio beeper
        audio.setTargetRssi(rssi);
    } else {
        lv_label_set_text(s_tracker_lbl_title, "No Target Selected");
        lv_label_set_text(s_tracker_lbl_details, "Select a device in Devices tab");
        lv_arc_set_value(s_tracker_arc, 0);
        lv_label_set_text(s_tracker_lbl_rssi, "-- dBm");
        lv_obj_align_to(s_tracker_lbl_rssi, s_tracker_arc, LV_ALIGN_CENTER, -6, -8);
        lv_label_set_text(s_tracker_lbl_proximity, "STANDBY");
        lv_obj_align_to(s_tracker_lbl_proximity, s_tracker_arc, LV_ALIGN_CENTER, -6, 14);
        lv_label_set_text(s_tracker_lbl_trend, LV_SYMBOL_MINUS " STABLE");
        lv_obj_set_style_text_color(s_tracker_lbl_trend, TO_LV_COLOR(p.yellow), LV_PART_MAIN);
        audio.clearTarget();
    }

    // Update Stats Label
    char statBuf[48];
    snprintf(statBuf, sizeof(statBuf), "Discovered: %u", (unsigned)bleTracker.getDiscoveredCount());
    if (s_tracker_lbl_stats) {
        lv_label_set_text(s_tracker_lbl_stats, statBuf);
    }

    // Update Devices List
    updateDevicesList(false);
}

void refreshThemeUI() {
    int flavor = getCurrentThemeFlavor();
    const auto& p = getCatppuccinFlavor(flavor);
    lv_obj_set_style_bg_color(lv_scr_act(), TO_LV_COLOR(p.base), LV_PART_MAIN);

    if (s_tabview) {
        lv_obj_set_style_bg_color(s_tabview, TO_LV_COLOR(p.base), LV_PART_MAIN);
        lv_obj_t* tab_btns = lv_tabview_get_tab_btns(s_tabview);
        lv_obj_set_style_bg_color(tab_btns, TO_LV_COLOR(p.mantle), LV_PART_MAIN);
        lv_obj_set_style_text_color(tab_btns, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
        lv_obj_set_style_bg_color(tab_btns, (flavor == CATPPUCCIN_LATTE ? TO_LV_COLOR(p.surface1) : TO_LV_COLOR(p.surface0)), LV_PART_ITEMS | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(tab_btns, TO_LV_COLOR(p.mauve), LV_PART_ITEMS | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(tab_btns, TO_LV_COLOR(p.blue), LV_PART_INDICATOR);
    }

    if (s_lbl_theme) lv_obj_set_style_text_color(s_lbl_theme, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_bright) lv_obj_set_style_text_color(s_lbl_bright, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_auto_bright) lv_obj_set_style_text_color(s_lbl_auto_bright, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_interval) lv_obj_set_style_text_color(s_lbl_interval, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_min_rssi) lv_obj_set_style_text_color(s_lbl_min_rssi, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_audio) lv_obj_set_style_text_color(s_lbl_audio, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_audio_vol) lv_obj_set_style_text_color(s_lbl_audio_vol, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_rotation) lv_obj_set_style_text_color(s_lbl_rotation, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_led) lv_obj_set_style_text_color(s_lbl_led, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_led_bright) lv_obj_set_style_text_color(s_lbl_led_bright, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_reset_text) lv_obj_set_style_text_color(s_lbl_reset_text, TO_LV_COLOR(p.red), LV_PART_MAIN);
    if (s_lbl_info) lv_obj_set_style_text_color(s_lbl_info, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);

    styleSettingsControls(p, flavor);

    updateUI();
}

void setTab(uint16_t index) {
    if (s_tabview && index < 3) {
        lv_tabview_set_act(s_tabview, index, LV_ANIM_OFF);
        if (index == 1) {
            updateDevicesList(true);
        }
    }
}

uint16_t getTab() {
    if (s_tabview) {
        return lv_tabview_get_tab_act(s_tabview);
    }
    return 0;
}
