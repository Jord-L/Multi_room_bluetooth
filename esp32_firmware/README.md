# ESP32 Multi-Room Speaker Firmware - Phase 1

This is the Phase 1 firmware for the ESP32 Multi-Room Speaker System. It implements core functionality including device discovery, WiFi connectivity, WebSocket communication, and basic audio control.

## Features (Phase 1)

### Device Discovery & Connection
- ✅ mDNS/Bonjour service broadcasting (`_esp32speaker._tcp`)
- ✅ Auto-discovery on local network
- ✅ WebSocket server for real-time communication
- ✅ HTTP REST API support (future)
- ✅ Connection timeout handling
- ✅ Automatic reconnection with exponential backoff

### Device Identification
- ✅ Unique device ID (MAC address based)
- ✅ Custom naming capability
- ✅ Room assignment
- ✅ Device information broadcasting
- ✅ Signal strength reporting
- ✅ Identify function (LED flash)

### Audio Control
- ✅ Volume control (0-100%)
- ✅ Mute/unmute
- ✅ Power on/off
- ✅ Volume adjustment (±5% increments)
- ✅ Equalizer settings (Bass/Treble: -12dB to +12dB)
- ✅ EQ presets (Flat, Rock, Jazz, Classical, Pop)
- ✅ I2S audio output support

### Network Management
- ✅ WiFi connection with WiFiManager
- ✅ Configuration portal for easy setup
- ✅ Persistent WiFi credentials
- ✅ Network monitoring and auto-reconnect
- ✅ mDNS service with TXT records

### Data Persistence
- ✅ Device settings storage (Preferences)
- ✅ Audio settings persistence
- ✅ Custom name and room assignment
- ✅ Auto-save on changes

### OTA Updates
- ✅ Over-the-air firmware updates
- ✅ ArduinoOTA support
- ✅ Safe update process (stops audio during update)

## Hardware Requirements

### Minimum Requirements
- ESP32 development board (ESP32-WROOM, ESP32-DevKitC, etc.)
- I2S DAC module (e.g., PCM5102, MAX98357A, UDA1334A)
- Speaker(s)
- Power supply (5V/2A recommended)

### Recommended Hardware
- ESP32-WROOM-32 or ESP32-WROVER module
- High-quality I2S DAC (PCM5102A or similar)
- Amplified speakers or external amplifier
- Stable 5V power supply

## Pin Configuration

Default I2S pin configuration (can be modified in `config.h`):

```
I2S_BCLK_PIN    = GPIO 26  (Bit Clock)
I2S_LRC_PIN     = GPIO 25  (Left/Right Clock / Word Select)
I2S_DOUT_PIN    = GPIO 22  (Data Out)
LED_STATUS_PIN  = GPIO 2   (Status LED - built-in LED)
```

### Wiring Example (PCM5102 DAC)
```
ESP32          PCM5102
-----          -------
GPIO 26   -->  BCK
GPIO 25   -->  LCK
GPIO 22   -->  DIN
GND       -->  GND
5V        -->  VIN
              (Connect XMT to GND)
              (Connect FLT and FMT to GND)
              (Connect SCL to GND for 44.1kHz)
```

## Software Requirements

