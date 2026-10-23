#include "audio_manager.h"

#ifndef NATIVE_TEST
#include <Arduino.h>
#else
#include "Arduino.h"
#endif

AudioManager audio(SPEAKER_PIN, 1);

AudioManager::AudioManager(uint8_t pin, uint8_t channel)
    : _pin(pin),
      _channel(channel),
      _enabled(DEFAULT_AUDIO_GEIGER),
      _volume(DEFAULT_AUDIO_VOLUME),
      _targetRssi(RSSI_FLOOR),
      _lastBeepTime(0),
      _toneStartTime(0),
      _toneDuration(0),
      _isTonePlaying(false),
      _currentIntervalMs(0) {}

void AudioManager::begin() {
#ifndef NATIVE_TEST
    #if defined(ESP_ARDUINO_VERSION) && ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcAttach(_pin, 2000, 8);
    #else
        ledcSetup(_channel, 2000, 8);
        ledcAttachPin(_pin, _channel);
    #endif
#else
    pinMode(_pin, OUTPUT);
#endif
    stopTone();
}

void AudioManager::playTone(uint16_t freqHz, uint16_t durationMs) {
    if (!_enabled || _volume == 0 || freqHz == 0) return;

    _toneDuration = durationMs;
    _toneStartTime = millis();
    _isTonePlaying = true;

#ifndef NATIVE_TEST
    uint32_t duty = (_volume * 128) / 100;
    #if defined(ESP_ARDUINO_VERSION) && ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcWriteTone(_pin, freqHz);
        ledcWrite(_pin, duty);
    #else
        ledcWriteTone(_channel, freqHz);
        ledcWrite(_channel, duty);
    #endif
#endif
}

void AudioManager::playBeep(uint16_t freqHz, uint16_t durationMs) {
    playTone(freqHz, durationMs);
}

void AudioManager::stopTone() {
    _isTonePlaying = false;
#ifndef NATIVE_TEST
    #if defined(ESP_ARDUINO_VERSION) && ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcWrite(_pin, 0);
    #else
        ledcWrite(_channel, 0);
    #endif
#endif
}

void AudioManager::setTargetRssi(int8_t rssi) {
    _targetRssi = rssi;
}

void AudioManager::clearTarget() {
    _targetRssi = RSSI_FLOOR;
    stopTone();
}

void AudioManager::setEnabled(bool enabled) {
    _enabled = enabled;
    if (!_enabled) {
        stopTone();
    }
}

void AudioManager::setVolume(uint8_t volumePercent) {
    if (volumePercent > 100) volumePercent = 100;
    _volume = volumePercent;
}

void AudioManager::update(unsigned long currentMillis) {
    if (_isTonePlaying) {
        if (currentMillis - _toneStartTime >= _toneDuration) {
            stopTone();
        }
    }

    if (!_enabled || _volume == 0 || _targetRssi <= RSSI_FLOOR) {
        return;
    }

    // Map RSSI (-95 dBm to -35 dBm) to beep interval (1200ms down to 70ms)
    // and pitch (900 Hz up to 2600 Hz)
    int clampedRssi = _targetRssi;
    if (clampedRssi < RSSI_FLOOR) clampedRssi = RSSI_FLOOR;
    if (clampedRssi > RSSI_CEILING) clampedRssi = RSSI_CEILING;

    // Linear interpolation
    float ratio = (float)(clampedRssi - RSSI_FLOOR) / (float)(RSSI_CEILING - RSSI_FLOOR);
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    _currentIntervalMs = (uint32_t)(1200.0f - (ratio * 1120.0f));
    uint16_t freq = (uint16_t)(900.0f + (ratio * 1700.0f));

    if (currentMillis - _lastBeepTime >= _currentIntervalMs) {
        _lastBeepTime = currentMillis;
        playTone(freq, 25);
    }
}
