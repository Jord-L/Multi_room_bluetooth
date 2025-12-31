# Multi-Hub Architecture Upgrade Plan

## Current Architecture (Single Hub)
- **One ESP32 Hub** per household
- Hub manages up to **7 Bluetooth speakers** simultaneously
- Mobile app connects to **one hub** at a time
- Audio streams to one hub via UDP

```
Mobile App → ESP32 Hub → 7 Bluetooth Speakers (all rooms)
```

---

## Target Architecture (Multi-Hub)
- **One ESP32 Hub per room/area**
- Each hub manages up to **7 Bluetooth speakers** in its area
- Mobile app discovers and manages **multiple hubs**
- Can stream to **one hub or all hubs simultaneously**
- **Bluetooth signal strength** determines optimal hub selection

```
Mobile App → ESP32 Hub (Living Room) → Up to 7 Speakers
           → ESP32 Hub (Bedroom)     → Up to 7 Speakers
           → ESP32 Hub (Kitchen)     → Up to 7 Speakers
           → ESP32 Hub (Garage)      → Up to 7 Speakers
           etc.
```

---

## Phase 1: Testing Current Single-Hub Implementation

### Goals:
1. ✅ Verify hub discovery via mDNS works
2. ✅ Test WebSocket connection and commands
3. ✅ Test Bluetooth speaker discovery and pairing
4. ✅ Test audio streaming (UDP → Hub → Bluetooth)
5. ✅ Test master volume and individual speaker volume
6. ✅ Test speaker management (rename, room assignment, connect/disconnect)
7. ✅ Verify real-time status updates
8. ✅ Test buffer monitoring and streaming stability

### Testing Checklist:
- [ ] Hub powers on and broadcasts mDNS
- [ ] Mobile app discovers hub automatically
- [ ] WebSocket connection established
- [ ] Hub info displayed correctly
- [ ] Master volume control works
- [ ] Mute/Unmute works
- [ ] Power on/off works
- [ ] Identify hub (LED flash) works
- [ ] Bluetooth speaker discovery works
- [ ] Can pair and connect to Bluetooth speakers
- [ ] Audio streams successfully to speakers
- [ ] Individual speaker volume control works
- [ ] Speaker renaming works
- [ ] Room assignment works
- [ ] Disconnect/reconnect speakers works
- [ ] Multi-speaker sync is acceptable (no noticeable lag)
- [ ] Audio quality is good
- [ ] No audio dropouts or buffer issues
- [ ] App handles hub disconnection gracefully
- [ ] App reconnects automatically when hub comes back

---

## Phase 2: ESP32 Firmware Changes for Multi-Hub Support

### 2.1 Hub Identification Enhancement

**File: `esp32_firmware/include/config.h`**

Add hub location/room configuration:
```cpp
// Hub identification
#define HUB_LOCATION_MAX_LENGTH 32

struct HubSettings {
    char customName[32] = "My Hub";
    char location[32] = "Living Room";  // Room/area identifier
    char hubGroup[32] = "Home";         // Group hubs by household
    bool powerState = true;
    int masterVolume = VOLUME_DEFAULT;
    bool isMuted = false;
    AudioSourceType audioSource = AUDIO_SOURCE_MOBILE_APP;
    uint8_t maxConnectedSpeakers = BT_MAX_CONNECTED_SPEAKERS;
    bool autoReconnect = true;

    // Multi-hub settings
    bool allowMultiHubSync = true;      // Allow syncing with other hubs
    int hubPriority = 0;                // Priority for automatic selection
};
```

### 2.2 mDNS Enhancement for Hub Discovery

**File: `esp32_firmware/src/NetworkManager.cpp`**

Update mDNS TXT records to include room/location:
```cpp
void NetworkManager::updateMDNSRecords(const String& firmwareVersion) {
    if (!mdnsStarted) return;

    MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "hubId", deviceId);
    MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "name", deviceName);
    MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "type", DEVICE_TYPE);
    MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "version", firmwareVersion);
    MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "ip", WiFi.localIP().toString());
    MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "mac", WiFi.macAddress());
    MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "maxSpeakers", String(BT_MAX_CONNECTED_SPEAKERS));

    // NEW: Multi-hub support
    MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "location", hubSettings.location);
    MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "group", hubSettings.hubGroup);
    MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "priority", String(hubSettings.hubPriority));

    Serial.println("[NetMgr] mDNS TXT records updated for multi-hub");
}
```

