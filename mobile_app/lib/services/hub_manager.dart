import 'dart:async';
import 'package:flutter/foundation.dart';
import '../models/hub.dart';
import '../models/bluetooth_speaker.dart';
import 'discovery_service.dart';
import 'websocket_service.dart';
import 'audio_streaming_service.dart';

/// Hub Manager
/// Manages ESP32 hub connection, Bluetooth speakers, and audio streaming
class HubManager with ChangeNotifier {
  Hub? _hub;
  final Map<String, BluetoothSpeaker> _speakers = {};
  final List<BluetoothSpeaker> _discoveredSpeakers = [];

  DiscoveryService? _discoveryService;
  WebSocketService? _webSocketService;
  AudioStreamingService? _audioStreamingService;

  bool _isDiscoveringHub = false;
  bool _isDiscoveringSpeakers = false;
  bool _isConnecting = false;
  Timer? _statusPollTimer;

  // Getters
  Hub? get hub => _hub;
  List<BluetoothSpeaker> get speakers => _speakers.values.toList();
  List<BluetoothSpeaker> get discoveredSpeakers => _discoveredSpeakers;
  bool get isDiscoveringHub => _isDiscoveringHub;
  bool get isDiscoveringSpeakers => _isDiscoveringSpeakers;
  bool get isConnected => _hub?.isConnected ?? false;
  bool get isConnecting => _isConnecting;
  bool get isStreaming => _hub?.isStreaming ?? false;

  HubManager() {
    _init();
  }

  void _init() {
    // Initialize services
    _discoveryService = DiscoveryService();

    // Listen for discovered hubs
    _discoveryService?.devicesFound.listen((device) {
      _handleDiscoveredHub(device);
    });
  }

  // =========================================================================
  // HUB DISCOVERY & CONNECTION
  // =========================================================================

  /// Start discovering ESP32 hub
  Future<void> startHubDiscovery() async {
    if (_isDiscoveringHub) {
      print('[HubManager] Already discovering hub');
      return;
    }

    _isDiscoveringHub = true;
    notifyListeners();

    print('[HubManager] Starting hub discovery...');
    await _discoveryService?.startDiscovery();
  }

  /// Stop discovering hub
  Future<void> stopHubDiscovery() async {
    _isDiscoveringHub = false;
    notifyListeners();

    await _discoveryService?.stopDiscovery();
    print('[HubManager] Stopped hub discovery');
  }

  /// Handle discovered hub
  void _handleDiscoveredHub(DiscoveredDevice device) {
    print('[HubManager] Discovered hub: ${device.hubId} at ${device.ipAddress}');

    // Create or update hub
    if (_hub == null || _hub!.hubId != device.hubId) {
      _hub = Hub(
        hubId: device.hubId ?? 'UNKNOWN',
        customName: device.hubName ?? 'ESP32 Hub',
        ipAddress: device.ipAddress,
        firmwareVersion: device.firmwareVersion ?? '2.0.0',
        maxSpeakers: device.maxSpeakers ?? 7,
      );
    } else {
      _hub = _hub!.copyWith(
        ipAddress: device.ipAddress,
        firmwareVersion: device.firmwareVersion,
      );
    }

    notifyListeners();

    // Auto-connect to hub
    connectToHub();
  }

  /// Connect to hub
  Future<bool> connectToHub() async {
    if (_hub == null) {
      print('[HubManager] No hub to connect to');
      return false;
    }

    if (_isConnecting || isConnected) {
      print('[HubManager] Already connected or connecting');
      return isConnected;
    }

    _isConnecting = true;
    _hub = _hub!.copyWith(isConnecting: true);
    notifyListeners();

    print('[HubManager] Connecting to hub at ${_hub!.ipAddress}...');

    // Create WebSocket service
    _webSocketService = WebSocketService(
      ipAddress: _hub!.ipAddress,
      port: _hub!.wsPort,
    );

    // Listen to WebSocket messages
    _webSocketService!.messages.listen(_handleWebSocketMessage);
    _webSocketService!.connectionStatus.listen(_handleConnectionStatus);

    // Connect to WebSocket
    final connected = await _webSocketService!.connect();

    if (connected) {
      print('[HubManager] Connected to hub');

      // Create audio streaming service
      _audioStreamingService = AudioStreamingService(
        hubIpAddress: _hub!.ipAddress,
        hubPort: _hub!.audioPort,
      );

      // Request initial status
      await _webSocketService!.getHubInfo();
      await _webSocketService!.getHubStatus();
      await _webSocketService!.getSpeakersStatus();

      // Start periodic status polling
      _startStatusPolling();

      _hub = _hub!.copyWith(isConnected: true, isConnecting: false);
    } else {
      print('[HubManager] Failed to connect to hub');
      _hub = _hub!.copyWith(isConnected: false, isConnecting: false);
    }

    _isConnecting = false;
    notifyListeners();

    return connected;
  }

