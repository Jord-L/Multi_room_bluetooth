/**
 * ESP32 Multi-Room Bluetooth Hub - Bluetooth Speaker Manager
 * Phase 1: Bluetooth A2DP Source Management
 *
 * Manages connections to multiple Bluetooth speakers
 * ESP32 acts as A2DP SOURCE (master) connecting TO speakers
 */

#ifndef BLUETOOTH_SPEAKER_MANAGER_H
#define BLUETOOTH_SPEAKER_MANAGER_H

#include <Arduino.h>
#include <BluetoothA2DPSource.h>
#include <vector>
#include "config.h"
#include "BluetoothSpeaker.h"

class BluetoothSpeakerManager {
public:
    BluetoothSpeakerManager();

    // Initialization
    bool begin();
    void end();

    // Speaker Discovery
    bool startDiscovery(unsigned int durationSeconds = BT_SCAN_DURATION);
    void stopDiscovery();
    bool isDiscovering();
    std::vector<BluetoothSpeaker> getDiscoveredSpeakers();

    // Speaker Pairing & Connection
    bool pairSpeaker(const String& btAddress);
    bool connectToSpeaker(const String& btAddress);
    bool disconnectFromSpeaker(const String& btAddress);
    bool removeSpeaker(const String& btAddress);

    // Multi-speaker connection
    bool connectToMultipleSpeakers(const std::vector<String>& addresses);
    void disconnectAll();

    // Speaker Management
    BluetoothSpeaker* getSpeaker(const String& btAddress);
    std::vector<BluetoothSpeaker> getConnectedSpeakers();
    std::vector<BluetoothSpeaker> getAllSpeakers();
    int getConnectedCount();

    // Volume Control
    void setGlobalVolume(int volume);
    void setSpeakerVolume(const String& btAddress, int volume);
    void setMuted(bool muted);
    void setSpeakerMuted(const String& btAddress, bool muted);

    // Audio Streaming
    bool startStreaming();
    bool stopStreaming();
    bool isStreaming();
    void writeAudioData(const uint8_t* data, size_t length);

    // Speaker naming
    void setSpeakerCustomName(const String& btAddress, const String& name);
    void setSpeakerRoom(const String& btAddress, const String& room);

    // Connection monitoring
    void checkConnections();
    void attemptReconnections();

    // Persistence
    bool saveSpeakers();
    bool loadSpeakers();

    // Status info (JSON format)
    String getSpeakersStatusJson();
    String getDiscoveredSpeakersJson();

private:
    BluetoothA2DPSource a2dpSource;
    std::vector<BluetoothSpeaker> speakers;
    std::vector<BluetoothSpeaker> discoveredSpeakers;

    bool discovering;
    bool streaming;
    int globalVolume;
    bool globalMuted;

    // Audio buffer for streaming
    QueueHandle_t audioQueue;
    uint8_t audioBuffer[AUDIO_BUFFER_SIZE];
    size_t audioBufferPos;
    size_t audioBufferAvailable;

    // Helper methods
    void onDiscoveryComplete();
    void onDeviceDiscovered(const char* name, const char* address, int rssi);
    void onConnectionStateChanged(const String& address, bool connected);

    BluetoothSpeaker* findSpeaker(const String& btAddress);
    void addOrUpdateSpeaker(const BluetoothSpeaker& speaker);
    void updateSpeakerState(const String& btAddress, BTSpeakerState state);

    // Audio callback (provides audio data to Bluetooth)
    static int32_t audioDataCallback(uint8_t* data, int32_t len);
    static BluetoothSpeakerManager* instance;
};

#endif // BLUETOOTH_SPEAKER_MANAGER_H
