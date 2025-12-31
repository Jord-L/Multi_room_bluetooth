/**
 * ESP32 Multi-Room Speaker System - Audio Controller Implementation
 */

#include "AudioController.h"
#include "DeviceManager.h"
#include <driver/i2s.h>

AudioController::AudioController(DeviceManager* deviceMgr) {
    deviceManager = deviceMgr;
    powered = true;
    playing = false;
    currentVolume = VOLUME_DEFAULT;
    muted = false;
    preMuteVolume = VOLUME_DEFAULT;

    eqSettings.bass = EQ_DEFAULT;
    eqSettings.treble = EQ_DEFAULT;
    eqSettings.preset = EQ_FLAT;
}

bool AudioController::begin() {
    // Initialize I2S for audio output
    if (!initI2S()) {
        Serial.println("[Audio] Failed to initialize I2S");
        return false;
    }

    // Load settings from device manager
    currentVolume = deviceManager->getVolume();
    muted = deviceManager->isMuted();
    powered = deviceManager->getPowerState();
    eqSettings.preset = deviceManager->getEQPreset();
    eqSettings.bass = deviceManager->getBassBoost();
    eqSettings.treble = deviceManager->getTrebleAdjust();

    // Apply initial volume
    setI2SVolume(muted ? 0 : currentVolume);

    // Apply EQ settings
    applyEQSettings();

    Serial.println("[Audio] Audio controller initialized");
    return true;
}

bool AudioController::initI2S() {
    // I2S configuration for audio output
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    // I2S pin configuration
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK_PIN,
        .ws_io_num = I2S_LRC_PIN,
        .data_out_num = I2S_DOUT_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    // Install and start I2S driver
    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[Audio] Failed to install I2S driver: %d\n", err);
        return false;
    }

    err = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[Audio] Failed to set I2S pins: %d\n", err);
        return false;
    }

    Serial.println("[Audio] I2S initialized successfully");
    return true;
}

void AudioController::setVolume(uint8_t volume) {
    if (volume > VOLUME_MAX) {
        volume = VOLUME_MAX;
    }

    currentVolume = volume;
    deviceManager->setVolume(volume);

    if (!muted) {
        setI2SVolume(volume);
    }

    Serial.printf("[Audio] Volume set to %d%%\n", volume);
}

uint8_t AudioController::getVolume() {
    return currentVolume;
}

void AudioController::volumeUp() {
    uint8_t newVolume = currentVolume + VOLUME_STEP;
    if (newVolume > VOLUME_MAX) {
        newVolume = VOLUME_MAX;
    }
    setVolume(newVolume);
}

void AudioController::volumeDown() {
    int16_t newVolume = currentVolume - VOLUME_STEP;
    if (newVolume < VOLUME_MIN) {
        newVolume = VOLUME_MIN;
    }
    setVolume((uint8_t)newVolume);
}

void AudioController::setMute(bool mute) {
    if (mute == muted) {
        return;  // No change
    }

    if (mute) {
        preMuteVolume = currentVolume;
        setI2SVolume(0);
    } else {
        setI2SVolume(currentVolume);
    }

    muted = mute;
    deviceManager->setMuted(mute);

    Serial.printf("[Audio] %s\n", mute ? "Muted" : "Unmuted");
}

bool AudioController::isMuted() {
    return muted;
}

void AudioController::toggleMute() {
    setMute(!muted);
}

void AudioController::setPower(bool on) {
    powered = on;
    deviceManager->setPowerState(on);

    if (!on) {
        // Stop playback and mute when powering off
        playing = false;
        setI2SVolume(0);
    } else {
        // Restore volume when powering on
        if (!muted) {
            setI2SVolume(currentVolume);
        }
    }

    Serial.printf("[Audio] Power %s\n", on ? "ON" : "OFF");
}

bool AudioController::isPowered() {
    return powered;
}

void AudioController::setAudioSource(AudioSource source) {
    deviceManager->setAudioSource(source);
    switchAudioSource(source);
    Serial.printf("[Audio] Audio source changed to %d\n", (int)source);
}

AudioSource AudioController::getAudioSource() {
    return deviceManager->getAudioSource();
}

