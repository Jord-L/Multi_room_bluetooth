# ESP32 Bluetooth Hub - Implementation TODO

**Project Status:** Partial implementation - Core components complete, integration pending

**Last Updated:** December 2025

---

## 📋 Overview

This document tracks all remaining work to complete the ESP32 Bluetooth Hub system. The hub receives audio from a mobile app via WiFi and forwards it to multiple Bluetooth speakers.

**Architecture Reference:** See `ARCHITECTURE.md` for complete system design.

---

## ✅ Completed Components

### ESP32 Firmware (40% Complete)
- [x] `config.h` - Hub configuration
- [x] `BluetoothSpeaker.h/.cpp` - Bluetooth speaker model (324 lines)
- [x] `BluetoothSpeakerManager.h/.cpp` - Multi-speaker BT management (483 lines)
- [x] `AudioStreamReceiver.h/.cpp` - WiFi audio streaming (392 lines)
- [x] `NetworkManager.h/.cpp` - WiFi and mDNS (existing, may need updates)

### Documentation
- [x] `ARCHITECTURE.md` - Complete system architecture
- [x] Updated `platformio.ini` for hub mode

---

## 🚧 TODO: ESP32 Firmware

### Priority 1: Core Integration (CRITICAL)

#### ☐ Task 1.1: Create HubManager
**File:** `esp32_firmware/include/HubManager.h` and `esp32_firmware/src/HubManager.cpp`

**Purpose:** Central manager for the hub (replaces DeviceManager)

**Responsibilities:**
- Hub identification and settings
- Master volume control
- Coordinate between BluetoothSpeakerManager and AudioStreamReceiver
- Settings persistence

**Key Methods:**
```cpp
class HubManager {
public:
    bool begin();

    // Hub info
    String getHubId();
    String getCustomName();
    void setCustomName(const String& name);

    // Audio control
    void setMasterVolume(int volume);
    int getMasterVolume();
    void setMuted(bool muted);
    bool isMuted();

    // Status
    String getHubStatusJson();
    String getHubInfoJson();

    // Settings
    bool saveSettings();
    bool loadSettings();
};
```

**Reference:** See old `DeviceManager.cpp` for patterns, but simplify for hub mode.

**Estimated Time:** 2-3 hours

---

#### ☐ Task 1.2: Update WebSocketServer for Hub Commands
**File:** `esp32_firmware/src/WebSocketServer.cpp`

**Purpose:** Add new WebSocket commands for hub operations

**New Commands to Add:**

```cpp
// Bluetooth Speaker Discovery
{"command": "startBTDiscovery"}
{"command": "stopBTDiscovery"}
{"command": "getDiscoveredSpeakers"}

// Bluetooth Speaker Connection
{"command": "pairBTSpeaker", "btAddress": "AA:BB:CC:DD:EE:FF"}
{"command": "connectBTSpeaker", "btAddress": "AA:BB:CC:DD:EE:FF"}
{"command": "disconnectBTSpeaker", "btAddress": "AA:BB:CC:DD:EE:FF"}
{"command": "removeBTSpeaker", "btAddress": "AA:BB:CC:DD:EE:FF"}

// Bluetooth Speaker Control
{"command": "setBTSpeakerVolume", "btAddress": "...", "volume": 75}
{"command": "setBTSpeakerMute", "btAddress": "...", "muted": true}
{"command": "setBTSpeakerName", "btAddress": "...", "name": "Living Room"}
{"command": "setBTSpeakerRoom", "btAddress": "...", "room": "Living Room"}

// Hub Control
{"command": "setMasterVolume", "volume": 75}
{"command": "setMasterMute", "muted": true}
{"command": "getConnectedSpeakers"}
{"command": "getHubStatus"}

// Audio Streaming
{"command": "startAudioStream"}  // Prepare to receive audio
{"command": "stopAudioStream"}
```

**Implementation Guide:**
1. Add handler methods in `WebSocketServer.cpp`
2. Call BluetoothSpeakerManager methods
3. Send responses back to client
4. Broadcast status updates to all connected clients