### 2.3 Inter-Hub Communication (Optional - Advanced)

For synchronized playback across hubs:

**New File: `esp32_firmware/include/HubSync.h`**
```cpp
class HubSync {
public:
    // Discover other hubs on network
    bool discoverPeerHubs();

    // Sync audio timestamp with other hubs
    void syncAudioTimestamp(unsigned long timestamp);

    // Broadcast to all hubs in group
    void broadcastToGroup(const String& command);

    // Master/slave selection for sync
    bool electSyncMaster();
};
```

**Note:** This is for advanced multi-room sync. Can be added later if needed.

---

## Phase 3: Flutter App Changes for Multi-Hub Support

### 3.1 Update Hub Model

**File: `mobile_app/lib/models/hub.dart`**

Add hub grouping and selection fields:
```dart
class Hub {
  final String hubId;
  String customName;
  String location;      // Room/area name
  String hubGroup;      // Household group
  int priority;         // Priority for auto-selection
  // ... existing fields ...

  // Multi-hub fields
  bool isSelected;      // Currently selected hub
  bool isStreaming;     // Is receiving audio stream
  int bluetoothRSSI;    // BT signal strength for auto-selection
}
```

### 3.2 Update HubManager for Multiple Hubs

**File: `mobile_app/lib/services/hub_manager.dart`**

Change from single hub to multiple hubs:
```dart
class HubManager with ChangeNotifier {
  // OLD: Hub? _hub;
  // NEW:
  final Map<String, Hub> _hubs = {};  // hubId -> Hub
  String? _selectedHubId;  // Currently selected hub

  List<Hub> get hubs => _hubs.values.toList();
  Hub? get selectedHub => _selectedHubId != null ? _hubs[_selectedHubId] : null;

  // Discover all hubs on network
  Future<void> discoverAllHubs() async { ... }

  // Select hub (for control and streaming)
  void selectHub(String hubId) { ... }

  // Connect to specific hub
  Future<bool> connectToHub(String hubId) async { ... }

  // Connect to all hubs
  Future<void> connectToAllHubs() async { ... }

  // Stream to selected hub only
  Future<bool> streamToSelectedHub() async { ... }

  // Stream to all hubs (whole-house audio)
  Future<bool> streamToAllHubs() async { ... }

  // Auto-select hub based on proximity (Bluetooth RSSI)
  void autoSelectHubByProximity() { ... }
}
```

### 3.3 Update Audio Streaming for Multi-Hub

**File: `mobile_app/lib/services/audio_streaming_service.dart`**

Support streaming to multiple hubs:
```dart
class AudioStreamingService {
  final Map<String, RawDatagramSocket?> _sockets = {};  // hubId -> socket

  // Start streaming to specific hub
  Future<bool> startStreaming(String hubIpAddress, int hubPort) async { ... }

  // Start streaming to multiple hubs
  Future<bool> startStreamingToMultiple(List<Hub> hubs) async {
    for (var hub in hubs) {
      await startStreaming(hub.ipAddress, hub.audioPort);
    }
  }

  // Send audio to specific hub
  Future<bool> sendAudioData(String hubId, Uint8List audioData) async { ... }

  // Broadcast audio to all hubs
  Future<bool> broadcastAudioData(Uint8List audioData) async {
    for (var hubId in _sockets.keys) {
      await sendAudioData(hubId, audioData);
    }
  }
}
```

### 3.4 Update UI for Multi-Hub

**File: `mobile_app/lib/screens/home_screen.dart`**

