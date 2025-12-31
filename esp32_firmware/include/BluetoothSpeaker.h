/**
 * ESP32 Multi-Room Bluetooth Hub - Bluetooth Speaker Model
 * Phase 1: Bluetooth Speaker Management
 *
 * Represents a Bluetooth speaker connected to the hub
 */

#ifndef BLUETOOTH_SPEAKER_H
#define BLUETOOTH_SPEAKER_H

#include <Arduino.h>
#include "config.h"

class BluetoothSpeaker {
public:
    // Constructor
    BluetoothSpeaker();
    BluetoothSpeaker(const String& btAddress, const String& btName);

    // Identification
    String btAddress;          // Bluetooth MAC address (unique ID)
    String btName;             // Bluetooth device name
    String customName;         // User-assigned name
    String room;               // Room assignment

    // Connection state
    BTSpeakerState state;
    bool isConnected;
    bool isConnecting;
    bool isPaired;
    unsigned long lastSeen;
    unsigned long connectionAttempts;

    // Audio state
    int volume;                // Individual speaker volume (0-100)
    bool isMuted;
    bool isPlaying;

    // Signal strength
    int rssi;                  // Received Signal Strength Indicator

    // Methods
    void updateState(BTSpeakerState newState);
    void updateRSSI(int newRssi);
    void setVolume(int vol);
    void setMuted(bool muted);

    // JSON serialization
    String toJson() const;
    void fromJson(const String& json);

    // Connection info
    String getConnectionInfo() const;
    bool isHealthy() const;
    unsigned long getTimeSinceLastSeen() const;

private:
    void resetConnectionAttempts();
};

#endif // BLUETOOTH_SPEAKER_H
