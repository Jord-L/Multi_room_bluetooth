/**
 * ESP32 Multi-Room Bluetooth Hub - Bluetooth Speaker Manager Implementation
 */

#include "BluetoothSpeakerManager.h"
#include <Preferences.h>

// Static instance for callbacks
BluetoothSpeakerManager* BluetoothSpeakerManager::instance = nullptr;

BluetoothSpeakerManager::BluetoothSpeakerManager() {
    discovering = false;
    streaming = false;
    globalVolume = VOLUME_DEFAULT;
    globalMuted = false;
    audioQueue = nullptr;
    audioBufferPos = 0;
    audioBufferAvailable = 0;
    memset(audioBuffer, 0, AUDIO_BUFFER_SIZE);
    instance = this;
}

bool BluetoothSpeakerManager::begin() {
    Serial.println("[BTMgr] Initializing Bluetooth Speaker Manager...");

    // Create audio queue for buffering
    audioQueue = xQueueCreate(AUDIO_QUEUE_SIZE, sizeof(AudioBuffer));
    if (audioQueue == nullptr) {
        Serial.println("[BTMgr] Failed to create audio queue");
        return false;
    }

    // Initialize Bluetooth A2DP Source with audio callback
    a2dpSource.set_auto_reconnect(false);
    a2dpSource.start(BT_DEVICE_NAME_PREFIX, audioDataCallback);

    // Load saved speakers from preferences
    if (!loadSpeakers()) {
        Serial.println("[BTMgr] No saved speakers found");
    }

    Serial.println("[BTMgr] Bluetooth Speaker Manager initialized");
    return true;
}

void BluetoothSpeakerManager::end() {
    stopStreaming();
    disconnectAll();

    if (audioQueue != nullptr) {
        vQueueDelete(audioQueue);
        audioQueue = nullptr;
    }

    a2dpSource.end();
}

bool BluetoothSpeakerManager::startDiscovery(unsigned int durationSeconds) {
    if (discovering) {
        Serial.println("[BTMgr] Already discovering");
        return false;
    }

    Serial.println("[BTMgr] Starting Bluetooth discovery for " + String(durationSeconds) + " seconds...");

    discovering = true;
    discoveredSpeakers.clear();

    // Start Bluetooth scan
    // Note: The actual implementation depends on ESP32-A2DP library capabilities
    // This is a simplified version - actual scanning may need custom BLE code

    Serial.println("[BTMgr] Scan started - looking for Bluetooth audio devices");

    // Simulate discovery for now (actual implementation would use BLE scan)
    // In production, use esp_bt_gap_start_discovery() or BLE scan

    return true;
}

void BluetoothSpeakerManager::stopDiscovery() {
    if (!discovering) return;

    Serial.println("[BTMgr] Stopping Bluetooth discovery");
    discovering = false;

    // Stop Bluetooth scan
    // esp_bt_gap_cancel_discovery();
}

bool BluetoothSpeakerManager::isDiscovering() {
    return discovering;
}

std::vector<BluetoothSpeaker> BluetoothSpeakerManager::getDiscoveredSpeakers() {
    return discoveredSpeakers;
}

bool BluetoothSpeakerManager::pairSpeaker(const String& btAddress) {
    Serial.println("[BTMgr] Pairing with speaker: " + btAddress);

    BluetoothSpeaker* speaker = findSpeaker(btAddress);
    if (speaker == nullptr) {
        // Create new speaker entry
        BluetoothSpeaker newSpeaker(btAddress, "Unknown");
        speakers.push_back(newSpeaker);
        speaker = &speakers.back();
    }

    speaker->isPaired = true;
    speaker->updateState(BT_SPEAKER_DISCONNECTED);

    saveSpeakers();
    return true;
}

bool BluetoothSpeakerManager::connectToSpeaker(const String& btAddress) {
    Serial.println("[BTMgr] Connecting to speaker: " + btAddress);

    BluetoothSpeaker* speaker = findSpeaker(btAddress);
    if (speaker == nullptr) {
        Serial.println("[BTMgr] Speaker not found, cannot connect");
        return false;
    }

    if (speaker->isConnected) {
        Serial.println("[BTMgr] Speaker already connected");
        return true;
    }

    // Check connection limit
    if (getConnectedCount() >= BT_MAX_CONNECTED_SPEAKERS) {
        Serial.println("[BTMgr] Maximum connections reached (" + String(BT_MAX_CONNECTED_SPEAKERS) + ")");
        return false;
    }

    speaker->updateState(BT_SPEAKER_CONNECTING);

    // Connect using A2DP Source
    // Note: ESP32-A2DP library typically connects to one device at a time
    // Multi-connection requires custom implementation or sequential connections
    bool success = a2dpSource.connect_to((char*)btAddress.c_str());

    if (success) {
        speaker->updateState(BT_SPEAKER_CONNECTED);
        Serial.println("[BTMgr] Connected to " + speaker->customName);
    } else {
        speaker->updateState(BT_SPEAKER_ERROR);
        Serial.println("[BTMgr] Failed to connect to " + speaker->customName);
    }

    return success;
}

