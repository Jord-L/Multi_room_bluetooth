/**
 * ESP32 Multi-Room Speaker System - Audio Controller
 * Phase 1: Audio Control and Processing
 *
 * Handles audio playback, volume control, and equalizer settings
 */

#ifndef AUDIO_CONTROLLER_H
#define AUDIO_CONTROLLER_H

#include <Arduino.h>
#include "config.h"

// Forward declaration
class DeviceManager;

class AudioController {
public:
    AudioController(DeviceManager* deviceMgr);

    // Initialization
    bool begin();

    // Volume Control
    void setVolume(uint8_t volume);
    uint8_t getVolume();
    void volumeUp();
    void volumeDown();
    void setMute(bool muted);
    bool isMuted();
    void toggleMute();

    // Power Control
    void setPower(bool on);
    bool isPowered();

    // Audio Source
    void setAudioSource(AudioSource source);
    AudioSource getAudioSource();

    // Equalizer
    void setEQPreset(EQPreset preset);
    EQPreset getEQPreset();
    void setBass(int8_t level);
    int8_t getBass();
    void setTreble(int8_t level);
    int8_t getTreble();
    void applyEQSettings();

    // Playback Status
    bool isPlaying();
    String getPlaybackStatus();

    // Audio Processing
    void processAudio();
    void loop();

private:
    DeviceManager* deviceManager;
    bool powered;
    bool playing;
    uint8_t currentVolume;
    bool muted;
    uint8_t preMuteVolume;

    // I2S Audio
    bool initI2S();
    void setI2SVolume(uint8_t volume);

    // Equalizer implementation
    struct EQSettings {
        int8_t bass;
        int8_t treble;
        EQPreset preset;
    };
    EQSettings eqSettings;

    void applyEQPreset(EQPreset preset);
    void updateHardwareEQ();

    // Audio source switching
    void switchAudioSource(AudioSource source);
};

#endif // AUDIO_CONTROLLER_H
