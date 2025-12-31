/**
 * ESP32 Multi-Room Bluetooth Hub - Bluetooth Speaker Implementation
 */

#include "BluetoothSpeaker.h"
#include <ArduinoJson.h>

BluetoothSpeaker::BluetoothSpeaker() {
    btAddress = "";
    btName = "Unknown";
    customName = "Speaker";
    room = "Unknown";
    state = BT_SPEAKER_DISCONNECTED;
    isConnected = false;
    isConnecting = false;
    isPaired = false;
    lastSeen = 0;
    connectionAttempts = 0;
    volume = 50;
    isMuted = false;
    isPlaying = false;
    rssi = 0;
}

BluetoothSpeaker::BluetoothSpeaker(const String& address, const String& name) {
    btAddress = address;
    btName = name;
    customName = name;
    room = "Unknown";
    state = BT_SPEAKER_DISCONNECTED;
    isConnected = false;
    isConnecting = false;
    isPaired = false;
    lastSeen = millis();
    connectionAttempts = 0;
    volume = 50;
    isMuted = false;
    isPlaying = false;
    rssi = 0;
}

void BluetoothSpeaker::updateState(BTSpeakerState newState) {
    state = newState;
    lastSeen = millis();

    switch (newState) {
        case BT_SPEAKER_CONNECTED:
            isConnected = true;
            isConnecting = false;
            resetConnectionAttempts();
            break;

        case BT_SPEAKER_CONNECTING:
            isConnected = false;
            isConnecting = true;
            connectionAttempts++;
            break;

        case BT_SPEAKER_DISCONNECTED:
        case BT_SPEAKER_ERROR:
            isConnected = false;
            isConnecting = false;
            isPlaying = false;
            break;

        case BT_SPEAKER_PLAYING:
            isPlaying = true;
            break;

        case BT_SPEAKER_PAUSED:
            isPlaying = false;
            break;
    }
}

void BluetoothSpeaker::updateRSSI(int newRssi) {
    rssi = newRssi;
    lastSeen = millis();
}

void BluetoothSpeaker::setVolume(int vol) {
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;
    volume = vol;
}

void BluetoothSpeaker::setMuted(bool muted) {
    isMuted = muted;
}

void BluetoothSpeaker::resetConnectionAttempts() {
    connectionAttempts = 0;
}

String BluetoothSpeaker::toJson() const {
    StaticJsonDocument<512> doc;

    doc["btAddress"] = btAddress;
    doc["btName"] = btName;
    doc["customName"] = customName;
    doc["room"] = room;
    doc["state"] = (int)state;
    doc["isConnected"] = isConnected;
    doc["isConnecting"] = isConnecting;
    doc["isPaired"] = isPaired;
    doc["volume"] = volume;
    doc["isMuted"] = isMuted;
    doc["isPlaying"] = isPlaying;
    doc["rssi"] = rssi;
    doc["lastSeen"] = lastSeen;

    String output;
    serializeJson(doc, output);
    return output;
}

void BluetoothSpeaker::fromJson(const String& json) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        Serial.println("[BTSpeaker] JSON parsing failed: " + String(error.c_str()));
        return;
    }

    btAddress = doc["btAddress"].as<String>();
    btName = doc["btName"].as<String>();
    customName = doc["customName"].as<String>();
    room = doc["room"].as<String>();
    state = (BTSpeakerState)doc["state"].as<int>();
    isConnected = doc["isConnected"].as<bool>();
    isConnecting = doc["isConnecting"].as<bool>();
    isPaired = doc["isPaired"].as<bool>();
    volume = doc["volume"].as<int>();
    isMuted = doc["isMuted"].as<bool>();
    isPlaying = doc["isPlaying"].as<bool>();
    rssi = doc["rssi"].as<int>();
    lastSeen = doc["lastSeen"].as<unsigned long>();
}

String BluetoothSpeaker::getConnectionInfo() const {
    String info = "Speaker: " + customName + "\n";
    info += "  BT Address: " + btAddress + "\n";
    info += "  BT Name: " + btName + "\n";
    info += "  State: ";

    switch (state) {
        case BT_SPEAKER_DISCONNECTED: info += "Disconnected"; break;
        case BT_SPEAKER_CONNECTING: info += "Connecting"; break;
        case BT_SPEAKER_CONNECTED: info += "Connected"; break;
        case BT_SPEAKER_PLAYING: info += "Playing"; break;
        case BT_SPEAKER_PAUSED: info += "Paused"; break;
        case BT_SPEAKER_ERROR: info += "Error"; break;
    }

    info += "\n  RSSI: " + String(rssi) + " dBm\n";
    info += "  Volume: " + String(volume) + "%\n";
    info += "  Muted: " + String(isMuted ? "Yes" : "No") + "\n";

    return info;
}

bool BluetoothSpeaker::isHealthy() const {
    // Consider speaker healthy if:
    // - Connected or connecting
    // - Last seen within 60 seconds
    // - Connection attempts < 5

    if (state == BT_SPEAKER_ERROR) return false;
    if (connectionAttempts >= 5) return false;

    unsigned long timeSinceLastSeen = millis() - lastSeen;
    if (timeSinceLastSeen > 60000) return false;  // 60 seconds

    return (isConnected || isConnecting);
}

unsigned long BluetoothSpeaker::getTimeSinceLastSeen() const {
    return millis() - lastSeen;
}
