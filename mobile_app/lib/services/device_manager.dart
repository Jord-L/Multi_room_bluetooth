import 'dart:async';
import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'package:shared_preferences.dart';
import '../models/speaker_device.dart';
import '../models/room.dart';
import 'websocket_service.dart';
import 'discovery_service.dart';

/// Device Manager - Manages all speakers and rooms
/// Uses Provider for state management
class DeviceManager extends ChangeNotifier {
  final Map<String, SpeakerDevice> _devices = {};
  final Map<String, Room> _rooms = {};
  final Map<String, WebSocketService> _connections = {};

  final DiscoveryService _discoveryService = DiscoveryService();
  StreamSubscription? _discoverySubscription;

  bool _isDiscovering = false;
  bool get isDiscovering => _isDiscovering;

  List<SpeakerDevice> get devices => _devices.values.toList();
  List<Room> get rooms => _rooms.values.toList();

  DeviceManager() {
    _init();
  }

  /// Initialize device manager
  Future<void> _init() async {
    await _loadFromStorage();
    _listenToDiscovery();
  }

  /// Listen to mDNS discovery
  void _listenToDiscovery() {
    _discoverySubscription = _discoveryService.devicesFound.listen((discovered) {
      _handleDiscoveredDevice(discovered);
    });
  }

  /// Handle discovered device
  Future<void> _handleDiscoveredDevice(DiscoveredDevice discovered) async {
    final deviceId = discovered.deviceId;
    if (deviceId == null) return;

    // Check if device already exists
    if (_devices.containsKey(deviceId)) {
      // Update IP address if changed
      final device = _devices[deviceId]!;
      if (device.ipAddress != discovered.ipAddress) {
        device.ipAddress = discovered.ipAddress;
        notifyListeners();
      }
      return;
    }

    // Create new device
    final device = SpeakerDevice(
      deviceId: deviceId,
      customName: discovered.deviceName ?? 'Speaker',
      ipAddress: discovered.ipAddress,
      firmwareVersion: discovered.firmwareVersion ?? '1.0.0',
    );

    _devices[deviceId] = device;
    notifyListeners();

    // Auto-connect to the device
    await connectToDevice(deviceId);
  }

  /// Start discovery
  Future<void> startDiscovery() async {
    _isDiscovering = true;
    notifyListeners();

    await _discoveryService.startDiscovery();

    // Stop after 30 seconds
    Future.delayed(const Duration(seconds: 30), () {
      stopDiscovery();
    });
  }

  /// Stop discovery
  Future<void> stopDiscovery() async {
    await _discoveryService.stopDiscovery();
    _isDiscovering = false;
    notifyListeners();
  }

  /// Connect to a device
  Future<bool> connectToDevice(String deviceId) async {
    final device = _devices[deviceId];
    if (device == null) return false;

    // Check if already connected
    if (_connections.containsKey(deviceId) && _connections[deviceId]!.isConnected) {
      return true;
    }

    device.isConnecting = true;
    notifyListeners();

    // Create WebSocket connection
    final ws = WebSocketService(ipAddress: device.ipAddress);

    // Listen to messages
    ws.messages.listen((message) {
      _handleDeviceMessage(deviceId, message);
    });

    // Listen to connection status
    ws.connectionStatus.listen((isConnected) {
      device.isConnected = isConnected;
      device.isConnecting = false;
      notifyListeners();
    });

    // Connect
    final success = await ws.connect();

    if (success) {
      _connections[deviceId] = ws;
      device.isConnected = true;
      device.isConnecting = false;

      // Request initial status
      await ws.getStatus();
      await ws.getInfo();
    } else {
      device.isConnecting = false;
    }

    notifyListeners();
    return success;
  }

  /// Disconnect from a device
  void disconnectFromDevice(String deviceId) {
    final ws = _connections[deviceId];
    if (ws != null) {
      ws.disconnect();
      _connections.remove(deviceId);
    }

    final device = _devices[deviceId];
    if (device != null) {
      device.isConnected = false;
      notifyListeners();
    }
  }

  /// Handle device message
  void _handleDeviceMessage(String deviceId, Map<String, dynamic> message) {
    final device = _devices[deviceId];
    if (device == null) return;

    final type = message['type'];

    switch (type) {
      case 'status':
        device.updateFromStatus(message);
        notifyListeners();
        break;

      case 'response':
        // Handle command responses
        print('[DeviceManager] Response: ${message['command']} - ${message['success']}');
        break;

      case 'heartbeat':
        device.lastSeen = DateTime.now();
        break;

      default:
        // Handle other message types
        break;
    }
  }