bool BluetoothSpeakerManager::disconnectFromSpeaker(const String& btAddress) {
    Serial.println("[BTMgr] Disconnecting from speaker: " + btAddress);

    BluetoothSpeaker* speaker = findSpeaker(btAddress);
    if (speaker == nullptr) {
        return false;
    }

    if (!speaker->isConnected) {
        return true;  // Already disconnected
    }

    // Disconnect
    a2dpSource.disconnect();
    speaker->updateState(BT_SPEAKER_DISCONNECTED);

    Serial.println("[BTMgr] Disconnected from " + speaker->customName);
    return true;
}

bool BluetoothSpeakerManager::removeSpeaker(const String& btAddress) {
    Serial.println("[BTMgr] Removing speaker: " + btAddress);

    // Disconnect first if connected
    disconnectFromSpeaker(btAddress);

    // Remove from speakers list
    for (auto it = speakers.begin(); it != speakers.end(); ++it) {
        if (it->btAddress == btAddress) {
            speakers.erase(it);
            saveSpeakers();
            return true;
        }
    }

    return false;
}

bool BluetoothSpeakerManager::connectToMultipleSpeakers(const std::vector<String>& addresses) {
    Serial.println("[BTMgr] Connecting to multiple speakers: " + String(addresses.size()));

    bool allSuccess = true;

    for (const String& address : addresses) {
        if (!connectToSpeaker(address)) {
            allSuccess = false;
            Serial.println("[BTMgr] Failed to connect to: " + address);
        }
        delay(1000);  // Brief delay between connections
    }

    return allSuccess;
}

void BluetoothSpeakerManager::disconnectAll() {
    Serial.println("[BTMgr] Disconnecting all speakers");

    a2dpSource.disconnect();

    for (auto& speaker : speakers) {
        if (speaker.isConnected) {
            speaker.updateState(BT_SPEAKER_DISCONNECTED);
        }
    }
}

BluetoothSpeaker* BluetoothSpeakerManager::getSpeaker(const String& btAddress) {
    return findSpeaker(btAddress);
}

std::vector<BluetoothSpeaker> BluetoothSpeakerManager::getConnectedSpeakers() {
    std::vector<BluetoothSpeaker> connected;

    for (const auto& speaker : speakers) {
        if (speaker.isConnected) {
            connected.push_back(speaker);
        }
    }

    return connected;
}

std::vector<BluetoothSpeaker> BluetoothSpeakerManager::getAllSpeakers() {
    return speakers;
}

int BluetoothSpeakerManager::getConnectedCount() {
    int count = 0;
    for (const auto& speaker : speakers) {
        if (speaker.isConnected) {
            count++;
        }
    }
    return count;
}

void BluetoothSpeakerManager::setGlobalVolume(int volume) {
    if (volume < VOLUME_MIN) volume = VOLUME_MIN;
    if (volume > VOLUME_MAX) volume = VOLUME_MAX;

    globalVolume = volume;

    // Apply to all speakers
    for (auto& speaker : speakers) {
        speaker.setVolume(volume);
    }

    Serial.println("[BTMgr] Global volume set to " + String(volume) + "%");
}

void BluetoothSpeakerManager::setSpeakerVolume(const String& btAddress, int volume) {
    BluetoothSpeaker* speaker = findSpeaker(btAddress);
    if (speaker != nullptr) {
        speaker->setVolume(volume);
        Serial.println("[BTMgr] Volume for " + speaker->customName + " set to " + String(volume) + "%");
    }
}

void BluetoothSpeakerManager::setMuted(bool muted) {
    globalMuted = muted;

    for (auto& speaker : speakers) {
        speaker.setMuted(muted);
    }

    Serial.println("[BTMgr] Global mute: " + String(muted ? "ON" : "OFF"));
}

