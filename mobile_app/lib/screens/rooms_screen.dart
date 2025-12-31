import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/device_manager.dart';
import '../models/room.dart';

class RoomsScreen extends StatelessWidget {
  const RoomsScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final deviceManager = context.watch<DeviceManager>();

    // Group devices by room
    final Map<String, List<String>> roomDevices = {};
    for (final device in deviceManager.devices) {
      final roomName = device.room;
      roomDevices.putIfAbsent(roomName, () => []);
      roomDevices[roomName]!.add(device.deviceId);
    }

    return Scaffold(
      appBar: AppBar(
        title: const Text('Rooms'),
        actions: [
          IconButton(
            icon: const Icon(Icons.add),
            onPressed: () => _showCreateRoomDialog(context),
            tooltip: 'Create Room',
          ),
        ],
      ),
      body: roomDevices.isEmpty
          ? _buildEmptyState(context)
          : ListView.builder(
              padding: const EdgeInsets.all(16),
              itemCount: roomDevices.length,
              itemBuilder: (context, index) {
                final roomName = roomDevices.keys.elementAt(index);
                final deviceIds = roomDevices[roomName]!;
                return _buildRoomCard(context, roomName, deviceIds, deviceManager);
              },
            ),
    );
  }

  Widget _buildEmptyState(BuildContext context) {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(
            Icons.meeting_room_outlined,
            size: 80,
            color: Theme.of(context).colorScheme.primary.withOpacity(0.5),
          ),
          const SizedBox(height: 16),
          Text(
            'No rooms configured',
            style: Theme.of(context).textTheme.headlineSmall,
          ),
          const SizedBox(height: 8),
          Text(
            'Add speakers to create rooms automatically\nor create a room manually',
            textAlign: TextAlign.center,
            style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                  color: Theme.of(context).colorScheme.onSurface.withOpacity(0.6),
                ),
          ),
        ],
      ),
    );
  }

  Widget _buildRoomCard(
    BuildContext context,
    String roomName,
    List<String> deviceIds,
    DeviceManager manager,
  ) {
    final devices = deviceIds.map((id) => manager.getDevice(id)).whereType<_>().toList();
    final averageVolume = devices.isEmpty
        ? 0
        : devices.map((d) => d.volume).reduce((a, b) => a + b) ~/ devices.length;
    final anyMuted = devices.any((d) => d.isMuted);
    final allPowered = devices.every((d) => d.powerState);

    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Header
            Row(
              children: [
                Container(
                  width: 48,
                  height: 48,
                  decoration: BoxDecoration(
                    color: Theme.of(context).colorScheme.secondaryContainer,
                    borderRadius: BorderRadius.circular(12),
                  ),
                  child: Icon(
                    Icons.meeting_room,
                    color: Theme.of(context).colorScheme.onSecondaryContainer,
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        roomName,
                        style: Theme.of(context).textTheme.titleMedium?.copyWith(
                              fontWeight: FontWeight.bold,
                            ),
                      ),
                      const SizedBox(height: 4),
                      Text(
                        '${devices.length} speaker${devices.length != 1 ? 's' : ''}',
                        style: Theme.of(context).textTheme.bodySmall?.copyWith(
                              color: Theme.of(context).colorScheme.onSurface.withOpacity(0.6),
                            ),
                      ),
                    ],
                  ),
                ),
                IconButton(
                  icon: Icon(
                    allPowered ? Icons.power_settings_new : Icons.power_off,
                    color: allPowered ? Colors.green : Colors.grey,
                  ),
                  onPressed: () {
                    for (final device in devices) {
                      manager.setDevicePower(device.deviceId, !allPowered);
                    }
                  },
                ),
              ],
            ),

            const SizedBox(height: 16),

            // Volume Control for Room
            Row(
              children: [
                IconButton(
                  icon: Icon(anyMuted ? Icons.volume_off : Icons.volume_up),
                  onPressed: () {
                    for (final device in devices) {
                      manager.setDeviceMute(device.deviceId, !anyMuted);
                    }
                  },
                ),
                Expanded(
                  child: Slider(
                    value: averageVolume.toDouble(),
                    min: 0,
                    max: 100,
                    divisions: 20,
                    label: '$averageVolume%',
                    onChanged: allPowered
                        ? (value) {
                            for (final device in devices) {
                              manager.setDeviceVolume(device.deviceId, value.toInt());
                            }
                          }
                        : null,
                  ),
                ),
                SizedBox(
                  width: 40,
                  child: Text(
                    '$averageVolume%',
                    style: Theme.of(context).textTheme.bodySmall,
                    textAlign: TextAlign.right,
                  ),
                ),
              ],
            ),

            const Divider(),

            // List of speakers in room
            ...devices.map((device) => ListTile(
                  dense: true,
                  leading: Icon(
                    Icons.speaker,
                    size: 20,
                    color: device.powerState ? Colors.green : Colors.grey,
                  ),
                  title: Text(device.customName),
                  trailing: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      if (device.isMuted)
                        const Icon(Icons.volume_off, size: 16, color: Colors.orange),
                      const SizedBox(width: 8),
                      Text('${device.volume}%'),
                    ],
                  ),
                )),
          ],
        ),
      ),
    );
  }

  void _showCreateRoomDialog(BuildContext context) {
    final TextEditingController controller = TextEditingController();
    String selectedIcon = RoomIcons.generic;

    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Create Room'),
        content: StatefulBuilder(
          builder: (context, setState) => Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              TextField(
                controller: controller,
                decoration: const InputDecoration(
                  labelText: 'Room Name',
                  hintText: 'e.g., Living Room',
                ),
                autofocus: true,
              ),
              const SizedBox(height: 16),
              const Text('Choose Icon:'),
              const SizedBox(height: 8),
              Wrap(
                spacing: 8,
                children: RoomIcons.all.map((icon) {
                  return ChoiceChip(
                    label: Text(icon),
                    selected: icon == selectedIcon,
                    onSelected: (selected) {
                      setState(() {
                        selectedIcon = icon;
                      });
                    },
                  );
                }).toList(),
              ),
            ],
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () {
              if (controller.text.isNotEmpty) {
                context.read<DeviceManager>().createRoom(
                      controller.text,
                      icon: selectedIcon,
                    );
                Navigator.pop(context);
              }
            },
            child: const Text('Create'),
          ),
        ],
      ),
    );
  }
}
