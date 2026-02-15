import 'package:flutter/material.dart';
import 'package:coroute_framework/generic_view.dart';

class UserScreen extends StatelessWidget {
  final Map<String, dynamic> model;

  const UserScreen({
    super.key,
    required this.model,
  });

  @override
  Widget build(BuildContext context) {
    final name = model['name'] ?? 'Unknown';
    final greeting = model['greeting'] ?? 'Hello';
    final isLoggedIn = model['logged_in'] == true;

    return Scaffold(
      appBar: AppBar(title: Text(name)),
      body: Center(
        child: Card(
          margin: const EdgeInsets.all(24),
          child: Padding(
            padding: const EdgeInsets.all(32.0),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                CircleAvatar(
                  radius: 48,
                  backgroundColor: Colors.deepPurple.shade100,
                  child: Text(name[0], style: const TextStyle(fontSize: 48)),
                ),
                const SizedBox(height: 24),
                Text(greeting, style: Theme.of(context).textTheme.headlineSmall),
                Text(name, style: Theme.of(context).textTheme.titleLarge),
                const SizedBox(height: 32),
                if (isLoggedIn)
                  const Chip(
                    avatar: Icon(Icons.check_circle, color: Colors.green),
                    label: Text("Logged In"),
                  )
                else
                  const Chip(
                    avatar: Icon(Icons.cancel, color: Colors.grey),
                    label: Text("Guest Access"),
                  ),
                const SizedBox(height: 32),
                ElevatedButton.icon(
                  onPressed: () => Navigator.pushAndRemoveUntil(
                    context,
                    MaterialPageRoute(builder: (_) => const GenericView(route: '/')),
                    (route) => false,
                  ),
                  icon: const Icon(Icons.home),
                  label: const Text("Back to Home"),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