**Example Handler:**
```cpp
void WebSocketServer::handleStartBTDiscovery(uint8_t clientNum) {
    if (btSpeakerManager->startDiscovery()) {
        sendToClient(clientNum, buildSuccessResponse("startBTDiscovery", "Scanning started"));
    } else {
        sendToClient(clientNum, buildErrorResponse("startBTDiscovery", "Failed to start scan"));
    }
}
```

**Estimated Time:** 4-5 hours

---

#### ☐ Task 1.3: Rewrite main.cpp for Hub Architecture
**File:** `esp32_firmware/src/main.cpp`

**Purpose:** Complete rewrite for hub mode

**Key Changes:**
1. Remove AudioController (not needed)
2. Add BluetoothSpeakerManager initialization
3. Add AudioStreamReceiver initialization
4. Add HubManager initialization
5. Update main loop to handle audio routing

**Main Loop Flow:**
```cpp
void loop() {
    // Handle OTA
    ArduinoOTA.handle();

    // Check network
    networkManager.checkConnection();

    // Handle WebSocket
    wsServer->loop();

    // Receive audio from WiFi
    audioReceiver->loop();

    // If audio available, forward to Bluetooth
    if (audioReceiver->hasAudioData() && btManager->isStreaming()) {
        uint8_t buffer[AUDIO_BUFFER_SIZE];
        size_t length = audioReceiver->readAudioData(buffer, AUDIO_BUFFER_SIZE);
        btManager->writeAudioData(buffer, length);
    }

    // Monitor Bluetooth connections
    btManager->checkConnections();
}
```

**Key Sections:**
1. Initialization order: WiFi → mDNS → HubManager → AudioReceiver → BTManager → WebSocket
2. Audio routing: AudioReceiver → BluetoothSpeakerManager
3. Status updates: Periodic broadcasts to mobile app

**Reference:** See old `main.cpp` for patterns, but adapt for hub.

**Estimated Time:** 3-4 hours

---

#### ☐ Task 1.4: Update NetworkManager for Hub mDNS
**File:** `esp32_firmware/src/NetworkManager.cpp`

**Purpose:** Update mDNS service name for hub

**Changes:**
```cpp
// Change service name from "_esp32speaker._tcp" to "_esp32hub._tcp"
MDNS.addService("_esp32hub", "_tcp", 80);

// Update TXT records
MDNS.addServiceTxt("_esp32hub", "_tcp", "type", "bluetooth_hub");
MDNS.addServiceTxt("_esp32hub", "_tcp", "maxSpeakers", String(BT_MAX_CONNECTED_SPEAKERS));
MDNS.addServiceTxt("_esp32hub", "_tcp", "connectedSpeakers", String(connectedCount));
```

**Estimated Time:** 30 minutes

---

#### ☐ Task 1.5: Remove Old Components
**Files to Delete:**
- `esp32_firmware/include/AudioController.h`
- `esp32_firmware/src/AudioController.cpp`
- `esp32_firmware/include/DeviceManager.h` (replaced by HubManager)
- `esp32_firmware/src/DeviceManager.cpp`

**Estimated Time:** 15 minutes

---

### Priority 2: Audio Integration (HIGH)

#### ☐ Task 2.1: Connect Audio Pipeline
**Location:** `BluetoothSpeakerManager.cpp`

**Purpose:** Implement actual audio forwarding to Bluetooth

**Current Issue:** `writeAudioData()` and `audioDataCallback()` are stubs

