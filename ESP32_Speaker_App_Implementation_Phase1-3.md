# ESP32 Multi-Speaker Control App - Implementation Guide
## Phases 1-3 Development Roadmap

---

## PHASE 1: Core Functionality & Foundation

### 1.1 Device Discovery & Connection

**Requirements:**
- Auto-discovery of ESP32 speakers on local network
- Support for both Android and iOS platforms
- Reliable connection management

**Technical Implementation:**
- Use mDNS/Bonjour for device discovery
- Implement WebSocket or MQTT for real-time communication
- Fallback to HTTP REST API for basic commands
- Connection timeout handling (3-5 second threshold)
- Automatic reconnection logic with exponential backoff

**ESP32 Side:**
```
- Broadcast mDNS service name: "_esp32speaker._tcp"
- Include device ID, firmware version, and IP in TXT records
- Maintain persistent WebSocket connection
- Heartbeat/ping every 30 seconds
```

**App Side:**
```
- Scan network every 5 seconds when on device discovery screen
- Cache discovered devices
- Show connection status indicator (connected/disconnected/connecting)
- Store last known IP addresses for faster reconnection
```

### 1.2 Device Identification & Management

**Features:**
- Unique device ID (MAC address based)
- Custom naming capability
- Device information display
- Signal strength indicator

**Data Structure:**
```json
{
  "deviceId": "ESP32_AB:CD:EF:01:23:45",
  "customName": "Living Room Speaker",
  "room": "Living Room",
  "ipAddress": "192.168.1.100",
  "firmwareVersion": "1.0.2",
  "signalStrength": -45,
  "lastSeen": "2025-01-15T10:30:00Z",
  "groups": ["Downstairs", "Party Mode"]
}
```

**UI Components:**
- Device list view (default)
- Device detail screen
- Rename dialog
- Delete/forget device option
- "Identify" button (plays sound/flashes LED on ESP32)

### 1.3 Room Assignment & Grouping

**Features:**
- Assign speakers to predefined rooms
- Create custom room names
- Move speakers between rooms
- Visual indication of room membership

**Room Management:**
- Add/edit/delete rooms
- Reorder rooms
- Set room icons (optional)
- Bulk assignment (assign multiple speakers at once)

**Data Structure:**
```json
{
  "roomId": "room_001",
  "roomName": "Living Room",
  "icon": "sofa",
  "speakers": ["ESP32_AB:CD:EF:01:23:45", "ESP32_AB:CD:EF:01:23:46"],
  "groupVolume": 75,
  "isMuted": false
}
```

### 1.4 Audio Control (Individual)

**Basic Controls:**
- Volume slider (0-100%)
- Mute/unmute toggle
- Power on/off
- Volume up/down buttons (±5% increments)

**Advanced Controls:**
- Equalizer settings:
  - Bass boost (-12dB to +12dB)
  - Treble adjustment (-12dB to +12dB)
  - Presets (Rock, Jazz, Classical, Pop, Flat)
- Balance control (if stereo paired)

**Real-time Feedback:**
- Volume level indicator
- Current playback status
- Audio source indicator

### 1.5 Group Control

**Features:**
- Master volume for entire group/room
- Synchronized mute/unmute
- Group power control
- Individual override capability

**Synchronization:**
- Send commands to all group members simultaneously
- Timestamp-based playback sync (±20ms accuracy target)
- Handle partial failures gracefully

### 1.6 User Interface (Phase 1)

**Screens:**
1. **Home/Dashboard:**
   - List of all speakers or rooms
   - Quick access to favorites
   - Overall system status
   - Search/filter capability

2. **Device Control:**
   - Individual speaker controls
   - Current settings display
   - Quick actions (mute, volume presets)

3. **Settings:**
   - App preferences
   - Network settings
   - Theme selection (dark/light mode)
   - About/version info

**Navigation:**
- Bottom tab bar (Home, Rooms, Settings)
- Swipe gestures for common actions
- Long-press for additional options

### 1.7 Data Persistence

