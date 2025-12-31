# ESP32 Multi-Room Bluetooth Hub - Architecture Overview

## System Architecture (Hub Mode)

### Overview

The ESP32 acts as a **Bluetooth Hub** that bridges WiFi and Bluetooth:
- Receives audio stream from mobile app via WiFi
- Connects to multiple Bluetooth speakers as A2DP Source (master)
- Forwards audio to all connected Bluetooth speakers simultaneously

```
┌─────────────────┐          WiFi (Audio Stream + Control)
│   Mobile App    │ ────────────────────────────────────┐
│   (Flutter)     │                                     │
└─────────────────┘                                     ▼
                                              ┌──────────────────┐
                                              │   ESP32 Hub      │
                                              │  ┌────────────┐  │
                                              │  │ WiFi Radio │  │
                                              │  └────────────┘  │
                                              │  ┌────────────┐  │
                                              │  │ Audio      │  │
                                              │  │ Buffer     │  │
                                              │  └────────────┘  │
                                              │  ┌────────────┐  │
                                              │  │ Bluetooth  │  │
                                              │  │ A2DP Source│  │
                                              │  └────────────┘  │
                                              └──────────────────┘
                                                       │
                               ┌───────────────────────┼──────────────────────┐
                               │                       │                      │
                        Bluetooth A2DP          Bluetooth A2DP         Bluetooth A2DP
                               │                       │                      │
                               ▼                       ▼                      ▼
                      ┌─────────────────┐   ┌─────────────────┐   ┌─────────────────┐
                      │ BT Speaker #1   │   │ BT Speaker #2   │   │ BT Speaker #3   │
                      │ (Living Room)   │   │ (Kitchen)       │   │ (Bedroom)       │
                      └─────────────────┘   └─────────────────┘   └─────────────────┘
```

## Communication Protocols

### 1. WiFi Communication (Mobile App ↔ ESP32 Hub)

#### Control Channel: WebSocket (Port 81)
- Real-time bidirectional communication
- JSON-based messages
- Commands:
  - Connect/disconnect Bluetooth speakers
  - Volume control
  - Mute/unmute
  - Start/stop audio streaming
  - Get hub status
  - Get connected speakers list

**Example WebSocket Commands:**
```json
// Connect to Bluetooth speaker
{
  "command": "connectBTSpeaker",
  "btAddress": "AA:BB:CC:DD:EE:FF"
}

// Set master volume
{
  "command": "setMasterVolume",
  "volume": 75
}

// Get connected speakers
{
  "command": "getConnectedSpeakers"
}
```

#### Audio Channel: UDP (Port 8888)
- Low-latency audio streaming
- Unidirectional (App → ESP32)
- PCM audio format:
  - Sample rate: 44.1kHz
  - Bit depth: 16-bit
  - Channels: Stereo (2)
  - Encoding: Raw PCM or compressed (Opus/AAC)

**Audio Packet Format:**
```
[Header: 8 bytes][Audio Data: up to 4088 bytes]

Header:
- Sequence number (4 bytes)
- Timestamp (4 bytes)

Payload:
- Raw PCM audio samples
```

### 2. Bluetooth Communication (ESP32 Hub ↔ Bluetooth Speakers)

#### Bluetooth A2DP Source Profile
- ESP32 acts as **A2DP Source** (master role)
- Connects TO Bluetooth speakers (slaves)
- Can maintain 3-7 simultaneous connections (ESP32 limitation)
- Uses SBC codec for audio transmission

**Connection Process:**
1. ESP32 scans for Bluetooth devices
2. User selects speakers in mobile app
3. ESP32 initiates pairing (if needed)
4. ESP32 connects to speakers via A2DP
5. Audio routed to all connected speakers

## Data Flow

### Audio Streaming Flow

```
1. Mobile App (Audio Source)
   │
   ├─ Capture audio (mic/file/streaming service)
   │
   ├─ Encode audio (PCM/Opus)
   │
   ├─ Packetize audio
   │
   ▼
2. Send via UDP (WiFi) ──────────────────────────────────────┐
                                                              │
                                                              ▼
3. ESP32 Hub
   │
   ├─ Receive UDP packets
   │
   ├─ Buffer audio (handle jitter/latency)
   │
   ├─ Distribute to Bluetooth stack
   │
   ▼
4. Bluetooth A2DP Source
   │
   ├─ Encode to SBC codec
   │
   ├─ Send to Speaker #1 ──────────────────────> BT Speaker #1
   │
   ├─ Send to Speaker #2 ──────────────────────> BT Speaker #2
   │
   └─ Send to Speaker #3 ──────────────────────> BT Speaker #3
```

## ESP32 Hub Components

### Core Modules

1. **NetworkManager**
   - WiFi connection management
   - mDNS service broadcasting (`_esp32hub._tcp`)
   - Auto-reconnection

2. **WebSocketServer**
   - Control command handling
   - Status updates to mobile app
   - Heartbeat/ping mechanism

3. **AudioStreamReceiver** (NEW)
   - UDP audio packet reception
   - Audio buffering
   - Jitter handling
   - Buffer underrun/overrun management

4. **BluetoothSpeakerManager** (NEW)
   - Bluetooth device discovery
   - Pairing management
   - Connection to multiple speakers
   - Connection monitoring
   - Auto-reconnection

5. **DeviceManager**
   - Hub configuration
   - Master volume control
   - Mute/unmute
   - Settings persistence