**Implementation:**
```cpp
void BluetoothSpeakerManager::writeAudioData(const uint8_t* data, size_t length) {
    if (!streaming || getConnectedCount() == 0) {
        return;
    }

    // Store in buffer for audioDataCallback to read
    // The A2DP source library will call audioDataCallback() when it needs data

    // Option 1: Use static buffer
    static uint8_t audioBuffer[AUDIO_BUFFER_SIZE];
    static size_t audioBufferLength = 0;

    memcpy(audioBuffer, data, min(length, (size_t)AUDIO_BUFFER_SIZE));
    audioBufferLength = length;

    // Option 2: Use FreeRTOS queue (better for thread safety)
    // xQueueSend(audioQueue, &data, 0);
}

int32_t BluetoothSpeakerManager::audioDataCallback(uint8_t* data, int32_t len) {
    // This is called by Bluetooth stack
    // Copy from buffer prepared by writeAudioData()

    // Return actual bytes written
    return len;
}
```

**Challenge:** ESP32-A2DP library may only support single speaker connections by default. You may need to:
- Use library's multi-connection features (if available)
- Send same audio to multiple connections sequentially
- Research if library supports broadcast mode

**Estimated Time:** 3-4 hours (includes research and testing)

---

#### ☐ Task 2.2: Audio Synchronization
**Location:** Multiple files

**Purpose:** Ensure audio plays in sync across all speakers

**Implementation Ideas:**
1. **Timestamp-based:** Add timestamps to audio packets, delay playback
2. **Buffer-based:** Use fixed buffer delay (100-200ms)
3. **Accept tolerance:** ±50ms is acceptable for multi-room audio

**Note:** Perfect sync across Bluetooth speakers is very difficult. Document limitations.

**Estimated Time:** 4-6 hours (experimental)

---

### Priority 3: Testing & Debugging (MEDIUM)

#### ☐ Task 3.1: Test Bluetooth Discovery
**Purpose:** Verify BT speaker discovery works

**Steps:**
1. Power on ESP32 hub
2. Send WebSocket command: `{"command": "startBTDiscovery"}`
3. Wait 10 seconds
4. Send: `{"command": "getDiscoveredSpeakers"}`
5. Verify response contains nearby Bluetooth speakers

**Expected Issues:**
- May need to manually put speakers in pairing mode
- Some speakers may not be discoverable
- Range limitations

**Estimated Time:** 2 hours

---

#### ☐ Task 3.2: Test Bluetooth Connection
**Purpose:** Verify connecting to discovered speakers

**Steps:**
1. Discover speakers (Task 3.1)
2. Get BT address from discovery results
3. Send: `{"command": "connectBTSpeaker", "btAddress": "AA:BB:CC:DD:EE:FF"}`
4. Verify speaker connects
5. Check WebSocket status updates

**Expected Issues:**
- Pairing PIN requirements
- Connection timeouts
- Multi-connection limitations

**Estimated Time:** 2-3 hours

---

#### ☐ Task 3.3: Test Audio Streaming (No Mobile App Yet)
**Purpose:** Test audio reception without mobile app

**Tools:** Use command-line UDP sender (e.g., `ffmpeg` or `gstreamer`)

**Example with ffmpeg:**
```bash
# Generate test tone and stream to ESP32
ffmpeg -f lavfi -i "sine=frequency=440:duration=10" \
  -ar 44100 -ac 2 -f s16le \
  udp://192.168.1.100:8888
```

**Verify:**
- ESP32 receives UDP packets
- Buffer fills up
- Audio data available for Bluetooth

**Estimated Time:** 2 hours

---

### Priority 4: Enhancements (LOW)

#### ☐ Task 4.1: Add Connection Health Monitoring
**Purpose:** Auto-reconnect to speakers that disconnect

**Implementation:** Already partially in BluetoothSpeakerManager, just needs to be called

```cpp
// In main.cpp loop
void loop() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 10000) {  // Every 10 seconds
        btManager->checkConnections();
        btManager->attemptReconnections();
        lastCheck = millis();
    }
}
```

**Estimated Time:** 1 hour

---

#### ☐ Task 4.2: Add Buffer Statistics to WebSocket
**Purpose:** Help debug audio streaming issues

**Implementation:**
```cpp
{"command": "getAudioStats"}

// Response:
{
  "type": "audioStats",
  "bufferLevel": 45,  // percentage
  "packetsReceived": 12345,
  "packetsDropped": 23,
  "bytesReceived": 1234567,
  "latency": 150,  // ms
  "clientConnected": true,
  "clientIP": "192.168.1.50"
}
```