**Local Storage:**
- Device configurations
- Room assignments
- User preferences
- Connection history

**Sync Considerations:**
- Save settings locally on device
- Optional cloud backup (Phase 3)
- Export/import configuration files

---

## PHASE 2: Enhanced Features

### 2.1 Audio Source Selection

**Supported Sources:**
- Bluetooth input
- Line-in/AUX input
- Network streaming (if ESP32 capable)
- Airplay/DLNA (future consideration)

**Implementation:**
- Source selection dropdown per speaker
- Auto-switch capability
- Priority handling (Bluetooth overrides line-in, etc.)

**UI:**
- Source indicator icon
- Quick source switch button
- Visual feedback when source changes

### 2.2 Scheduled Playback

**Alarm Clock Feature:**
- Set wake-up time
- Choose speaker(s) for alarm
- Volume ramp-up (gradual increase)
- Snooze functionality (5-10 minute intervals)
- Repeat schedule (weekdays, weekends, custom)

**Timer Functions:**
- Play for X minutes then stop
- Start playing at specific time
- Recurring schedules

**Data Structure:**
```json
{
  "scheduleId": "sched_001",
  "type": "alarm",
  "time": "07:00",
  "days": ["MON", "TUE", "WED", "THU", "FRI"],
  "speakers": ["ESP32_001", "ESP32_002"],
  "volume": 50,
  "rampDuration": 300,
  "enabled": true
}
```

### 2.3 Sleep Timer

**Features:**
- Countdown timer (15, 30, 45, 60, 90 minutes, custom)
- Fade-out option (gradual volume decrease)
- Auto-power-off after timer expires
- Per-speaker or per-group

**UI:**
- Quick access from playback controls
- Remaining time display
- Cancel/extend options

### 2.4 Stereo Pairing

**Configuration:**
- Pair two speakers as left/right channels
- Automatic channel assignment based on relative position
- Manual override for channel selection
- Unpair functionality

**Synchronization:**
- Locked volume control (maintain stereo balance)
- Synchronized playback (critical timing)
- Shared settings (EQ, source)

**Visual Indicators:**
- L/R badges on paired speakers
- Connection line between paired devices (in list view)

### 2.5 Volume Leveling

**Purpose:**
- Compensate for different speaker sensitivities
- Ensure consistent perceived volume across all speakers

**Implementation:**
- Calibration mode (play test tone, measure perceived loudness)
- Per-speaker volume offset (-20dB to +20dB)
- Global volume scaling

**User Flow:**
1. Play reference tone through all speakers
2. User adjusts each speaker to match perceived volume
3. App stores offset values
4. Applied automatically to all volume commands

### 2.6 Firmware Management

**OTA Updates:**
- Check for firmware updates
- Download and stage updates
- Update single or multiple devices
- Rollback capability (if update fails)

**Update Process:**
1. Check version on device vs. available version
2. Notify user of available update
3. Download firmware binary
4. Upload to ESP32 via HTTP POST
5. ESP32 reboots and applies update
6. Verify successful update

**Safety Features:**
- Don't update during active playback
- Battery/power check before update
- Progress indicator
- Automatic retry on failure

---

## PHASE 3: Advanced Features & Integration

### 3.1 Multi-User Support

**User Profiles:**
- Create multiple user accounts
- Separate preferences per user
- Individual room/group configurations
- Shared device pool

**Permissions:**
- Admin: full control, can add/remove users
- User: standard control, can't modify system settings
- Guest: limited control (volume only, no configuration)

**Implementation:**
- Local authentication (PIN/biometric)
- Optional cloud-based account system
- Profile switching in app settings

### 3.2 Guest Mode

**Features:**
- Temporary access link/QR code
- Limited functionality (volume, play/pause only)
- Time-limited access (expires after X hours)
- No ability to change settings or configurations

**Use Cases:**
- Party guests controlling music
- Temporary visitors
- Service technicians (view-only mode)

**Security:**
- Generate one-time access codes
- Automatic expiration
- Admin can revoke access anytime