Add hub selection UI:
```dart
// Hub selector dropdown or tab bar
Widget _buildHubSelector(BuildContext context, HubManager hubManager) {
  return DropdownButton<String>(
    value: hubManager.selectedHub?.hubId,
    items: hubManager.hubs.map((hub) {
      return DropdownMenuItem(
        value: hub.hubId,
        child: Row(
          children: [
            Icon(Icons.hub),
            SizedBox(width: 8),
            Text('${hub.customName} (${hub.location})'),
            if (hub.isStreaming) Icon(Icons.volume_up, color: Colors.green),
          ],
        ),
      );
    }).toList(),
    onChanged: (hubId) {
      if (hubId != null) hubManager.selectHub(hubId);
    },
  );
}

// Hub list view
Widget _buildHubList(BuildContext context, HubManager hubManager) {
  return ListView.builder(
    itemCount: hubManager.hubs.length,
    itemBuilder: (context, index) {
      final hub = hubManager.hubs[index];
      return Card(
        child: ListTile(
          leading: CircleAvatar(
            backgroundColor: hub.isConnected ? Colors.green : Colors.grey,
            child: Icon(Icons.hub),
          ),
          title: Text(hub.customName),
          subtitle: Text('${hub.location} • ${hub.speakers.length} speakers'),
          trailing: PopupMenuButton(
            itemBuilder: (context) => [
              PopupMenuItem(value: 'select', child: Text('Select Hub')),
              PopupMenuItem(value: 'stream', child: Text('Stream to This Hub')),
              PopupMenuItem(value: 'settings', child: Text('Hub Settings')),
            ],
          ),
        ),
      );
    },
  );
}

// Streaming mode selector
Widget _buildStreamingModeSelector() {
  return SegmentedButton<StreamingMode>(
    segments: [
      ButtonSegment(value: StreamingMode.single, label: Text('Selected Hub')),
      ButtonSegment(value: StreamingMode.all, label: Text('All Hubs')),
      ButtonSegment(value: StreamingMode.auto, label: Text('Auto (Proximity)')),
    ],
    selected: {_streamingMode},
    onSelectionChanged: (Set<StreamingMode> selected) {
      setState(() => _streamingMode = selected.first);
    },
  );
}
```

---

## Phase 4: Proximity-Based Hub Selection

### 4.1 Bluetooth RSSI Scanning

Use phone's Bluetooth to scan for ESP32 hubs and measure signal strength:

**File: `mobile_app/lib/services/proximity_service.dart`**
```dart
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

class ProximityService {
  // Scan for hubs via Bluetooth
  Future<Map<String, int>> scanHubProximity() async {
    Map<String, int> hubRSSI = {};

    // Scan for Bluetooth devices
    await FlutterBluePlus.startScan(timeout: Duration(seconds: 5));

    var subscription = FlutterBluePlus.scanResults.listen((results) {
      for (ScanResult r in results) {
        // Check if device is ESP32 hub (by name pattern or service UUID)
        if (r.device.name.startsWith('ESP32Hub')) {
          String hubId = extractHubIdFromBluetoothName(r.device.name);
          hubRSSI[hubId] = r.rssi;
        }
      }
    });

    await Future.delayed(Duration(seconds: 5));
    await FlutterBluePlus.stopScan();
    subscription.cancel();

    return hubRSSI;
  }

  // Get closest hub based on RSSI
  String? getClosestHub(Map<String, int> hubRSSI) {
    if (hubRSSI.isEmpty) return null;

    // Highest RSSI = strongest signal = closest
    return hubRSSI.entries.reduce((a, b) => a.value > b.value ? a : b).key;
  }
}
```

### 4.2 Auto-Selection Logic

```dart
// In HubManager
Future<void> autoSelectHubByProximity() async {
  final proximityService = ProximityService();
  final hubRSSI = await proximityService.scanHubProximity();

  final closestHubId = proximityService.getClosestHub(hubRSSI);

  if (closestHubId != null && _hubs.containsKey(closestHubId)) {
    selectHub(closestHubId);
    print('[HubManager] Auto-selected closest hub: $closestHubId');
  }
}

// Periodic proximity updates
Timer.periodic(Duration(seconds: 10), (timer) {
  if (autoSelectEnabled) {
    autoSelectHubByProximity();
  }
});
```

### 4.3 ESP32 Bluetooth Advertising Enhancement

**File: `esp32_firmware/src/main.cpp`**

Ensure ESP32 advertises its Bluetooth presence even when not paired:
```cpp
void enableBluetoothAdvertising() {
    // Set device name to include hub ID
    String btName = String(BT_DEVICE_NAME_PREFIX) + "_" + hubManager.getHubId();
    esp_bt_dev_set_device_name(btName.c_str());

    // Enable Bluetooth discoverability
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    Serial.println("[BT] Advertising enabled for proximity detection");
}
```

---

## Phase 5: Advanced Features (Future Enhancements)

### 5.1 Synchronized Multi-Hub Playback
- Network time sync (NTP)
- Audio buffer timestamping
- Master hub election for sync coordination
- Jitter buffering for perfect sync

### 5.2 Smart Room Detection
- Use phone's Bluetooth to detect which room user is in
- Auto-switch to that room's hub
- Follow-me audio (audio follows user between rooms)