  /// Get device by ID
  SpeakerDevice? getDevice(String deviceId) {
    return _devices[deviceId];
  }

  /// Update device name
  Future<void> updateDeviceName(String deviceId, String name) async {
    final device = _devices[deviceId];
    if (device == null) return;

    device.customName = name;
    notifyListeners();
    await _saveToStorage();
  }

  /// Set device volume
  Future<void> setDeviceVolume(String deviceId, int volume) async {
    final ws = _connections[deviceId];
    if (ws == null) return;

    await ws.setVolume(volume);
  }

  /// Set device mute
  Future<void> setDeviceMute(String deviceId, bool muted) async {
    final ws = _connections[deviceId];
    if (ws == null) return;

    await ws.setMute(muted);
  }

  /// Set device power
  Future<void> setDevicePower(String deviceId, bool power) async {
    final ws = _connections[deviceId];
    if (ws == null) return;

    await ws.setPower(power);
  }

  /// Identify device
  Future<void> identifyDevice(String deviceId) async {
    final ws = _connections[deviceId];
    if (ws == null) return;

    await ws.identify();
  }

  /// Create room
  void createRoom(String name, {String icon = '🏠'}) {
    final roomId = 'room_${DateTime.now().millisecondsSinceEpoch}';
    final room = Room(
      roomId: roomId,
      roomName: name,
      icon: icon,
    );

    _rooms[roomId] = room;
    notifyListeners();
    _saveToStorage();
  }

  /// Delete room
  void deleteRoom(String roomId) {
    _rooms.remove(roomId);
    notifyListeners();
    _saveToStorage();
  }

  /// Assign device to room
  Future<void> assignDeviceToRoom(String deviceId, String roomName) async {
    final device = _devices[deviceId];
    if (device == null) return;

    device.room = roomName;
    notifyListeners();
    await _saveToStorage();
  }

  /// Set room volume (all speakers in room)
  Future<void> setRoomVolume(String roomId, int volume) async {
    final room = _rooms[roomId];
    if (room == null) return;

    room.groupVolume = volume;

    for (final deviceId in room.speakerIds) {
      await setDeviceVolume(deviceId, volume);
    }

    notifyListeners();
  }

  /// Get devices in room
  List<SpeakerDevice> getDevicesInRoom(String roomName) {
    return _devices.values.where((device) => device.room == roomName).toList();
  }

  /// Save to local storage
  Future<void> _saveToStorage() async {
    final prefs = await SharedPreferences.getInstance();

    // Save devices
    final devicesJson = _devices.map((key, value) => MapEntry(key, value.toJson()));
    await prefs.setString('devices', jsonEncode(devicesJson));

    // Save rooms
    final roomsJson = _rooms.map((key, value) => MapEntry(key, value.toJson()));
    await prefs.setString('rooms', jsonEncode(roomsJson));
  }

  /// Load from local storage
  Future<void> _loadFromStorage() async {
    final prefs = await SharedPreferences.getInstance();

    // Load devices
    final devicesStr = prefs.getString('devices');
    if (devicesStr != null) {
      try {
        final Map<String, dynamic> devicesJson = jsonDecode(devicesStr);
        _devices.clear();
        devicesJson.forEach((key, value) {
          _devices[key] = SpeakerDevice.fromJson(value as Map<String, dynamic>);
        });
      } catch (e) {
        print('[DeviceManager] Error loading devices: $e');
      }
    }

    // Load rooms
    final roomsStr = prefs.getString('rooms');
    if (roomsStr != null) {
      try {
        final Map<String, dynamic> roomsJson = jsonDecode(roomsStr);
        _rooms.clear();
        roomsJson.forEach((key, value) {
          _rooms[key] = Room.fromJson(value as Map<String, dynamic>);
        });
      } catch (e) {
        print('[DeviceManager] Error loading rooms: $e');
      }
    }

    notifyListeners();
  }

  @override
  void dispose() {
    _discoverySubscription?.cancel();
    _discoveryService.dispose();

    for (final ws in _connections.values) {
      ws.dispose();
    }

    super.dispose();
  }
}
