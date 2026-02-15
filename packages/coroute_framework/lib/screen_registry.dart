import 'package:flutter/widgets.dart';

typedef ScreenFactory = Widget Function(Map<String, dynamic> model);

class ScreenRegistry {
  static final ScreenRegistry _instance = ScreenRegistry._internal();
  factory ScreenRegistry() => _instance;
  ScreenRegistry._internal();

  final Map<String, ScreenFactory> _screens = {};

  void register(String name, ScreenFactory factory) {
    _screens[name] = factory;
  }

  Widget? build(String name, Map<String, dynamic> model) {
    final factory = _screens[name];
    if (factory != null) {
      return factory(model);
    }
    return null;
  }
}
