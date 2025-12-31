/**
 * ESP32 Multi-Room Speaker System - Device Manager Implementation
 */

#include "DeviceManager.h"
#include <WiFi.h>
#include <ArduinoJson.h>

DeviceManager::DeviceManager() {
    macAddress = "";
    deviceId = "";
}

bool DeviceManager::begin() {
    // Get MAC address and generate device ID
    macAddress = getMacAddress();
    deviceId = generateDeviceId();

    // Load saved settings or use defaults
    if (!loadSettings()) {
        Serial.println("[DeviceMgr] No saved settings, using defaults");
        resetToDefaults();
        saveSettings();
    }

    return true;
}

String DeviceManager::generateDeviceId() {
    String mac = getMacAddress();
    mac.replace(":", "");
    return "ESP32_" + mac;
}

String DeviceManager::getMacAddress() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

String DeviceManager::getDeviceId() {
    return deviceId;
}

String DeviceManager::getCustomName() {
    return String(settings.customName);
}

String DeviceManager::getRoom() {
    return String(settings.room);
}

String DeviceManager::getIPAddress() {
    return WiFi.localIP().toString();
}

String DeviceManager::getFirmwareVersion() {
    return String(FIRMWARE_VERSION);
}

int DeviceManager::getSignalStrength() {
    return WiFi.RSSI();
}

void DeviceManager::setCustomName(const String& name) {
    strncpy(settings.customName, name.c_str(), sizeof(settings.customName) - 1);
    settings.customName[sizeof(settings.customName) - 1] = '\0';
    saveSettings();
}

void DeviceManager::setRoom(const String& room) {
    strncpy(settings.room, room.c_str(), sizeof(settings.room) - 1);
    settings.room[sizeof(settings.room) - 1] = '\0';
    saveSettings();
}

bool DeviceManager::getPowerState() {
    return settings.powerState;
}

void DeviceManager::setPowerState(bool state) {
    settings.powerState = state;
    saveSettings();
}

uint8_t DeviceManager::getVolume() {
    return settings.volume;
}

void DeviceManager::setVolume(uint8_t volume) {
    if (volume > VOLUME_MAX) volume = VOLUME_MAX;
    settings.volume = volume;
    saveSettings();
}

bool DeviceManager::isMuted() {
    return settings.isMuted;
}

void DeviceManager::setMuted(bool muted) {
    settings.isMuted = muted;
    saveSettings();
}

void DeviceManager::toggleMute() {
    settings.isMuted = !settings.isMuted;
    saveSettings();
}

AudioSource DeviceManager::getAudioSource() {
    return settings.audioSource;
}

void DeviceManager::setAudioSource(AudioSource source) {
    settings.audioSource = source;
    saveSettings();
}

EQPreset DeviceManager::getEQPreset() {
    return settings.eqPreset;
}

void DeviceManager::setEQPreset(EQPreset preset) {
    settings.eqPreset = preset;
    saveSettings();
}

int8_t DeviceManager::getBassBoost() {
    return settings.bassBoost;
}

void DeviceManager::setBassBoost(int8_t level) {
    if (level < EQ_MIN) level = EQ_MIN;
    if (level > EQ_MAX) level = EQ_MAX;
    settings.bassBoost = level;
    saveSettings();
}

int8_t DeviceManager::getTrebleAdjust() {
    return settings.trebleAdjust;
}

void DeviceManager::setTrebleAdjust(int8_t level) {
    if (level < EQ_MIN) level = EQ_MIN;
    if (level > EQ_MAX) level = EQ_MAX;
    settings.trebleAdjust = level;
    saveSettings();
}

void DeviceManager::addToGroup(const String& groupName) {
    // Phase 1: Basic implementation - store in preferences
    // Future: More sophisticated group management
    preferences.begin(PREFS_NAMESPACE, false);
    preferences.putString("groups", groupName);
    preferences.end();
}

void DeviceManager::removeFromGroup(const String& groupName) {
    // Phase 1: Basic implementation
    preferences.begin(PREFS_NAMESPACE, false);
    preferences.remove("groups");
    preferences.end();
}

