/**
 * ESP32 Multi-Room Speaker System - WebSocket Server Implementation
 */

#include "WebSocketServer.h"
#include "DeviceManager.h"

// Static instance pointer for callback
WebSocketServer* WebSocketServer::instance = nullptr;

WebSocketServer::WebSocketServer(DeviceManager* deviceMgr) {
    deviceManager = deviceMgr;
    wsServer = nullptr;
    lastHeartbeat = 0;
    instance = this;  // Set static instance for callbacks
}

bool WebSocketServer::begin() {
    wsServer = new WebSocketsServer(WEBSOCKET_PORT);

    if (wsServer == nullptr) {
        return false;
    }

    wsServer->begin();
    wsServer->onEvent(webSocketEvent);

    Serial.println("[WebSocket] Server started on port " + String(WEBSOCKET_PORT));
    return true;
}

void WebSocketServer::loop() {
    if (wsServer != nullptr) {
        wsServer->loop();
    }
}

void WebSocketServer::stop() {
    if (wsServer != nullptr) {
        wsServer->close();
    }
}

int WebSocketServer::getClientCount() {
    if (wsServer != nullptr) {
        return wsServer->connectedClients();
    }
    return 0;
}

void WebSocketServer::disconnectAll() {
    if (wsServer != nullptr) {
        wsServer->disconnect();
    }
}

void WebSocketServer::broadcastStatus() {
    if (wsServer != nullptr && getClientCount() > 0) {
        String status = buildStatusUpdate();
        wsServer->broadcastTXT(status);
    }
}

void WebSocketServer::broadcastMessage(const String& message) {
    if (wsServer != nullptr) {
        wsServer->broadcastTXT(message);
    }
}

void WebSocketServer::sendToClient(uint8_t clientNum, const String& message) {
    if (wsServer != nullptr) {
        wsServer->sendTXT(clientNum, message);
    }
}

void WebSocketServer::sendHeartbeat() {
    if (wsServer != nullptr && getClientCount() > 0) {
        StaticJsonDocument<128> doc;
        doc["type"] = "heartbeat";
        doc["timestamp"] = millis();

        String output;
        serializeJson(doc, output);
        wsServer->broadcastTXT(output);

        lastHeartbeat = millis();
    }
}

unsigned long WebSocketServer::getLastHeartbeat() {
    return lastHeartbeat;
}

// Static callback function
void WebSocketServer::webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (instance == nullptr) return;

    switch (type) {
        case WStype_DISCONNECTED:
            Serial.printf("[WebSocket] Client #%u disconnected\n", num);
            break;

        case WStype_CONNECTED: {
            IPAddress ip = instance->wsServer->remoteIP(num);
            Serial.printf("[WebSocket] Client #%u connected from %s\n", num, ip.toString().c_str());

            // Send initial status to newly connected client
            instance->sendToClient(num, instance->buildStatusUpdate());
            instance->sendToClient(num, instance->deviceManager->getDeviceInfoJson());
            break;
        }

        case WStype_TEXT:
            Serial.printf("[WebSocket] Received from #%u: %s\n", num, payload);
            instance->handleMessage(num, String((char*)payload));
            break;

        case WStype_BIN:
            Serial.printf("[WebSocket] Binary data from #%u (length: %u)\n", num, length);
            break;

        case WStype_ERROR:
            Serial.printf("[WebSocket] Error on client #%u\n", num);
            break;

        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            // Handle fragmented messages if needed
            break;

        case WStype_PING:
            // Ping is handled automatically
            break;

        case WStype_PONG:
            // Pong received
            break;
    }
}

void WebSocketServer::handleMessage(uint8_t clientNum, const String& message) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        Serial.println("[WebSocket] JSON parsing failed: " + String(error.c_str()));
        sendToClient(clientNum, buildErrorResponse("parse", "Invalid JSON"));
        return;
    }

    processCommand(clientNum, doc);
}

void WebSocketServer::processCommand(uint8_t clientNum, JsonDocument& doc) {
    if (!doc.containsKey("command")) {
        sendToClient(clientNum, buildErrorResponse("unknown", "No command specified"));
        return;
    }

    String command = doc["command"].as<String>();

    // Route command to appropriate handler
    if (command == "setVolume") {
        handleVolumeCommand(clientNum, doc);
    } else if (command == "setMute") {
        handleMuteCommand(clientNum, doc);
    } else if (command == "setPower") {
        handlePowerCommand(clientNum, doc);
    } else if (command == "setSource") {
        handleSourceCommand(clientNum, doc);
    } else if (command == "setEQ") {
        handleEQCommand(clientNum, doc);
    } else if (command == "identify") {
        handleIdentifyCommand(clientNum);
    } else if (command == "getStatus") {
        handleGetStatusCommand(clientNum);
    } else if (command == "getInfo") {
        handleGetInfoCommand(clientNum);
    } else {
        sendToClient(clientNum, buildErrorResponse(command, "Unknown command"));
    }
}

