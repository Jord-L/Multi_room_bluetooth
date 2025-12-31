/**
 * ESP32 Multi-Room Speaker System - Network Manager
 * Phase 1: WiFi and mDNS Service Discovery
 *
 * Handles WiFi connection, mDNS broadcasting, and network status
 */

#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include "config.h"

class NetworkManager {
public:
    NetworkManager();

    // Initialization
    bool begin(const String& deviceId, const String& deviceName);

    // WiFi Management
    bool connectWiFi();
    bool isConnected();
    void disconnect();
    String getIPAddress();
    int getSignalStrength();  // Returns RSSI in dBm

    // mDNS Service Discovery
    bool startMDNS(const String& hostname);
    void updateMDNSRecords(const String& firmwareVersion);
    void stopMDNS();

    // Connection Monitoring
    void checkConnection();
    void handleReconnection();

    // Network Information
    String getSSID();
    String getMacAddress();
    String getNetworkInfoJson();

    // Configuration Portal
    void startConfigPortal();
    bool isConfigPortalActive();

private:
    WiFiManager wifiManager;
    String deviceId;
    String deviceName;
    String hostname;
    bool mdnsStarted;
    unsigned long lastReconnectAttempt;
    unsigned int reconnectDelay;

    // Helper methods
    void setupMDNSTxtRecords(const String& firmwareVersion);
    void onWiFiConnected();
    void onWiFiDisconnected();
    unsigned int calculateBackoff();
};

#endif // NETWORK_MANAGER_H