void AudioController::setEQPreset(EQPreset preset) {
    eqSettings.preset = preset;
    deviceManager->setEQPreset(preset);
    applyEQPreset(preset);
    Serial.printf("[Audio] EQ preset changed to %d\n", (int)preset);
}

EQPreset AudioController::getEQPreset() {
    return eqSettings.preset;
}

void AudioController::setBass(int8_t level) {
    if (level < EQ_MIN) level = EQ_MIN;
    if (level > EQ_MAX) level = EQ_MAX;

    eqSettings.bass = level;
    deviceManager->setBassBoost(level);
    updateHardwareEQ();

    Serial.printf("[Audio] Bass set to %d dB\n", level);
}

int8_t AudioController::getBass() {
    return eqSettings.bass;
}

void AudioController::setTreble(int8_t level) {
    if (level < EQ_MIN) level = EQ_MIN;
    if (level > EQ_MAX) level = EQ_MAX;

    eqSettings.treble = level;
    deviceManager->setTrebleAdjust(level);
    updateHardwareEQ();

    Serial.printf("[Audio] Treble set to %d dB\n", level);
}

int8_t AudioController::getTreble() {
    return eqSettings.treble;
}

void AudioController::applyEQSettings() {
    applyEQPreset(eqSettings.preset);
}

void AudioController::applyEQPreset(EQPreset preset) {
    // Apply preset values
    switch (preset) {
        case EQ_ROCK:
            eqSettings.bass = 6;
            eqSettings.treble = 4;
            break;

        case EQ_JAZZ:
            eqSettings.bass = -2;
            eqSettings.treble = 3;
            break;

        case EQ_CLASSICAL:
            eqSettings.bass = -3;
            eqSettings.treble = 2;
            break;

        case EQ_POP:
            eqSettings.bass = 4;
            eqSettings.treble = 2;
            break;

        case EQ_FLAT:
        default:
            eqSettings.bass = 0;
            eqSettings.treble = 0;
            break;
    }

    if (preset != EQ_CUSTOM) {
        // Save preset values to device manager
        deviceManager->setBassBoost(eqSettings.bass);
        deviceManager->setTrebleAdjust(eqSettings.treble);
    }

    updateHardwareEQ();
}

void AudioController::updateHardwareEQ() {
    // In Phase 1, this is a placeholder for hardware EQ implementation
    // In future phases, this could control:
    // - Hardware EQ on DAC chip
    // - DSP processing
    // - Software filtering

    Serial.printf("[Audio] EQ updated - Bass: %d dB, Treble: %d dB\n",
                  eqSettings.bass, eqSettings.treble);
}

void AudioController::switchAudioSource(AudioSource source) {
    // In Phase 1, this is a basic implementation
    // Future phases will add actual source switching logic

    playing = false;

    switch (source) {
        case SOURCE_BLUETOOTH:
            Serial.println("[Audio] Switching to Bluetooth input");
            // Initialize Bluetooth A2DP sink
            break;

        case SOURCE_LINE_IN:
            Serial.println("[Audio] Switching to Line-In input");
            // Configure I2S for line input
            break;

        case SOURCE_NETWORK:
            Serial.println("[Audio] Switching to Network streaming");
            // Setup network streaming
            break;

        default:
            Serial.println("[Audio] Unknown audio source");
            break;
    }
}

void AudioController::setI2SVolume(uint8_t volume) {
    // Convert 0-100 volume to 0.0-1.0 scale
    float volumeScale = volume / 100.0f;

    // In Phase 1, we update a volume multiplier
    // In future phases, this could control:
    // - I2S DMA buffer scaling
    // - External DAC volume control
    // - Digital volume control chip

    Serial.printf("[Audio] I2S volume set to %.2f (hardware volume control)\n", volumeScale);
}

bool AudioController::isPlaying() {
    return playing && powered;
}

String AudioController::getPlaybackStatus() {
    if (!powered) return "off";
    if (playing) return "playing";
    return "idle";
}

void AudioController::processAudio() {
    // This method would handle real-time audio processing
    // - Apply EQ filtering
    // - Handle volume scaling
    // - Synchronization with other speakers
    // - Buffer management

    // Phase 1: Placeholder implementation
}

void AudioController::loop() {
    // Audio processing loop
    if (powered && playing) {
        processAudio();
    }
}
