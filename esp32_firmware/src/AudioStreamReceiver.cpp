/**
 * ESP32 Multi-Room Bluetooth Hub - Audio Stream Receiver Implementation
 */

#include "AudioStreamReceiver.h"

AudioStreamReceiver::AudioStreamReceiver() {
    receiving = false;
    clientConnected = false;
    clientIP = "";
    clientPort = 0;
    bytesReceived = 0;
    packetsReceived = 0;
    packetsDropped = 0;
    lastPacketTime = 0;
    currentBufferLevel = 0;
    audioQueue = nullptr;
}

bool AudioStreamReceiver::begin() {
    Serial.println("[AudioRx] Initializing Audio Stream Receiver...");

    // Create audio queue
    audioQueue = xQueueCreate(AUDIO_QUEUE_SIZE, sizeof(AudioBuffer));
    if (audioQueue == nullptr) {
        Serial.println("[AudioRx] Failed to create audio queue");
        return false;
    }

    // Initialize buffer pool
    bufferPool.reserve(AUDIO_BUFFER_COUNT);

    Serial.println("[AudioRx] Audio Stream Receiver initialized");
    return true;
}

void AudioStreamReceiver::end() {
    stopReceiving();

    if (audioQueue != nullptr) {
        vQueueDelete(audioQueue);
        audioQueue = nullptr;
    }

    bufferPool.clear();
}

bool AudioStreamReceiver::startReceiving() {
    if (receiving) {
        Serial.println("[AudioRx] Already receiving");
        return true;
    }

    Serial.println("[AudioRx] Starting UDP audio receiver on port " + String(AUDIO_STREAM_PORT));

    // Start UDP server
    if (!udpServer.begin(AUDIO_STREAM_PORT)) {
        Serial.println("[AudioRx] Failed to start UDP server");
        return false;
    }

    receiving = true;
    bytesReceived = 0;
    packetsReceived = 0;
    packetsDropped = 0;
    lastPacketTime = millis();

    Serial.println("[AudioRx] Listening for audio stream on port " + String(AUDIO_STREAM_PORT));
    return true;
}

void AudioStreamReceiver::stopReceiving() {
    if (!receiving) return;

    Serial.println("[AudioRx] Stopping audio receiver");

    udpServer.stop();
    receiving = false;
    clientConnected = false;

    clearBuffer();
}

bool AudioStreamReceiver::isReceiving() {
    return receiving;
}

bool AudioStreamReceiver::hasAudioData() {
    if (audioQueue == nullptr) return false;
    return uxQueueMessagesWaiting(audioQueue) > 0;
}

size_t AudioStreamReceiver::readAudioData(uint8_t* buffer, size_t maxLength) {
    if (audioQueue == nullptr || buffer == nullptr) {
        return 0;
    }

    AudioBuffer audioBuffer;

    // Try to read from queue (non-blocking)
    if (xQueueReceive(audioQueue, &audioBuffer, 0) == pdTRUE) {
        size_t copyLength = min(audioBuffer.length, maxLength);
        memcpy(buffer, audioBuffer.data, copyLength);
        return copyLength;
    }

    return 0;
}

size_t AudioStreamReceiver::getAvailableBytes() {
    if (audioQueue == nullptr) return 0;

    int queueCount = uxQueueMessagesWaiting(audioQueue);
    return queueCount * AUDIO_BUFFER_SIZE;
}

void AudioStreamReceiver::clearBuffer() {
    if (audioQueue == nullptr) return;

    xQueueReset(audioQueue);
    currentBufferLevel = 0;
}

int AudioStreamReceiver::getBufferLevel() {
    if (audioQueue == nullptr) return 0;

    int queueCount = uxQueueMessagesWaiting(audioQueue);
    int percentage = (queueCount * 100) / AUDIO_QUEUE_SIZE;
    return percentage;
}

bool AudioStreamReceiver::isBufferHealthy() {
    int level = getBufferLevel();

    // Healthy buffer level is between 20% and 80%
    return (level >= 20 && level <= 80);
}

unsigned long AudioStreamReceiver::getBytesReceived() {
    return bytesReceived;
}

unsigned long AudioStreamReceiver::getPacketsReceived() {
    return packetsReceived;
}

unsigned long AudioStreamReceiver::getPacketsDropped() {
    return packetsDropped;
}

unsigned long AudioStreamReceiver::getAverageLatency() {
    // Calculate average latency based on packet timestamps
    // This is a simplified version
    if (packetsReceived == 0) return 0;

    unsigned long timeSinceLastPacket = millis() - lastPacketTime;
    return timeSinceLastPacket;
}

