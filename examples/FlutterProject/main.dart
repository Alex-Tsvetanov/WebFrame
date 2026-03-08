import 'package:flutter/material.dart';
import 'package:coroute_framework/coroute_framework.dart';

import 'templates/app/login_screen.dart';
import 'templates/app/dashboard_screen.dart';
import 'templates/app/task_detail_screen.dart';

void main() {
  // Initialize the Coroute C++ bridge
  try {
    Bridge.initialize();
    Bridge.initApp();
  } catch (e) {
    // Bridge may not be available in pure web mode; continue
    debugPrint('Bridge initialization skipped: $e');
  }

  // Register Flutter screen widgets for each view route
  ScreenRegistry().register('LoginScreen', (model) => LoginScreen(model: model));
  ScreenRegistry().register('DashboardScreen', (model) => DashboardScreen(model: model));
  ScreenRegistry().register('TaskDetailScreen', (model) => TaskDetailScreen(model: model));

  runApp(const _App());
}

class _App extends StatelessWidget {
  const _App();

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Task Dashboard',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: const Color(0xFF1a1a2e)),
        useMaterial3: true,
      ),
      home: const GenericView(route: '/login'),
    );
  }
}
