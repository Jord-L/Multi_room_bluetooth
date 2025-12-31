import 'dart:async';
import 'dart:convert';
import 'package:web_socket_channel/web_socket_channel.dart';
import 'package:web_socket_channel/status.dart' as status;

/// WebSocket Service for communicating with ESP32 speakers
class WebSocketService {
  WebSocketChannel? _channel;
  final String ipAddress;
  final int port;
  bool _isConnected = false;
  Timer? _heartbeatTimer;
  Timer? _reconnectTimer;

  final StreamController<Map<String, dynamic>> _messageController =
      StreamController<Map<String, dynamic>>.broadcast();
  final StreamController<bool> _connectionController =
      StreamController<bool>.broadcast();

  Stream<Map<String, dynamic>> get messages => _messageController.stream;
  Stream<bool> get connectionStatus => _connectionController.stream;
  bool get isConnected => _isConnected;

  WebSocketService({
    required this.ipAddress,
    this.port = 81,
  });

  /// Connect to WebSocket server
  Future<bool> connect() async {
    try {
      final uri = Uri.parse('ws://$ipAddress:$port');
      _channel = WebSocketChannel.connect(uri);

      // Listen to messages
      _channel!.stream.listen(
        _handleMessage,
        onError: _handleError,
        onDone: _handleDisconnection,
        cancelOnError: false,
      );

      _isConnected = true;
      _connectionController.add(true);

      // Start heartbeat monitoring
      _startHeartbeat();

      print('[WebSocket] Connected to $ipAddress:$port');
      return true;
    } catch (e) {
      print('[WebSocket] Connection failed: $e');
      _isConnected = false;
      _connectionController.add(false);
      return false;
    }
  }

  /// Disconnect from WebSocket server
  void disconnect() {
    _heartbeatTimer?.cancel();
    _reconnectTimer?.cancel();
    _channel?.sink.close(status.goingAway);
    _isConnected = false;
    _connectionController.add(false);
    print('[WebSocket] Disconnected from $ipAddress:$port');
  }

  /// Handle incoming messages
  void _handleMessage(dynamic message) {
    try {
      final Map<String, dynamic> data = jsonDecode(message as String);
      _messageController.add(data);

      // Log message type
      final type = data['type'] ?? 'unknown';
      print('[WebSocket] Received: $type');
    } catch (e) {
      print('[WebSocket] Error parsing message: $e');
    }
  }

  /// Handle errors
  void _handleError(error) {
    print('[WebSocket] Error: $error');
    _isConnected = false;
    _connectionController.add(false);
    _attemptReconnect();
  }

  /// Handle disconnection
  void _handleDisconnection() {
    print('[WebSocket] Connection closed');
    _isConnected = false;
    _connectionController.add(false);
    _heartbeatTimer?.cancel();
    _attemptReconnect();
  }

  /// Attempt to reconnect
  void _attemptReconnect() {
    _reconnectTimer?.cancel();
    _reconnectTimer = Timer(const Duration(seconds: 5), () {
      print('[WebSocket] Attempting to reconnect...');
      connect();
    });
  }

  /// Start heartbeat monitoring
  void _startHeartbeat() {
    _heartbeatTimer?.cancel();
    _heartbeatTimer = Timer.periodic(const Duration(seconds: 35), (timer) {
      // If no heartbeat received in 35 seconds, assume disconnection
      // The ESP32 sends heartbeat every 30 seconds
      if (!_isConnected) {
        timer.cancel();
      }
    });
  }

  /// Send command to speaker
  Future<bool> sendCommand(Map<String, dynamic> command) async {
    if (!_isConnected || _channel == null) {
      print('[WebSocket] Cannot send command: not connected');
      return false;
    }

    try {
      final message = jsonEncode(command);
      _channel!.sink.add(message);
      print('[WebSocket] Sent: ${command['command']}');
      return true;
    } catch (e) {
      print('[WebSocket] Error sending command: $e');
      return false;
    }
  }

  /// Set volume
  Future<bool> setVolume(int volume) {
    return sendCommand({
      'command': 'setVolume',
      'volume': volume,
    });
  }

  /// Set mute
  Future<bool> setMute(bool muted) {
    return sendCommand({
      'command': 'setMute',
      'muted': muted,
    });
  }

  /// Set power
  Future<bool> setPower(bool power) {
    return sendCommand({
      'command': 'setPower',
      'power': power,
    });
  }

  /// Set audio source
  Future<bool> setSource(int source) {
    return sendCommand({
      'command': 'setSource',
      'source': source,
    });
  }

  /// Set EQ preset
  Future<bool> setEQPreset(int preset) {
    return sendCommand({
      'command': 'setEQ',
      'preset': preset,
    });
  }

  /// Set bass and treble
  Future<bool> setEQ({int? bass, int? treble}) {
    final Map<String, dynamic> command = {'command': 'setEQ'};
    if (bass != null) command['bass'] = bass;
    if (treble != null) command['treble'] = treble;
    return sendCommand(command);
  }

  /// Identify device (flash LED)
  Future<bool> identify() {
    return sendCommand({'command': 'identify'});
  }

  /// Get current status
  Future<bool> getStatus() {
    return sendCommand({'command': 'getStatus'});
  }

  /// Get device info
  Future<bool> getInfo() {
    return sendCommand({'command': 'getInfo'});
  }

  /// Dispose resources
  void dispose() {
    disconnect();
    _messageController.close();
    _connectionController.close();
  }
}
