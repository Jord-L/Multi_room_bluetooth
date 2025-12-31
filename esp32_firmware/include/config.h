/**
 * ESP32 Multi-Room Speaker System - Configuration Header
 * Phase 1: Core Functionality
 *
 * This file contains all configuration constants and settings
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// DEVICE IDENTIFICATION
// ============================================================================
#define DEVICE_NAME_PREFIX "ESP32Speaker"
#define FIRMWARE_VERSION "1.0.0"
#define HARDWARE_VERSION "1.0"

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================
// WiFi Configuration Portal
#define WIFI_PORTAL_TIMEOUT 180  // 3 minutes timeout for config portal
#define WIFI_PORTAL_AP_NAME "ESP32_Speaker_Setup"

// mDNS Service Discovery
#define MDNS_SERVICE_NAME "_esp32speaker"
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
// HTTP REST API CONFIGURATION
// ============================================================================
#define HTTP_PORT 80
#define API_VERSION "v1"

// ============================================================================
// AUDIO CONFIGURATION
// ============================================================================
// Volume Settings
#define VOLUME_MIN 0
#define VOLUME_MAX 100
#define VOLUME_DEFAULT 50
#define VOLUME_STEP 5

// Equalizer Settings (dB range)
#define EQ_MIN -12
#define EQ_MAX 12
#define EQ_DEFAULT 0

// I2S Audio Output Pins (adjust based on your DAC)
#define I2S_BCLK_PIN 26    // Bit clock
#define I2S_LRC_PIN 25     // Left/Right clock (Word select)
#define I2S_DOUT_PIN 22    // Data out

// Audio Sources
enum AudioSource {
    SOURCE_NONE = 0,
    SOURCE_BLUETOOTH = 1,
    SOURCE_LINE_IN = 2,
    SOURCE_NETWORK = 3
};

// Equalizer Presets
enum EQPreset {
    EQ_FLAT = 0,
    EQ_ROCK = 1,
    EQ_JAZZ = 2,
    EQ_CLASSICAL = 3,
    EQ_POP = 4,
    EQ_CUSTOM = 5
};

// ============================================================================
// STORAGE CONFIGURATION
// ============================================================================
// Preferences namespaces
#define PREFS_NAMESPACE "speaker"
#define PREFS_DEVICE_KEY "device"
#define PREFS_AUDIO_KEY "audio"
#define PREFS_NETWORK_KEY "network"

// ============================================================================
// OTA UPDATE CONFIGURATION
// ============================================================================
#define OTA_HOSTNAME_PREFIX "esp32speaker-ota"
#define OTA_PASSWORD "esp32speaker"  // Change this in production!
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
#define JSON_BUFFER_SIZE 1024
#define COMMAND_BUFFER_SIZE 512

// ============================================================================
// DEBUG CONFIGURATION
// ============================================================================
#define DEBUG_SERIAL_ENABLED true
#define DEBUG_BAUD_RATE 115200

// Debug levels (matches Arduino core)
// 0 = None, 1 = Error, 2 = Warn, 3 = Info, 4 = Debug, 5 = Verbose
#ifndef CORE_DEBUG_LEVEL
#define CORE_DEBUG_LEVEL 3
#endif

// ============================================================================
// FEATURE FLAGS (Phase 1)
// ============================================================================
#define FEATURE_MDNS_DISCOVERY      true
#define FEATURE_WEBSOCKET_SERVER    true
#define FEATURE_HTTP_API            true
#define FEATURE_OTA_UPDATES         true
#define FEATURE_AUDIO_CONTROL       true
#define FEATURE_EQUALIZER           true
#define FEATURE_POWER_MANAGEMENT    true

// Future phase features (disabled for Phase 1)
#define FEATURE_STEREO_PAIRING      false  // Phase 2
#define FEATURE_SCHEDULING          false  // Phase 2
#define FEATURE_MULTI_USER          false  // Phase 3
#define FEATURE_STREAMING_SERVICES  false  // Phase 3

// ============================================================================
// PIN DEFINITIONS (Adjust based on your hardware)
// ============================================================================
// Power control (if supported by hardware)
#define POWER_CONTROL_PIN -1  // -1 = not used

// Audio source selection (if hardware supports multiple inputs)
#define AUDIO_SOURCE_SELECT_PIN -1  // -1 = not used

// ============================================================================
// DEFAULT DEVICE SETTINGS
// ============================================================================
struct DeviceSettings {
    char customName[32] = "Speaker";
    char room[32] = "Unknown";
    bool powerState = true;
    uint8_t volume = VOLUME_DEFAULT;
    bool isMuted = false;
    AudioSource audioSource = SOURCE_BLUETOOTH;
    EQPreset eqPreset = EQ_FLAT;
    int8_t bassBoost = EQ_DEFAULT;
    int8_t trebleAdjust = EQ_DEFAULT;
};

#endif // CONFIG_H