  /// Disconnect from hub
  Future<void> disconnectFromHub() async {
    if (!isConnected) return;

    print('[HubManager] Disconnecting from hub');

    // Stop streaming
    if (isStreaming) {
      await stopAudioStreaming();
    }

    // Stop status polling
    _stopStatusPolling();

    // Disconnect WebSocket
    _webSocketService?.disconnect();
    _webSocketService = null;

    // Dispose audio streaming service
    _audioStreamingService?.dispose();
    _audioStreamingService = null;

    if (_hub != null) {
      _hub = _hub!.copyWith(isConnected: false);
    }

    notifyListeners();
  }

  /// Start periodic status polling
  void _startStatusPolling() {
    _statusPollTimer?.cancel();
    _statusPollTimer = Timer.periodic(const Duration(seconds: 5), (timer) {
      if (isConnected) {
        _webSocketService?.getHubStatus();
        _webSocketService?.getSpeakersStatus();
      } else {
        timer.cancel();
      }
    });
  }

  /// Stop status polling
  void _stopStatusPolling() {
    _statusPollTimer?.cancel();
    _statusPollTimer = null;
  }

  /// Handle WebSocket connection status
  void _handleConnectionStatus(bool connected) {
    if (_hub != null) {
      _hub = _hub!.copyWith(isConnected: connected);
      notifyListeners();
    }

    if (!connected) {
      _stopStatusPolling();
    }
  }

  /// Handle WebSocket messages
  void _handleWebSocketMessage(Map<String, dynamic> message) {
    final type = message['type'] as String?;

    switch (type) {
      case 'hubInfo':
        _handleHubInfo(message);
        break;
      case 'status':
        _handleHubStatus(message);
        break;
      case 'speakersStatus':
        _handleSpeakersStatus(message);
        break;
      case 'discoveredSpeakers':
        _handleDiscoveredSpeakers(message);
        break;
      case 'response':
        _handleCommandResponse(message);
        break;
      case 'heartbeat':
        // Hub is alive
        break;
      default:
        print('[HubManager] Unknown message type: $type');
    }
  }

  /// Handle hub info message
  void _handleHubInfo(Map<String, dynamic> message) {
    if (_hub != null) {
      _hub = Hub.fromJson(message);
      _hub = _hub!.copyWith(isConnected: true);
      notifyListeners();
    }
  }

  /// Handle hub status message
  void _handleHubStatus(Map<String, dynamic> message) {
    _hub?.updateFromStatus(message);
    notifyListeners();
  }

  /// Handle speakers status message
  void _handleSpeakersStatus(Map<String, dynamic> message) {
    final speakersData = message['speakers'] as List?;
    if (speakersData == null) return;

    _speakers.clear();
    for (final speakerData in speakersData) {
      final speaker = BluetoothSpeaker.fromJson(speakerData as Map<String, dynamic>);
      _speakers[speaker.btAddress] = speaker;
    }

    notifyListeners();
  }

  /// Handle discovered speakers message
  void _handleDiscoveredSpeakers(Map<String, dynamic> message) {
    final speakersData = message['speakers'] as List?;
    if (speakersData == null) return;

    _discoveredSpeakers.clear();
    for (final speakerData in speakersData) {
      final speaker = BluetoothSpeaker.fromJson(speakerData as Map<String, dynamic>);
      _discoveredSpeakers.add(speaker);
    }

    _isDiscoveringSpeakers = message['discovering'] as bool? ?? false;
    notifyListeners();
  }

  /// Handle command response
  void _handleCommandResponse(Map<String, dynamic> message) {
    final success = message['success'] as bool? ?? false;
    final command = message['command'] as String?;
    final error = message['error'] as String?;

    if (success) {
      print('[HubManager] Command "$command" succeeded');
    } else {
      print('[HubManager] Command "$command" failed: $error');
    }
  }

  // =========================================================================
  // HUB CONTROL
  // =========================================================================

  /// Set hub name
  Future<bool> setHubName(String name) async {
    if (!isConnected) return false;
    final success = await _webSocketService!.setHubName(name);
    if (success && _hub != null) {
      _hub = _hub!.copyWith(customName: name);
      notifyListeners();
    }
    return success;
  }

  /// Set hub location
  Future<bool> setHubLocation(String location) async {
    if (!isConnected) return false;
    final success = await _webSocketService!.setHubLocation(location);
    if (success && _hub != null) {
      _hub = _hub!.copyWith(location: location);
      notifyListeners();
    }
    return success;
  }

  /// Set master volume
  Future<bool> setMasterVolume(int volume) async {
    if (!isConnected) return false;
    return await _webSocketService!.setMasterVolume(volume);
  }

  /// Set mute
  Future<bool> setMute(bool muted) async {
    if (!isConnected) return false;
    return await _webSocketService!.setMute(muted);
  }