void BluetoothSpeakerManager::setSpeakerMuted(const String& btAddress, bool muted) {
    BluetoothSpeaker* speaker = findSpeaker(btAddress);
    if (speaker != nullptr) {
        speaker->setMuted(muted);
        Serial.println("[BTMgr] Mute for " + speaker->customName + ": " + String(muted ? "ON" : "OFF"));
    }
}

bool BluetoothSpeakerManager::startStreaming() {
    if (streaming) {
        return true;
    }

    if (getConnectedCount() == 0) {
        Serial.println("[BTMgr] No speakers connected, cannot start streaming");
        return false;
    }

    streaming = true;
    Serial.println("[BTMgr] Audio streaming started");

    // Update speaker states
    for (auto& speaker : speakers) {
        if (speaker.isConnected) {
            speaker.updateState(BT_SPEAKER_PLAYING);
        }
    }

    return true;
}

bool BluetoothSpeakerManager::stopStreaming() {
    if (!streaming) {
        return true;
    }

    streaming = false;
    Serial.println("[BTMgr] Audio streaming stopped");

    // Update speaker states
    for (auto& speaker : speakers) {
        if (speaker.isConnected) {
            speaker.updateState(BT_SPEAKER_PAUSED);
        }
    }

    return true;
}

bool BluetoothSpeakerManager::isStreaming() {
    return streaming;
}

void BluetoothSpeakerManager::writeAudioData(const uint8_t* data, size_t length) {
    if (!streaming || getConnectedCount() == 0 || audioQueue == nullptr || data == nullptr) {
        return;
    }

    // Enqueue audio data for Bluetooth transmission
    // The audioDataCallback will pull from this queue
    AudioBuffer audioBuffer;
    audioBuffer.length = min(length, (size_t)AUDIO_BUFFER_SIZE);
    memcpy(audioBuffer.data, data, audioBuffer.length);
    audioBuffer.timestamp = millis();

    // Try to add to queue (non-blocking)
    if (xQueueSend(audioQueue, &audioBuffer, 0) != pdTRUE) {
        // Queue full - audio overrun
        #if CORE_DEBUG_LEVEL >= 4
        static unsigned long lastWarning = 0;
        if (millis() - lastWarning > 5000) {
            Serial.println("[BTMgr] Audio queue full - dropping packet");
            lastWarning = millis();
        }
        #endif
    }
}

void BluetoothSpeakerManager::setSpeakerCustomName(const String& btAddress, const String& name) {
    BluetoothSpeaker* speaker = findSpeaker(btAddress);
    if (speaker != nullptr) {
        speaker->customName = name;
        saveSpeakers();
        Serial.println("[BTMgr] Speaker renamed to: " + name);
    }
}

void BluetoothSpeakerManager::setSpeakerRoom(const String& btAddress, const String& room) {
    BluetoothSpeaker* speaker = findSpeaker(btAddress);
    if (speaker != nullptr) {
        speaker->room = room;
        saveSpeakers();
        Serial.println("[BTMgr] Speaker assigned to room: " + room);
    }
}

void BluetoothSpeakerManager::checkConnections() {
    // Check health of all connected speakers
    for (auto& speaker : speakers) {
        if (speaker.isConnected && !speaker.isHealthy()) {
            Serial.println("[BTMgr] Speaker " + speaker.customName + " appears unhealthy");
            speaker.updateState(BT_SPEAKER_ERROR);
        }
    }
}

void BluetoothSpeakerManager::attemptReconnections() {
    // Try to reconnect to known speakers that are disconnected
    for (auto& speaker : speakers) {
        if (!speaker.isConnected && speaker.isPaired) {
            if (speaker.connectionAttempts < BT_RECONNECT_ATTEMPTS) {
                Serial.println("[BTMgr] Attempting to reconnect to " + speaker.customName);
                connectToSpeaker(speaker.btAddress);
            }
        }
    }
}

bool BluetoothSpeakerManager::saveSpeakers() {
    Preferences prefs;
    prefs.begin(PREFS_NAMESPACE, false);

    // Save each speaker as JSON
    int count = 0;
    for (const auto& speaker : speakers) {
        String key = "speaker_" + String(count);
        String json = speaker.toJson();
        prefs.putString(key.c_str(), json);
        count++;
    }

    prefs.putInt("speaker_count", count);
    prefs.end();

    Serial.println("[BTMgr] Saved " + String(count) + " speakers to preferences");
    return true;
}