**Estimated Time:** 1 hour

---

#### ☐ Task 4.3: Implement Volume Control per Speaker
**Purpose:** Actually control volume on each Bluetooth speaker

**Challenge:** Not all Bluetooth speakers support remote volume control via A2DP.

**Research Needed:**
- Check if ESP32-A2DP library supports AVRCP (volume control profile)
- May need to implement AVRCP separately
- Fallback: Document as limitation

**Estimated Time:** 3-5 hours (research-heavy)

---

## 🚧 TODO: Mobile App (Complete Rebuild)

### Priority 1: Foundation (CRITICAL)

#### ☐ Task A.1: Create New Data Models
**Files:** `mobile_app/lib/models/`

**New Models Needed:**

**1. ESP32Hub Model:**
```dart
class ESP32Hub {
  String hubId;
  String customName;
  String ipAddress;
  bool isConnected;
  int masterVolume;
  bool isMuted;
  bool isStreaming;
  List<BluetoothSpeaker> connectedSpeakers;

  // Audio stats
  int bufferLevel;
  int packetsReceived;
  int packetsDropped;
}
```

**2. BluetoothSpeaker Model:**
```dart
class BluetoothSpeaker {
  String btAddress;
  String btName;
  String customName;
  String room;
  BTSpeakerState state;
  bool isConnected;
  bool isPaired;
  int volume;
  bool isMuted;
  bool isPlaying;
  int rssi;
}

enum BTSpeakerState {
  disconnected,
  connecting,
  connected,
  playing,
  paused,
  error
}
```

**3. AudioStreamStats Model:**
```dart
class AudioStreamStats {
  bool isStreaming;
  int sampleRate;
  int bitDepth;
  int channels;
  int bytesPerSecond;
  Duration totalDuration;
}
```

**Files to Create:**
- `hub_model.dart`
- `bluetooth_speaker_model.dart`
- `audio_stream_stats.dart`

**Estimated Time:** 2 hours

---

#### ☐ Task A.2: Create Hub Discovery Service
**File:** `mobile_app/lib/services/hub_discovery_service.dart`

**Purpose:** Find ESP32 hubs on network via mDNS

**Changes from Current:**
- Change service name from `_esp32speaker._tcp` to `_esp32hub._tcp`
- Parse hub-specific TXT records

```dart
class HubDiscoveryService {
  final Discovery _discovery = Discovery();

  Stream<DiscoveredHub> get hubsFound => ...;

  Future<void> startDiscovery() async {
    await for (final service in _discovery.discoverServices('_esp32hub._tcp')) {
      // Parse hub info
      final hub = ESP32Hub.fromDiscovery(service);
      _hubController.add(hub);
    }
  }
}
```

**Estimated Time:** 1 hour

---

#### ☐ Task A.3: Update WebSocket Service for Hub Commands
**File:** `mobile_app/lib/services/websocket_service.dart`

**Purpose:** Add new hub-specific commands

**New Methods:**
```dart
// Bluetooth speaker commands
Future<bool> startBTDiscovery();
Future<bool> stopBTDiscovery();
Future<bool> connectBTSpeaker(String btAddress);
Future<bool> disconnectBTSpeaker(String btAddress);
Future<bool> setBTSpeakerVolume(String btAddress, int volume);
Future<bool> setBTSpeakerName(String btAddress, String name);

// Hub commands
Future<bool> setMasterVolume(int volume);
Future<bool> setMasterMute(bool muted);
Future<Map<String, dynamic>> getConnectedSpeakers();

// Audio streaming
Future<bool> startAudioStream();
Future<bool> stopAudioStream();
```

**Estimated Time:** 2 hours

---

### Priority 2: Audio Streaming (HIGH - COMPLEX)

#### ☐ Task A.4: Create Audio Capture Service
**File:** `mobile_app/lib/services/audio_capture_service.dart`

