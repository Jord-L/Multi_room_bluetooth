/**
 * ESP32 Multi-Room Bluetooth Hub - WebSocket Server Implementation
 */

#include "WebSocketServer.h"
#include "HubManager.h"
#include "BluetoothSpeakerManager.h"
#include "AudioStreamReceiver.h"

// Static instance pointer for callback
WebSocketServer* WebSocketServer::instance = nullptr;

WebSocketServer::WebSocketServer(HubManager& hubMgr, BluetoothSpeakerManager* btMgr, AudioStreamReceiver* audioRx)
    : hubManager(hubMgr) {
    btSpeakerManager = btMgr;
    audioReceiver = audioRx;
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

            // Send initial hub info and status to newly connected client
            instance->sendToClient(num, instance->hubManager.getHubInfoJson());
            instance->sendToClient(num, instance->buildStatusUpdate());

            // Send Bluetooth speakers status if manager is available
            if (instance->btSpeakerManager != nullptr) {
                instance->sendToClient(num, instance->btSpeakerManager->getSpeakersStatusJson());
            }
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
    StaticJsonDocument<1024> doc;
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
    // Hub commands
    if (command == "getHubInfo") {
        handleGetHubInfoCommand(clientNum);
    } else if (command == "getHubStatus") {
        handleGetHubStatusCommand(clientNum);
    } else if (command == "setHubName") {
        handleSetHubNameCommand(clientNum, doc);
    } else if (command == "setHubLocation") {
        handleSetHubLocationCommand(clientNum, doc);
    } else if (command == "setMasterVolume") {
        handleSetMasterVolumeCommand(clientNum, doc);
    } else if (command == "setMute") {
        handleSetMuteCommand(clientNum, doc);
    } else if (command == "setPower") {
        handleSetPowerCommand(clientNum, doc);
    } else if (command == "identify") {
        handleIdentifyCommand(clientNum);
    }
    // Bluetooth speaker commands
    else if (command == "startBTDiscovery") {
        handleStartBTDiscoveryCommand(clientNum, doc);
    } else if (command == "stopBTDiscovery") {
        handleStopBTDiscoveryCommand(clientNum);
    } else if (command == "getDiscoveredSpeakers") {
        handleGetDiscoveredSpeakersCommand(clientNum);
    } else if (command == "getSpeakersStatus") {
        handleGetSpeakersStatusCommand(clientNum);
    } else if (command == "connectBTSpeaker") {
        handleConnectBTSpeakerCommand(clientNum, doc);
    } else if (command == "disconnectBTSpeaker") {
        handleDisconnectBTSpeakerCommand(clientNum, doc);
    } else if (command == "removeBTSpeaker") {
        handleRemoveBTSpeakerCommand(clientNum, doc);
    } else if (command == "setBTSpeakerVolume") {
        handleSetBTSpeakerVolumeCommand(clientNum, doc);
    } else if (command == "setBTSpeakerMuted") {
        handleSetBTSpeakerMutedCommand(clientNum, doc);
    } else if (command == "setBTSpeakerName") {
        handleSetBTSpeakerNameCommand(clientNum, doc);
    } else if (command == "setBTSpeakerRoom") {
        handleSetBTSpeakerRoomCommand(clientNum, doc);
    }
    // Audio streaming commands
    else if (command == "startAudioStream") {
        handleStartAudioStreamCommand(clientNum);
    } else if (command == "stopAudioStream") {
        handleStopAudioStreamCommand(clientNum);
    } else if (command == "getAudioStreamInfo") {
        handleGetAudioStreamInfoCommand(clientNum);
    }
    else {
        sendToClient(clientNum, buildErrorResponse(command, "Unknown command"));
    }
}

// ============================================================================
// HUB COMMAND HANDLERS
// ============================================================================

void WebSocketServer::handleGetHubInfoCommand(uint8_t clientNum) {
    sendToClient(clientNum, hubManager.getHubInfoJson());
}

