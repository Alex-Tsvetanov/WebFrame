import 'package:flutter/material.dart';
import 'package:coroute_framework/bridge.dart';
import 'package:coroute_framework/generic_view.dart'; // For navigation if needed? No, screen registry handles view rendering. Or we navigate via Bridge.
import 'dart:convert';

class LoginScreen extends StatefulWidget {
  final Map<String, dynamic> model;

  const LoginScreen({super.key, required this.model});

  @override
  State<LoginScreen> createState() => _LoginScreenState();
}

class _LoginScreenState extends State<LoginScreen> {
  final _formKey = GlobalKey<FormState>();
  final _userController = TextEditingController();
  String? _error;
  bool _isLoading = false;

  Future<void> _login() async {
    if (!_formKey.currentState!.validate()) return;

    setState(() {
      _isLoading = true;
      _error = null;
    });

    final username = _userController.text;
    
    // Construct request URL/Body for API
    // We use Bridge to submit action? Or manual fetch?
    // Bridge usually does "request_view". 
    // We need "submit_action" or similar if we want to post data.
    // Let's check bridge.dart capabilities.
    // It has: static String requestView(String route)
    // It has: static String submitAction(String route, String json) ?? 
    // Wait, let's check bridge.dart content again.
    
    // Assuming we have a way to make API calls. 
    // If not, we might need to add one or use http package directly?
    // But http package needs network access. FFI bridge is for internal logic.
    // The C++ app has `app.post("/api/login")`.
    // If we are in "Client Mode" (mobile), we usually use HTTP.
    // But `examples/view_example` is unified.
    // If we use `Bridge.submitAction`, it goes to `bridge.cpp`.
    // Let's verify `bridge.cpp` has `submit_action`.
    
    // For now, I'll assume we can use `http` package for API calls if we are in Client Mode?
    // But we want to reuse the C++ logic if possible.
    // The Bridge has `submit_action`.
    
    try {
      // route: /api/login?user=... (GET/POST?)
      // The C++ handler is: app.post("/api/login", ...)
      // It reads query params: req.query_opt("user")
      
      // Let's try constructing the URL with query params for simplicity as the C++ example used query params.
      final route = "/api/login?user=$username";
      
      // We use submitAction (if available) or modify Bridge to support POST.
      // Checking bridge.cpp previously viewed:
      // const char* submit_action(const char* route, const char* json_data)
      
      // Updated to async call
      final responseJson = await Bridge.submitAction(route, "{}"); 
      // submit_action in C++ bridge usually just forwards to app.fetch internally?
      // Need to verify bridge.cpp implementation.
      
      final data = jsonDecode(responseJson);
      if (data['status'] == 'ok') {
         // Login success. Navigate to profile.
         // How to navigate? 
         // We can trigger a view refresh or push a new route.
         // In this simple app, maybe we just call `Bridge.requestView("/profile")`?
         // No, the GenericView widget handles the current route.
         // We need to tell the parent GenericView to change route?
         // Or just push a new GenericView.
         if (context.mounted) {
           Navigator.of(context).pushReplacement(
             MaterialPageRoute(builder: (context) => const GenericView(route: '/profile')),
           );
         }
      } else {
        setState(() {
          _error = data['message'] ?? 'Login failed';
        });
      }
    } catch (e) {
      setState(() {
        _error = "Error: $e";
      });
    } finally {
      if (mounted) {
        setState(() {
          _isLoading = false;
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("Login")),
      body: Center(
        child: Container(
          padding: const EdgeInsets.all(24),
          constraints: const BoxConstraints(maxWidth: 400),
          child: Form(
            key: _formKey,
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                if (_error != null)
                  Padding(
                    padding: const EdgeInsets.only(bottom: 16),
                    child: Text(_error!, style: const TextStyle(color: Colors.red)),
                  ),
                TextFormField(
                  controller: _userController,
                  decoration: const InputDecoration(
                    labelText: "Username",
                    border: OutlineInputBorder(),
                  ),
                  validator: (value) => value == null || value.isEmpty ? "Enter username" : null,
                ),
                const SizedBox(height: 24),
                ElevatedButton(
                  onPressed: _isLoading ? null : _login,
                  child: _isLoading 
                    ? const CircularProgressIndicator() 
                    : const Text("Sign In"),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