  /// Set power
  Future<bool> setPower(bool power) async {
    if (!isConnected) return false;
    return await _webSocketService!.setPower(power);
  }

  /// Identify hub (flash LED)
  Future<bool> identifyHub() async {
    if (!isConnected) return false;
    return await _webSocketService!.identify();
  }

  // =========================================================================
  // BLUETOOTH SPEAKER DISCOVERY & MANAGEMENT
  // =========================================================================

  /// Start Bluetooth speaker discovery
  Future<bool> startSpeakerDiscovery({int duration = 30}) async {
    if (!isConnected) return false;

    _isDiscoveringSpeakers = true;
    notifyListeners();

    final success = await _webSocketService!.startBTDiscovery(duration: duration);

    if (success) {
      // Periodically request discovered speakers
      Timer.periodic(const Duration(seconds: 3), (timer) {
        if (_isDiscoveringSpeakers) {
          _webSocketService?.getDiscoveredSpeakers();
        } else {
          timer.cancel();
        }
      });
    }

    return success;
  }

  /// Stop Bluetooth speaker discovery
  Future<bool> stopSpeakerDiscovery() async {
    if (!isConnected) return false;

    _isDiscoveringSpeakers = false;
    notifyListeners();

    return await _webSocketService!.stopBTDiscovery();
  }

  /// Connect to Bluetooth speaker
  Future<bool> connectToSpeaker(String btAddress) async {
    if (!isConnected) return false;
    return await _webSocketService!.connectBTSpeaker(btAddress);
  }

  /// Disconnect from Bluetooth speaker
  Future<bool> disconnectFromSpeaker(String btAddress) async {
    if (!isConnected) return false;
    return await _webSocketService!.disconnectBTSpeaker(btAddress);
  }

  /// Remove Bluetooth speaker
  Future<bool> removeSpeaker(String btAddress) async {
    if (!isConnected) return false;
    final success = await _webSocketService!.removeBTSpeaker(btAddress);
    if (success) {
      _speakers.remove(btAddress);
      notifyListeners();
    }
    return success;
  }

  /// Set speaker volume
  Future<bool> setSpeakerVolume(String btAddress, int volume) async {
    if (!isConnected) return false;
    return await _webSocketService!.setBTSpeakerVolume(btAddress, volume);
  }

  /// Set speaker muted
  Future<bool> setSpeakerMuted(String btAddress, bool muted) async {
    if (!isConnected) return false;
    return await _webSocketService!.setBTSpeakerMuted(btAddress, muted);
  }

  /// Set speaker name
  Future<bool> setSpeakerName(String btAddress, String name) async {
    if (!isConnected) return false;
    return await _webSocketService!.setBTSpeakerName(btAddress, name);
  }

  /// Set speaker room
  Future<bool> setSpeakerRoom(String btAddress, String room) async {
    if (!isConnected) return false;
    return await _webSocketService!.setBTSpeakerRoom(btAddress, room);
  }

  // =========================================================================
  // AUDIO STREAMING
  // =========================================================================

  /// Start audio streaming
  Future<bool> startAudioStreaming() async {
    if (!isConnected || _audioStreamingService == null) {
      print('[HubManager] Cannot start streaming: not connected');
      return false;
    }

    // Start UDP audio streaming
    final streamingStarted = await _audioStreamingService!.startStreaming();
    if (!streamingStarted) {
      print('[HubManager] Failed to start UDP audio stream');
      return false;
    }

    // Tell hub to start receiving audio
    final success = await _webSocketService!.startAudioStream();
    if (success && _hub != null) {
      _hub = _hub!.copyWith(isStreaming: true);
      notifyListeners();
    }

    return success;
  }

  /// Stop audio streaming
  Future<bool> stopAudioStreaming() async {
    if (!isConnected || _audioStreamingService == null) {
      return false;
    }

    // Stop UDP audio streaming
    await _audioStreamingService!.stopStreaming();

    // Tell hub to stop receiving audio
    final success = await _webSocketService!.stopAudioStream();
    if (success && _hub != null) {
      _hub = _hub!.copyWith(isStreaming: false);
      notifyListeners();
    }

    return success;
  }

  /// Send audio data (for testing or actual streaming)
  Future<bool> sendAudioData(List<int> audioData) async {
    if (!isStreaming || _audioStreamingService == null) {
      return false;
    }

    return await _audioStreamingService!.sendAudioBuffer(audioData);
  }

  /// Get audio streaming service (for direct access if needed)
  AudioStreamingService? get audioStreamingService => _audioStreamingService;

  // =========================================================================
  // CLEANUP
  // =========================================================================

  @override
  void dispose() {
    _statusPollTimer?.cancel();
    _discoveryService?.dispose();
    _webSocketService?.dispose();
    _audioStreamingService?.dispose();
    super.dispose();
  }
}