void WebSocketServer::handleVolumeCommand(uint8_t clientNum, JsonDocument& doc) {
    if (!doc.containsKey("volume")) {
        sendToClient(clientNum, buildErrorResponse("setVolume", "Missing volume parameter"));
        return;
    }

    uint8_t volume = doc["volume"].as<uint8_t>();
    deviceManager->setVolume(volume);

    sendToClient(clientNum, buildSuccessResponse("setVolume", "Volume set to " + String(volume)));
    broadcastStatus();  // Notify all clients
}

void WebSocketServer::handleMuteCommand(uint8_t clientNum, JsonDocument& doc) {
    if (!doc.containsKey("muted")) {
        sendToClient(clientNum, buildErrorResponse("setMute", "Missing muted parameter"));
        return;
    }

    bool muted = doc["muted"].as<bool>();
    deviceManager->setMuted(muted);

    sendToClient(clientNum, buildSuccessResponse("setMute", muted ? "Muted" : "Unmuted"));
    broadcastStatus();
}

void WebSocketServer::handlePowerCommand(uint8_t clientNum, JsonDocument& doc) {
    if (!doc.containsKey("power")) {
        sendToClient(clientNum, buildErrorResponse("setPower", "Missing power parameter"));
        return;
    }

    bool power = doc["power"].as<bool>();
    deviceManager->setPowerState(power);

    sendToClient(clientNum, buildSuccessResponse("setPower", power ? "Powered on" : "Powered off"));
    broadcastStatus();
}

void WebSocketServer::handleSourceCommand(uint8_t clientNum, JsonDocument& doc) {
    if (!doc.containsKey("source")) {
        sendToClient(clientNum, buildErrorResponse("setSource", "Missing source parameter"));
        return;
    }

    uint8_t source = doc["source"].as<uint8_t>();
    deviceManager->setAudioSource((AudioSource)source);

    sendToClient(clientNum, buildSuccessResponse("setSource", "Audio source changed"));
    broadcastStatus();
}

void WebSocketServer::handleEQCommand(uint8_t clientNum, JsonDocument& doc) {
    if (doc.containsKey("preset")) {
        uint8_t preset = doc["preset"].as<uint8_t>();
        deviceManager->setEQPreset((EQPreset)preset);
        sendToClient(clientNum, buildSuccessResponse("setEQ", "EQ preset changed"));
    }

    if (doc.containsKey("bass")) {
        int8_t bass = doc["bass"].as<int8_t>();
        deviceManager->setBassBoost(bass);
        sendToClient(clientNum, buildSuccessResponse("setEQ", "Bass adjusted"));
    }

    if (doc.containsKey("treble")) {
        int8_t treble = doc["treble"].as<int8_t>();
        deviceManager->setTrebleAdjust(treble);
        sendToClient(clientNum, buildSuccessResponse("setEQ", "Treble adjusted"));
    }

    broadcastStatus();
}

void WebSocketServer::handleIdentifyCommand(uint8_t clientNum) {
    deviceManager->identify();
    sendToClient(clientNum, buildSuccessResponse("identify", "Device identified"));
}

void WebSocketServer::handleGetStatusCommand(uint8_t clientNum) {
    sendToClient(clientNum, deviceManager->getDeviceStatusJson());
}

void WebSocketServer::handleGetInfoCommand(uint8_t clientNum) {
    sendToClient(clientNum, deviceManager->getDeviceInfoJson());
}

String WebSocketServer::buildSuccessResponse(const String& command, const String& message) {
    StaticJsonDocument<256> doc;
    doc["type"] = "response";
    doc["command"] = command;
    doc["success"] = true;
    if (message.length() > 0) {
        doc["message"] = message;
    }

    String output;
    serializeJson(doc, output);
    return output;
}

String WebSocketServer::buildErrorResponse(const String& command, const String& error) {
    StaticJsonDocument<256> doc;
    doc["type"] = "response";
    doc["command"] = command;
    doc["success"] = false;
    doc["error"] = error;

    String output;
    serializeJson(doc, output);
    return output;
}

String WebSocketServer::buildStatusUpdate() {
    StaticJsonDocument<512> doc;
    doc["type"] = "status";
    doc["deviceId"] = deviceManager->getDeviceId();
    doc["powerState"] = deviceManager->getPowerState();
    doc["volume"] = deviceManager->getVolume();
    doc["isMuted"] = deviceManager->isMuted();
    doc["audioSource"] = (int)deviceManager->getAudioSource();
    doc["eqPreset"] = (int)deviceManager->getEQPreset();
    doc["bassBoost"] = deviceManager->getBassBoost();
    doc["trebleAdjust"] = deviceManager->getTrebleAdjust();

    String output;
    serializeJson(doc, output);
    return output;
}
