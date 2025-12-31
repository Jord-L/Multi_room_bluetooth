/// Room Model
/// Represents a room containing one or more speakers

class Room {
  final String roomId;
  String roomName;
  String icon;
  List<String> speakerIds;
  int groupVolume;
  bool isMuted;

  Room({
    required this.roomId,
    required this.roomName,
    this.icon = '🏠',
    this.speakerIds = const [],
    this.groupVolume = 50,
    this.isMuted = false,
  });

  /// Create from JSON
  factory Room.fromJson(Map<String, dynamic> json) {
    return Room(
      roomId: json['roomId'] as String,
      roomName: json['roomName'] as String,
      icon: json['icon'] as String? ?? '🏠',
      speakerIds: (json['speakers'] as List<dynamic>?)
              ?.map((e) => e.toString())
              .toList() ??
          [],
      groupVolume: json['groupVolume'] as int? ?? 50,
      isMuted: json['isMuted'] as bool? ?? false,
    );
  }

  /// Convert to JSON
  Map<String, dynamic> toJson() {
    return {
      'roomId': roomId,
      'roomName': roomName,
      'icon': icon,
      'speakers': speakerIds,
      'groupVolume': groupVolume,
      'isMuted': isMuted,
    };
  }

  /// Copy with modifications
  Room copyWith({
    String? roomId,
    String? roomName,
    String? icon,
    List<String>? speakerIds,
    int? groupVolume,
    bool? isMuted,
  }) {
    return Room(
      roomId: roomId ?? this.roomId,
      roomName: roomName ?? this.roomName,
      icon: icon ?? this.icon,
      speakerIds: speakerIds ?? this.speakerIds,
      groupVolume: groupVolume ?? this.groupVolume,
      isMuted: isMuted ?? this.isMuted,
    );
  }

  /// Add speaker to room
  void addSpeaker(String speakerId) {
    if (!speakerIds.contains(speakerId)) {
      speakerIds = [...speakerIds, speakerId];
    }
  }

  /// Remove speaker from room
  void removeSpeaker(String speakerId) {
    speakerIds = speakerIds.where((id) => id != speakerId).toList();
  }
}

/// Predefined room icons
class RoomIcons {
  static const String livingRoom = '🛋️';
  static const String bedroom = '🛏️';
  static const String kitchen = '🍳';
  static const String bathroom = '🚿';
  static const String office = '💼';
  static const String garage = '🚗';
  static const String outdoor = '🌳';
  static const String basement = '⬇️';
  static const String attic = '⬆️';
  static const String generic = '🏠';

  static List<String> get all => [
        livingRoom,
        bedroom,
        kitchen,
        bathroom,
        office,
        garage,
        outdoor,
        basement,
        attic,
        generic,
      ];
}
