/**
 * ESP32 Multi-Room Bluetooth Hub - Audio Stream Receiver
 * Phase 1: WiFi Audio Streaming
 *
 * Receives audio stream from mobile app via WiFi (UDP)
 * Buffers audio for forwarding to Bluetooth speakers
 */

#ifndef AUDIO_STREAM_RECEIVER_H
#define AUDIO_STREAM_RECEIVER_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "config.h"

// Audio buffer structure
struct AudioBuffer {
    uint8_t data[AUDIO_BUFFER_SIZE];
    size_t length;
    unsigned long timestamp;
};

class AudioStreamReceiver {
public:
    AudioStreamReceiver();

    // Initialization
    bool begin();
    void end();

    // Streaming control
    bool startReceiving();
    void stopReceiving();
    bool isReceiving();

    // Audio data access
    bool hasAudioData();
    size_t readAudioData(uint8_t* buffer, size_t maxLength);
    size_t getAvailableBytes();

    // Buffer management
    void clearBuffer();
    int getBufferLevel();  // Returns percentage (0-100)
    bool isBufferHealthy();

    // Statistics
    unsigned long getBytesReceived();
    unsigned long getPacketsReceived();
    unsigned long getPacketsDropped();
    unsigned long getAverageLatency();

    // Connection info
    String getStreamInfo();
    bool isClientConnected();
    String getClientIP();

    // Processing loop
    void loop();

private:
    WiFiUDP udpServer;
    QueueHandle_t audioQueue;

    bool receiving;
    bool clientConnected;
    String clientIP;
    unsigned int clientPort;

    // Statistics
    unsigned long bytesReceived;
    unsigned long packetsReceived;
    unsigned long packetsDropped;
    unsigned long lastPacketTime;

    // Buffer management
    uint8_t receiveBuffer[AUDIO_BUFFER_SIZE];
    std::vector<AudioBuffer> bufferPool;
    int currentBufferLevel;

    // Helper methods
    void processIncomingPacket();
    void handleAudioPacket(const uint8_t* data, size_t length);
    void updateBufferLevel();
    void handleBufferUnderrun();
    void handleBufferOverrun();

    // Packet processing
    bool validatePacket(const uint8_t* data, size_t length);
    void enqueueAudioData(const uint8_t* data, size_t length);
};

#endif // AUDIO_STREAM_RECEIVER_H
