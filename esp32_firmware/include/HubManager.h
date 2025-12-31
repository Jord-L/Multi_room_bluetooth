/**
 * ESP32 Multi-Room Bluetooth Hub - Hub Manager Header
 * Replaces DeviceManager - Central manager for hub operations
 */

#ifndef HUB_MANAGER_H
#define HUB_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

class HubManager {
public:
    HubManager();

    // Initialization
    bool begin();

    // Hub Identification
    String getHubId();           // MAC-based unique ID
    String getCustomName();
    String getLocation();
    String getIPAddress();
    String getFirmwareVersion();
    int getSignalStrength();

    // Hub Settings
    void setCustomName(const String& name);
    void setLocation(const String& location);

    // Power Management
    bool getPowerState();
    void setPowerState(bool state);

    // Master Audio Control
    int getMasterVolume();
    void setMasterVolume(int volume);
    bool isMuted();
    void setMuted(bool muted);
    void toggleMute();

    // Audio Source
    AudioSourceType getAudioSource();
    void setAudioSource(AudioSourceType source);

    // Hub Status
    bool isStreaming();
    void setStreaming(bool streaming);

    // Hub Information (JSON format)
    String getHubInfoJson();
    String getHubStatusJson();

    // Settings Persistence
    bool saveSettings();
    bool loadSettings();
    void resetToDefaults();

    // Identification (LED flash)
    void identify();

private:
    Preferences preferences;
    HubSettings settings;
    String hubId;
    String macAddress;
    bool streaming;

    // Helper methods
    String generateHubId();
    String getMacAddress();
};

#endif // HUB_MANAGER_H