String DeviceManager::getGroups() {
    preferences.begin(PREFS_NAMESPACE, true);
    String groups = preferences.getString("groups", "");
    preferences.end();
    return groups;
}

String DeviceManager::getDeviceInfoJson() {
    StaticJsonDocument<512> doc;

    doc["deviceId"] = deviceId;
    doc["customName"] = settings.customName;
    doc["room"] = settings.room;
    doc["ipAddress"] = getIPAddress();
    doc["firmwareVersion"] = FIRMWARE_VERSION;
    doc["hardwareVersion"] = HARDWARE_VERSION;
    doc["signalStrength"] = getSignalStrength();
    doc["macAddress"] = macAddress;

    String output;
    serializeJson(doc, output);
    return output;
}

String DeviceManager::getDeviceStatusJson() {
    StaticJsonDocument<512> doc;

    doc["deviceId"] = deviceId;
    doc["powerState"] = settings.powerState;
    doc["volume"] = settings.volume;
    doc["isMuted"] = settings.isMuted;
    doc["audioSource"] = (int)settings.audioSource;
    doc["eqPreset"] = (int)settings.eqPreset;
    doc["bassBoost"] = settings.bassBoost;
    doc["trebleAdjust"] = settings.trebleAdjust;
    doc["groups"] = getGroups();

    String output;
    serializeJson(doc, output);
    return output;
}

bool DeviceManager::saveSettings() {
    preferences.begin(PREFS_NAMESPACE, false);

    preferences.putString("customName", settings.customName);
    preferences.putString("room", settings.room);
    preferences.putBool("powerState", settings.powerState);
    preferences.putUChar("volume", settings.volume);
    preferences.putBool("isMuted", settings.isMuted);
    preferences.putUChar("audioSource", (uint8_t)settings.audioSource);
    preferences.putUChar("eqPreset", (uint8_t)settings.eqPreset);
    preferences.putChar("bassBoost", settings.bassBoost);
    preferences.putChar("trebleAdjust", settings.trebleAdjust);

    preferences.end();
    return true;
}

bool DeviceManager::loadSettings() {
    preferences.begin(PREFS_NAMESPACE, true);

    // Check if settings exist
    if (!preferences.isKey("customName")) {
        preferences.end();
        return false;
    }

    String customName = preferences.getString("customName", "Speaker");
    strncpy(settings.customName, customName.c_str(), sizeof(settings.customName) - 1);

    String room = preferences.getString("room", "Unknown");
    strncpy(settings.room, room.c_str(), sizeof(settings.room) - 1);

    settings.powerState = preferences.getBool("powerState", true);
    settings.volume = preferences.getUChar("volume", VOLUME_DEFAULT);
    settings.isMuted = preferences.getBool("isMuted", false);
    settings.audioSource = (AudioSource)preferences.getUChar("audioSource", SOURCE_BLUETOOTH);
    settings.eqPreset = (EQPreset)preferences.getUChar("eqPreset", EQ_FLAT);
    settings.bassBoost = preferences.getChar("bassBoost", EQ_DEFAULT);
    settings.trebleAdjust = preferences.getChar("trebleAdjust", EQ_DEFAULT);

    preferences.end();
    return true;
}

void DeviceManager::resetToDefaults() {
    // Reset to default settings
    strncpy(settings.customName, "Speaker", sizeof(settings.customName));
    strncpy(settings.room, "Unknown", sizeof(settings.room));
    settings.powerState = true;
    settings.volume = VOLUME_DEFAULT;
    settings.isMuted = false;
    settings.audioSource = SOURCE_BLUETOOTH;
    settings.eqPreset = EQ_FLAT;
    settings.bassBoost = EQ_DEFAULT;
    settings.trebleAdjust = EQ_DEFAULT;
}

void DeviceManager::identify() {
    // Flash LED or play sound to identify device
    #if LED_STATUS_PIN >= 0
    for (int i = 0; i < 6; i++) {
        digitalWrite(LED_STATUS_PIN, HIGH);
        delay(100);
        digitalWrite(LED_STATUS_PIN, LOW);
        delay(100);
    }
    #endif
    Serial.println("[DeviceMgr] Identify request - LED flashed");
}
