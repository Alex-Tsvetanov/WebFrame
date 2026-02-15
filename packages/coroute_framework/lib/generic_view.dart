import 'dart:convert';
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

  @override
  void initState() {
    super.initState();
    _currentRoute = widget.route;
    _fetchView();
  }

  Future<void> _fetchView() async {
    setState(() {
      _loading = true;
      _error = null;
    });

    try {
      // Call C++ bridge
      final jsonStr = await Bridge.requestView(_currentRoute);
      print("Received JSON for $_currentRoute: $jsonStr");
      
      final data = jsonDecode(jsonStr);
      setState(() {
        _viewData = data;
        _loading = false;
      });
    } catch (e) {
      setState(() {
        _error = e.toString();
        _loading = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text("Route: $_currentRoute"),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: _fetchView,
          ),
        ],
      ),
      body: _buildBody(),
    );
  }

  Widget _buildBody() {
    if (_loading) {
      return const Center(child: CircularProgressIndicator());
    }

    if (_error != null) {
      return Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Icon(Icons.error, color: Colors.red, size: 48),
            const SizedBox(height: 16),
            Text("Error: $_error"),
            ElevatedButton(onPressed: _fetchView, child: const Text("Retry")),
          ],
        ),
      );
    }

    if (_viewData == null) {
      return const Center(child: Text("No data"));
    }

    // Determine template
    final templates = _viewData!['templates'];
    final model = _viewData!['model'];
    final String mobileTemplate = templates['mobile'] ?? 'Unknown';

    final screen = ScreenRegistry().build(mobileTemplate, model);
    
    if (screen != null) {
      return screen;
    }

    return Center(
      child: Text("Unknown screen: $mobileTemplate\nModel: $model"),
    );
  }
}
