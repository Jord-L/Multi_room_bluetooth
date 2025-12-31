/**
 * ESP32 Multi-Room Bluetooth Hub - Hub Manager Implementation
 */

#include "HubManager.h"
#include <WiFi.h>
#include <ArduinoJson.h>

HubManager::HubManager() {
    macAddress = "";
    hubId = "";
    streaming = false;
}

bool HubManager::begin() {
    // Get MAC address and generate hub ID
    macAddress = getMacAddress();
    hubId = generateHubId();

    // Load saved settings or use defaults
    if (!loadSettings()) {
        Serial.println("[HubMgr] No saved settings, using defaults");
        resetToDefaults();
        saveSettings();
    }

    Serial.println("[HubMgr] Hub Manager initialized");
    Serial.println("[HubMgr] Hub ID: " + hubId);
    Serial.println("[HubMgr] Hub Name: " + getCustomName());

    return true;
}

String HubManager::generateHubId() {
    String mac = getMacAddress();
    mac.replace(":", "");
    return "HUB_" + mac;
}

String HubManager::getMacAddress() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

String HubManager::getHubId() {
    return hubId;
}

String HubManager::getCustomName() {
    return String(settings.customName);
}

String HubManager::getLocation() {
    return String(settings.location);
}

String HubManager::getIPAddress() {
    return WiFi.localIP().toString();
}

String HubManager::getFirmwareVersion() {
    return String(FIRMWARE_VERSION);
}

int HubManager::getSignalStrength() {
    return WiFi.RSSI();
}

void HubManager::setCustomName(const String& name) {
    strncpy(settings.customName, name.c_str(), sizeof(settings.customName) - 1);
    settings.customName[sizeof(settings.customName) - 1] = '\0';
    saveSettings();
    Serial.println("[HubMgr] Hub renamed to: " + name);
}

void HubManager::setLocation(const String& location) {
    strncpy(settings.location, location.c_str(), sizeof(settings.location) - 1);
    settings.location[sizeof(settings.location) - 1] = '\0';
    saveSettings();
    Serial.println("[HubMgr] Hub location set to: " + location);
}

bool HubManager::getPowerState() {
    return settings.powerState;
}

void HubManager::setPowerState(bool state) {
    settings.powerState = state;
    saveSettings();
    Serial.println("[HubMgr] Power state: " + String(state ? "ON" : "OFF"));
}

int HubManager::getMasterVolume() {
    return settings.masterVolume;
}

void HubManager::setMasterVolume(int volume) {
    if (volume < VOLUME_MIN) volume = VOLUME_MIN;
    if (volume > VOLUME_MAX) volume = VOLUME_MAX;

    settings.masterVolume = volume;
    saveSettings();
    Serial.println("[HubMgr] Master volume set to " + String(volume) + "%");
}

bool HubManager::isMuted() {
    return settings.isMuted;
}

void HubManager::setMuted(bool muted) {
    settings.isMuted = muted;
    saveSettings();
    Serial.println("[HubMgr] Muted: " + String(muted ? "ON" : "OFF"));
}

void HubManager::toggleMute() {
    setMuted(!settings.isMuted);
}

AudioSourceType HubManager::getAudioSource() {
    return settings.audioSource;
}

void HubManager::setAudioSource(AudioSourceType source) {
    settings.audioSource = source;
    saveSettings();
    Serial.println("[HubMgr] Audio source changed to " + String((int)source));
}

bool HubManager::isStreaming() {
    return streaming;
}

void HubManager::setStreaming(bool state) {
    streaming = state;
    Serial.println("[HubMgr] Streaming: " + String(state ? "ACTIVE" : "STOPPED"));
}

String HubManager::getHubInfoJson() {
    StaticJsonDocument<512> doc;

    doc["hubId"] = hubId;
    doc["deviceType"] = DEVICE_TYPE;
    doc["customName"] = settings.customName;
    doc["location"] = settings.location;
    doc["ipAddress"] = getIPAddress();
    doc["firmwareVersion"] = FIRMWARE_VERSION;
    doc["hardwareVersion"] = HARDWARE_VERSION;
    doc["signalStrength"] = getSignalStrength();
    doc["macAddress"] = macAddress;
    doc["maxSpeakers"] = settings.maxConnectedSpeakers;
    doc["autoReconnect"] = settings.autoReconnect;

    String output;
    serializeJson(doc, output);
    return output;
}

String HubManager::getHubStatusJson() {
    StaticJsonDocument<512> doc;

    doc["hubId"] = hubId;
    doc["powerState"] = settings.powerState;
    doc["masterVolume"] = settings.masterVolume;
    doc["isMuted"] = settings.isMuted;
    doc["audioSource"] = (int)settings.audioSource;
    doc["isStreaming"] = streaming;
    doc["uptime"] = millis() / 1000;  // seconds

    String output;
    serializeJson(doc, output);
    return output;
}

bool HubManager::saveSettings() {
    preferences.begin(PREFS_NAMESPACE, false);

    preferences.putString("customName", settings.customName);
    preferences.putString("location", settings.location);
    preferences.putBool("powerState", settings.powerState);
    preferences.putInt("masterVolume", settings.masterVolume);
    preferences.putBool("isMuted", settings.isMuted);
    preferences.putUChar("audioSource", (uint8_t)settings.audioSource);
    preferences.putUChar("maxSpeakers", settings.maxConnectedSpeakers);
    preferences.putBool("autoReconnect", settings.autoReconnect);

    preferences.end();

    Serial.println("[HubMgr] Settings saved");
    return true;
}

bool HubManager::loadSettings() {
    preferences.begin(PREFS_NAMESPACE, true);

    // Check if settings exist
    if (!preferences.isKey("customName")) {
        preferences.end();
        return false;
    }

    String customName = preferences.getString("customName", "My Hub");
    strncpy(settings.customName, customName.c_str(), sizeof(settings.customName) - 1);

    String location = preferences.getString("location", "Home");
    strncpy(settings.location, location.c_str(), sizeof(settings.location) - 1);

    settings.powerState = preferences.getBool("powerState", true);
    settings.masterVolume = preferences.getInt("masterVolume", VOLUME_DEFAULT);
    settings.isMuted = preferences.getBool("isMuted", false);
    settings.audioSource = (AudioSourceType)preferences.getUChar("audioSource", AUDIO_SOURCE_MOBILE_APP);
    settings.maxConnectedSpeakers = preferences.getUChar("maxSpeakers", BT_MAX_CONNECTED_SPEAKERS);
    settings.autoReconnect = preferences.getBool("autoReconnect", true);

    preferences.end();

    Serial.println("[HubMgr] Settings loaded");
    return true;
}

void HubManager::resetToDefaults() {
    // Reset to default settings
    strncpy(settings.customName, "My Hub", sizeof(settings.customName));
    strncpy(settings.location, "Home", sizeof(settings.location));
    settings.powerState = true;
    settings.masterVolume = VOLUME_DEFAULT;
    settings.isMuted = false;
    settings.audioSource = AUDIO_SOURCE_MOBILE_APP;
    settings.maxConnectedSpeakers = BT_MAX_CONNECTED_SPEAKERS;
    settings.autoReconnect = true;

    Serial.println("[HubMgr] Settings reset to defaults");
}

void HubManager::identify() {
    // Flash LED to identify hub
    #if LED_STATUS_PIN >= 0
    for (int i = 0; i < 6; i++) {
        digitalWrite(LED_STATUS_PIN, HIGH);
        delay(100);
        digitalWrite(LED_STATUS_PIN, LOW);
        delay(100);
    }
    #endif
    Serial.println("[HubMgr] Identify request - LED flashed");
}