### 3.3 Streaming Service Integration

**Target Services:**
- Spotify Connect
- Apple Music (Airplay)
- YouTube Music
- Pandora
- Amazon Music

**Implementation Approach:**
- Use service-specific APIs/SDKs
- OAuth authentication for user accounts
- Playback control through app
- Display currently playing track info

**Challenges:**
- Each service has different APIs
- DRM and licensing requirements
- May need cloud backend for some services

**Phase 3 Scope:**
- Start with one service (Spotify recommended)
- Prove concept and user demand
- Expand to others in later phases

### 3.4 Voice Control Integration

**Supported Assistants:**
- Amazon Alexa
- Google Assistant
- Apple Siri Shortcuts

**Voice Commands:**
- "Turn on living room speaker"
- "Set bedroom volume to 50%"
- "Play music in all downstairs speakers"
- "Mute the kitchen"

**Implementation:**
- Register custom intents/actions
- Map voice commands to app functions
- Requires cloud backend for most assistants

### 3.5 Scenario Presets

**Feature:**
- Save complete system state as named preset
- One-tap activation of preset
- Presets include: volumes, groups, muting, EQ settings

**Example Presets:**
- "Movie Night": Living room speakers, volume 80%, bass boost
- "Dinner Party": Kitchen + dining room, volume 40%, jazz EQ
- "Morning Routine": Bedroom speaker, volume 30%, alarm schedule
- "Sleep Mode": All speakers off except bedroom, volume 20%

**Data Structure:**
```json
{
  "presetId": "preset_001",
  "name": "Movie Night",
  "icon": "movie",
  "settings": {
    "speakers": {
      "ESP32_001": {"power": true, "volume": 80, "mute": false, "eq": "cinema"},
      "ESP32_002": {"power": true, "volume": 80, "mute": false, "eq": "cinema"},
      "ESP32_003": {"power": false}
    },
    "groups": ["Living Room"]
  }
}
```

**UI:**
- Preset library screen
- Create from current state
- Edit existing presets
- Quick access widget/shortcut

### 3.6 Network Diagnostics

**Tools:**
- Ping test to each speaker
- Latency measurement
- Packet loss detection
- Signal strength history graph
- Connection log

**Troubleshooting Assistant:**
- Identify connection issues
- Suggest fixes (move closer to router, check WiFi band, etc.)
- Test connectivity before reporting issue
- Export diagnostic report

**Features:**
- Real-time network quality indicator
- Alert when speaker goes offline
- Automatic problem detection
- Help documentation links

### 3.7 Cloud Backup & Sync

**What to Backup:**
- Device configurations
- Room assignments
- User preferences
- Presets and schedules
- Firmware update history

**Benefits:**
- Restore settings on new phone
- Sync across multiple devices (phone + tablet)
- Backup before factory reset

**Implementation:**
- Encrypted cloud storage
- Periodic auto-backup (daily)
- Manual backup/restore options
- Conflict resolution (last-write-wins or user choice)

**Privacy:**
- User opt-in required
- Anonymous telemetry (optional)
- GDPR/privacy law compliance

---

## Technical Architecture Recommendations

### Communication Protocol Stack

**Option 1: MQTT (Recommended)**
- Lightweight, efficient for IoT
- Built-in QoS levels
- Topic-based pub/sub model
- Good for real-time updates

**Implementation:**
- Mosquitto broker on local network or cloud
- Topics: `speaker/{deviceId}/command`, `speaker/{deviceId}/status`
- Retained messages for current state
- Last Will & Testament for offline detection

**Option 2: WebSockets**
- Bidirectional real-time communication
- Works well for local network
- Simple implementation

**Option 3: HTTP REST API**
- Fallback for basic commands
- Easy debugging
- Universal compatibility

### Mobile App Tech Stack

**Cross-Platform (Recommended):**
- **Flutter:** Single codebase, native performance, rich UI
- **React Native:** Large community, JS ecosystem
- **Xamarin:** C# developers, tight MS integration