## Mobile App Architecture

### Key Changes for Hub Mode

1. **Audio Capture**
   - Microphone input
   - Media file playback
   - Streaming service integration (future)

2. **Audio Encoding**
   - PCM encoding
   - Compression (optional)
   - Packetization

3. **UDP Streaming**
   - Send audio packets to ESP32 hub
   - Handle network congestion
   - Adaptive bitrate (future)

4. **Bluetooth Speaker Management UI**
   - Scan for Bluetooth speakers
   - Pair/unpair speakers
   - Connect/disconnect speakers
   - Assign speakers to rooms
   - Volume control per speaker

### Mobile App Data Models

```dart
// Hub model
class ESP32Hub {
  String hubId;
  String customName;
  String ipAddress;
  bool isConnected;
  int masterVolume;
  bool isMuted;
  List<BluetoothSpeaker> connectedSpeakers;
}

// Bluetooth Speaker model
class BluetoothSpeaker {
  String btAddress;        // MAC address
  String btName;           // Bluetooth device name
  String customName;       // User-assigned name
  String room;             // Room assignment
  bool isConnected;
  bool isPaired;
  int volume;
  bool isMuted;
  int rssi;                // Signal strength
}
```

## Technical Challenges & Solutions

### Challenge 1: Synchronization Across Multiple Bluetooth Speakers

**Problem:** Bluetooth introduces latency, different speakers may have different delays.

**Solution:**
- Audio buffering on ESP32 (100-200ms)
- Timestamp-based synchronization
- Accept ±50ms tolerance (acceptable for multi-room audio)

### Challenge 2: ESP32 Bluetooth Connection Limits

**Problem:** ESP32 can only maintain 3-7 simultaneous Bluetooth connections.

**Solution:**
- Phase 1: Support up to 7 speakers per hub
- Phase 2: Multiple ESP32 hubs for larger setups
- Document limitation clearly

### Challenge 3: WiFi Audio Streaming Reliability

**Problem:** WiFi packet loss can cause audio dropouts.

**Solution:**
- Use UDP for low latency (accept occasional packet loss)
- Implement audio buffering (smooth out jitter)
- Packet loss concealment (interpolate missing samples)
- Monitor network quality, warn user

### Challenge 4: Audio Quality vs. Bandwidth

**Problem:** Uncompressed audio requires significant bandwidth.

**Solution:**
- Use compressed audio (Opus codec) for WiFi streaming
- Decompress on ESP32
- Bluetooth A2DP uses SBC codec automatically

## Performance Specifications

### Network Requirements

**WiFi Audio Streaming:**
- Bandwidth: ~500 kbps - 1 Mbps per hub
- Latency: <50ms (local network)
- Packet loss tolerance: <1%

**WebSocket Control:**
- Bandwidth: Negligible (~5 kbps)
- Latency: <100ms acceptable

### ESP32 Resource Usage

**Memory:**
- Audio buffers: ~50-100 KB
- Bluetooth stack: ~100 KB
- WiFi stack: ~50 KB
- Application: ~100 KB
- **Total:** ~300-400 KB (ESP32 has 520 KB SRAM)

**CPU:**
- Audio processing: 30-40%
- Bluetooth management: 20-30%
- WiFi: 10-20%
- **Total:** 60-90% utilization

### Audio Latency

**Total latency breakdown:**
- Mobile app capture/encoding: 10-20ms
- WiFi transmission: 5-10ms
- ESP32 buffering: 50-100ms
- Bluetooth transmission: 100-200ms
- Speaker processing: 10-50ms
- **Total:** 175-380ms

*Note: This latency is acceptable for multi-room audio but NOT for video sync (lip-sync).*

## Scalability

### Single Hub Setup
- 1 ESP32 Hub
- Up to 7 Bluetooth speakers
- 1 room or multiple rooms (small house)

### Multi-Hub Setup (Future - Phase 2)
- Multiple ESP32 Hubs
- Each hub manages 3-7 speakers
- Synchronized across hubs via WiFi
- Suitable for large houses

## Security Considerations

1. **WiFi Security:**
   - Use WPA2/WPA3 encrypted WiFi
   - Local network only (no internet required)
   - Optional: VPN for remote access

2. **Bluetooth Security:**
   - Bluetooth pairing with PIN (if supported)
   - Only connect to user-authorized speakers
   - Prevent unauthorized connections

3. **App Security:**
   - No cloud dependency (privacy)
   - Local storage encryption (future)

## Power Consumption

**ESP32 Hub:**
- WiFi active: ~160-260 mA
- Bluetooth active (multi-connection): ~100-200 mA
- Total: ~260-460 mA @ 3.3V
- Power: ~1-1.5W

**Recommended Power Supply:**
- 5V/2A USB power adapter

## Future Enhancements (Phase 2+)

1. **Audio Sources:**
   - Line-in (3.5mm jack)
   - Bluetooth audio input (phone streams directly to hub)
   - Network streaming (Spotify Connect, AirPlay)

2. **Advanced Features:**
   - Multi-hub synchronization
   - Room-specific EQ
   - Automatic speaker discovery
   - Guest mode

3. **Platform Support:**
   - Web interface for control
   - Desktop apps (Windows, macOS, Linux)

---

**Document Version:** 2.0
**Architecture:** Bluetooth Hub Mode
**Last Updated:** December 2025
