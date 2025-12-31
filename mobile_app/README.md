# Multi-Room Speaker Control App - Phase 1

Flutter-based mobile application for controlling ESP32 multi-room speakers. Works on both Android and iOS.

## Features (Phase 1)

### Device Discovery & Management
- ✅ Automatic mDNS/Bonjour discovery of ESP32 speakers
- ✅ Real-time WebSocket connection to each speaker
- ✅ Connection status monitoring
- ✅ Device identification (flash LED)
- ✅ Custom device naming
- ✅ Room assignment

### Audio Control
- ✅ Volume control (0-100% with slider)
- ✅ Mute/unmute toggle
- ✅ Power on/off
- ✅ Audio source selection (Bluetooth, Line-In, Network)
- ✅ Equalizer presets (Flat, Rock, Jazz, Classical, Pop)
- ✅ Custom bass/treble adjustment (-12dB to +12dB)

### Room Management
- ✅ Group speakers by room
- ✅ Room-level volume control
- ✅ Room mute/unmute
- ✅ Power control for all speakers in a room
- ✅ Custom room icons

### User Interface
- ✅ Material Design 3 (Material You)
- ✅ Dark mode support (system default)
- ✅ Three-tab navigation (Speakers, Rooms, Settings)
- ✅ Pull-to-refresh for speaker discovery
- ✅ Real-time status updates

## Prerequisites