void WebSocketServer::handleGetHubStatusCommand(uint8_t clientNum) {
    sendToClient(clientNum, hubManager.getHubStatusJson());
}

void WebSocketServer::handleSetHubNameCommand(uint8_t clientNum, JsonDocument& doc) {
    if (!doc.containsKey("name")) {
        sendToClient(clientNum, buildErrorResponse("setHubName", "Missing name parameter"));
        return;
    }

    String name = doc["name"].as<String>();
    hubManager.setCustomName(name);

    sendToClient(clientNum, buildSuccessResponse("setHubName", "Hub renamed to " + name));
    broadcastStatus();
}

void WebSocketServer::handleSetHubLocationCommand(uint8_t clientNum, JsonDocument& doc) {
    if (!doc.containsKey("location")) {
        sendToClient(clientNum, buildErrorResponse("setHubLocation", "Missing location parameter"));
        return;
    }

    String location = doc["location"].as<String>();
    hubManager.setLocation(location);

    sendToClient(clientNum, buildSuccessResponse("setHubLocation", "Location set to " + location));
    broadcastStatus();
}

void WebSocketServer::handleSetMasterVolumeCommand(uint8_t clientNum, JsonDocument& doc) {
    if (!doc.containsKey("volume")) {
        sendToClient(clientNum, buildErrorResponse("setMasterVolume", "Missing volume parameter"));
        return;
    }

    int volume = doc["volume"].as<int>();
    hubManager.setMasterVolume(volume);

    // Also apply to Bluetooth speaker manager if available
    if (btSpeakerManager != nullptr) {
        btSpeakerManager->setGlobalVolume(volume);
    }

    sendToClient(clientNum, buildSuccessResponse("setMasterVolume", "Volume set to " + String(volume)));
    broadcastStatus();
}

void WebSocketServer::handleSetMuteCommand(uint8_t clientNum, JsonDocument& doc) {
    if (!doc.containsKey("muted")) {
        sendToClient(clientNum, buildErrorResponse("setMute", "Missing muted parameter"));
        return;
    }

    bool muted = doc["muted"].as<bool>();
    hubManager.setMuted(muted);

    // Also apply to Bluetooth speaker manager if available
    if (btSpeakerManager != nullptr) {
        btSpeakerManager->setMuted(muted);
    }

    sendToClient(clientNum, buildSuccessResponse("setMute", muted ? "Muted" : "Unmuted"));
    broadcastStatus();
}

void WebSocketServer::handleSetPowerCommand(uint8_t clientNum, JsonDocument& doc) {
    if (!doc.containsKey("power")) {
        sendToClient(clientNum, buildErrorResponse("setPower", "Missing power parameter"));
        return;
    }

    bool power = doc["power"].as<bool>();
    hubManager.setPowerState(power);

    if (!power && btSpeakerManager != nullptr) {
        // Stop streaming when powering off
        btSpeakerManager->stopStreaming();
    }

    sendToClient(clientNum, buildSuccessResponse("setPower", power ? "Powered on" : "Powered off"));
    broadcastStatus();
}

void WebSocketServer::handleIdentifyCommand(uint8_t clientNum) {
    hubManager.identify();
    sendToClient(clientNum, buildSuccessResponse("identify", "Hub identified"));
}

// ============================================================================
// BLUETOOTH SPEAKER COMMAND HANDLERS
// ============================================================================

void WebSocketServer::handleStartBTDiscoveryCommand(uint8_t clientNum, JsonDocument& doc) {
    if (btSpeakerManager == nullptr) {
        sendToClient(clientNum, buildErrorResponse("startBTDiscovery", "Bluetooth manager not available"));
        return;
    }

    unsigned int duration = doc.containsKey("duration") ? doc["duration"].as<unsigned int>() : 30;

    if (btSpeakerManager->startDiscovery(duration)) {
        sendToClient(clientNum, buildSuccessResponse("startBTDiscovery", "Discovery started for " + String(duration) + " seconds"));
    } else {
        sendToClient(clientNum, buildErrorResponse("startBTDiscovery", "Failed to start discovery"));
    }
}

