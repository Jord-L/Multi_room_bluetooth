/// Bluetooth Speaker Model
/// Represents a commercial Bluetooth speaker connected TO the ESP32 hub

class BluetoothSpeaker {
  final String btAddress;
  String deviceName;
  String customName;
  String room;
  int rssi;
  DateTime lastSeen;

  // Speaker state
  BTSpeakerState state;
  int volume;
  bool isMuted;
  bool isPlaying;

  // Connection info
  DateTime? lastConnected;
  DateTime? lastDisconnected;
  int connectionCount;

  BluetoothSpeaker({
    required this.btAddress,
    this.deviceName = 'Unknown Speaker',
    String? customName,
    this.room = 'Unassigned',
    this.rssi = 0,
    DateTime? lastSeen,
    this.state = BTSpeakerState.disconnected,
    this.volume = 50,
    this.isMuted = false,
    this.isPlaying = false,
    this.lastConnected,
    this.lastDisconnected,
    this.connectionCount = 0,
  })  : customName = customName ?? deviceName,
        lastSeen = lastSeen ?? DateTime.now();

  /// Create from JSON (from ESP32)
  factory BluetoothSpeaker.fromJson(Map<String, dynamic> json) {
    return BluetoothSpeaker(
      btAddress: json['btAddress'] as String,
      deviceName: json['deviceName'] as String? ?? 'Unknown Speaker',
      customName: json['customName'] as String?,
      room: json['room'] as String? ?? 'Unassigned',
      rssi: json['rssi'] as int? ?? 0,
      state: BTSpeakerState.values[json['state'] as int? ?? 0],
      volume: json['volume'] as int? ?? 50,
      isMuted: json['isMuted'] as bool? ?? false,
      isPlaying: json['isPlaying'] as bool? ?? false,
      connectionCount: json['connectionCount'] as int? ?? 0,
    );
  }

  /// Convert to JSON
  Map<String, dynamic> toJson() {
    return {
      'btAddress': btAddress,
      'deviceName': deviceName,
      'customName': customName,
      'room': room,
      'rssi': rssi,
      'lastSeen': lastSeen.toIso8601String(),
      'state': state.index,
      'volume': volume,
      'isMuted': isMuted,
      'isPlaying': isPlaying,
      'connectionCount': connectionCount,
    };
  }

  /// Update from status message
  void updateFromStatus(Map<String, dynamic> status) {
    if (status['state'] != null) {
      state = BTSpeakerState.values[status['state'] as int];
    }
    volume = status['volume'] as int? ?? volume;
    isMuted = status['isMuted'] as bool? ?? isMuted;
    isPlaying = status['isPlaying'] as bool? ?? isPlaying;
    rssi = status['rssi'] as int? ?? rssi;
    lastSeen = DateTime.now();

    if (state == BTSpeakerState.connected || state == BTSpeakerState.playing) {
      lastConnected = DateTime.now();
    } else if (state == BTSpeakerState.disconnected) {
      lastDisconnected = DateTime.now();
    }
  }

  /// Copy with modifications
  BluetoothSpeaker copyWith({
    String? btAddress,
    String? deviceName,
    String? customName,
    String? room,
    int? rssi,
    DateTime? lastSeen,
    BTSpeakerState? state,
    int? volume,
    bool? isMuted,
    bool? isPlaying,
    DateTime? lastConnected,
    DateTime? lastDisconnected,
    int? connectionCount,
  }) {
    return BluetoothSpeaker(
      btAddress: btAddress ?? this.btAddress,
      deviceName: deviceName ?? this.deviceName,
      customName: customName ?? this.customName,
      room: room ?? this.room,
      rssi: rssi ?? this.rssi,
      lastSeen: lastSeen ?? this.lastSeen,
      state: state ?? this.state,
      volume: volume ?? this.volume,
      isMuted: isMuted ?? this.isMuted,
      isPlaying: isPlaying ?? this.isPlaying,
      lastConnected: lastConnected ?? this.lastConnected,
      lastDisconnected: lastDisconnected ?? this.lastDisconnected,
      connectionCount: connectionCount ?? this.connectionCount,
    );
  }

  /// Get display name (custom or device name)
  String get displayName => customName.isNotEmpty ? customName : deviceName;

  /// Is speaker connected?
  bool get isConnected =>
      state == BTSpeakerState.connected ||
      state == BTSpeakerState.playing ||
      state == BTSpeakerState.paused;

  /// Is speaker in error state?
  bool get hasError => state == BTSpeakerState.error;

  /// Get signal strength as percentage
  int get signalStrengthPercent {
    // RSSI typically ranges from -100 (worst) to -40 (best)
    if (rssi >= -40) return 100;
    if (rssi <= -100) return 0;
    return ((rssi + 100) * 100 ~/ 60);
  }
}

/// Bluetooth Speaker State Enumeration
/// Must match BTSpeakerState in ESP32 config.h
enum BTSpeakerState {
  disconnected,
  connecting,
  connected,
  playing,
  paused,
  error,
}

extension BTSpeakerStateExtension on BTSpeakerState {
  String get displayName {
    switch (this) {
      case BTSpeakerState.disconnected:
        return 'Disconnected';
      case BTSpeakerState.connecting:
        return 'Connecting';
      case BTSpeakerState.connected:
        return 'Connected';
      case BTSpeakerState.playing:
        return 'Playing';
      case BTSpeakerState.paused:
        return 'Paused';
      case BTSpeakerState.error:
        return 'Error';
    }
  }

  String get icon {
    switch (this) {
      case BTSpeakerState.disconnected:
        return '🔴';
      case BTSpeakerState.connecting:
        return '🟡';
      case BTSpeakerState.connected:
        return '🟢';
      case BTSpeakerState.playing:
        return '▶️';
      case BTSpeakerState.paused:
        return '⏸️';
      case BTSpeakerState.error:
        return '❌';
    }
  }
}