**Purpose:** Capture audio from microphone or files

**Dependencies:**
Add to `pubspec.yaml`:
```yaml
dependencies:
  record: ^5.0.0  # Audio recording
  audioplayers: ^5.2.0  # Audio file playback
  permission_handler: ^11.0.1  # Microphone permissions
```

**Implementation:**
```dart
class AudioCaptureService {
  AudioRecorder? _recorder;

  // Start capturing from microphone
  Future<Stream<Uint8List>> startMicrophoneCapture() async {
    // Request permission
    // Start recording
    // Return audio stream
  }

  // Play audio file
  Future<Stream<Uint8List>> playAudioFile(String filePath) async {
    // Load file
    // Decode to PCM
    // Return audio stream
  }
}
```

**Challenge:** Getting raw PCM audio data in correct format (44.1kHz, 16-bit, stereo)

**Estimated Time:** 4-6 hours

---

#### ☐ Task A.5: Create Audio Encoder Service
**File:** `mobile_app/lib/services/audio_encoder_service.dart`

**Purpose:** Encode audio to required format

**Options:**
1. **Raw PCM** - No encoding, large bandwidth (~1.4 Mbps)
2. **Opus Codec** - Compressed, better quality, lower bandwidth (~128 kbps)

**For Phase 1: Use Raw PCM (simpler)**

```dart
class AudioEncoderService {
  // Ensure audio is in correct format
  Uint8List encodePCM(Uint8List rawAudio) {
    // Convert to 44.1kHz, 16-bit, stereo if needed
    // Add packet header (sequence number, timestamp)
    return encodedData;
  }
}
```

**Estimated Time:** 2-3 hours

---

#### ☐ Task A.6: Create UDP Audio Streaming Service
**File:** `mobile_app/lib/services/udp_audio_streamer.dart`

**Purpose:** Send audio to ESP32 hub via UDP

**Dependencies:**
```yaml
dependencies:
  udp: ^5.0.3
```

**Implementation:**
```dart
class UDPAudioStreamer {
  late UDP _udpSender;
  String hubIP;
  int port = 8888;

  int _sequenceNumber = 0;

  Future<void> connect(String ip) async {
    hubIP = ip;
    await _udpSender.bind(Endpoint.any());
  }

  Future<void> sendAudioData(Uint8List audioData) async {
    // Add header: [sequence:4 bytes][timestamp:4 bytes][audio data]
    final packet = _buildPacket(audioData);

    await _udpSender.send(
      packet,
      Endpoint.unicast(
        InternetAddress(hubIP),
        port: Port(port),
      ),
    );

    _sequenceNumber++;
  }

  Uint8List _buildPacket(Uint8List audioData) {
    final header = ByteData(8);
    header.setUint32(0, _sequenceNumber);
    header.setUint32(4, DateTime.now().millisecondsSinceEpoch);

    return Uint8List.fromList([
      ...header.buffer.asUint8List(),
      ...audioData,
    ]);
  }
}
```

**Estimated Time:** 3-4 hours

---

#### ☐ Task A.7: Integrate Audio Pipeline
**File:** `mobile_app/lib/services/audio_streaming_manager.dart`

**Purpose:** Coordinate audio capture → encoding → streaming

```dart
class AudioStreamingManager {
  final AudioCaptureService _capture;
  final AudioEncoderService _encoder;
  final UDPAudioStreamer _streamer;

  StreamSubscription? _audioSubscription;

  Future<void> startStreaming(String hubIP, AudioSource source) async {
    // Connect UDP
    await _streamer.connect(hubIP);

    // Get audio stream based on source
    Stream<Uint8List> audioStream;
    if (source == AudioSource.microphone) {
      audioStream = await _capture.startMicrophoneCapture();
    } else {
      audioStream = await _capture.playAudioFile(source.filePath);
    }

    // Process and send
    _audioSubscription = audioStream.listen((rawAudio) {
      final encoded = _encoder.encodePCM(rawAudio);
      _streamer.sendAudioData(encoded);
    });
  }

  Future<void> stopStreaming() async {
    await _audioSubscription?.cancel();
    _audioSubscription = null;
  }
}
```

