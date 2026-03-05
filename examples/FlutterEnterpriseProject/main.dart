import 'package:coroute_framework/bridge.dart';
import 'package:coroute_framework/screen_registry.dart';
import 'package:coroute_framework/generic_view.dart';
import 'package:flutter/material.dart';

import 'templates/app/dashboard_screen.dart';
import 'templates/app/login_screen.dart';

void main() {
  // Initialize bridge
  try {
    Bridge.initialize();
    Bridge.initApp();
  } catch (e) {
    print("Failed to initialize bridge: $e");
  }

  // Register screens for routing
  ScreenRegistry().register(
    'DashboardScreen',
    (model) => DashboardScreen(model: model),
  );
  ScreenRegistry().register(
    'LoginScreen',
    (model) => LoginScreen(model: model),
  );

  runApp(const App(homeRoute: '/'));
}

class App extends StatelessWidget {
  final String homeRoute;

  const App({super.key, this.homeRoute = '/'});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Task Dashboard',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue),
        useMaterial3: true,
      ),
      home: GenericView(route: homeRoute),
    );
  }
}
