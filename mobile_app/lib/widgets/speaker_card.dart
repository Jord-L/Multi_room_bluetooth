import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/speaker_device.dart';
import '../services/device_manager.dart';
import '../screens/device_control_screen.dart';

class SpeakerCard extends StatelessWidget {
  final SpeakerDevice device;

  const SpeakerCard({
    super.key,
    required this.device,
  });

  @override
  Widget build(BuildContext context) {
    final deviceManager = context.watch<DeviceManager>();

    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      child: InkWell(
        onTap: () {
          Navigator.push(
            context,
            MaterialPageRoute(
              builder: (context) => DeviceControlScreen(deviceId: device.deviceId),
            ),
          );
        },
        borderRadius: BorderRadius.circular(12),
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              // Header Row
              Row(
                children: [
                  // Speaker Icon
                  Container(
                    width: 48,
                    height: 48,
                    decoration: BoxDecoration(
                      color: device.powerState
                          ? Theme.of(context).colorScheme.primaryContainer
                          : Theme.of(context).colorScheme.surfaceVariant,
                      borderRadius: BorderRadius.circular(12),
                    ),
                    child: Icon(
                      Icons.speaker,
                      color: device.powerState
                          ? Theme.of(context).colorScheme.onPrimaryContainer
                          : Theme.of(context).colorScheme.onSurfaceVariant,
                    ),
                  ),
                  const SizedBox(width: 12),

                  // Device Info
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(
                          device.customName,
                          style: Theme.of(context).textTheme.titleMedium?.copyWith(
                                fontWeight: FontWeight.bold,
                              ),
                        ),
                        const SizedBox(height: 4),
                        Row(
                          children: [
                            Icon(
                              Icons.room,
                              size: 14,
                              color: Theme.of(context).colorScheme.onSurface.withOpacity(0.6),
                            ),
                            const SizedBox(width: 4),
                            Text(
                              device.room,
                              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                                    color: Theme.of(context).colorScheme.onSurface.withOpacity(0.6),
                                  ),
                            ),
                            const SizedBox(width: 12),
                            Text(
                              device.audioSource.icon,
                              style: const TextStyle(fontSize: 12),
                            ),
                          ],
                        ),
                      ],
                    ),
                  ),

                  // Connection Status
                  _buildConnectionStatus(context),

                  // Power Button
                  IconButton(
                    icon: Icon(
                      device.powerState ? Icons.power_settings_new : Icons.power_off,
                      color: device.powerState ? Colors.green : Colors.grey,
                    ),
                    onPressed: () {
                      deviceManager.setDevicePower(device.deviceId, !device.powerState);
                    },
                  ),
                ],
              ),

              const SizedBox(height: 16),

              // Volume Control
              Row(
                children: [
                  IconButton(
                    icon: Icon(
                      device.isMuted ? Icons.volume_off : Icons.volume_up,
                    ),
                    onPressed: () {
                      deviceManager.setDeviceMute(device.deviceId, !device.isMuted);
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
                              deviceManager.setDeviceVolume(device.deviceId, value.toInt());
                            }
                          : null,
                    ),
                  ),
                  SizedBox(
                    width: 40,
                    child: Text(
                      '${device.volume}%',
                      style: Theme.of(context).textTheme.bodySmall,
                      textAlign: TextAlign.right,
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildConnectionStatus(BuildContext context) {
    if (device.isConnecting) {
      return const SizedBox(
        width: 16,
        height: 16,
        child: CircularProgressIndicator(strokeWidth: 2),
      );
    }

    return Container(
      width: 8,
      height: 8,
      decoration: BoxDecoration(
        color: device.isConnected ? Colors.green : Colors.red,
        shape: BoxShape.circle,
      ),
    );
  }
}
