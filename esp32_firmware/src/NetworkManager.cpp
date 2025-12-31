/**
 * ESP32 Multi-Room Speaker System - Network Manager Implementation
 */

#include "NetworkManager.h"

NetworkManager::NetworkManager() {
    mdnsStarted = false;
    lastReconnectAttempt = 0;
    reconnectDelay = RECONNECT_DELAY_MIN;
}

bool NetworkManager::begin(const String& devId, const String& devName) {
    deviceId = devId;
    deviceName = devName;
    hostname = String(DEVICE_NAME_PREFIX) + "_" + deviceId;

    // Set WiFi mode
    WiFi.mode(WIFI_STA);

    // Set hostname
    WiFi.setHostname(hostname.c_str());

    return true;
}

bool NetworkManager::connectWiFi() {
    // Configure WiFiManager
    wifiManager.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT);
    wifiManager.setConnectTimeout(CONNECTION_TIMEOUT / 1000);

    // Try to connect to saved WiFi credentials
    Serial.println("[NetMgr] Attempting to connect to WiFi...");

    if (wifiManager.autoConnect(WIFI_PORTAL_AP_NAME)) {
        Serial.println("[NetMgr] Connected to WiFi!");
        Serial.println("[NetMgr] IP Address: " + WiFi.localIP().toString());
        Serial.println("[NetMgr] SSID: " + WiFi.SSID());
        onWiFiConnected();
        return true;
    } else {
        Serial.println("[NetMgr] Failed to connect to WiFi");
        return false;
    }
}

bool NetworkManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void NetworkManager::disconnect() {
    WiFi.disconnect();
}

String NetworkManager::getIPAddress() {
    return WiFi.localIP().toString();
}

int NetworkManager::getSignalStrength() {
    return WiFi.RSSI();
}

bool NetworkManager::startMDNS(const String& host) {
    hostname = host;

    if (!MDNS.begin(hostname.c_str())) {
        Serial.println("[NetMgr] Error starting mDNS");
        return false;
    }

    // Add service
    MDNS.addService(MDNS_SERVICE_NAME, MDNS_PROTOCOL, MDNS_PORT);

    mdnsStarted = true;
    return true;
}

void NetworkManager::updateMDNSRecords(const String& firmwareVersion) {
    if (!mdnsStarted) return;

    // Add TXT records for device discovery
    MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "deviceId", deviceId);
    MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "name", deviceName);
    MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "version", firmwareVersion);
    MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "ip", WiFi.localIP().toString());
    MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "mac", WiFi.macAddress());
}

void NetworkManager::stopMDNS() {
    if (mdnsStarted) {
        MDNS.end();
        mdnsStarted = false;
    }
}

void NetworkManager::checkConnection() {
    if (!isConnected()) {
        Serial.println("[NetMgr] WiFi connection lost");
        handleReconnection();
    }
}

void NetworkManager::handleReconnection() {
    unsigned long currentMillis = millis();

    // Check if enough time has passed since last reconnection attempt
    if (currentMillis - lastReconnectAttempt >= reconnectDelay) {
        Serial.println("[NetMgr] Attempting to reconnect to WiFi...");

        WiFi.disconnect();
        WiFi.reconnect();

        lastReconnectAttempt = currentMillis;

        // Increase backoff delay
        reconnectDelay = calculateBackoff();

        // Wait a bit to see if connection succeeds
        delay(CONNECTION_TIMEOUT);

        if (isConnected()) {
            Serial.println("[NetMgr] Reconnected successfully!");
            onWiFiConnected();
            // Reset backoff delay
            reconnectDelay = RECONNECT_DELAY_MIN;
        } else {
            Serial.println("[NetMgr] Reconnection failed, will retry in " +
                         String(reconnectDelay / 1000) + " seconds");
        }
    }
}

unsigned int NetworkManager::calculateBackoff() {
    // Exponential backoff with max limit
    unsigned int newDelay = reconnectDelay * 2;
    if (newDelay > RECONNECT_DELAY_MAX) {
        newDelay = RECONNECT_DELAY_MAX;
    }
    return newDelay;
}

String NetworkManager::getSSID() {
    return WiFi.SSID();
}

String NetworkManager::getMacAddress() {
    return WiFi.macAddress();
}

String NetworkManager::getNetworkInfoJson() {
    StaticJsonDocument<256> doc;

    doc["connected"] = isConnected();
    doc["ssid"] = getSSID();
    doc["ipAddress"] = getIPAddress();
    doc["macAddress"] = getMacAddress();
    doc["signalStrength"] = getSignalStrength();
    doc["hostname"] = hostname;

    String output;
    serializeJson(doc, output);
    return output;
}

void NetworkManager::startConfigPortal() {
    Serial.println("[NetMgr] Starting configuration portal...");
    Serial.println("[NetMgr] Connect to WiFi network: " + String(WIFI_PORTAL_AP_NAME));
    wifiManager.startConfigPortal(WIFI_PORTAL_AP_NAME);
}

bool NetworkManager::isConfigPortalActive() {
    // Check if config portal is running
    return !isConnected();
}

void NetworkManager::onWiFiConnected() {
    Serial.println("[NetMgr] WiFi connected event");
    // Reset reconnection delay
    reconnectDelay = RECONNECT_DELAY_MIN;
}

void NetworkManager::onWiFiDisconnected() {
    Serial.println("[NetMgr] WiFi disconnected event");
}
