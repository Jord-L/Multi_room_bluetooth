import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/speaker_device.dart';
import '../services/device_manager.dart';

class DeviceControlScreen extends StatelessWidget {
  final String deviceId;

  const DeviceControlScreen({
    super.key,
    required this.deviceId,
  });

  @override
  Widget build(BuildContext context) {
    final deviceManager = context.watch<DeviceManager>();
    final device = deviceManager.getDevice(deviceId);

    if (device == null) {
      return Scaffold(
        appBar: AppBar(title: const Text('Device Not Found')),
        body: const Center(child: Text('Device not found')),
      );
    }

    return Scaffold(
      appBar: AppBar(
        title: Text(device.customName),
        actions: [
          IconButton(
            icon: const Icon(Icons.lightbulb_outline),
            onPressed: () => deviceManager.identifyDevice(deviceId),
            tooltip: 'Identify (flash LED)',
          ),
        ],
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          // Device Info Card
          _buildDeviceInfoCard(context, device),
          const SizedBox(height: 16),

          // Power Control
          _buildPowerControl(context, device, deviceManager),
          const SizedBox(height: 16),

          // Volume Control
          _buildVolumeControl(context, device, deviceManager),
          const SizedBox(height: 16),

          // Audio Source
          _buildAudioSourceCard(context, device, deviceManager),
          const SizedBox(height: 16),

          // Equalizer
          _buildEqualizerCard(context, device, deviceManager),
        ],
      ),
    );
  }

  Widget _buildDeviceInfoCard(BuildContext context, SpeakerDevice device) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Device Information',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
            ),
            const SizedBox(height: 12),
            _buildInfoRow('Device ID', device.deviceId),
            _buildInfoRow('IP Address', device.ipAddress),
            _buildInfoRow('Room', device.room),
            _buildInfoRow('Firmware', device.firmwareVersion),
            _buildInfoRow(
              'Signal',
              '${device.signalStrength} dBm',
              trailing: _buildSignalIcon(device.signalStrength),
            ),
            _buildInfoRow(
              'Status',
              device.isConnected ? 'Connected' : 'Disconnected',
              trailing: Icon(
                device.isConnected ? Icons.check_circle : Icons.cancel,
                color: device.isConnected ? Colors.green : Colors.red,
                size: 20,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildInfoRow(String label, String value, {Widget? trailing}) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label, style: const TextStyle(fontWeight: FontWeight.w500)),
          Row(
            children: [
              Text(value),
              if (trailing != null) ...[
                const SizedBox(width: 8),
                trailing,
              ],
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildSignalIcon(int rssi) {
    IconData icon;
    Color color;

    if (rssi > -50) {
      icon = Icons.signal_wifi_4_bar;
      color = Colors.green;
    } else if (rssi > -70) {
      icon = Icons.signal_wifi_3_bar;
      color = Colors.orange;
    } else {
      icon = Icons.signal_wifi_1_bar;
      color = Colors.red;
    }

    return Icon(icon, color: color, size: 20);
  }

  Widget _buildPowerControl(
    BuildContext context,
    SpeakerDevice device,
    DeviceManager manager,
  ) {
    return Card(
      child: SwitchListTile(
        title: const Text('Power'),
        subtitle: Text(device.powerState ? 'On' : 'Off'),
        value: device.powerState,
        onChanged: (value) {
          manager.setDevicePower(deviceId, value);
        },
        secondary: Icon(
          device.powerState ? Icons.power_settings_new : Icons.power_off,
          color: device.powerState ? Colors.green : Colors.grey,
        ),
      ),
    );
  }

  Widget _buildVolumeControl(
    BuildContext context,
    SpeakerDevice device,
    DeviceManager manager,
  ) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text(
                  'Volume',
                  style: Theme.of(context).textTheme.titleMedium?.copyWith(
                        fontWeight: FontWeight.bold,
                      ),
                ),
                Text(
                  '${device.volume}%',
                  style: Theme.of(context).textTheme.titleLarge?.copyWith(
                        fontWeight: FontWeight.bold,
                      ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            Row(
              children: [
                IconButton(
                  icon: Icon(
                    device.isMuted ? Icons.volume_off : Icons.volume_up,
                  ),
                  onPressed: () {
                    manager.setDeviceMute(deviceId, !device.isMuted);
                  },
                ),
                Expanded(
                  child: Slider(
                    value: device.volume.toDouble(),
                    min: 0,
                    max: 100,
                    divisions: 20,
                    label: '${device.volume}%',
                    onChanged: device.powerState
                        ? (value) {
                            manager.setDeviceVolume(deviceId, value.toInt());
                          }
                        : null,
                  ),
                ),
                IconButton(
                  icon: const Icon(Icons.volume_up),
                  onPressed: device.powerState && device.volume < 100
                      ? () {
                          manager.setDeviceVolume(
                            deviceId,
                            (device.volume + 5).clamp(0, 100),
                          );
                        }
                      : null,
                ),
              ],
            ),
            if (device.isMuted)
              Container(
                padding: const EdgeInsets.all(8),
                decoration: BoxDecoration(
                  color: Colors.orange.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: const Row(
                  children: [
                    Icon(Icons.volume_off, size: 20, color: Colors.orange),
                    SizedBox(width: 8),
                    Text('Speaker is muted'),
                  ],
                ),
              ),
          ],
        ),
      ),
    );
  }

  Widget _buildAudioSourceCard(
    BuildContext context,
    SpeakerDevice device,
    DeviceManager manager,
  ) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Audio Source',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
            ),
            const SizedBox(height: 12),
            ...AudioSource.values.map((source) {
              return RadioListTile<AudioSource>(
                title: Text(source.displayName),
                subtitle: Text(source.icon),
                value: source,
                groupValue: device.audioSource,
                onChanged: device.powerState
                    ? (value) {
                        // TODO: Implement source switching via WebSocket
                        // manager.setDeviceSource(deviceId, value!.index);
                      }
                    : null,
              );
            }),
          ],
        ),
      ),
    );
  }

  Widget _buildEqualizerCard(
    BuildContext context,
    SpeakerDevice device,
    DeviceManager manager,
  ) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Equalizer',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
            ),
            const SizedBox(height: 12),
            // Preset selector
            DropdownButton<EQPreset>(
              value: device.eqPreset,
              isExpanded: true,
              items: EQPreset.values.map((preset) {
                return DropdownMenuItem(
                  value: preset,
                  child: Text(preset.displayName),
                );
              }).toList(),
              onChanged: device.powerState
                  ? (value) {
                      // TODO: Implement EQ preset via WebSocket
                      // manager.setDeviceEQPreset(deviceId, value!.index);
                    }
                  : null,
            ),
            const SizedBox(height: 16),
            // Bass control
            Text('Bass: ${device.bassBoost} dB'),
            Slider(
              value: device.bassBoost.toDouble(),
              min: -12,
              max: 12,
              divisions: 24,
              label: '${device.bassBoost} dB',
              onChanged: device.powerState
                  ? (value) {
                      // TODO: Implement bass control via WebSocket
                      // manager.setDeviceBass(deviceId, value.toInt());
                    }
                  : null,
            ),
            // Treble control
            Text('Treble: ${device.trebleAdjust} dB'),
            Slider(
              value: device.trebleAdjust.toDouble(),
              min: -12,
              max: 12,
              divisions: 24,
              label: '${device.trebleAdjust} dB',
              onChanged: device.powerState
                  ? (value) {
                      // TODO: Implement treble control via WebSocket
                      // manager.setDeviceTreble(deviceId, value.toInt());
                    }
                  : null,
            ),
          ],
        ),
      ),
    );
  }
}
