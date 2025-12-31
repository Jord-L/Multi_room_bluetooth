/**
 * ESP32 Multi-Room Speaker System - Device Manager
 * Phase 1: Device Identification & Management
 *
 * Handles device identification, settings persistence, and device information
 */

#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

class DeviceManager {
public:
    DeviceManager();

    // Initialization
    bool begin();

    // Device Identification
    String getDeviceId();           // MAC-based unique ID
    String getCustomName();
    String getRoom();
    String getIPAddress();
    String getFirmwareVersion();
    int getSignalStrength();

    // Device Settings
    void setCustomName(const String& name);
    void setRoom(const String& room);

    // Power Management
    bool getPowerState();
    void setPowerState(bool state);

    // Audio Settings
    uint8_t getVolume();
    void setVolume(uint8_t volume);
    bool isMuted();
    void setMuted(bool muted);
    void toggleMute();

    // Audio Source
    AudioSource getAudioSource();
    void setAudioSource(AudioSource source);

    // Equalizer
    EQPreset getEQPreset();
    void setEQPreset(EQPreset preset);
    int8_t getBassBoost();
    void setBassBoost(int8_t level);
    int8_t getTrebleAdjust();
    void setTrebleAdjust(int8_t level);

    // Grouping (Phase 1 - basic support)
    void addToGroup(const String& groupName);
    void removeFromGroup(const String& groupName);
    String getGroups();  // Returns comma-separated list

    // Device Information (JSON format)
    String getDeviceInfoJson();
    String getDeviceStatusJson();

    // Settings Persistence
    bool saveSettings();
    bool loadSettings();
    void resetToDefaults();

    // Identification (LED flash or sound)
    void identify();

private:
    Preferences preferences;
    DeviceSettings settings;
    String deviceId;
    String macAddress;

    // Helper methods
    String generateDeviceId();
    String getMacAddress();
    void updateDeviceId();
};

#endif // DEVICE_MANAGER_H
