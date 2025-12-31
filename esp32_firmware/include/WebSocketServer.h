/**
 * ESP32 Multi-Room Bluetooth Hub - WebSocket Server
 * Phase 1: Real-time Communication
 *
 * Handles WebSocket connections for low-latency control and status updates
 */

#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <Arduino.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "config.h"

// Forward declarations
class HubManager;
class BluetoothSpeakerManager;
class AudioStreamReceiver;

class WebSocketServer {
public:
    WebSocketServer(HubManager& hubMgr, BluetoothSpeakerManager* btMgr, AudioStreamReceiver* audioRx);

    // Initialization
    bool begin();

    // Server control
    void loop();
    void stop();

    // Client management
    int getClientCount();
    void disconnectAll();

    // Message handling
    void broadcastStatus();
    void broadcastMessage(const String& message);
    void sendToClient(uint8_t clientNum, const String& message);

    // Heartbeat/Ping
    void sendHeartbeat();
    unsigned long getLastHeartbeat();

private:
    WebSocketsServer* wsServer;
    HubManager& hubManager;
    BluetoothSpeakerManager* btSpeakerManager;
    AudioStreamReceiver* audioReceiver;
    unsigned long lastHeartbeat;

    // Event handlers
    static void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    static WebSocketServer* instance;  // For static callback

    // Message processors
    void handleMessage(uint8_t clientNum, const String& message);
    void processCommand(uint8_t clientNum, JsonDocument& doc);

    // Hub command handlers
    void handleGetHubInfoCommand(uint8_t clientNum);
    void handleGetHubStatusCommand(uint8_t clientNum);
    void handleSetHubNameCommand(uint8_t clientNum, JsonDocument& doc);
    void handleSetHubLocationCommand(uint8_t clientNum, JsonDocument& doc);
    void handleSetMasterVolumeCommand(uint8_t clientNum, JsonDocument& doc);
    void handleSetMuteCommand(uint8_t clientNum, JsonDocument& doc);
    void handleSetPowerCommand(uint8_t clientNum, JsonDocument& doc);
    void handleIdentifyCommand(uint8_t clientNum);

    // Bluetooth speaker command handlers
    void handleStartBTDiscoveryCommand(uint8_t clientNum, JsonDocument& doc);
    void handleStopBTDiscoveryCommand(uint8_t clientNum);
    void handleGetDiscoveredSpeakersCommand(uint8_t clientNum);
    void handleGetSpeakersStatusCommand(uint8_t clientNum);
    void handleConnectBTSpeakerCommand(uint8_t clientNum, JsonDocument& doc);
    void handleDisconnectBTSpeakerCommand(uint8_t clientNum, JsonDocument& doc);
    void handleRemoveBTSpeakerCommand(uint8_t clientNum, JsonDocument& doc);
    void handleSetBTSpeakerVolumeCommand(uint8_t clientNum, JsonDocument& doc);
    void handleSetBTSpeakerMutedCommand(uint8_t clientNum, JsonDocument& doc);
    void handleSetBTSpeakerNameCommand(uint8_t clientNum, JsonDocument& doc);
    void handleSetBTSpeakerRoomCommand(uint8_t clientNum, JsonDocument& doc);

    // Audio streaming command handlers
    void handleStartAudioStreamCommand(uint8_t clientNum);
    void handleStopAudioStreamCommand(uint8_t clientNum);
    void handleGetAudioStreamInfoCommand(uint8_t clientNum);

    // Response builders
    String buildSuccessResponse(const String& command, const String& message = "");
    String buildErrorResponse(const String& command, const String& error);
    String buildStatusUpdate();
};

#endif // WEBSOCKET_SERVER_H
