import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/device_manager.dart';
import '../widgets/speaker_card.dart';
import 'rooms_screen.dart';
import 'settings_screen.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  int _currentIndex = 0;

  final List<Widget> _screens = [
    const DevicesTab(),
    const RoomsScreen(),
    const SettingsScreen(),
  ];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: _screens[_currentIndex],
      bottomNavigationBar: NavigationBar(
        selectedIndex: _currentIndex,
        onDestinationSelected: (index) {
          setState(() {
            _currentIndex = index;
          });
        },
        destinations: const [
          NavigationDestination(
            icon: Icon(Icons.speaker_outlined),
            selectedIcon: Icon(Icons.speaker),
            label: 'Speakers',
          ),
          NavigationDestination(
            icon: Icon(Icons.meeting_room_outlined),
            selectedIcon: Icon(Icons.meeting_room),
            label: 'Rooms',
          ),
          NavigationDestination(
            icon: Icon(Icons.settings_outlined),
            selectedIcon: Icon(Icons.settings),
            label: 'Settings',
          ),
        ],
      ),
    );
  }
}

/// Devices Tab - Shows all speakers
class DevicesTab extends StatelessWidget {
  const DevicesTab({super.key});

  @override
  Widget build(BuildContext context) {
    final deviceManager = context.watch<DeviceManager>();

    return Scaffold(
      appBar: AppBar(
        title: const Text('My Speakers'),
        actions: [
          IconButton(
            icon: deviceManager.isDiscovering
                ? const SizedBox(
                    width: 24,
                    height: 24,
                    child: CircularProgressIndicator(strokeWidth: 2),
                  )
                : const Icon(Icons.refresh),
            onPressed: deviceManager.isDiscovering
                ? null
                : () => deviceManager.startDiscovery(),
            tooltip: 'Scan for speakers',
          ),
        ],
      ),
      body: deviceManager.devices.isEmpty
          ? _buildEmptyState(context, deviceManager)
          : _buildDeviceList(deviceManager),
    );
  }

  Widget _buildEmptyState(BuildContext context, DeviceManager deviceManager) {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(
            Icons.speaker_outlined,
            size: 80,
            color: Theme.of(context).colorScheme.primary.withOpacity(0.5),
          ),
          const SizedBox(height: 16),
          Text(
            'No speakers found',
            style: Theme.of(context).textTheme.headlineSmall,
          ),
          const SizedBox(height: 8),
          Text(
            'Make sure your speakers are powered on\nand connected to the same WiFi network',
            textAlign: TextAlign.center,
            style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                  color: Theme.of(context).colorScheme.onSurface.withOpacity(0.6),
                ),
          ),
          const SizedBox(height: 24),
          FilledButton.icon(
            onPressed: () => deviceManager.startDiscovery(),
            icon: const Icon(Icons.search),
            label: const Text('Scan for Speakers'),
          ),
        ],
      ),
    );
  }

  Widget _buildDeviceList(DeviceManager deviceManager) {
    return RefreshIndicator(
      onRefresh: () async {
        await deviceManager.startDiscovery();
      },
      child: ListView.builder(
        padding: const EdgeInsets.all(16),
        itemCount: deviceManager.devices.length,
        itemBuilder: (context, index) {
          final device = deviceManager.devices[index];
          return SpeakerCard(device: device);
        },
      ),
    );
  }
}
