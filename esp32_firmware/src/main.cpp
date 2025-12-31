/**
 * ESP32 Multi-Room Bluetooth Hub - Main Firmware
 * Phase 1: Bluetooth Hub Architecture
 *
 * ESP32 Hub receives audio from mobile app via WiFi and
 * forwards it to multiple Bluetooth speakers
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

#include "config.h"
#include "HubManager.h"
#include "NetworkManager.h"
#include "WebSocketServer.h"
#include "BluetoothSpeakerManager.h"
#include "AudioStreamReceiver.h"

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================
HubManager hubManager;
NetworkManager networkManager;
WebSocketServer* wsServer = nullptr;
BluetoothSpeakerManager* btSpeakerManager = nullptr;
AudioStreamReceiver* audioReceiver = nullptr;

// ============================================================================
// TIMING VARIABLES
// ============================================================================
unsigned long lastHeartbeat = 0;
unsigned long lastStatusBroadcast = 0;
unsigned long lastNetworkCheck = 0;
unsigned long lastConnectionCheck = 0;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
void setupOTA();
void handleOTA();
void sendHeartbeat();
void broadcastStatus();
void setupStatusLED();
void updateStatusLED();
void routeAudio();

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    // Initialize serial communication
    #if DEBUG_SERIAL_ENABLED
    Serial.begin(DEBUG_BAUD_RATE);
    delay(100);
    Serial.println();
    Serial.println("=========================================");
    Serial.println("ESP32 Multi-Room Bluetooth Hub");
    Serial.println("Phase 1: Bluetooth Hub Architecture");
    Serial.println("Firmware Version: " + String(FIRMWARE_VERSION));
    Serial.println("=========================================");
    #endif

    // Setup status LED
    setupStatusLED();

    // Initialize hub manager
    Serial.println("[SETUP] Initializing Hub Manager...");
    if (!hubManager.begin()) {
        Serial.println("[ERROR] Failed to initialize Hub Manager!");
        delay(5000);
        ESP.restart();
    }
    Serial.println("[SETUP] Hub ID: " + hubManager.getHubId());
    Serial.println("[SETUP] Hub Name: " + hubManager.getCustomName());

    // Initialize network manager
    Serial.println("[SETUP] Initializing Network Manager...");
    if (!networkManager.begin(hubManager.getHubId(), hubManager.getCustomName())) {
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

    // Start mDNS service (broadcasts as hub)
    Serial.println("[SETUP] Starting mDNS service...");
    String hostname = String(DEVICE_NAME_PREFIX) + "_" + hubManager.getHubId();
    if (networkManager.startMDNS(hostname)) {
        Serial.println("[SETUP] mDNS started: " + hostname + ".local");
        networkManager.updateMDNSRecords(FIRMWARE_VERSION);
    } else {
        Serial.println("[ERROR] Failed to start mDNS service!");
    }

    // Initialize Bluetooth speaker manager
    Serial.println("[SETUP] Initializing Bluetooth Speaker Manager...");
    btSpeakerManager = new BluetoothSpeakerManager();
    if (!btSpeakerManager->begin()) {
        Serial.println("[ERROR] Failed to initialize Bluetooth Speaker Manager!");
    } else {
        Serial.println("[SETUP] Bluetooth Speaker Manager initialized");
        Serial.println("[SETUP] Max simultaneous speakers: " + String(BT_MAX_CONNECTED_SPEAKERS));
    }

    // Initialize audio stream receiver
    Serial.println("[SETUP] Initializing Audio Stream Receiver...");
    audioReceiver = new AudioStreamReceiver();
    if (!audioReceiver->begin()) {
        Serial.println("[ERROR] Failed to initialize Audio Stream Receiver!");
    } else {
        Serial.println("[SETUP] Audio Stream Receiver initialized");
        Serial.println("[SETUP] Listening on UDP port " + String(AUDIO_STREAM_PORT));
    }

    // Initialize WebSocket server
    Serial.println("[SETUP] Initializing WebSocket Server...");
    wsServer = new WebSocketServer(hubManager, btSpeakerManager, audioReceiver);
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
    Serial.println("=========================================");
    Serial.println("Network Information:");
    Serial.println("  IP Address: " + networkManager.getIPAddress());
    Serial.println("  SSID: " + networkManager.getSSID());
    Serial.println("  Signal Strength: " + String(networkManager.getSignalStrength()) + " dBm");
    Serial.println("  MAC Address: " + networkManager.getMacAddress());
    Serial.println("=========================================");

    // Display hub settings
    Serial.println("Hub Settings:");
    Serial.println("  Master Volume: " + String(hubManager.getMasterVolume()) + "%");
    Serial.println("  Muted: " + String(hubManager.isMuted() ? "Yes" : "No"));
    Serial.println("  Power: " + String(hubManager.getPowerState() ? "On" : "Off"));
    Serial.println("  Audio Source: Mobile App");
    Serial.println("=========================================");

    Serial.println("[SETUP] Initialization complete!");
    Serial.println("Ready to accept connections.");
    Serial.println("Waiting for mobile app to connect...");
    Serial.println("=========================================");

    // Flash LED to indicate ready
    hubManager.identify();
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
    unsigned long currentMillis = millis();

    // Handle OTA updates
    #if FEATURE_OTA_UPDATES
    handleOTA();
    #endif

    // Check network connection (every 10 seconds)
    if (currentMillis - lastNetworkCheck >= 10000) {
        networkManager.checkConnection();
        lastNetworkCheck = currentMillis;
    }

    // Handle WebSocket server
    if (wsServer != nullptr) {
        wsServer->loop();

        // Send heartbeat (every 30 seconds)
        if (currentMillis - lastHeartbeat >= HEARTBEAT_INTERVAL) {
            sendHeartbeat();
            lastHeartbeat = currentMillis;
        }

        // Broadcast status updates (every 5 seconds)
        if (currentMillis - lastStatusBroadcast >= 5000) {
            broadcastStatus();
            lastStatusBroadcast = currentMillis;
        }
    }

    // Handle audio stream receiver
    if (audioReceiver != nullptr) {
        audioReceiver->loop();
    }

    // Route audio from WiFi to Bluetooth speakers
    if (hubManager.getPowerState() && !hubManager.isMuted()) {
        routeAudio();
    }

    // Monitor Bluetooth speaker connections (every 10 seconds)
    if (btSpeakerManager != nullptr && currentMillis - lastConnectionCheck >= 10000) {
        btSpeakerManager->checkConnections();

        if (hubManager.getPowerState()) {
            btSpeakerManager->attemptReconnections();
        }

        lastConnectionCheck = currentMillis;
    }

    // Update status LED
    updateStatusLED();

    // Small delay to prevent watchdog timer issues
    delay(1);
}

// ============================================================================
// AUDIO ROUTING
// ============================================================================
void routeAudio() {
    // Check if we have audio data and connected speakers
    if (audioReceiver == nullptr || btSpeakerManager == nullptr) {
        return;
    }

    if (!audioReceiver->hasAudioData()) {
        return;  // No audio data available
    }

    if (btSpeakerManager->getConnectedCount() == 0) {
        return;  // No speakers connected
    }

    // Read audio from WiFi stream
    uint8_t audioBuffer[AUDIO_BUFFER_SIZE];
    size_t length = audioReceiver->readAudioData(audioBuffer, AUDIO_BUFFER_SIZE);

    if (length > 0) {
        // Forward audio to Bluetooth speakers
        btSpeakerManager->writeAudioData(audioBuffer, length);

        #if CORE_DEBUG_LEVEL >= 5
        static unsigned long lastDebug = 0;
        if (millis() - lastDebug > 5000) {  // Debug every 5 seconds
            Serial.println("[AUDIO] Routing: " + String(length) + " bytes to " +
                         String(btSpeakerManager->getConnectedCount()) + " speaker(s)");
            lastDebug = millis();
        }
        #endif
    }
}

// ============================================================================
// OTA SETUP
// ============================================================================
void setupOTA() {
    String hostname = String(OTA_HOSTNAME_PREFIX) + "-" + hubManager.getHubId();
    ArduinoOTA.setHostname(hostname.c_str());
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.setPort(OTA_PORT);

    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("[OTA] Start updating " + type);

        // Stop audio during update
        if (audioReceiver != nullptr) {
            audioReceiver->stopReceiving();
        }

        if (btSpeakerManager != nullptr) {
            btSpeakerManager->stopStreaming();
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

        #if CORE_DEBUG_LEVEL >= 4
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

        #if CORE_DEBUG_LEVEL >= 5
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

    // Blink pattern based on state
    int blinkInterval;

    if (!networkManager.isConnected()) {
        blinkInterval = LED_BLINK_FAST;  // Fast blink = no WiFi
    } else if (btSpeakerManager != nullptr && btSpeakerManager->getConnectedCount() > 0) {
        blinkInterval = LED_BLINK_SLOW;  // Slow blink = speakers connected
    } else {
        blinkInterval = LED_BLINK_SLOW * 2;  // Very slow = WiFi only
    }

    if (currentMillis - lastBlink >= blinkInterval) {
        ledState = !ledState;
        digitalWrite(LED_STATUS_PIN, ledState);
        lastBlink = currentMillis;
    }
    #endif
}
