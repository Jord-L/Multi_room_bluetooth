/**
 * ESP32 Multi-Room Speaker System - Main Firmware
 * Phase 1: Core Functionality & Foundation
 *
 * This is the main entry point for the ESP32 speaker firmware.
 * It initializes all modules and handles the main control loop.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

#include "config.h"
#include "DeviceManager.h"
#include "NetworkManager.h"
#include "WebSocketServer.h"
#include "AudioController.h"

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================
DeviceManager deviceManager;
NetworkManager networkManager;
WebSocketServer* wsServer = nullptr;
AudioController* audioController = nullptr;

// ============================================================================
// TIMING VARIABLES
// ============================================================================
unsigned long lastHeartbeat = 0;
unsigned long lastStatusBroadcast = 0;
unsigned long lastNetworkCheck = 0;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
void setupOTA();
void handleOTA();
void sendHeartbeat();
void broadcastStatus();
void setupStatusLED();
void updateStatusLED();

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    // Initialize serial communication
    #if DEBUG_SERIAL_ENABLED
    Serial.begin(DEBUG_BAUD_RATE);
    delay(100);
    Serial.println();
    Serial.println("=====================================");
    Serial.println("ESP32 Multi-Room Speaker System");
    Serial.println("Phase 1: Core Functionality");
    Serial.println("Firmware Version: " + String(FIRMWARE_VERSION));
    Serial.println("=====================================");
    #endif

    // Setup status LED
    setupStatusLED();

    // Initialize device manager
    Serial.println("[SETUP] Initializing Device Manager...");
    if (!deviceManager.begin()) {
        Serial.println("[ERROR] Failed to initialize Device Manager!");
        delay(5000);
        ESP.restart();
    }
    Serial.println("[SETUP] Device ID: " + deviceManager.getDeviceId());
    Serial.println("[SETUP] Device Name: " + deviceManager.getCustomName());

    // Initialize network manager
    Serial.println("[SETUP] Initializing Network Manager...");
    if (!networkManager.begin(deviceManager.getDeviceId(), deviceManager.getCustomName())) {
        Serial.println("[ERROR] Failed to initialize Network Manager!");
        delay(5000);
        ESP.restart();
    }

    // Connect to WiFi
    Serial.println("[SETUP] Connecting to WiFi...");
    if (!networkManager.connectWiFi()) {
        Serial.println("[WARN] WiFi connection failed, starting config portal...");
        networkManager.startConfigPortal();
        // Wait for configuration
        while (networkManager.isConfigPortalActive()) {
            delay(100);
        }
    }

    // Start mDNS service
    Serial.println("[SETUP] Starting mDNS service...");
    String hostname = String(DEVICE_NAME_PREFIX) + "_" + deviceManager.getDeviceId();
    if (networkManager.startMDNS(hostname)) {
        Serial.println("[SETUP] mDNS started: " + hostname + ".local");
        networkManager.updateMDNSRecords(FIRMWARE_VERSION);
    } else {
        Serial.println("[ERROR] Failed to start mDNS service!");
    }

    // Initialize audio controller
    Serial.println("[SETUP] Initializing Audio Controller...");
    audioController = new AudioController(&deviceManager);
    if (!audioController->begin()) {
        Serial.println("[ERROR] Failed to initialize Audio Controller!");
    }

    // Initialize WebSocket server
    Serial.println("[SETUP] Initializing WebSocket Server...");
    wsServer = new WebSocketServer(&deviceManager);
    if (!wsServer->begin()) {
        Serial.println("[ERROR] Failed to initialize WebSocket Server!");
    } else {
        Serial.println("[SETUP] WebSocket server listening on port " + String(WEBSOCKET_PORT));
    }

    // Setup OTA updates
    #if FEATURE_OTA_UPDATES
    Serial.println("[SETUP] Setting up OTA updates...");
    setupOTA();
    #endif

    // Display network information
    Serial.println("=====================================");
    Serial.println("Network Information:");
    Serial.println("  IP Address: " + networkManager.getIPAddress());
    Serial.println("  SSID: " + networkManager.getSSID());
    Serial.println("  Signal Strength: " + String(networkManager.getSignalStrength()) + " dBm");
    Serial.println("  MAC Address: " + networkManager.getMacAddress());
    Serial.println("=====================================");

    // Display audio settings
    Serial.println("Audio Settings:");
    Serial.println("  Volume: " + String(deviceManager.getVolume()) + "%");
    Serial.println("  Muted: " + String(deviceManager.isMuted() ? "Yes" : "No"));
    Serial.println("  Power: " + String(deviceManager.getPowerState() ? "On" : "Off"));
    Serial.println("=====================================");

    Serial.println("[SETUP] Initialization complete!");
    Serial.println("Ready to accept connections.");
    Serial.println("=====================================");

    // Flash LED to indicate ready
    deviceManager.identify();
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
    // Handle OTA updates
    #if FEATURE_OTA_UPDATES
    handleOTA();
    #endif

    // Check network connection
    unsigned long currentMillis = millis();
    if (currentMillis - lastNetworkCheck >= 10000) {  // Check every 10 seconds
        networkManager.checkConnection();
        lastNetworkCheck = currentMillis;
    }

    // Handle WebSocket server
    if (wsServer != nullptr) {
        wsServer->loop();

        // Send heartbeat
        if (currentMillis - lastHeartbeat >= HEARTBEAT_INTERVAL) {
            sendHeartbeat();
            lastHeartbeat = currentMillis;
        }

        // Broadcast status updates
        if (currentMillis - lastStatusBroadcast >= 5000) {  // Every 5 seconds
            broadcastStatus();
            lastStatusBroadcast = currentMillis;
        }
    }

    // Handle audio processing
    if (audioController != nullptr) {
        audioController->loop();
    }

    // Update status LED
    updateStatusLED();

    // Small delay to prevent watchdog timer issues
    delay(1);
}

// ============================================================================
// OTA SETUP
// ============================================================================
void setupOTA() {
    String hostname = String(OTA_HOSTNAME_PREFIX) + "-" + deviceManager.getDeviceId();
    ArduinoOTA.setHostname(hostname.c_str());
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.setPort(OTA_PORT);

    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("[OTA] Start updating " + type);

        // Stop audio during update
        if (audioController != nullptr) {
            audioController->setPower(false);
        }

        // Disconnect WebSocket clients
        if (wsServer != nullptr) {
            wsServer->disconnectAll();
        }
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Update complete!");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.begin();
    Serial.println("[OTA] OTA update service started");
}

// ============================================================================
// OTA HANDLER
// ============================================================================
void handleOTA() {
    ArduinoOTA.handle();
}

// ============================================================================
// HEARTBEAT
// ============================================================================
void sendHeartbeat() {
    if (wsServer != nullptr && wsServer->getClientCount() > 0) {
        wsServer->sendHeartbeat();
        #if DEBUG_SERIAL_ENABLED && CORE_DEBUG_LEVEL >= 4
        Serial.println("[HEARTBEAT] Sent to " + String(wsServer->getClientCount()) + " client(s)");
        #endif
    }
}

// ============================================================================
// STATUS BROADCAST
// ============================================================================
void broadcastStatus() {
    if (wsServer != nullptr && wsServer->getClientCount() > 0) {
        wsServer->broadcastStatus();
        #if DEBUG_SERIAL_ENABLED && CORE_DEBUG_LEVEL >= 5
        Serial.println("[STATUS] Broadcasted to " + String(wsServer->getClientCount()) + " client(s)");
        #endif
    }
}

// ============================================================================
// STATUS LED
// ============================================================================
void setupStatusLED() {
    #if LED_STATUS_PIN >= 0
    pinMode(LED_STATUS_PIN, OUTPUT);
    digitalWrite(LED_STATUS_PIN, LOW);
    #endif
}

void updateStatusLED() {
    #if LED_STATUS_PIN >= 0
    static unsigned long lastBlink = 0;
    static bool ledState = false;
    unsigned long currentMillis = millis();

    if (networkManager.isConnected()) {
        // Slow blink when connected
        if (currentMillis - lastBlink >= LED_BLINK_SLOW) {
            ledState = !ledState;
            digitalWrite(LED_STATUS_PIN, ledState);
            lastBlink = currentMillis;
        }
    } else {
        // Fast blink when disconnected
        if (currentMillis - lastBlink >= LED_BLINK_FAST) {
            ledState = !ledState;
            digitalWrite(LED_STATUS_PIN, ledState);
            lastBlink = currentMillis;
        }
    }
    #endif
}
