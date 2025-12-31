/**
 * ESP32 Multi-Room Speaker System - WebSocket Server
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

// Forward declaration
class DeviceManager;

class WebSocketServer {
public:
    WebSocketServer(DeviceManager* deviceMgr);

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
    DeviceManager* deviceManager;
    unsigned long lastHeartbeat;

    // Event handlers
    static void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    static WebSocketServer* instance;  // For static callback

    // Message processors
    void handleMessage(uint8_t clientNum, const String& message);
    void processCommand(uint8_t clientNum, JsonDocument& doc);

    // Command handlers
    void handleVolumeCommand(uint8_t clientNum, JsonDocument& doc);
    void handleMuteCommand(uint8_t clientNum, JsonDocument& doc);
    void handlePowerCommand(uint8_t clientNum, JsonDocument& doc);
    void handleSourceCommand(uint8_t clientNum, JsonDocument& doc);
    void handleEQCommand(uint8_t clientNum, JsonDocument& doc);
    void handleIdentifyCommand(uint8_t clientNum);
    void handleGetStatusCommand(uint8_t clientNum);
    void handleGetInfoCommand(uint8_t clientNum);

    // Response builders
    String buildSuccessResponse(const String& command, const String& message = "");
    String buildErrorResponse(const String& command, const String& error);
    String buildStatusUpdate();
};

#endif // WEBSOCKET_SERVER_H