void WebSocketServer::handleStopBTDiscoveryCommand(uint8_t clientNum) {
    if (btSpeakerManager == nullptr) {
        sendToClient(clientNum, buildErrorResponse("stopBTDiscovery", "Bluetooth manager not available"));
        return;
    }

    btSpeakerManager->stopDiscovery();
    sendToClient(clientNum, buildSuccessResponse("stopBTDiscovery", "Discovery stopped"));
}

void WebSocketServer::handleGetDiscoveredSpeakersCommand(uint8_t clientNum) {
    if (btSpeakerManager == nullptr) {
        sendToClient(clientNum, buildErrorResponse("getDiscoveredSpeakers", "Bluetooth manager not available"));
        return;
    }

    sendToClient(clientNum, btSpeakerManager->getDiscoveredSpeakersJson());
}

void WebSocketServer::handleGetSpeakersStatusCommand(uint8_t clientNum) {
    if (btSpeakerManager == nullptr) {
        sendToClient(clientNum, buildErrorResponse("getSpeakersStatus", "Bluetooth manager not available"));
        return;
    }

    sendToClient(clientNum, btSpeakerManager->getSpeakersStatusJson());
}

void WebSocketServer::handleConnectBTSpeakerCommand(uint8_t clientNum, JsonDocument& doc) {
    if (btSpeakerManager == nullptr) {
        sendToClient(clientNum, buildErrorResponse("connectBTSpeaker", "Bluetooth manager not available"));
        return;
    }

    if (!doc.containsKey("btAddress")) {
        sendToClient(clientNum, buildErrorResponse("connectBTSpeaker", "Missing btAddress parameter"));
        return;
    }

    String btAddress = doc["btAddress"].as<String>();

    // Check if connecting to multiple speakers
    if (doc.containsKey("addresses")) {
        JsonArray addresses = doc["addresses"].as<JsonArray>();
        std::vector<String> addressList;

        for (JsonVariant v : addresses) {
            addressList.push_back(v.as<String>());
        }

        if (btSpeakerManager->connectToMultipleSpeakers(addressList)) {
            sendToClient(clientNum, buildSuccessResponse("connectBTSpeaker", "Connecting to multiple speakers"));
        } else {
            sendToClient(clientNum, buildErrorResponse("connectBTSpeaker", "Failed to connect to all speakers"));
        }
    } else {
        if (btSpeakerManager->connectToSpeaker(btAddress)) {
            sendToClient(clientNum, buildSuccessResponse("connectBTSpeaker", "Connecting to speaker"));
        } else {
            sendToClient(clientNum, buildErrorResponse("connectBTSpeaker", "Failed to connect to speaker"));
        }
    }

    broadcastStatus();
}

void WebSocketServer::handleDisconnectBTSpeakerCommand(uint8_t clientNum, JsonDocument& doc) {
    if (btSpeakerManager == nullptr) {
        sendToClient(clientNum, buildErrorResponse("disconnectBTSpeaker", "Bluetooth manager not available"));
        return;
    }

    if (!doc.containsKey("btAddress")) {
        sendToClient(clientNum, buildErrorResponse("disconnectBTSpeaker", "Missing btAddress parameter"));
        return;
    }

    String btAddress = doc["btAddress"].as<String>();

    if (btSpeakerManager->disconnectFromSpeaker(btAddress)) {
        sendToClient(clientNum, buildSuccessResponse("disconnectBTSpeaker", "Speaker disconnected"));
    } else {
        sendToClient(clientNum, buildErrorResponse("disconnectBTSpeaker", "Failed to disconnect speaker"));
    }

    broadcastStatus();
}