### Required Tools
- [PlatformIO](https://platformio.org/) (recommended) OR Arduino IDE
- USB drivers for your ESP32 board

### Dependencies (auto-installed by PlatformIO)
- WiFiManager (for easy WiFi setup)
- WebSockets library (v2.4.1+)
- ArduinoJson (v6.21.3+)
- ESP8266Audio library (v1.9.7+)

## Installation & Setup

### Method 1: PlatformIO (Recommended)

1. **Install PlatformIO**
   - Install [VS Code](https://code.visualstudio.com/)
   - Install PlatformIO IDE extension from VS Code marketplace

2. **Clone the Repository**
   ```bash
   git clone <repository-url>
   cd Multi_room_bluetooth/esp32_firmware
   ```

3. **Open Project in PlatformIO**
   - Open VS Code
   - File → Open Folder → Select `esp32_firmware` directory
   - PlatformIO will automatically detect the project

4. **Configure Settings (Optional)**
   - Edit `include/config.h` to customize:
     - Device name prefix
     - I2S pin assignments
     - WiFi portal settings
     - Debug levels

5. **Build the Firmware**
   ```bash
   pio run
   ```

6. **Upload to ESP32**
   - Connect ESP32 via USB
   - Click "Upload" button in PlatformIO
   - OR run: `pio run --target upload`

7. **Monitor Serial Output**
   ```bash
   pio device monitor
   ```

### Method 2: Arduino IDE

1. **Install Arduino IDE**
   - Download from [arduino.cc](https://www.arduino.cc/en/software)

2. **Install ESP32 Board Support**
   - File → Preferences
   - Add to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Tools → Board → Boards Manager
   - Search "ESP32" and install "esp32 by Espressif Systems"

3. **Install Libraries**
   - Sketch → Include Library → Manage Libraries
   - Install the following:
     - WiFiManager by tzapu
     - WebSockets by Markus Sattler
     - ArduinoJson by Benoit Blanchon
     - ESP8266Audio by Earle F. Philhower

4. **Open the Project**
   - Copy all files from `src/` to a new sketch folder
   - Copy all files from `include/` to the same folder

5. **Configure Board**
   - Tools → Board → ESP32 Arduino → ESP32 Dev Module
   - Tools → Upload Speed → 921600
   - Tools → Port → (Select your ESP32's COM port)

6. **Upload**
   - Click "Upload" button

## First-Time Setup

### WiFi Configuration

1. **Power on the ESP32**
   - The device will create a WiFi access point named `ESP32_Speaker_Setup`

2. **Connect to the AP**
   - Use your phone or computer to connect to `ESP32_Speaker_Setup`

3. **Configure WiFi**
   - A captive portal should open automatically
   - If not, navigate to `http://192.168.4.1`
   - Select your WiFi network and enter the password
   - Click "Save"

4. **Device Connects**
   - The ESP32 will connect to your WiFi network
   - Check serial monitor for the assigned IP address

### Finding Your Device

#### Via mDNS (Recommended)
- The device broadcasts as `ESP32Speaker_<DEVICE_ID>.local`
- On macOS/Linux: `ping ESP32Speaker_<DEVICE_ID>.local`
- On Windows: Install [Bonjour Print Services](https://support.apple.com/kb/DL999)

#### Via Serial Monitor
- Connect via USB and open serial monitor
- The IP address is displayed during startup

#### Via Router
- Check your router's DHCP client list
- Look for hostname `ESP32Speaker_<DEVICE_ID>`

## Usage

### WebSocket Communication

The device listens for WebSocket connections on port **81**.

#### Connection
```javascript
const ws = new WebSocket('ws://192.168.1.100:81');

ws.onopen = () => {
  console.log('Connected to ESP32 speaker');
};

ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  console.log('Received:', data);
};
```

#### Command Format
All commands are sent as JSON:

```json
{
  "command": "commandName",
  "parameter": "value"
}
```

### Available Commands

#### Set Volume
```json
{
  "command": "setVolume",
  "volume": 75
}
```
- `volume`: 0-100

#### Mute/Unmute
```json
{
  "command": "setMute",
  "muted": true
}
```
- `muted`: true or false

#### Power Control
```json
{
  "command": "setPower",
  "power": true
}
```
- `power`: true (on) or false (off)

#### Set Audio Source
```json
{
  "command": "setSource",
  "source": 1
}
```
- `source`: 0 = None, 1 = Bluetooth, 2 = Line-In, 3 = Network

#### Set EQ Preset
```json
{
  "command": "setEQ",
  "preset": 1
}
```
- `preset`: 0 = Flat, 1 = Rock, 2 = Jazz, 3 = Classical, 4 = Pop, 5 = Custom

#### Adjust Bass/Treble
```json
{
  "command": "setEQ",
  "bass": 6,
  "treble": 3
}
```
- `bass`: -12 to +12 (dB)
- `treble`: -12 to +12 (dB)

#### Identify Device
```json
{
  "command": "identify"
}
```
Flashes the LED to identify the physical device.

#### Get Status
```json
{
  "command": "getStatus"
}
```
Returns current device status.

#### Get Device Info
```json
{
  "command": "getInfo"
}
```
Returns device information (ID, IP, firmware version, etc.).

### Response Format

#### Success Response
```json
{
  "type": "response",
  "command": "setVolume",
  "success": true,
  "message": "Volume set to 75"
}
```

#### Error Response
```json
{
  "type": "response",
  "command": "setVolume",
  "success": false,
  "error": "Missing volume parameter"
}
```

#### Status Update (Broadcast)
```json
{
  "type": "status",
  "deviceId": "ESP32_AABBCCDDEEFF",
  "powerState": true,
  "volume": 75,
  "isMuted": false,
  "audioSource": 1,
  "eqPreset": 0,
  "bassBoost": 0,
  "trebleAdjust": 0
}
```

#### Heartbeat (every 30 seconds)
```json
{
  "type": "heartbeat",
  "timestamp": 123456789
}
```

## OTA Updates

### Via Arduino IDE OTA
1. Tools → Port → Select Network Port (ESP32 speaker)
2. Click "Upload"
3. Enter OTA password (default: `esp32speaker`)

### Via PlatformIO
```bash
pio run --target upload --upload-port 192.168.1.100
```

### Via Web Interface (Future)
HTTP-based OTA will be added in a future phase.

## Troubleshooting

### Device Won't Connect to WiFi
- **Reset WiFi Settings**: Hold reset button for 10 seconds
- **Check Credentials**: Ensure correct SSID and password
- **Signal Strength**: Move closer to router
- **2.4GHz Only**: ESP32 doesn't support 5GHz WiFi

### Can't Find Device on Network
- **Check mDNS**: Some routers block mDNS traffic
- **Use IP Address**: Check router or serial monitor for IP
- **Firewall**: Ensure ports 80 and 81 are not blocked

### No Audio Output
- **Check Wiring**: Verify I2S connections
- **Check DAC**: Ensure DAC is powered and configured correctly
- **Volume**: Make sure volume > 0 and not muted
- **Power**: Verify device is powered on

### WebSocket Connection Fails
- **Network**: Ensure device and client on same network
- **Port**: WebSocket uses port 81
- **Firewall**: Check firewall settings

### Serial Monitor Shows Errors
- **Baud Rate**: Set to 115200
- **USB Cable**: Use data cable, not charging-only
- **Drivers**: Install CH340/CP2102 drivers if needed

## Debug Levels

Adjust debug verbosity in `platformio.ini`:
```ini
build_flags =
    -DCORE_DEBUG_LEVEL=3
```

Levels:
- 0 = None
- 1 = Error
- 2 = Warn
- 3 = Info (default)
- 4 = Debug
- 5 = Verbose

## Development & Customization

### Changing Device Name
Edit `include/config.h`:
```cpp
#define DEVICE_NAME_PREFIX "MyCustomSpeaker"
```

### Changing I2S Pins
Edit `include/config.h`:
```cpp
#define I2S_BCLK_PIN 26
#define I2S_LRC_PIN 25
#define I2S_DOUT_PIN 22
```

### Changing Default Volume
Edit `include/config.h`:
```cpp
#define VOLUME_DEFAULT 50  // 0-100
```

### Adding Custom EQ Presets
Edit `src/AudioController.cpp` → `applyEQPreset()` function

## Phase 1 Limitations

The following features are planned for future phases:

- **Not Included in Phase 1:**
  - HTTP REST API (WebSocket only)
  - Stereo pairing (Phase 2)
  - Scheduled playback / alarms (Phase 2)
  - Multi-user support (Phase 3)
  - Streaming service integration (Phase 3)
  - Voice control (Phase 3)

## Project Structure

```
esp32_firmware/
├── platformio.ini          # PlatformIO configuration
├── include/                # Header files
│   ├── config.h           # Main configuration
│   ├── DeviceManager.h    # Device management
│   ├── NetworkManager.h   # WiFi and mDNS
│   ├── WebSocketServer.h  # WebSocket communication
│   └── AudioController.h  # Audio processing
├── src/                   # Source files
│   ├── main.cpp           # Main program
│   ├── DeviceManager.cpp
│   ├── NetworkManager.cpp
│   ├── WebSocketServer.cpp
│   └── AudioController.cpp
├── data/                  # Filesystem data (future use)
└── README.md              # This file
```

## Contributing

This is Phase 1 of the ESP32 Multi-Room Speaker System. Contributions are welcome!

Please see the main project planning documents:
- `ESP32_Speaker_App_Implementation_Phase1-3.md`
- `ESP32_Speaker_App_GUI_Room_Layout_Feature.md`

## License

[Specify your license here]

## Support & Documentation

For issues, questions, or feature requests, please refer to the main project repository.

## Version History

### v1.0.0 - Phase 1 (Current)
- Initial release
- Device discovery and identification
- WiFi connectivity with auto-reconnect
- WebSocket server for real-time control
- Basic audio control (volume, mute, power)
- Equalizer with presets
- OTA update support
- mDNS service broadcasting
- Settings persistence

### Roadmap
- **Phase 2**: Stereo pairing, scheduling, sleep timer, firmware updates
- **Phase 3**: Multi-user, streaming services, voice control, cloud sync

---

**Firmware Version:** 1.0.0
**Phase:** 1
**Last Updated:** December 2025
