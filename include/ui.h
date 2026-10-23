#ifndef UI_H
#define UI_H

#include <lvgl.h>
#include "bluetooth_tracker.h"

void initUI();
void updateUI();
void refreshThemeUI();
void setTab(uint16_t index);
uint16_t getTab();

void openDetailModal();
void openDetailModal(const BleDevice& dev);
void closeDetailModal();
void openTrackerDemo();
void closeTrackerView();
void showUIStatusMessage(const char* message);

#endif // UI_H
