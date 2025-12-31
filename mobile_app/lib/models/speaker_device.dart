/// Speaker Device Model
/// Represents an ESP32 speaker on the network

class SpeakerDevice {
  final String deviceId;
  String customName;
  String room;
  String ipAddress;
  String firmwareVersion;
  int signalStrength;
  DateTime lastSeen;

  // Device state
  bool powerState;
  int volume;
  bool isMuted;
  AudioSource audioSource;
  EQPreset eqPreset;
  int bassBoost;
  int trebleAdjust;

  // Connection state
  bool isConnected;
  bool isConnecting;

  // Groups
  List<String> groups;

  SpeakerDevice({
    required this.deviceId,
    this.customName = 'Speaker',
    this.room = 'Unknown',
    required this.ipAddress,
    this.firmwareVersion = '1.0.0',
    this.signalStrength = 0,
    DateTime? lastSeen,
    this.powerState = true,
    this.volume = 50,
    this.isMuted = false,
    this.audioSource = AudioSource.bluetooth,
    this.eqPreset = EQPreset.flat,
    this.bassBoost = 0,
    this.trebleAdjust = 0,
    this.isConnected = false,
    this.isConnecting = false,
    this.groups = const [],
  }) : lastSeen = lastSeen ?? DateTime.now();

  /// Create from JSON (from ESP32)
  factory SpeakerDevice.fromJson(Map<String, dynamic> json) {
    return SpeakerDevice(
      deviceId: json['deviceId'] as String,
      customName: json['customName'] as String? ?? 'Speaker',
      room: json['room'] as String? ?? 'Unknown',
      ipAddress: json['ipAddress'] as String,
      firmwareVersion: json['firmwareVersion'] as String? ?? '1.0.0',
      signalStrength: json['signalStrength'] as int? ?? 0,
      powerState: json['powerState'] as bool? ?? true,
      volume: json['volume'] as int? ?? 50,
      isMuted: json['isMuted'] as bool? ?? false,
      audioSource: AudioSource.values[json['audioSource'] as int? ?? 1],
      eqPreset: EQPreset.values[json['eqPreset'] as int? ?? 0],
      bassBoost: json['bassBoost'] as int? ?? 0,
      trebleAdjust: json['trebleAdjust'] as int? ?? 0,
    );
  }

  /// Convert to JSON
  Map<String, dynamic> toJson() {
    return {
      'deviceId': deviceId,
      'customName': customName,
      'room': room,
      'ipAddress': ipAddress,
      'firmwareVersion': firmwareVersion,
      'signalStrength': signalStrength,
      'lastSeen': lastSeen.toIso8601String(),
      'powerState': powerState,
      'volume': volume,
      'isMuted': isMuted,
      'audioSource': audioSource.index,
      'eqPreset': eqPreset.index,
      'bassBoost': bassBoost,
      'trebleAdjust': trebleAdjust,
      'groups': groups,
    };
  }

  /// Update from status message
  void updateFromStatus(Map<String, dynamic> status) {
    powerState = status['powerState'] as bool? ?? powerState;
    volume = status['volume'] as int? ?? volume;
    isMuted = status['isMuted'] as bool? ?? isMuted;
    if (status['audioSource'] != null) {
      audioSource = AudioSource.values[status['audioSource'] as int];
    }
    if (status['eqPreset'] != null) {
      eqPreset = EQPreset.values[status['eqPreset'] as int];
    }
    bassBoost = status['bassBoost'] as int? ?? bassBoost;
    trebleAdjust = status['trebleAdjust'] as int? ?? trebleAdjust;
    lastSeen = DateTime.now();
  }

  /// Copy with modifications
  SpeakerDevice copyWith({
    String? deviceId,
    String? customName,
    String? room,
    String? ipAddress,
    String? firmwareVersion,
    int? signalStrength,
    DateTime? lastSeen,
    bool? powerState,
    int? volume,
    bool? isMuted,
    AudioSource? audioSource,
    EQPreset? eqPreset,
    int? bassBoost,
    int? trebleAdjust,
    bool? isConnected,
    bool? isConnecting,
    List<String>? groups,
  }) {
    return SpeakerDevice(
      deviceId: deviceId ?? this.deviceId,
      customName: customName ?? this.customName,
      room: room ?? this.room,
      ipAddress: ipAddress ?? this.ipAddress,
      firmwareVersion: firmwareVersion ?? this.firmwareVersion,
      signalStrength: signalStrength ?? this.signalStrength,
      lastSeen: lastSeen ?? this.lastSeen,
      powerState: powerState ?? this.powerState,
      volume: volume ?? this.volume,
      isMuted: isMuted ?? this.isMuted,
      audioSource: audioSource ?? this.audioSource,
      eqPreset: eqPreset ?? this.eqPreset,
      bassBoost: bassBoost ?? this.bassBoost,
      trebleAdjust: trebleAdjust ?? this.trebleAdjust,
      isConnected: isConnected ?? this.isConnected,
      isConnecting: isConnecting ?? this.isConnecting,
      groups: groups ?? this.groups,
    );
  }
}

/// Audio Source Enumeration
enum AudioSource {
  none,
  bluetooth,
  lineIn,
  network,
}

extension AudioSourceExtension on AudioSource {
  String get displayName {
    switch (this) {
      case AudioSource.none:
        return 'None';
      case AudioSource.bluetooth:
        return 'Bluetooth';
      case AudioSource.lineIn:
        return 'Line In';
      case AudioSource.network:
        return 'Network';
    }
  }

  String get icon {
    switch (this) {
      case AudioSource.none:
        return '🔇';
      case AudioSource.bluetooth:
        return '📱';
      case AudioSource.lineIn:
        return '🎵';
      case AudioSource.network:
        return '🌐';
    }
  }
}

/// Equalizer Preset Enumeration
enum EQPreset {
  flat,
  rock,
  jazz,
  classical,
  pop,
  custom,
}

extension EQPresetExtension on EQPreset {
  String get displayName {
    switch (this) {
      case EQPreset.flat:
        return 'Flat';
      case EQPreset.rock:
        return 'Rock';
      case EQPreset.jazz:
        return 'Jazz';
      case EQPreset.classical:
        return 'Classical';
      case EQPreset.pop:
        return 'Pop';
      case EQPreset.custom:
        return 'Custom';
    }
  }
}
