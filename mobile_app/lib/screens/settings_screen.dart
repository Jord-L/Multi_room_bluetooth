import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/device_manager.dart';

class SettingsScreen extends StatelessWidget {
  const SettingsScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final deviceManager = context.watch<DeviceManager>();

    return Scaffold(
      appBar: AppBar(
        title: const Text('Settings'),
      ),
      body: ListView(
        children: [
          // App Information
          const ListTile(
            title: Text('App Information'),
            subtitle: Text('Multi-Room Speaker Control'),
            leading: Icon(Icons.info_outline),
          ),
          const Divider(),

          // Network Section
          ListTile(
            title: const Text('Network'),
            leading: const Icon(Icons.wifi),
            subtitle: Text('${deviceManager.devices.length} speakers found'),
          ),
          ListTile(
            title: const Text('Scan for Speakers'),
            leading: const Icon(Icons.search),
            trailing: deviceManager.isDiscovering
                ? const SizedBox(
                    width: 24,
                    height: 24,
                    child: CircularProgressIndicator(strokeWidth: 2),
                  )
                : null,
            onTap: deviceManager.isDiscovering
                ? null
                : () => deviceManager.startDiscovery(),
          ),
          const Divider(),

          // Theme Section
          const ListTile(
            title: Text('Appearance'),
            leading: Icon(Icons.palette_outlined),
          ),
          SwitchListTile(
            title: const Text('Dark Mode'),
            subtitle: const Text('System default'),
            secondary: const Icon(Icons.dark_mode_outlined),
            value: false,
            onChanged: null, // TODO: Implement theme switching
          ),
          const Divider(),

          // About Section
          const ListTile(
            title: Text('About'),
            leading: Icon(Icons.info_outline),
          ),
          const ListTile(
            title: Text('Version'),
            subtitle: Text('1.0.0 - Phase 1'),
            leading: Icon(Icons.code),
          ),
          ListTile(
            title: const Text('Open Source Licenses'),
            leading: const Icon(Icons.article_outlined),
            onTap: () {
              showLicensePage(context: context);
            },
          ),
          const Divider(),

          // Debug Section
          ExpansionTile(
            title: const Text('Debug Information'),
            leading: const Icon(Icons.bug_report_outlined),
            children: [
              ListTile(
                title: const Text('Connected Devices'),
                subtitle: Text(
                  '${deviceManager.devices.where((d) => d.isConnected).length} / ${deviceManager.devices.length}',
                ),
              ),
              ListTile(
                title: const Text('Rooms'),
                subtitle: Text('${deviceManager.rooms.length}'),
              ),
            ],
          ),

          const SizedBox(height: 32),

          // Reset/Clear Data
          Padding(
            padding: const EdgeInsets.all(16),
            child: OutlinedButton.icon(
              onPressed: () => _showClearDataDialog(context),
              icon: const Icon(Icons.delete_outline),
              label: const Text('Clear All Data'),
              style: OutlinedButton.styleFrom(
                foregroundColor: Theme.of(context).colorScheme.error,
              ),
            ),
          ),
        ],
      ),
    );
  }

  void _showClearDataDialog(BuildContext context) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Clear All Data?'),
        content: const Text(
          'This will remove all saved speakers and room configurations. '
          'You can rediscover speakers by scanning again.',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () {
              // TODO: Implement clear data
              Navigator.pop(context);
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(content: Text('Data cleared')),
              );
            },
            style: FilledButton.styleFrom(
              backgroundColor: Theme.of(context).colorScheme.error,
            ),
            child: const Text('Clear'),
          ),
        ],
      ),
    );
  }
}