bool BluetoothSpeakerManager::loadSpeakers() {
    Preferences prefs;
    prefs.begin(PREFS_NAMESPACE, true);

    int count = prefs.getInt("speaker_count", 0);
    if (count == 0) {
        prefs.end();
        return false;
    }

    speakers.clear();

    for (int i = 0; i < count; i++) {
        String key = "speaker_" + String(i);
        String json = prefs.getString(key.c_str(), "");

        if (json.length() > 0) {
            BluetoothSpeaker speaker;
            speaker.fromJson(json);
            speakers.push_back(speaker);
        }
    }

    prefs.end();

    Serial.println("[BTMgr] Loaded " + String(speakers.size()) + " speakers from preferences");
    return true;
}

String BluetoothSpeakerManager::getSpeakersStatusJson() {
    StaticJsonDocument<2048> doc;
    JsonArray speakersArray = doc.createNestedArray("speakers");

    for (const auto& speaker : speakers) {
        StaticJsonDocument<512> speakerDoc;
        deserializeJson(speakerDoc, speaker.toJson());
        speakersArray.add(speakerDoc);
    }

    doc["totalSpeakers"] = speakers.size();
    doc["connectedSpeakers"] = getConnectedCount();
    doc["isStreaming"] = streaming;
    doc["globalVolume"] = globalVolume;
    doc["globalMuted"] = globalMuted;

    String output;
    serializeJson(doc, output);
    return output;
}

String BluetoothSpeakerManager::getDiscoveredSpeakersJson() {
    StaticJsonDocument<2048> doc;
    JsonArray speakersArray = doc.createNestedArray("discovered");

    for (const auto& speaker : discoveredSpeakers) {
        StaticJsonDocument<512> speakerDoc;
        deserializeJson(speakerDoc, speaker.toJson());
        speakersArray.add(speakerDoc);
    }

    doc["count"] = discoveredSpeakers.size();
    doc["isDiscovering"] = discovering;

    String output;
    serializeJson(doc, output);
    return output;
}

// Helper methods

BluetoothSpeaker* BluetoothSpeakerManager::findSpeaker(const String& btAddress) {
    for (auto& speaker : speakers) {
        if (speaker.btAddress == btAddress) {
            return &speaker;
        }
    }
    return nullptr;
}

void BluetoothSpeakerManager::addOrUpdateSpeaker(const BluetoothSpeaker& speaker) {
    BluetoothSpeaker* existing = findSpeaker(speaker.btAddress);

    if (existing != nullptr) {
        // Update existing speaker
        *existing = speaker;
    } else {
        // Add new speaker
        speakers.push_back(speaker);
    }
}

void BluetoothSpeakerManager::updateSpeakerState(const String& btAddress, BTSpeakerState state) {
    BluetoothSpeaker* speaker = findSpeaker(btAddress);
    if (speaker != nullptr) {
        speaker->updateState(state);
    }
}

// Static audio data callback (for A2DP source)
int32_t BluetoothSpeakerManager::audioDataCallback(uint8_t* data, int32_t len) {
    if (instance == nullptr || !instance->streaming || instance->audioQueue == nullptr) {
        // No audio data available - return silence
        memset(data, 0, len);
        return len;
    }

    int32_t bytesWritten = 0;

    // Fill the requested buffer from our audio queue
    while (bytesWritten < len) {
        // Check if we have data in our internal buffer
        if (instance->audioBufferAvailable == 0) {
            // Try to get more data from the queue
            AudioBuffer audioBuffer;
            if (xQueueReceive(instance->audioQueue, &audioBuffer, 0) == pdTRUE) {
                // Got new audio data from queue
                memcpy(instance->audioBuffer, audioBuffer.data, audioBuffer.length);
                instance->audioBufferPos = 0;
                instance->audioBufferAvailable = audioBuffer.length;
            } else {
                // No more data in queue - fill rest with silence
                memset(data + bytesWritten, 0, len - bytesWritten);
                bytesWritten = len;
                break;
            }
        }

        // Copy from internal buffer to output
        size_t bytesToCopy = min((size_t)(len - bytesWritten), instance->audioBufferAvailable);
        memcpy(data + bytesWritten, instance->audioBuffer + instance->audioBufferPos, bytesToCopy);

        instance->audioBufferPos += bytesToCopy;
        instance->audioBufferAvailable -= bytesToCopy;
        bytesWritten += bytesToCopy;
    }

    return len;
}
