#include "catppuccin.h"
#include "settings_manager.h"
#include "config/config.h"

const Catppuccin::Palette& getCatppuccinFlavor(int flavor) {
    switch (flavor) {
        case CATPPUCCIN_MACCHIATO: return Catppuccin::getPalette(Catppuccin::Flavor::Macchiato);
        case CATPPUCCIN_FRAPPE:    return Catppuccin::getPalette(Catppuccin::Flavor::Frappe);
        case CATPPUCCIN_LATTE:     return Catppuccin::getPalette(Catppuccin::Flavor::Latte);
        case CATPPUCCIN_MOCHA:
        default:                   return Catppuccin::getPalette(Catppuccin::Flavor::Mocha);
    }
}

int getCurrentThemeFlavor() {
    return settings.getThemeFlavor();
}

void setCurrentThemeFlavor(int flavor) {
    settings.setThemeFlavor(flavor);
}

uint16_t getZoneColor(int8_t rssi, int flavor) {
    const auto& p = getCatppuccinFlavor(flavor);
    if (rssi >= RSSI_ZONE_IMMEDIATE) {
        return p.green;
    } else if (rssi >= RSSI_ZONE_NEAR) {
        return p.teal;
    } else if (rssi >= RSSI_ZONE_MID) {
        return p.yellow;
    } else {
        return p.blue;
    }
}

lv_color_t getZoneLvColor(int8_t rssi, int flavor) {
    uint16_t rgb565 = getZoneColor(rssi, flavor);
    lv_color_t color;
    color.full = rgb565;
    return color;
}