### Development Tools
- [Flutter SDK](https://flutter.dev/docs/get-started/install) (3.0.0 or higher)
- [Android Studio](https://developer.android.com/studio) (for Android development)
- [Xcode](https://developer.apple.com/xcode/) (for iOS development, macOS only)
- Git

### Runtime Requirements
- Android 5.0 (API 21) or higher
- iOS 12.0 or higher
- WiFi network (same network as ESP32 speakers)

## Installation & Setup

### 1. Clone the Repository

```bash
git clone <repository-url>
cd Multi_room_bluetooth/mobile_app
```

### 2. Install Dependencies

```bash
flutter pub get
```

### 3. Verify Flutter Installation

```bash
flutter doctor
```

Fix any issues reported by `flutter doctor` before proceeding.

### 4. Run on Device/Emulator

#### Android

```bash
# Connect Android device via USB or start emulator
flutter run
```

#### iOS (macOS only)

```bash
# Connect iOS device or start simulator
cd ios
pod install
cd ..
flutter run
```

## Project Structure

```
mobile_app/
├── lib/
│   ├── main.dart                      # App entry point
│   ├── models/                        # Data models
│   │   ├── speaker_device.dart        # Speaker device model
│   │   └── room.dart                  # Room model
│   ├── services/                      # Business logic & services
│   │   ├── device_manager.dart        # State management & device orchestration
│   │   ├── websocket_service.dart     # WebSocket communication
│   │   └── discovery_service.dart     # mDNS device discovery
│   ├── screens/                       # UI screens
│   │   ├── home_screen.dart           # Main screen with tabs
│   │   ├── device_control_screen.dart # Individual device control
│   │   ├── rooms_screen.dart          # Room management
│   │   └── settings_screen.dart       # App settings
│   └── widgets/                       # Reusable widgets
│       └── speaker_card.dart          # Speaker list item widget
├── android/                           # Android-specific files
├── ios/                               # iOS-specific files
├── test/                              # Unit tests
├── pubspec.yaml                       # Dependencies & app metadata
└── README.md                          # This file
```

## Usage

### First-Time Setup

1. **Launch the App**
   - The app will automatically start searching for speakers

2. **Ensure Speakers are On**
   - Make sure ESP32 speakers are powered on
   - Verify speakers are connected to WiFi
   - Check that phone/tablet is on the same WiFi network

3. **Discover Speakers**
   - Tap the refresh icon in the top-right corner
   - Wait 5-30 seconds for speakers to appear
   - Speakers will automatically connect when discovered

### Controlling Speakers

#### Individual Speaker Control
- Tap on any speaker card to open detailed controls
- Adjust volume with slider
- Tap mute icon to mute/unmute
- Tap power icon to turn on/off
- Use "Identify" button to flash LED on physical device

#### Room Control
- Navigate to "Rooms" tab
- Control all speakers in a room simultaneously
- Adjust room-wide volume
- Power on/off all speakers in room

### Managing Rooms

1. **Create a Room**
   - Go to "Rooms" tab
   - Tap the "+" icon
   - Enter room name and choose an icon
   - Tap "Create"

2. **Assign Speakers to Rooms**
   - Tap on a speaker card
   - Edit device information
   - Change room assignment

### Troubleshooting

#### Speakers Not Discovered

**Problem:** App doesn't find any speakers

**Solutions:**
- Verify speakers are powered on and connected to WiFi
- Ensure phone is on the **same WiFi network** as speakers
- Tap the refresh button to scan again
- Check router settings - mDNS must be enabled
- Try restarting the app
- On Android 13+, grant "Nearby Devices" permission

#### Connection Fails

**Problem:** Speaker discovered but won't connect

**Solutions:**
- Check speaker's WiFi connection (LED status)
- Verify firewall isn't blocking ports 80 and 81
- Restart the speaker (power cycle)
- Forget and rediscover the speaker

#### Volume Control Not Working

**Problem:** Can't adjust volume

**Solutions:**
- Verify speaker is powered on (not in standby)
- Check connection status indicator (green dot)
- Tap speaker card to reconnect
- Ensure speaker isn't muted

## Permissions

### Android

The app requires the following permissions:

- **INTERNET** - To communicate with speakers over WiFi
- **ACCESS_NETWORK_STATE** - To check network connectivity
- **ACCESS_WIFI_STATE** - To access WiFi information
- **CHANGE_WIFI_MULTICAST_STATE** - For mDNS discovery
- **NEARBY_WIFI_DEVICES** (Android 13+) - For local network discovery

All permissions are automatically requested when needed.

### iOS

The app requires:

- **NSLocalNetworkUsageDescription** - For discovering speakers on local network
- **NSBonjourServices** - For mDNS/Bonjour discovery

Permissions are requested automatically on first use.

## Development

### Running in Debug Mode

```bash
flutter run
```

### Running Tests

```bash
flutter test
```

### Building Release APK (Android)

```bash
flutter build apk --release
```

Output: `build/app/outputs/flutter-apk/app-release.apk`

### Building iOS App (macOS only)

```bash
flutter build ios --release
```

Then open `ios/Runner.xcworkspace` in Xcode to archive and distribute.

### Code Formatting

```bash
flutter format lib/
```

### Analyzing Code

```bash
flutter analyze
```

## Architecture

### State Management

The app uses **Provider** for state management:

- `DeviceManager` - Central state manager for all speakers and rooms
- Notifies UI of changes automatically
- Persists data to local storage (SharedPreferences)

### Communication

- **mDNS Discovery** - Finds speakers broadcasting `_esp32speaker._tcp` service
- **WebSocket** - Real-time bidirectional communication on port 81
- **JSON** - All messages use JSON format

### Data Persistence

- Device information saved to `SharedPreferences`
- Room configurations saved locally
- Automatic save on changes
- Restored on app launch

## Phase 1 Limitations

The following features are planned for future phases:

**Not Included in Phase 1:**
- Stereo speaker pairing (Phase 2)
- Scheduled playback / alarms (Phase 2)
- Sleep timer (Phase 2)
- Multi-user support (Phase 3)
- Guest mode (Phase 3)
- Streaming service integration (Phase 3)
- Voice control (Phase 3)
- Cloud backup & sync (Phase 3)
- Room layout visualization (Future phase)

## Dependencies

Key dependencies used in this project:

- `provider` - State management
- `web_socket_channel` - WebSocket communication
- `nsd` - mDNS/Bonjour service discovery
- `shared_preferences` - Local data storage
- `http` - HTTP requests (future use)

See `pubspec.yaml` for complete list.

## Contributing

This is Phase 1 of the Multi-Room Speaker System. Contributions welcome!

Please refer to the planning documents:
- `ESP32_Speaker_App_Implementation_Phase1-3.md`
- `ESP32_Speaker_App_GUI_Room_Layout_Feature.md`

## Known Issues

- [ ] Audio source switching not yet implemented in UI
- [ ] EQ adjustments not yet connected to WebSocket
- [ ] Theme switching not yet implemented
- [ ] Clear data function not yet implemented

## Version History

### v1.0.0 - Phase 1 (Current)
- Initial release
- Device discovery via mDNS
- WebSocket communication
- Volume, mute, power control
- Room management
- EQ presets and custom adjustments
- Local data persistence
- Material Design 3 UI
- Dark mode support

### Roadmap
- **Phase 2**: Stereo pairing, scheduling, sleep timer
- **Phase 3**: Multi-user, streaming services, voice control

## Support

For issues or questions:
- Check the ESP32 firmware README
- Review the planning documents
- Submit an issue on GitHub

## License

[Specify your license here]

---

**App Version:** 1.0.0
**Phase:** 1
**Flutter SDK:** 3.0.0+
**Last Updated:** December 2025