**Estimated Time:** 2-3 hours

---

### Priority 3: User Interface (MEDIUM)

#### ☐ Task A.8: Create Hub Manager
**File:** `mobile_app/lib/services/hub_manager.dart`

**Purpose:** Central state manager for hub (using Provider)

```dart
class HubManager extends ChangeNotifier {
  ESP32Hub? _currentHub;
  final Map<String, BluetoothSpeaker> _bluetoothSpeakers = {};

  AudioStreamingManager? _audioStreaming;
  WebSocketService? _wsConnection;

  // Hub discovery
  Future<void> discoverHubs();
  Future<void> connectToHub(String hubId);

  // Bluetooth speaker management
  Future<void> startBluetoothDiscovery();
  Future<void> connectToSpeaker(String btAddress);
  List<BluetoothSpeaker> get connectedSpeakers;

  // Audio streaming
  Future<void> startAudioStreaming(AudioSource source);
  Future<void> stopAudioStreaming();

  // Volume control
  Future<void> setMasterVolume(int volume);
  Future<void> setSpeakerVolume(String btAddress, int volume);
}
```

**Estimated Time:** 3-4 hours

---

#### ☐ Task A.9: Create Hub Discovery Screen
**File:** `mobile_app/lib/screens/hub_discovery_screen.dart`

**Purpose:** Find and connect to ESP32 hubs

**UI Elements:**
- Scan button
- List of discovered hubs
- Connection status
- Select hub to connect

**Estimated Time:** 2-3 hours

---

#### ☐ Task A.10: Create Bluetooth Speaker Management Screen
**File:** `mobile_app/lib/screens/bluetooth_speakers_screen.dart`

**Purpose:** Manage Bluetooth speakers connected to hub

**UI Sections:**
1. **Scan for Speakers** - Start/stop BT discovery
2. **Discovered Speakers** - List with "Connect" buttons
3. **Connected Speakers** - List with controls:
   - Speaker name (editable)
   - Room assignment
   - Volume slider
   - Mute button
   - Disconnect button
   - Connection status

**Estimated Time:** 4-5 hours

---

#### ☐ Task A.11: Create Audio Source Selection Screen
**File:** `mobile_app/lib/screens/audio_source_screen.dart`

**Purpose:** Select audio source to stream to hub

**Options:**
1. **Microphone** - Live audio capture
2. **Audio File** - Play file from device
3. **Streaming Services** - (Future: Spotify, etc.)

**UI:**
- Radio buttons for source selection
- File picker for audio files
- Microphone permission request
- Start/stop streaming button
- Real-time stats (bitrate, duration, buffer level)

**Estimated Time:** 3-4 hours

---

#### ☐ Task A.12: Create Main Hub Control Screen
**File:** `mobile_app/lib/screens/hub_control_screen.dart`

**Purpose:** Main control interface for hub

**UI Layout:**
```
┌─────────────────────────────┐
│ Hub: My ESP32 Hub           │ Header
│ 3 speakers connected        │
├─────────────────────────────┤
│ ┌───────────────────┐       │
│ │ Master Volume     │       │ Master Controls
│ │ [====■====] 75%   │       │
│ │ [Mute] [Unmute]   │       │
│ └───────────────────┘       │
├─────────────────────────────┤
│ Audio Source: Microphone ▼  │ Audio Source
│ [Start Streaming]           │
├─────────────────────────────┤
│ Connected Speakers:         │ Speaker List
│  ● Living Room - 80% 🔊     │
│  ● Kitchen - 60% 🔇         │
│  ● Bedroom - 70% 🔊         │
├─────────────────────────────┤
│ [Manage Speakers]           │ Actions
│ [Audio Settings]            │
│ [Disconnect Hub]            │
└─────────────────────────────┘
```

**Estimated Time:** 4-5 hours

