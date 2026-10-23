#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <stdint.h>
#include "config/config.h"

class AudioManager {
public:
    AudioManager(uint8_t pin = SPEAKER_PIN, uint8_t channel = 1);

    void begin();
    void update(unsigned long currentMillis);

    void playTone(uint16_t freqHz, uint16_t durationMs);
    void playBeep(uint16_t freqHz = 2000, uint16_t durationMs = 30);

    void setTargetRssi(int8_t rssi);
    void clearTarget();

    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }

    void setVolume(uint8_t volumePercent);
    uint8_t getVolume() const { return _volume; }

    // Test inspection
    bool isBeeping() const { return _isTonePlaying; }
    uint32_t getBeepIntervalMs() const { return _currentIntervalMs; }

private:
    void stopTone();

    uint8_t _pin;
    uint8_t _channel;
    bool _enabled;
    uint8_t _volume;

    int8_t _targetRssi;
    unsigned long _lastBeepTime;
    unsigned long _toneStartTime;
    uint16_t _toneDuration;
    bool _isTonePlaying;
    uint32_t _currentIntervalMs;
};

extern AudioManager audio;

#endif // AUDIO_MANAGER_H
