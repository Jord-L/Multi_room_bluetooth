import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/hub_manager.dart';
import '../models/bluetooth_speaker.dart';
import 'settings_screen.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  @override
  void initState() {
    super.initState();
    // Start hub discovery on app launch
    WidgetsBinding.instance.addPostFrameCallback((_) {
      context.read<HubManager>().startHubDiscovery();
    });
  }

  @override
  Widget build(BuildContext context) {
    final hubManager = context.watch<HubManager>();

    return Scaffold(
      appBar: AppBar(
        title: const Text('Bluetooth Hub Controller'),
        actions: [
          // Hub connection status
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            child: Center(
              child: _buildConnectionStatus(hubManager),
            ),
          ),
        ],
      ),
      body: hubManager.hub == null
          ? _buildHubDiscovery(context, hubManager)
          : _buildHubControl(context, hubManager),
    );
  }

  /// Build connection status indicator
  Widget _buildConnectionStatus(HubManager hubManager) {
    if (hubManager.isConnecting) {
      return const Row(
        children: [
          SizedBox(
            width: 16,
            height: 16,
            child: CircularProgressIndicator(strokeWidth: 2),
          ),
          SizedBox(width: 8),
          Text('Connecting...', style: TextStyle(fontSize: 12)),
        ],
      );
    }

    if (hubManager.isConnected) {
      return Row(
        children: [
          Container(
            width: 12,
            height: 12,
            decoration: const BoxDecoration(
              color: Colors.green,
              shape: BoxShape.circle,
            ),
          ),
          const SizedBox(width: 8),
          const Text('Connected', style: TextStyle(fontSize: 12)),
        ],
      );
    }

    return Row(
      children: [
        Container(
          width: 12,
          height: 12,
          decoration: const BoxDecoration(
            color: Colors.red,
            shape: BoxShape.circle,
          ),
        ),
        const SizedBox(width: 8),
        const Text('Disconnected', style: TextStyle(fontSize: 12)),
      ],
    );
  }

  /// Build hub discovery screen
  Widget _buildHubDiscovery(BuildContext context, HubManager hubManager) {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(
            Icons.hub_outlined,
            size: 80,
            color: Theme.of(context).colorScheme.primary.withOpacity(0.5),
          ),
          const SizedBox(height: 24),
          const Text(
            'No Hub Found',
            style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
          ),
          const SizedBox(height: 8),
          Text(
            'Searching for ESP32 Bluetooth Hub...',
            style: TextStyle(
              fontSize: 14,
              color: Theme.of(context).colorScheme.onSurface.withOpacity(0.6),
            ),
          ),
          const SizedBox(height: 32),
          if (hubManager.isDiscoveringHub)
            const CircularProgressIndicator()
          else
            ElevatedButton.icon(
              onPressed: () => hubManager.startHubDiscovery(),
              icon: const Icon(Icons.search),
              label: const Text('Search for Hub'),
            ),
        ],
      ),
    );
  }

  /// Build hub control screen
  Widget _buildHubControl(BuildContext context, HubManager hubManager) {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // Hub info card
          _buildHubInfoCard(context, hubManager),
          const SizedBox(height: 16),

          // Hub controls
          _buildHubControlsCard(context, hubManager),
          const SizedBox(height: 16),

          // Audio streaming card
          _buildAudioStreamingCard(context, hubManager),
          const SizedBox(height: 16),

          // Connected speakers section
          _buildSpeakersSection(context, hubManager),
        ],
      ),
    );
  }

  /// Build hub info card
  Widget _buildHubInfoCard(BuildContext context, HubManager hubManager) {
    final hub = hubManager.hub!;

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.hub),
                const SizedBox(width: 12),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        hub.customName,
                        style: const TextStyle(
                          fontSize: 18,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                      Text(
                        hub.location,
                        style: TextStyle(
                          fontSize: 14,
                          color: Theme.of(context).colorScheme.onSurface.withOpacity(0.6),
                        ),
                      ),
                    ],
                  ),
                ),
                IconButton(
                  icon: const Icon(Icons.edit),
                  onPressed: () => _showEditHubDialog(context, hubManager),
                  tooltip: 'Edit hub name',
                ),
              ],
            ),
            const Divider(height: 24),
            _buildInfoRow('Hub ID', hub.hubId),
            _buildInfoRow('IP Address', hub.ipAddress),
            _buildInfoRow('Firmware', hub.firmwareVersion),
            _buildInfoRow('Uptime', hub.uptimeFormatted),
            _buildInfoRow('Max Speakers', '${hub.maxSpeakers}'),
          ],
        ),
      ),
    );
  }

  Widget _buildInfoRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(
            label,
            style: TextStyle(
              fontSize: 14,
              color: Colors.grey[600],
            ),
          ),
          Text(
            value,
            style: const TextStyle(
              fontSize: 14,
              fontWeight: FontWeight.w500,
            ),
          ),
        ],
      ),
    );
  }

  /// Build hub controls card
  Widget _buildHubControlsCard(BuildContext context, HubManager hubManager) {
    final hub = hubManager.hub!;

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Hub Controls',
              style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 16),

            // Master volume
            Row(
              children: [
                const Icon(Icons.volume_up),
                const SizedBox(width: 12),
                const Text('Master Volume'),
                const Spacer(),
                Text('${hub.masterVolume}'),
              ],
            ),
            Slider(
              value: hub.masterVolume.toDouble(),
              min: 0,
              max: 100,
              divisions: 100,
              onChanged: (value) {
                hubManager.setMasterVolume(value.toInt());
              },
            ),

            const SizedBox(height: 8),

            // Mute and power buttons
            Row(
              children: [
                Expanded(
                  child: OutlinedButton.icon(
                    onPressed: () => hubManager.setMute(!hub.isMuted),
                    icon: Icon(hub.isMuted ? Icons.volume_off : Icons.volume_up),
                    label: Text(hub.isMuted ? 'Unmute' : 'Mute'),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: OutlinedButton.icon(
                    onPressed: () => hubManager.setPower(!hub.powerState),
                    icon: Icon(hub.powerState ? Icons.power_settings_new : Icons.power_off),
                    label: Text(hub.powerState ? 'Power Off' : 'Power On'),
                  ),
                ),
              ],
            ),

            const SizedBox(height: 8),

            // Identify button
            SizedBox(
              width: double.infinity,
              child: OutlinedButton.icon(
                onPressed: () => hubManager.identifyHub(),
                icon: const Icon(Icons.lightbulb_outline),
                label: const Text('Identify Hub (Flash LED)'),
              ),
            ),
          ],
        ),
      ),
    );
  }

  /// Build audio streaming card
  Widget _buildAudioStreamingCard(BuildContext context, HubManager hubManager) {
    final hub = hubManager.hub!;
    final canStream = hubManager.speakers.where((s) => s.isConnected).isNotEmpty;

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Text(
                  'Audio Streaming',
                  style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
                ),
                const Spacer(),
                if (hub.isStreaming)
                  Container(
                    padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                    decoration: BoxDecoration(
                      color: Colors.green,
                      borderRadius: BorderRadius.circular(12),
                    ),
                    child: const Text(
                      'STREAMING',
                      style: TextStyle(
                        color: Colors.white,
                        fontSize: 10,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                  ),
              ],
            ),
            const SizedBox(height: 16),

            if (!canStream)
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.orange.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(8),
                  border: Border.all(color: Colors.orange),
                ),
                child: const Row(
                  children: [
                    Icon(Icons.warning_amber, color: Colors.orange),
                    SizedBox(width: 12),
                    Expanded(
                      child: Text(
                        'No speakers connected. Connect at least one speaker to start streaming.',
                        style: TextStyle(fontSize: 12),
                      ),
                    ),
                  ],
                ),
              )
            else
              SizedBox(
                width: double.infinity,
                child: ElevatedButton.icon(
                  onPressed: hub.isStreaming
                      ? () => hubManager.stopAudioStreaming()
                      : () => hubManager.startAudioStreaming(),
                  icon: Icon(hub.isStreaming ? Icons.stop : Icons.play_arrow),
                  label: Text(hub.isStreaming ? 'Stop Streaming' : 'Start Streaming'),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: hub.isStreaming ? Colors.red : Colors.green,
                    foregroundColor: Colors.white,
                  ),
                ),
              ),

            if (hub.audioClientConnected) ...[
              const SizedBox(height: 12),
              _buildInfoRow('Buffer Level', '${hub.audioBufferLevel}%'),
            ],
          ],
        ),
      ),
    );
  }

  /// Build speakers section
  Widget _buildSpeakersSection(BuildContext context, HubManager hubManager) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            const Text(
              'Bluetooth Speakers',
              style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
            ),
            TextButton.icon(
              onPressed: () => _showSpeakerDiscovery(context, hubManager),
              icon: const Icon(Icons.add),
              label: const Text('Add Speaker'),
            ),
          ],
        ),
        const SizedBox(height: 8),

        if (hubManager.speakers.isEmpty)
          Card(
            child: Padding(
              padding: const EdgeInsets.all(32),
              child: Center(
                child: Column(
                  children: [
                    Icon(
                      Icons.speaker_outlined,
                      size: 48,
                      color: Theme.of(context).colorScheme.primary.withOpacity(0.5),
                    ),
                    const SizedBox(height: 16),
                    const Text('No speakers connected'),
                    const SizedBox(height: 8),
                    TextButton(
                      onPressed: () => _showSpeakerDiscovery(context, hubManager),
                      child: const Text('Discover Speakers'),
                    ),
                  ],
                ),
              ),
            ),
          )
        else
          ...hubManager.speakers.map((speaker) => _buildSpeakerCard(context, hubManager, speaker)),
      ],
    );
  }

  /// Build speaker card
  Widget _buildSpeakerCard(BuildContext context, HubManager hubManager, BluetoothSpeaker speaker) {
    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      child: ListTile(
        leading: CircleAvatar(
          backgroundColor: speaker.isConnected ? Colors.green : Colors.grey,
          child: Icon(
            speaker.isConnected ? Icons.bluetooth_connected : Icons.bluetooth_disabled,
            color: Colors.white,
          ),
        ),
        title: Text(speaker.displayName),
        subtitle: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('${speaker.state.displayName} • ${speaker.room}'),
            if (speaker.isConnected) ...[
              const SizedBox(height: 8),
              Row(
                children: [
                  Expanded(
                    child: Slider(
                      value: speaker.volume.toDouble(),
                      min: 0,
                      max: 100,
                      onChanged: (value) {
                        hubManager.setSpeakerVolume(speaker.btAddress, value.toInt());
                      },
                    ),
                  ),
                  Text('${speaker.volume}'),
                ],
              ),
            ],
          ],
        ),
        trailing: PopupMenuButton(
          itemBuilder: (context) => [
            if (speaker.isConnected)
              const PopupMenuItem(
                value: 'disconnect',
                child: Text('Disconnect'),
              )
            else
              const PopupMenuItem(
                value: 'connect',
                child: Text('Connect'),
              ),
            const PopupMenuItem(
              value: 'rename',
              child: Text('Rename'),
            ),
            const PopupMenuItem(
              value: 'room',
              child: Text('Set Room'),
            ),
            const PopupMenuItem(
              value: 'remove',
              child: Text('Remove'),
            ),
          ],
          onSelected: (value) => _handleSpeakerAction(context, hubManager, speaker, value as String),
        ),
        isThreeLine: speaker.isConnected,
      ),
    );
  }

  /// Handle speaker actions
  void _handleSpeakerAction(BuildContext context, HubManager hubManager, BluetoothSpeaker speaker, String action) {
    switch (action) {
      case 'connect':
        hubManager.connectToSpeaker(speaker.btAddress);
        break;
      case 'disconnect':
        hubManager.disconnectFromSpeaker(speaker.btAddress);
        break;
      case 'rename':
        _showRenameSpeakerDialog(context, hubManager, speaker);
        break;
      case 'room':
        _showSetRoomDialog(context, hubManager, speaker);
        break;
      case 'remove':
        _showRemoveSpeakerDialog(context, hubManager, speaker);
        break;
    }
  }

  /// Show edit hub dialog
  void _showEditHubDialog(BuildContext context, HubManager hubManager) {
    final nameController = TextEditingController(text: hubManager.hub!.customName);
    final locationController = TextEditingController(text: hubManager.hub!.location);

    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Edit Hub'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              controller: nameController,
              decoration: const InputDecoration(labelText: 'Hub Name'),
            ),
            const SizedBox(height: 16),
            TextField(
              controller: locationController,
              decoration: const InputDecoration(labelText: 'Location'),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          TextButton(
            onPressed: () {
              hubManager.setHubName(nameController.text);
              hubManager.setHubLocation(locationController.text);
              Navigator.pop(context);
            },
            child: const Text('Save'),
          ),
        ],
      ),
    );
  }

  /// Show speaker discovery dialog
  void _showSpeakerDiscovery(BuildContext context, HubManager hubManager) {
    showDialog(
      context: context,
      builder: (context) => _SpeakerDiscoveryDialog(hubManager: hubManager),
    );
  }

  /// Show rename speaker dialog
  void _showRenameSpeakerDialog(BuildContext context, HubManager hubManager, BluetoothSpeaker speaker) {
    final controller = TextEditingController(text: speaker.customName);

    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Rename Speaker'),
        content: TextField(
          controller: controller,
          decoration: const InputDecoration(labelText: 'Speaker Name'),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          TextButton(
            onPressed: () {
              hubManager.setSpeakerName(speaker.btAddress, controller.text);
              Navigator.pop(context);
            },
            child: const Text('Save'),
          ),
        ],
      ),
    );
  }

  /// Show set room dialog
  void _showSetRoomDialog(BuildContext context, HubManager hubManager, BluetoothSpeaker speaker) {
    final controller = TextEditingController(text: speaker.room);

    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Set Room'),
        content: TextField(
          controller: controller,
          decoration: const InputDecoration(labelText: 'Room Name'),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          TextButton(
            onPressed: () {
              hubManager.setSpeakerRoom(speaker.btAddress, controller.text);
              Navigator.pop(context);
            },
            child: const Text('Save'),
          ),
        ],
      ),
    );
  }

  /// Show remove speaker confirmation dialog
  void _showRemoveSpeakerDialog(BuildContext context, HubManager hubManager, BluetoothSpeaker speaker) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Remove Speaker'),
        content: Text('Are you sure you want to remove ${speaker.displayName}?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          TextButton(
            onPressed: () {
              hubManager.removeSpeaker(speaker.btAddress);
              Navigator.pop(context);
            },
            child: const Text('Remove'),
            style: TextButton.styleFrom(foregroundColor: Colors.red),
          ),
        ],
      ),
    );
  }
}

