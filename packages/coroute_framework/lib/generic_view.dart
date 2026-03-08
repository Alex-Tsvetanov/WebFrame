import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'package:flutter/material.dart';
import 'bridge.dart';
import 'screen_registry.dart';

class GenericView extends StatefulWidget {
  final String route;
  const GenericView({super.key, required this.route});

  @override
  State<GenericView> createState() => _GenericViewState();
}

class _GenericViewState extends State<GenericView> {
  late String _currentRoute;
  Map<String, dynamic>? _viewData;
  bool _loading = true;
  String? _error;

  // FFI broadcast — fires when Flutter and the C++ backend are in the same process.
  StreamSubscription<String>? _broadcastSub;

  // WebSocket — fires when Flutter connects to a remote web server (separate process).
  WebSocket? _ws;
  bool _wsConnecting = false;

  @override
  void initState() {
    super.initState();
    _currentRoute = widget.route;
    _fetchView();
    _broadcastSub = Bridge.broadcastStream.listen(_onBroadcast);
    _connectWebSocket();
  }

  // Connect to the server WebSocket for cross-process push events.
  // This covers the case where Flutter is a pure client talking to a remote
  // web server (separate process), where the FFI broadcast has no effect.
  Future<void> _connectWebSocket() async {
    final wsBase = Bridge.wsBaseUrl;
    if (wsBase == null) return;

    // Remap localhost to Android emulator host when needed.
    var wsUrl = '$wsBase/ws';
    if (Platform.isAndroid &&
        (wsUrl.contains('localhost') || wsUrl.contains('127.0.0.1'))) {
      wsUrl = wsUrl.replaceFirst(
          RegExp(r'localhost|127\.0\.0\.1'), '10.0.2.2');
    }

    if (_wsConnecting) return;
    _wsConnecting = true;

    try {
      _ws = await WebSocket.connect(wsUrl);
      _ws!.listen(
        (data) {
          if (data is String) _onBroadcast(data);
        },
        onDone: () {
          _wsConnecting = false;
          // Reconnect after a short delay if widget is still mounted.
          if (mounted) {
            Future.delayed(const Duration(seconds: 3), _connectWebSocket);
          }
        },
        onError: (_) {
          _wsConnecting = false;
        },
        cancelOnError: true,
      );
    } catch (_) {
      _wsConnecting = false;
      // Server may not be up yet; retry silently.
      if (mounted) {
        Future.delayed(const Duration(seconds: 3), _connectWebSocket);
      }
    }
  }

  void _onBroadcast(String message) {
    try {
      final data = jsonDecode(message) as Map<String, dynamic>;
      final event = data['event'] as String? ?? data['type'] as String? ?? '';
      if (event.startsWith('task_')) {
        _fetchView(silent: true);
      }
    } catch (_) {
      // Ignore unparseable messages
    }
  }

  @override
  void dispose() {
    _broadcastSub?.cancel();
    _ws?.close();
    super.dispose();
  }

  Future<void> _fetchView({bool silent = false}) async {
    if (!silent) {
      setState(() {
        _loading = true;
        _error = null;
      });
    }

    try {
      // Call C++ bridge
      final jsonStr = await Bridge.requestView(_currentRoute);
      print(
          "[GenericView] Fetched jsonStr: ${jsonStr.length > 500 ? jsonStr.substring(0, 500) + '...' : jsonStr}");

      if (mounted) {
        final data = jsonDecode(jsonStr);
        setState(() {
          _viewData = data;
          _loading = false;
        });
      }
    } catch (e) {
      if (mounted) {
        setState(() {
          _error = e.toString();
          _loading = false;
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    if (_loading && _viewData == null) {
      return const Scaffold(body: Center(child: CircularProgressIndicator()));
    }

    if (_error != null && _viewData == null) {
      return Scaffold(
        appBar: AppBar(title: const Text("Error")),
        body: Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              const Icon(Icons.error, color: Colors.red, size: 48),
              const SizedBox(height: 16),
              Text("Error: $_error"),
              ElevatedButton(onPressed: _fetchView, child: const Text("Retry")),
            ],
          ),
        ),
      );
    }

    if (_viewData == null) {
      return const Scaffold(body: Center(child: Text("No data")));
    }

    // Determine template
    final templates = _viewData!['templates'];
    final model = _viewData!['model'];

    if (templates == null) {
      return Scaffold(
        body: Center(
          child: Text("Error: Templates missing in response\nJSON: $_viewData"),
        ),
      );
    }

    final String mobileTemplate = templates['mobile'] ?? 'Unknown';
    final screen = ScreenRegistry().build(mobileTemplate, model);

    if (screen != null) {
      return screen;
    }

    return Scaffold(
      body: Center(
        child: Text("Unknown screen: $mobileTemplate\nModel: $model"),
      ),
    );
  }
}
