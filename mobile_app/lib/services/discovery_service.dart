import 'dart:async';
import 'package:nsd/nsd.dart';

/// mDNS Discovery Service for finding ESP32 speakers on the network
class DiscoveryService {
  final Discovery _discovery = Discovery();
  final StreamController<DiscoveredDevice> _devicesController =
      StreamController<DiscoveredDevice>.broadcast();

  Stream<DiscoveredDevice> get devicesFound => _devicesController.stream;

  bool _isDiscovering = false;
  bool get isDiscovering => _isDiscovering;

  /// Start discovering ESP32 hub
  Future<void> startDiscovery() async {
    if (_isDiscovering) {
      print('[Discovery] Already discovering');
      return;
    }

    try {
      print('[Discovery] Starting mDNS discovery for _esp32hub._tcp');
      _isDiscovering = true;

      await for (final service in _discovery.discoverServices('_esp32hub._tcp')) {
        print('[Discovery] Found hub: ${service.name}');

        // Resolve the service to get IP address
        final resolved = await _discovery.resolveService(service);

        if (resolved != null && resolved.host != null) {
          final device = DiscoveredDevice(
            name: service.name ?? 'Unknown',
            host: resolved.host!,
            port: resolved.port ?? 80,
            txt: resolved.txt ?? {},
          );

          _devicesController.add(device);
          print('[Discovery] Resolved: ${device.host}:${device.port}');
        }
      }
    } catch (e) {
      print('[Discovery] Error during discovery: $e');
      _isDiscovering = false;
    }
  }

  /// Stop discovery
  Future<void> stopDiscovery() async {
    if (!_isDiscovering) return;

    print('[Discovery] Stopping discovery');
    await _discovery.stopDiscovery();
    _isDiscovering = false;
  }

  /// Dispose resources
  void dispose() {
    stopDiscovery();
    _devicesController.close();
  }
}

/// Discovered Device Information
class DiscoveredDevice {
  final String name;
  final String host;
  final int port;
  final Map<String, String> txt;

  DiscoveredDevice({
    required this.name,
    required this.host,
    required this.port,
    required this.txt,
  });

  /// Extract hub ID from TXT records
  String? get hubId => txt['hubId'];

  /// Extract hub name from TXT records
  String? get hubName => txt['name'];

  /// Extract firmware version from TXT records
  String? get firmwareVersion => txt['version'];

  /// Extract IP address from TXT records (fallback to host)
  String get ipAddress => txt['ip'] ?? host;

  /// Extract MAC address from TXT records
  String? get macAddress => txt['mac'];

  /// Extract max speakers from TXT records
  int? get maxSpeakers {
    final value = txt['maxSpeakers'];
    return value != null ? int.tryParse(value) : null;
  }

  /// Extract device type from TXT records
  String? get deviceType => txt['type'];

  @override
  String toString() {
    return 'DiscoveredDevice(name: $name, host: $host, port: $port, hubId: $hubId, type: $deviceType)';
  }
}