/// Speaker Discovery Dialog
class _SpeakerDiscoveryDialog extends StatefulWidget {
  final HubManager hubManager;

  const _SpeakerDiscoveryDialog({required this.hubManager});

  @override
  State<_SpeakerDiscoveryDialog> createState() => _SpeakerDiscoveryDialogState();
}

class _SpeakerDiscoveryDialogState extends State<_SpeakerDiscoveryDialog> {
  @override
  void initState() {
    super.initState();
    widget.hubManager.startSpeakerDiscovery();
  }

  @override
  void dispose() {
    widget.hubManager.stopSpeakerDiscovery();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: Row(
        children: [
          const Text('Discover Speakers'),
          const Spacer(),
          if (widget.hubManager.isDiscoveringSpeakers)
            const SizedBox(
              width: 20,
              height: 20,
              child: CircularProgressIndicator(strokeWidth: 2),
            ),
        ],
      ),
      content: SizedBox(
        width: double.maxFinite,
        child: widget.hubManager.discoveredSpeakers.isEmpty
            ? const Center(
                child: Padding(
                  padding: EdgeInsets.all(32.0),
                  child: Text('Searching for Bluetooth speakers...'),
                ),
              )
            : ListView.builder(
                shrinkWrap: true,
                itemCount: widget.hubManager.discoveredSpeakers.length,
                itemBuilder: (context, index) {
                  final speaker = widget.hubManager.discoveredSpeakers[index];
                  return ListTile(
                    leading: const Icon(Icons.bluetooth),
                    title: Text(speaker.deviceName),
                    subtitle: Text('Signal: ${speaker.signalStrengthPercent}%'),
                    trailing: ElevatedButton(
                      onPressed: () {
                        widget.hubManager.connectToSpeaker(speaker.btAddress);
                        Navigator.pop(context);
                      },
                      child: const Text('Connect'),
                    ),
                  );
                },
              ),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(context),
          child: const Text('Close'),
        ),
      ],
    );
  }
}