**Native:**
- **Android:** Kotlin + Jetpack Compose
- **iOS:** Swift + SwiftUI

### Data Storage

**Local:**
- **SQLite:** Structured data (devices, rooms, schedules)
- **SharedPreferences/UserDefaults:** Simple key-value (settings)
- **Encrypted storage:** Sensitive data (user credentials)

**Cloud (Phase 3):**
- **Firebase:** Real-time sync, authentication, cloud storage
- **AWS Amplify:** Serverless backend
- **Custom API:** Maximum control

### ESP32 Firmware Requirements

**Core Libraries:**
- WiFi management (WiFiManager for easy setup)
- mDNS responder
- WebSocket or MQTT client
- OTA update capability
- Audio processing (I2S for DAC)

**Persistent Storage:**
- SPIFFS or LittleFS for settings
- NVS (Non-Volatile Storage) for WiFi credentials

---

## Development Milestones

### Phase 1 Milestones (3-4 months)
1. **Week 1-2:** Project setup, architecture design
2. **Week 3-4:** Device discovery and basic connection
3. **Week 5-6:** Individual speaker control (volume, mute, power)
4. **Week 7-8:** Room assignment and grouping
5. **Week 9-10:** Group control and synchronization
6. **Week 11-12:** UI polish, testing, bug fixes

### Phase 2 Milestones (2-3 months)
1. **Week 1-2:** Audio source selection
2. **Week 3-4:** Scheduled playback and alarms
3. **Week 5-6:** Sleep timer and stereo pairing
4. **Week 7-8:** Volume leveling and firmware updates
5. **Week 9-10:** Testing and refinement

### Phase 3 Milestones (3-4 months)
1. **Week 1-2:** Multi-user support and permissions
2. **Week 3-4:** Guest mode implementation
3. **Week 5-6:** Streaming service integration (single service)
4. **Week 7-8:** Voice control integration (single assistant)
5. **Week 9-10:** Scenario presets
6. **Week 11-12:** Cloud backup, network diagnostics, final polish

---

## Testing Strategy

### Unit Testing
- Individual component testing
- Mock ESP32 devices for development
- Automated test suites

### Integration Testing
- Multi-device scenarios
- Network failure simulation
- Synchronization accuracy tests

### User Acceptance Testing
- Beta testing program (10-20 users)
- Gather feedback on UI/UX
- Identify edge cases

### Performance Testing
- Latency measurements
- Connection reliability over time
- Battery usage on mobile devices
- Memory usage

---

## Security Considerations

### Network Security
- WPA2/WPA3 encrypted WiFi required
- Local network isolation (speakers don't need internet)
- Optional VPN support for remote access

### App Security
- Secure storage of credentials
- Certificate pinning for cloud connections
- Input validation on all commands

### ESP32 Security
- Encrypted firmware updates
- Secure boot (optional)
- No default passwords
- Rate limiting on commands

---

## Future Expansion Ideas (Phase 4+)

- Room correction using phone microphone
- Audio analytics and usage patterns
- Smart home integration (Home Assistant, SmartThings)
- Mesh network between speakers
- Multi-room audio with different sources
- Audio streaming from phone to speakers
- Playlist management
- Audio effects and processing
- Integration with home automation scenes
- Energy usage monitoring

---

## Resources & References

### Development Tools
- Android Studio / Xcode
- Flutter SDK / React Native CLI
- MQTT broker (Mosquitto)
- Postman for API testing

### ESP32 Resources
- ESP-IDF documentation
- Arduino core for ESP32
- Audio codec datasheets
- OTA update examples

### Design Resources
- Material Design guidelines (Android)
- Human Interface Guidelines (iOS)
- Icon libraries (Material Icons, SF Symbols)
- Color palette tools

---

## Notes

- Prioritize stability and reliability over feature richness
- Focus on user experience - make it intuitive
- Keep ESP32 firmware modular for easier updates
- Document API for future third-party integrations
- Consider open-sourcing to build community

---

**Document Version:** 1.0  
**Last Updated:** December 2025  
**Author:** Development Planning Team