---

#### ☐ Task A.13: Update Navigation
**File:** `mobile_app/lib/screens/home_screen.dart`

**Purpose:** Update app navigation for hub architecture

**New Tab Structure:**
1. **Hub** - Hub discovery and connection
2. **Speakers** - Bluetooth speaker management
3. **Audio** - Audio source selection and streaming
4. **Settings** - App settings

**Estimated Time:** 2 hours

---

### Priority 4: Testing & Polish (LOW)

#### ☐ Task A.14: Add Permissions Handling
**Files:** `AndroidManifest.xml`, `Info.plist`

**Android Permissions:**
```xml
<uses-permission android:name="android.permission.RECORD_AUDIO"/>
<uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE"/>
<uses-permission android:name="android.permission.BLUETOOTH"/>
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT"/>
<uses-permission android:name="android.permission.BLUETOOTH_SCAN"/>
```

**iOS Permissions:**
```xml
<key>NSMicrophoneUsageDescription</key>
<string>Record audio to stream to speakers</string>
<key>NSLocalNetworkUsageDescription</key>
<string>Find ESP32 hubs on local network</string>
```

**Estimated Time:** 1 hour

---

#### ☐ Task A.15: Add Error Handling
**Purpose:** Handle errors gracefully throughout app

**Key Areas:**
- Hub discovery failures
- Connection timeouts
- Bluetooth pairing failures
- Audio streaming errors
- Network issues

**Implementation:**
- Try-catch blocks
- User-friendly error messages
- Retry mechanisms
- Fallback behaviors

**Estimated Time:** 3-4 hours

---

#### ☐ Task A.16: Add Loading States
**Purpose:** Show progress during async operations

**UI Elements:**
- Spinners during discovery
- Progress bars for connections
- Skeleton screens during loading
- "Connecting..." overlays

**Estimated Time:** 2-3 hours

---

#### ☐ Task A.17: Update README Files
**Files:**
- `esp32_firmware/README.md`
- `mobile_app/README.md`
- Main `README.md`

**Content:**
- Update architecture description
- Update setup instructions
- Add Bluetooth speaker pairing guide
- Add audio streaming guide
- Update troubleshooting section

**Estimated Time:** 2-3 hours

---

## 📊 Effort Summary

### ESP32 Firmware (Remaining)
| Priority | Tasks | Estimated Time |
|----------|-------|----------------|
| P1 (Critical) | 5 tasks | 10-13 hours |
| P2 (High) | 2 tasks | 7-10 hours |
| P3 (Medium) | 3 tasks | 6-8 hours |
| P4 (Low) | 3 tasks | 5-7 hours |
| **TOTAL** | **13 tasks** | **28-38 hours** |

### Mobile App (Complete Rebuild)
| Priority | Tasks | Estimated Time |
|----------|-------|----------------|
| P1 (Critical) | 3 tasks | 5-6 hours |
| P2 (High) | 4 tasks | 14-19 hours |
| P3 (Medium) | 6 tasks | 20-26 hours |
| P4 (Low) | 4 tasks | 8-11 hours |
| **TOTAL** | **17 tasks** | **47-62 hours** |

### Grand Total
**30 tasks** | **75-100 hours** of development work

---

## 🎯 Recommended Implementation Order

### Phase 1: Get Basic Hub Working (ESP32)
1. Task 1.1 - Create HubManager
2. Task 1.4 - Update NetworkManager
3. Task 1.3 - Rewrite main.cpp
4. Task 1.2 - Update WebSocketServer
5. Task 3.1 - Test Bluetooth discovery
6. Task 3.2 - Test Bluetooth connection

**Milestone:** Hub can discover and connect to Bluetooth speakers (no audio yet)

---

### Phase 2: Add Audio Pipeline (ESP32)
1. Task 2.1 - Connect audio pipeline
2. Task 3.3 - Test audio streaming with command-line tools
3. Task 2.2 - Audio synchronization (optional, iterate)