void WebSocketServer::handleRemoveBTSpeakerCommand(uint8_t clientNum, JsonDocument& doc) {
    if (btSpeakerManager == nullptr) {
        sendToClient(clientNum, buildErrorResponse("removeBTSpeaker", "Bluetooth manager not available"));
        return;
    }

    if (!doc.containsKey("btAddress")) {
        sendToClient(clientNum, buildErrorResponse("removeBTSpeaker", "Missing btAddress parameter"));
        return;
    }

    String btAddress = doc["btAddress"].as<String>();

    if (btSpeakerManager->removeSpeaker(btAddress)) {
        sendToClient(clientNum, buildSuccessResponse("removeBTSpeaker", "Speaker removed"));
    } else {
        sendToClient(clientNum, buildErrorResponse("removeBTSpeaker", "Failed to remove speaker"));
    }

    broadcastStatus();
}

void WebSocketServer::handleSetBTSpeakerVolumeCommand(uint8_t clientNum, JsonDocument& doc) {
    if (btSpeakerManager == nullptr) {
        sendToClient(clientNum, buildErrorResponse("setBTSpeakerVolume", "Bluetooth manager not available"));
        return;
    }

    if (!doc.containsKey("btAddress") || !doc.containsKey("volume")) {
        sendToClient(clientNum, buildErrorResponse("setBTSpeakerVolume", "Missing btAddress or volume parameter"));
        return;
    }

    String btAddress = doc["btAddress"].as<String>();
    int volume = doc["volume"].as<int>();

    btSpeakerManager->setSpeakerVolume(btAddress, volume);
    sendToClient(clientNum, buildSuccessResponse("setBTSpeakerVolume", "Speaker volume set to " + String(volume)));
    broadcastStatus();
}

void WebSocketServer::handleSetBTSpeakerMutedCommand(uint8_t clientNum, JsonDocument& doc) {
    if (btSpeakerManager == nullptr) {
        sendToClient(clientNum, buildErrorResponse("setBTSpeakerMuted", "Bluetooth manager not available"));
        return;
    }

    if (!doc.containsKey("btAddress") || !doc.containsKey("muted")) {
        sendToClient(clientNum, buildErrorResponse("setBTSpeakerMuted", "Missing btAddress or muted parameter"));
        return;
    }

    String btAddress = doc["btAddress"].as<String>();
    bool muted = doc["muted"].as<bool>();

    btSpeakerManager->setSpeakerMuted(btAddress, muted);
    sendToClient(clientNum, buildSuccessResponse("setBTSpeakerMuted", muted ? "Speaker muted" : "Speaker unmuted"));
    broadcastStatus();
}

void WebSocketServer::handleSetBTSpeakerNameCommand(uint8_t clientNum, JsonDocument& doc) {
    if (btSpeakerManager == nullptr) {
        sendToClient(clientNum, buildErrorResponse("setBTSpeakerName", "Bluetooth manager not available"));
        return;
    }

    if (!doc.containsKey("btAddress") || !doc.containsKey("name")) {
        sendToClient(clientNum, buildErrorResponse("setBTSpeakerName", "Missing btAddress or name parameter"));
        return;
    }

    String btAddress = doc["btAddress"].as<String>();
    String name = doc["name"].as<String>();

    btSpeakerManager->setSpeakerCustomName(btAddress, name);
    sendToClient(clientNum, buildSuccessResponse("setBTSpeakerName", "Speaker renamed to " + name));
    broadcastStatus();
}

void WebSocketServer::handleSetBTSpeakerRoomCommand(uint8_t clientNum, JsonDocument& doc) {
    if (btSpeakerManager == nullptr) {
        sendToClient(clientNum, buildErrorResponse("setBTSpeakerRoom", "Bluetooth manager not available"));
        return;
    }

    if (!doc.containsKey("btAddress") || !doc.containsKey("room")) {
        sendToClient(clientNum, buildErrorResponse("setBTSpeakerRoom", "Missing btAddress or room parameter"));
        return;
    }

    String btAddress = doc["btAddress"].as<String>();
    String room = doc["room"].as<String>();

    btSpeakerManager->setSpeakerRoom(btAddress, room);
    sendToClient(clientNum, buildSuccessResponse("setBTSpeakerRoom", "Speaker assigned to room " + room));
    broadcastStatus();
}

