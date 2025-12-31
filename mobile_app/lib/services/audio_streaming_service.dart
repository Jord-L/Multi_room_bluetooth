import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

/// Audio Streaming Service
/// Streams audio data to ESP32 hub via UDP
class AudioStreamingService {
  RawDatagramSocket? _socket;
  final String hubIpAddress;
  final int hubPort;
  bool _isStreaming = false;
  int _packetsSent = 0;
  int _bytesSent = 0;
  DateTime? _streamStartTime;

  // Audio format (must match ESP32 expectations)
  static const int sampleRate = 44100;
  static const int channels = 2; // Stereo
  static const int bitsPerSample = 16;
  static const int bufferSize = 4096; // Bytes per packet

  final StreamController<AudioStreamStats> _statsController =
      StreamController<AudioStreamStats>.broadcast();

  Stream<AudioStreamStats> get stats => _statsController.stream;
  bool get isStreaming => _isStreaming;
  int get packetsSent => _packetsSent;
  int get bytesSent => _bytesSent;

  AudioStreamingService({
    required this.hubIpAddress,
    this.hubPort = 8888,
  });

  /// Start audio streaming
  Future<bool> startStreaming() async {
    if (_isStreaming) {
      print('[AudioStream] Already streaming');
      return true;
    }

    try {
      // Create UDP socket
      _socket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);

      if (_socket == null) {
        print('[AudioStream] Failed to create UDP socket');
        return false;
      }

      _isStreaming = true;
      _packetsSent = 0;
      _bytesSent = 0;
      _streamStartTime = DateTime.now();

      print('[AudioStream] Started streaming to $hubIpAddress:$hubPort');
      print('[AudioStream] Format: ${sampleRate}Hz, $channels ch, $bitsPerSample-bit');

      return true;
    } catch (e) {
      print('[AudioStream] Error starting stream: $e');
      _isStreaming = false;
      return false;
    }
  }

  /// Stop audio streaming
  Future<void> stopStreaming() async {
    if (!_isStreaming) return;

    _socket?.close();
    _socket = null;
    _isStreaming = false;

    print('[AudioStream] Stopped streaming');
    print('[AudioStream] Stats: $_packetsSent packets, $_bytesSent bytes');

    _updateStats();
  }

  /// Send audio data packet
  Future<bool> sendAudioData(Uint8List audioData) async {
    if (!_isStreaming || _socket == null) {
      return false;
    }

    try {
      // Send to hub via UDP
      final address = InternetAddress(hubIpAddress);
      final bytesSent = _socket!.send(audioData, address, hubPort);

      if (bytesSent > 0) {
        _packetsSent++;
        _bytesSent += bytesSent;

        // Update stats periodically (every 100 packets)
        if (_packetsSent % 100 == 0) {
          _updateStats();
        }

        return true;
      }

      return false;
    } catch (e) {
      print('[AudioStream] Error sending audio: $e');
      return false;
    }
  }

  /// Send audio data from buffer
  Future<bool> sendAudioBuffer(List<int> buffer) async {
    return sendAudioData(Uint8List.fromList(buffer));
  }

  /// Create audio packet header
  /// Format: [magic(2), sequence(4), timestamp(8), length(4)]
  Uint8List createPacketHeader(int sequence, int timestamp, int dataLength) {
    final header = ByteData(18);

    // Magic number (0x4155 = "AU" for audio)
    header.setUint16(0, 0x4155, Endian.little);

    // Sequence number
    header.setUint32(2, sequence, Endian.little);

    // Timestamp (milliseconds)
    header.setUint64(6, timestamp, Endian.little);

    // Data length
    header.setUint32(14, dataLength, Endian.little);

    return header.buffer.asUint8List();
  }

  /// Send audio packet with header
  Future<bool> sendAudioPacket(Uint8List audioData) async {
    if (!_isStreaming) return false;

    // Create packet with header
    final header = createPacketHeader(
      _packetsSent,
      DateTime.now().millisecondsSinceEpoch,
      audioData.length,
    );

    // Combine header and data
    final packet = Uint8List(header.length + audioData.length);
    packet.setRange(0, header.length, header);
    packet.setRange(header.length, packet.length, audioData);

    return sendAudioData(packet);
  }

  /// Update streaming statistics
  void _updateStats() {
    if (_streamStartTime == null) return;

    final duration = DateTime.now().difference(_streamStartTime!);
    final stats = AudioStreamStats(
      packetsSent: _packetsSent,
      bytesSent: _bytesSent,
      duration: duration,
      bitrate: duration.inSeconds > 0
          ? (_bytesSent * 8) ~/ duration.inSeconds
          : 0,
    );

    _statsController.add(stats);
  }

  /// Get streaming statistics
  AudioStreamStats getStats() {
    final duration = _streamStartTime != null
        ? DateTime.now().difference(_streamStartTime!)
        : Duration.zero;

    return AudioStreamStats(
      packetsSent: _packetsSent,
      bytesSent: _bytesSent,
      duration: duration,
      bitrate: duration.inSeconds > 0
          ? (_bytesSent * 8) ~/ duration.inSeconds
          : 0,
    );
  }

  /// Dispose resources
  void dispose() {
    stopStreaming();
    _statsController.close();
  }
}

/// Audio Streaming Statistics
class AudioStreamStats {
  final int packetsSent;
  final int bytesSent;
  final Duration duration;
  final int bitrate; // bits per second

  AudioStreamStats({
    required this.packetsSent,
    required this.bytesSent,
    required this.duration,
    required this.bitrate,
  });

  /// Get bitrate as formatted string
  String get bitrateFormatted {
    if (bitrate < 1000) {
      return '$bitrate bps';
    } else if (bitrate < 1000000) {
      return '${(bitrate / 1000).toStringAsFixed(1)} kbps';
    } else {
      return '${(bitrate / 1000000).toStringAsFixed(1)} Mbps';
    }
  }

  /// Get bytes sent as formatted string
  String get bytesSentFormatted {
    if (bytesSent < 1024) {
      return '$bytesSent B';
    } else if (bytesSent < 1024 * 1024) {
      return '${(bytesSent / 1024).toStringAsFixed(1)} KB';
    } else {
      return '${(bytesSent / (1024 * 1024)).toStringAsFixed(1)} MB';
    }
  }

  /// Get duration as formatted string
  String get durationFormatted {
    final hours = duration.inHours;
    final minutes = duration.inMinutes % 60;
    final seconds = duration.inSeconds % 60;

    if (hours > 0) {
      return '$hours:${minutes.toString().padLeft(2, '0')}:${seconds.toString().padLeft(2, '0')}';
    } else {
      return '$minutes:${seconds.toString().padLeft(2, '0')}';
    }
  }
}
