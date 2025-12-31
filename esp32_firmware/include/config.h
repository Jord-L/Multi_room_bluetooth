/**
 * ESP32 Multi-Room Bluetooth Hub - Configuration Header
 * Phase 1: Bluetooth Hub Architecture
 *
 * ESP32 acts as a Bluetooth hub that:
 * - Receives audio stream from mobile app via WiFi
 * - Connects to multiple Bluetooth speakers as A2DP source
 * - Forwards audio to connected Bluetooth speakers
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// DEVICE IDENTIFICATION
// ============================================================================
#define DEVICE_NAME_PREFIX "ESP32Hub"
#define FIRMWARE_VERSION "2.0.0"
#define HARDWARE_VERSION "2.0"
#define DEVICE_TYPE "bluetooth_hub"

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================
// WiFi Configuration Portal
#define WIFI_PORTAL_TIMEOUT 180  // 3 minutes timeout for config portal
#define WIFI_PORTAL_AP_NAME "ESP32_Hub_Setup"

// mDNS Service Discovery
#define MDNS_SERVICE_NAME "_esp32hub"
#define MDNS_PROTOCOL "_tcp"
#define MDNS_PORT 80

// Connection Management
#define HEARTBEAT_INTERVAL 30000      // 30 seconds (milliseconds)
#define CONNECTION_TIMEOUT 5000       // 5 seconds
#define RECONNECT_DELAY_MIN 1000      // 1 second
#define RECONNECT_DELAY_MAX 30000     // 30 seconds max backoff

// ============================================================================
// WEBSOCKET CONFIGURATION
// ============================================================================
#define WEBSOCKET_PORT 81
#define WEBSOCKET_MAX_CLIENTS 4
#define WEBSOCKET_PING_INTERVAL 30000  // 30 seconds

// ============================================================================
// AUDIO STREAMING CONFIGURATION
// ============================================================================
// UDP Audio Streaming (from mobile app)
#define AUDIO_STREAM_PORT 8888
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BITS_PER_SAMPLE 16
#define AUDIO_CHANNELS 2  // Stereo

// Audio Buffer Settings
#define AUDIO_BUFFER_SIZE 4096
#define AUDIO_BUFFER_COUNT 3
#define AUDIO_QUEUE_SIZE 10

// ============================================================================
// BLUETOOTH CONFIGURATION
// ============================================================================
// Bluetooth A2DP Source (ESP32 connects TO speakers)
#define BT_DEVICE_NAME_PREFIX "ESP32Hub"
#define BT_MAX_CONNECTED_SPEAKERS 7  // ESP32 can handle 3-7 simultaneous connections
#define BT_SCAN_DURATION 10          // Scan for 10 seconds
#define BT_PAIRING_TIMEOUT 60000     // 60 seconds pairing timeout

// Bluetooth Speaker Management
#define BT_RECONNECT_ATTEMPTS 3
#define BT_RECONNECT_DELAY 5000      // 5 seconds between reconnect attempts

// ============================================================================
// VOLUME & AUDIO SETTINGS
// ============================================================================
// Volume Settings
#define VOLUME_MIN 0
#define VOLUME_MAX 100
#define VOLUME_DEFAULT 50
#define VOLUME_STEP 5

// Audio Synchronization
#define SYNC_BUFFER_MS 100           // 100ms buffer for sync
#define MAX_LATENCY_MS 200           // Maximum acceptable latency

// ============================================================================
// STORAGE CONFIGURATION
// ============================================================================
// Preferences namespaces
#define PREFS_NAMESPACE "hub"
#define PREFS_DEVICE_KEY "device"
#define PREFS_BT_SPEAKERS_KEY "bt_speakers"
#define PREFS_NETWORK_KEY "network"
#define PREFS_AUDIO_KEY "audio"

// ============================================================================
// OTA UPDATE CONFIGURATION
// ============================================================================
#define OTA_HOSTNAME_PREFIX "esp32hub-ota"
#define OTA_PASSWORD "esp32hub"  // Change this in production!
#define OTA_PORT 3232

// ============================================================================
// LED INDICATORS (Optional - for status feedback)
// ============================================================================
#define LED_STATUS_PIN 2  // Built-in LED on most ESP32 boards
#define LED_BLINK_FAST 100
#define LED_BLINK_SLOW 500

// ============================================================================
// TIMING CONFIGURATION
// ============================================================================
#define SYNC_ACCURACY_TARGET 20  // ±20ms synchronization target

// ============================================================================
// BUFFER SIZES
// ============================================================================
#define JSON_BUFFER_SIZE 2048        // Larger for BT speaker lists
#define COMMAND_BUFFER_SIZE 512

// ============================================================================
// DEBUG CONFIGURATION
// ============================================================================
#define DEBUG_SERIAL_ENABLED true
#define DEBUG_BAUD_RATE 115200

// Debug levels (matches Arduino core)
#ifndef CORE_DEBUG_LEVEL
#define CORE_DEBUG_LEVEL 3  // 0=None, 1=Error, 2=Warn, 3=Info, 4=Debug, 5=Verbose
#endif

// ============================================================================
// FEATURE FLAGS (Phase 1 - Hub Mode)
// ============================================================================
#define FEATURE_MDNS_DISCOVERY      true
#define FEATURE_WEBSOCKET_SERVER    true
#define FEATURE_AUDIO_STREAMING     true  // WiFi audio streaming from app
#define FEATURE_BT_SOURCE           true  // Bluetooth A2DP source
#define FEATURE_BT_MULTI_CONNECT    true  // Multiple BT speaker connections
#define FEATURE_OTA_UPDATES         true
#define FEATURE_AUDIO_SYNC          true  // Audio synchronization

// Future phase features (disabled for Phase 1)
#define FEATURE_STEREO_PAIRING      false  // Phase 2
#define FEATURE_MULTI_USER          false  // Phase 3
#define FEATURE_CLOUD_SYNC          false  // Phase 3

// ============================================================================
// BLUETOOTH SPEAKER STATE
// ============================================================================
enum BTSpeakerState {
    BT_SPEAKER_DISCONNECTED = 0,
    BT_SPEAKER_CONNECTING = 1,
    BT_SPEAKER_CONNECTED = 2,
    BT_SPEAKER_PLAYING = 3,
    BT_SPEAKER_PAUSED = 4,
    BT_SPEAKER_ERROR = 5
};

// ============================================================================
// AUDIO SOURCE TYPES
// ============================================================================
enum AudioSourceType {
    AUDIO_SOURCE_NONE = 0,
    AUDIO_SOURCE_MOBILE_APP = 1,    // Stream from mobile app
    AUDIO_SOURCE_MICROPHONE = 2,     // Future: ESP32 microphone input
    AUDIO_SOURCE_LINE_IN = 3,        // Future: Line-in input
    AUDIO_SOURCE_NETWORK = 4         // Future: Network streaming
};

// ============================================================================
// DEFAULT HUB SETTINGS
// ============================================================================
struct HubSettings {
    char customName[32] = "My Hub";
    char location[32] = "Home";
    bool powerState = true;
    int masterVolume = VOLUME_DEFAULT;
    bool isMuted = false;
    AudioSourceType audioSource = AUDIO_SOURCE_MOBILE_APP;
    uint8_t maxConnectedSpeakers = BT_MAX_CONNECTED_SPEAKERS;
    bool autoReconnect = true;
};

#endif // CONFIG_H