// ============================================================================
// AUDIO STREAMING COMMAND HANDLERS
// ============================================================================

void WebSocketServer::handleStartAudioStreamCommand(uint8_t clientNum) {
    if (audioReceiver == nullptr) {
        sendToClient(clientNum, buildErrorResponse("startAudioStream", "Audio receiver not available"));
        return;
    }

    if (audioReceiver->startReceiving()) {
        hubManager.setStreaming(true);

        // Start Bluetooth streaming if speakers are connected
        if (btSpeakerManager != nullptr && btSpeakerManager->getConnectedCount() > 0) {
            btSpeakerManager->startStreaming();
        }

        sendToClient(clientNum, buildSuccessResponse("startAudioStream", "Audio stream started"));
    } else {
        sendToClient(clientNum, buildErrorResponse("startAudioStream", "Failed to start audio stream"));
    }

    broadcastStatus();
}

void WebSocketServer::handleStopAudioStreamCommand(uint8_t clientNum) {
    if (audioReceiver == nullptr) {
        sendToClient(clientNum, buildErrorResponse("stopAudioStream", "Audio receiver not available"));
        return;
    }

    audioReceiver->stopReceiving();
    hubManager.setStreaming(false);

    // Stop Bluetooth streaming
    if (btSpeakerManager != nullptr) {
        btSpeakerManager->stopStreaming();
    }

    sendToClient(clientNum, buildSuccessResponse("stopAudioStream", "Audio stream stopped"));
    broadcastStatus();
}

void WebSocketServer::handleGetAudioStreamInfoCommand(uint8_t clientNum) {
    if (audioReceiver == nullptr) {
        sendToClient(clientNum, buildErrorResponse("getAudioStreamInfo", "Audio receiver not available"));
        return;
    }

    StaticJsonDocument<512> doc;
    doc["type"] = "audioStreamInfo";
    doc["isReceiving"] = audioReceiver->isReceiving();
    doc["clientConnected"] = audioReceiver->isClientConnected();
    doc["clientIP"] = audioReceiver->getClientIP();
    doc["bufferLevel"] = audioReceiver->getBufferLevel();
    doc["packetsReceived"] = audioReceiver->getPacketsReceived();
    doc["packetsDropped"] = audioReceiver->getPacketsDropped();
    doc["bytesReceived"] = audioReceiver->getBytesReceived();

    String output;
    serializeJson(doc, output);
    sendToClient(clientNum, output);
}

// ============================================================================
// RESPONSE BUILDERS
// ============================================================================

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
    StaticJsonDocument<1024> doc;
    doc["type"] = "status";

    // Hub status
    doc["hubId"] = hubManager.getHubId();
    doc["powerState"] = hubManager.getPowerState();
    doc["masterVolume"] = hubManager.getMasterVolume();
    doc["isMuted"] = hubManager.isMuted();
    doc["isStreaming"] = hubManager.isStreaming();
    doc["uptime"] = millis() / 1000;

    // Bluetooth speaker status
    if (btSpeakerManager != nullptr) {
        doc["connectedSpeakers"] = btSpeakerManager->getConnectedCount();
        doc["btStreaming"] = btSpeakerManager->isStreaming();
    }

    // Audio stream status
    if (audioReceiver != nullptr) {
        doc["audioReceiving"] = audioReceiver->isReceiving();
        doc["audioClientConnected"] = audioReceiver->isClientConnected();
        doc["audioBufferLevel"] = audioReceiver->getBufferLevel();
    }

    String output;
    serializeJson(doc, output);
    return output;
}