String AudioStreamReceiver::getStreamInfo() {
    String info = "Audio Stream Status:\n";
    info += "  Receiving: " + String(receiving ? "Yes" : "No") + "\n";
    info += "  Client Connected: " + String(clientConnected ? "Yes" : "No") + "\n";

    if (clientConnected) {
        info += "  Client IP: " + clientIP + "\n";
        info += "  Client Port: " + String(clientPort) + "\n";
    }

    info += "  Packets Received: " + String(packetsReceived) + "\n";
    info += "  Packets Dropped: " + String(packetsDropped) + "\n";
    info += "  Bytes Received: " + String(bytesReceived) + "\n";
    info += "  Buffer Level: " + String(getBufferLevel()) + "%\n";
    info += "  Avg Latency: " + String(getAverageLatency()) + "ms\n";

    return info;
}

bool AudioStreamReceiver::isClientConnected() {
    // Consider client connected if we've received packets recently (within 5 seconds)
    return clientConnected && (millis() - lastPacketTime) < 5000;
}

String AudioStreamReceiver::getClientIP() {
    return clientIP;
}

void AudioStreamReceiver::loop() {
    if (!receiving) return;

    // Check for incoming packets
    int packetSize = udpServer.parsePacket();

    if (packetSize > 0) {
        processIncomingPacket();
    }

    // Update buffer level
    updateBufferLevel();

    // Check for client timeout
    if (clientConnected && (millis() - lastPacketTime) > 5000) {
        Serial.println("[AudioRx] Client connection timeout");
        clientConnected = false;
    }
}

void AudioStreamReceiver::processIncomingPacket() {
    int packetSize = udpServer.available();

    if (packetSize > AUDIO_BUFFER_SIZE) {
        Serial.println("[AudioRx] Packet too large: " + String(packetSize));
        udpServer.flush();
        packetsDropped++;
        return;
    }

    // Read packet
    int bytesRead = udpServer.read(receiveBuffer, min(packetSize, (int)AUDIO_BUFFER_SIZE));

    if (bytesRead <= 0) {
        packetsDropped++;
        return;
    }

    // Update client info
    if (!clientConnected) {
        clientIP = udpServer.remoteIP().toString();
        clientPort = udpServer.remotePort();
        clientConnected = true;
        Serial.println("[AudioRx] Client connected: " + clientIP + ":" + String(clientPort));
    }

    // Validate packet (basic validation)
    if (!validatePacket(receiveBuffer, bytesRead)) {
        packetsDropped++;
        return;
    }

    // Process audio data (skip header if present)
    // Assuming first 8 bytes are header: [sequence:4][timestamp:4]
    const uint8_t* audioData = receiveBuffer + 8;
    size_t audioLength = bytesRead - 8;

    if (audioLength > 0) {
        handleAudioPacket(audioData, audioLength);
    }

    // Update statistics
    packetsReceived++;
    bytesReceived += bytesRead;
    lastPacketTime = millis();
}

void AudioStreamReceiver::handleAudioPacket(const uint8_t* data, size_t length) {
    // Check buffer level before enqueuing
    int bufferLevel = getBufferLevel();

    if (bufferLevel >= 90) {
        handleBufferOverrun();
        packetsDropped++;
        return;
    }

    if (bufferLevel < 10) {
        handleBufferUnderrun();
    }

    // Enqueue audio data
    enqueueAudioData(data, length);
}

void AudioStreamReceiver::updateBufferLevel() {
    currentBufferLevel = getBufferLevel();

    #if CORE_DEBUG_LEVEL >= 5
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 5000) {  // Print every 5 seconds
        Serial.println("[AudioRx] Buffer level: " + String(currentBufferLevel) + "%");
        lastPrint = millis();
    }
    #endif
}

void AudioStreamReceiver::handleBufferUnderrun() {
    Serial.println("[AudioRx] WARNING: Buffer underrun");
    // Could insert silence or repeat last frame
}

void AudioStreamReceiver::handleBufferOverrun() {
    Serial.println("[AudioRx] WARNING: Buffer overrun - dropping packet");
    // Drop oldest packet to make room
    if (audioQueue != nullptr) {
        AudioBuffer dummy;
        xQueueReceive(audioQueue, &dummy, 0);
    }
}

bool AudioStreamReceiver::validatePacket(const uint8_t* data, size_t length) {
    // Basic validation
    if (data == nullptr || length < 8) {
        return false;
    }

    // Could add more validation:
    // - Check sequence numbers
    // - Verify checksums
    // - Validate audio format markers

    return true;
}

void AudioStreamReceiver::enqueueAudioData(const uint8_t* data, size_t length) {
    if (audioQueue == nullptr || data == nullptr) {
        return;
    }

    AudioBuffer audioBuffer;
    audioBuffer.length = min(length, (size_t)AUDIO_BUFFER_SIZE);
    memcpy(audioBuffer.data, data, audioBuffer.length);
    audioBuffer.timestamp = millis();

    // Try to add to queue (non-blocking)
    if (xQueueSend(audioQueue, &audioBuffer, 0) != pdTRUE) {
        // Queue full
        handleBufferOverrun();
        packetsDropped++;
    }
}
