import 'package:flutter/material.dart';

class DashboardScreen extends StatelessWidget {
  final Map<String, dynamic> model;

  const DashboardScreen({super.key, required this.model});

  @override
  Widget build(BuildContext context) {
    bool authenticated = model['authenticated'] ?? false;
    String username = model['username'] ?? '';
    String title = model['title'] ?? 'Task Dashboard';

    return Scaffold(
      appBar: AppBar(
        title: Text(title),
        actions: [
          if (authenticated)
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 16.0),
              child: Center(child: Text('User: $username')),
            ),
        ],
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            if (authenticated) ...[
              const Text(
                'Welcome to Task Dashboard! real-time logic goes here.',
              ),
              // The logic to render tasks would be here using WebSockets
            ] else ...[
              const Text('Please log in to see your tasks.'),
            ],
          ],
        ),
      ),
    );
  }
}