**Milestone:** Hub receives audio via WiFi and forwards to Bluetooth speakers

---

### Phase 3: Build Mobile App Foundation
1. Task A.1 - Create data models
2. Task A.2 - Hub discovery service
3. Task A.3 - Update WebSocket service
4. Task A.8 - Hub manager
5. Task A.9 - Hub discovery screen
6. Task A.10 - Bluetooth speaker management screen

**Milestone:** Mobile app can discover hub, connect, and manage Bluetooth speakers

---

### Phase 4: Add Audio Streaming to App
1. Task A.4 - Audio capture service
2. Task A.5 - Audio encoder service
3. Task A.6 - UDP streaming service
4. Task A.7 - Integrate audio pipeline
5. Task A.11 - Audio source screen
6. Task A.12 - Main control screen

**Milestone:** End-to-end audio streaming works (app → hub → BT speakers)

---

### Phase 5: Polish & Testing
1. All Priority 4 tasks
2. Integration testing
3. Bug fixes
4. Documentation

**Milestone:** Production-ready system

---

## 🐛 Known Issues & Limitations

### Bluetooth Connection Limits
- ESP32 can only maintain 3-7 simultaneous Bluetooth connections
- Some speakers may not support multi-device connections
- Connection reliability varies by speaker brand

### Audio Synchronization
- Perfect sync across Bluetooth speakers is very difficult
- Expect ±50-200ms latency variation
- Not suitable for video lip-sync
- Acceptable for multi-room background music

### ESP32-A2DP Library Limitations
- May only support single speaker connection by default
- Multi-connection requires custom implementation or workarounds
- Check library documentation for capabilities

### Network Requirements
- Requires strong WiFi signal for audio streaming
- ~1 Mbps bandwidth for uncompressed audio
- Local network only (no internet required)

### Audio Quality
- Double compression: WiFi stream + Bluetooth encoding
- Maximum quality: 44.1kHz, 16-bit stereo (CD quality input)
- Bluetooth output limited by SBC codec (~256 kbps)

---

## 📚 Resources

### ESP32 Bluetooth
- [ESP32-A2DP Library](https://github.com/pschatzmann/ESP32-A2DP)
- [ESP-IDF Bluetooth Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/index.html)

### Flutter Audio
- [record package](https://pub.dev/packages/record) - Audio recording
- [audioplayers package](https://pub.dev/packages/audioplayers) - Audio playback
- [udp package](https://pub.dev/packages/udp) - UDP networking

### Audio Formats
- [PCM Audio Format](https://en.wikipedia.org/wiki/Pulse-code_modulation)
- [Opus Codec](https://opus-codec.org/) - Future compression option

### Testing Tools
- [ffmpeg](https://ffmpeg.org/) - Audio file conversion and streaming
- [VLC Media Player](https://www.videolan.org/) - Network streaming testing

---

## 💡 Tips & Best Practices

1. **Start Small:** Get one speaker working before adding multiple
2. **Test Incrementally:** Test each component in isolation
3. **Use Serial Monitor:** ESP32 serial output is invaluable for debugging
4. **Monitor Memory:** ESP32 has limited RAM, watch for memory leaks
5. **Handle Errors:** Bluetooth connections are unreliable, plan for failures
6. **Document Findings:** Keep notes on what works/doesn't work with different speakers
7. **Version Control:** Commit frequently with clear messages
8. **Ask Community:** ESP32 and Flutter communities are helpful

---

## 🆘 Getting Help

If you get stuck:

1. **Check Serial Monitor** - ESP32 logs provide clues
2. **Review Architecture.md** - Understand the system design
3. **Search Issues** - Check ESP32-A2DP library GitHub issues
4. **ESP32 Forums** - [esp32.com forum](https://esp32.com/)
5. **Flutter Community** - [Flutter Discord](https://discord.gg/flutter)
6. **Stack Overflow** - Tag questions with `esp32`, `bluetooth`, `flutter`

---

**End of TODO Document**

Last Updated: December 2025
Version: 1.0
