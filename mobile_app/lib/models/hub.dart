/// ESP32 Bluetooth Hub Model
/// Represents the ESP32 hub that manages Bluetooth speakers

class Hub {
  final String hubId;
  String customName;
  String location;
  String ipAddress;
  String firmwareVersion;
  int signalStrength;
  DateTime lastSeen;

  // Hub state
  bool powerState;
  int masterVolume;
  bool isMuted;
  bool isStreaming;
  int maxSpeakers;
  int uptime;

  // Connection state
  bool isConnected;
  bool isConnecting;

  // WebSocket connection
  int wsPort;

  // Audio streaming
  int audioPort;
  bool audioClientConnected;
  int audioBufferLevel;

  Hub({
    required this.hubId,
    this.customName = 'My Hub',
    this.location = 'Home',
    required this.ipAddress,
    this.firmwareVersion = '2.0.0',
    this.signalStrength = 0,
    DateTime? lastSeen,
    this.powerState = true,
    this.masterVolume = 50,
    this.isMuted = false,
    this.isStreaming = false,
    this.maxSpeakers = 7,
    this.uptime = 0,
    this.isConnected = false,
    this.isConnecting = false,
    this.wsPort = 81,
    this.audioPort = 8888,
    this.audioClientConnected = false,
    this.audioBufferLevel = 0,
  }) : lastSeen = lastSeen ?? DateTime.now();

  /// Create from JSON (from ESP32)
  factory Hub.fromJson(Map<String, dynamic> json) {
    return Hub(
      hubId: json['hubId'] as String,
      customName: json['customName'] as String? ?? 'My Hub',
      location: json['location'] as String? ?? 'Home',
      ipAddress: json['ipAddress'] as String? ?? json['ip'] as String,
      firmwareVersion: json['firmwareVersion'] as String? ?? json['version'] as String? ?? '2.0.0',
      signalStrength: json['signalStrength'] as int? ?? 0,
      powerState: json['powerState'] as bool? ?? true,
      masterVolume: json['masterVolume'] as int? ?? 50,
      isMuted: json['isMuted'] as bool? ?? false,
      isStreaming: json['isStreaming'] as bool? ?? false,
      maxSpeakers: json['maxSpeakers'] as int? ?? 7,
      uptime: json['uptime'] as int? ?? 0,
      wsPort: json['wsPort'] as int? ?? 81,
      audioPort: json['audioPort'] as int? ?? 8888,
    );
  }

  /// Convert to JSON
  Map<String, dynamic> toJson() {
    return {
      'hubId': hubId,
      'customName': customName,
      'location': location,
      'ipAddress': ipAddress,
      'firmwareVersion': firmwareVersion,
      'signalStrength': signalStrength,
      'lastSeen': lastSeen.toIso8601String(),
      'powerState': powerState,
      'masterVolume': masterVolume,
      'isMuted': isMuted,
      'isStreaming': isStreaming,
      'maxSpeakers': maxSpeakers,
      'uptime': uptime,
      'wsPort': wsPort,
      'audioPort': audioPort,
    };
  }

  /// Update from status message
  void updateFromStatus(Map<String, dynamic> status) {
    powerState = status['powerState'] as bool? ?? powerState;
    masterVolume = status['masterVolume'] as int? ?? masterVolume;
    isMuted = status['isMuted'] as bool? ?? isMuted;
    isStreaming = status['isStreaming'] as bool? ?? isStreaming;
    uptime = status['uptime'] as int? ?? uptime;
    audioClientConnected = status['audioClientConnected'] as bool? ?? audioClientConnected;
    audioBufferLevel = status['audioBufferLevel'] as int? ?? audioBufferLevel;
    lastSeen = DateTime.now();
  }

  /// Copy with modifications
  Hub copyWith({
    String? hubId,
    String? customName,
    String? location,
    String? ipAddress,
    String? firmwareVersion,
    int? signalStrength,
    DateTime? lastSeen,
    bool? powerState,
    int? masterVolume,
    bool? isMuted,
    bool? isStreaming,
    int? maxSpeakers,
    int? uptime,
    bool? isConnected,
    bool? isConnecting,
    int? wsPort,
    int? audioPort,
    bool? audioClientConnected,
    int? audioBufferLevel,
  }) {
    return Hub(
      hubId: hubId ?? this.hubId,
      customName: customName ?? this.customName,
      location: location ?? this.location,
      ipAddress: ipAddress ?? this.ipAddress,
      firmwareVersion: firmwareVersion ?? this.firmwareVersion,
      signalStrength: signalStrength ?? this.signalStrength,
      lastSeen: lastSeen ?? this.lastSeen,
      powerState: powerState ?? this.powerState,
      masterVolume: masterVolume ?? this.masterVolume,
      isMuted: isMuted ?? this.isMuted,
      isStreaming: isStreaming ?? this.isStreaming,
      maxSpeakers: maxSpeakers ?? this.maxSpeakers,
      uptime: uptime ?? this.uptime,
      isConnected: isConnected ?? this.isConnected,
      isConnecting: isConnecting ?? this.isConnecting,
      wsPort: wsPort ?? this.wsPort,
      audioPort: audioPort ?? this.audioPort,
      audioClientConnected: audioClientConnected ?? this.audioClientConnected,
      audioBufferLevel: audioBufferLevel ?? this.audioBufferLevel,
    );
  }

  /// Get uptime as formatted string
  String get uptimeFormatted {
    final hours = uptime ~/ 3600;
    final minutes = (uptime % 3600) ~/ 60;
    final seconds = uptime % 60;
    return '$hours:${minutes.toString().padLeft(2, '0')}:${seconds.toString().padLeft(2, '0')}';
  }
}