### 5.3 Hub Groups and Zones
- Group hubs by floor (Upstairs, Downstairs)
- Group by usage (Party Mode, Quiet Hours)
- Create custom zones

### 5.4 Hub-to-Hub Communication
- Direct ESP32-to-ESP32 communication
- Share speaker lists
- Coordinated volume control
- Failover (if one hub fails, another takes over)

---

## Implementation Timeline

### Phase 1: Testing (Current)
- **Duration**: 1-2 weeks
- **Goal**: Verify single-hub architecture works perfectly
- **Deliverables**: Working system, bug list, performance metrics

### Phase 2: ESP32 Multi-Hub Firmware (Next)
- **Duration**: 3-5 hours development
- **Changes**: mDNS updates, hub identification
- **Testing**: Deploy to multiple ESP32s

### Phase 3: Flutter Multi-Hub App (Next)
- **Duration**: 8-12 hours development
- **Changes**: Multi-hub management, UI updates
- **Testing**: App with multiple hubs

### Phase 4: Proximity Selection (Later)
- **Duration**: 4-6 hours development
- **Changes**: Bluetooth scanning, auto-selection
- **Testing**: Movement between rooms

### Phase 5: Advanced Features (Future)
- **Duration**: 20-40 hours total
- **Priority**: Based on user needs
- **Testing**: Real-world multi-room scenarios

---

## Migration Path from Single to Multi-Hub

### User Experience:
1. **Current users** (single hub): No changes needed, works as-is
2. **Add second hub**: App auto-discovers, prompts to set up
3. **Hub selection**: Manual at first, auto-proximity later
4. **Gradual rollout**: Add hubs room-by-room

### Backward Compatibility:
- Single-hub mode still works
- App detects single vs multi-hub
- No firmware breaking changes
- Settings migrate automatically

---

## Testing Strategy

### Single Hub Testing (Phase 1):
- [ ] Discovery and connection
- [ ] All WebSocket commands
- [ ] Bluetooth speaker pairing
- [ ] Audio streaming quality
- [ ] Multi-speaker sync
- [ ] Edge cases (disconnects, errors)

### Multi-Hub Testing (Phase 3):
- [ ] Discover multiple hubs
- [ ] Switch between hubs
- [ ] Stream to different hubs
- [ ] Stream to all hubs simultaneously
- [ ] Proximity-based selection
- [ ] Hub offline/online handling

### Performance Metrics to Track:
- mDNS discovery time
- WebSocket connection time
- Bluetooth pairing time
- Audio latency (phone → hub → speaker)
- Multi-speaker sync accuracy
- Network bandwidth usage
- ESP32 CPU and memory usage
- Battery drain on phone

---

## Known Limitations and Solutions

### Limitation 1: Bluetooth Range
- **Issue**: Bluetooth typically 10-30 meters
- **Solution**: One hub per room ensures coverage

### Limitation 2: WiFi Network Load
- **Issue**: Streaming to many hubs simultaneously
- **Solution**: Use multicast UDP (future enhancement)

### Limitation 3: ESP32 A2DP Library
- **Issue**: May not support 7 simultaneous connections
- **Solution**: Test incrementally, may need to reduce to 3-5

### Limitation 4: Audio Sync Between Hubs
- **Issue**: Network jitter causes slight delays
- **Solution**: Timestamp-based sync with jitter buffer

---

## Dependencies

### ESP32 Firmware:
- ESP32-A2DP library (current)
- ArduinoJson (current)
- Preferences (current)
- WiFi/mDNS (current)
- NTP client (for multi-hub sync - new)

### Flutter App:
- Provider (current)
- nsd (mDNS - current)
- web_socket_channel (current)
- flutter_blue_plus (for proximity - new)
- shared_preferences (current)

---

## Next Steps (After Phase 1 Testing)

1. **Document all bugs and issues found**
2. **Performance benchmarking results**
3. **User feedback on single-hub experience**
4. **Decision**: Proceed with multi-hub or improve single-hub first?
5. **If proceeding**: Start Phase 2 (ESP32 firmware updates)

---

## Notes

- Keep this document updated as we progress
- Add actual test results to testing checklists
- Document any architectural changes made during testing
- Track all bugs and their fixes
- Measure performance metrics for baseline

---

**Document Version**: 1.0
**Date Created**: 2025-12-31
**Last Updated**: 2025-12-31
**Status**: Planning / Pre-Implementation
