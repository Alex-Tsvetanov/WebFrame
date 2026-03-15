import 'dart:async';
import 'dart:convert';
import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';
import 'package:http/http.dart' as http;

// Asset ID matched by @Native at compile time. The Dart runtime appends the
// platform-appropriate extension (.dylib/.so/.dll) when resolving the symbol.
const String _assetId = 'package:coroute_framework/src/coroute_app';

// Callback type definitions (used for NativeCallable — not @Native-able)
typedef FetchRequestCallbackC = Void Function(Int32 reqId, Pointer<Utf8> url,
    Pointer<Utf8> method, Pointer<Utf8> headers, Pointer<Utf8> body);

typedef ViewResponseCallbackC = Void Function(Int32 reqId, Pointer<Utf8> json);

typedef BroadcastCallbackC = Void Function(Pointer<Utf8> json);

// ---------------------------------------------------------------------------
// @Native bindings — resolved at compile time against the coroute_app asset.
// ---------------------------------------------------------------------------

@Native<Void Function()>(assetId: _assetId, symbol: 'init_app')
external void _initApp();

@Native<Void Function(Int32, Pointer<Utf8>, Pointer<Utf8>)>(
    assetId: _assetId, symbol: 'request_view_async')
external void _requestViewAsync(
    int reqId, Pointer<Utf8> route, Pointer<Utf8> headers);

@Native<Void Function(Int32, Pointer<Utf8>, Pointer<Utf8>, Pointer<Utf8>)>(
    assetId: _assetId, symbol: 'submit_action_async')
external void _submitActionAsync(
    int reqId, Pointer<Utf8> route, Pointer<Utf8> json, Pointer<Utf8> headers);

@Native<Void Function(Pointer<NativeFunction<FetchRequestCallbackC>>)>(
    assetId: _assetId, symbol: 'register_fetch_callback')
external void _registerFetchCallback(
    Pointer<NativeFunction<FetchRequestCallbackC>> callback);

@Native<Void Function(Int32, Int32, Pointer<Utf8>)>(
    assetId: _assetId, symbol: 'complete_fetch_request')
external void _completeFetchRequest(int reqId, int status, Pointer<Utf8> body);

@Native<Void Function(Pointer<NativeFunction<ViewResponseCallbackC>>)>(
    assetId: _assetId, symbol: 'register_view_callback')
external void _registerViewCallback(
    Pointer<NativeFunction<ViewResponseCallbackC>> callback);

@Native<Void Function(Pointer<NativeFunction<BroadcastCallbackC>>)>(
    assetId: _assetId, symbol: 'register_broadcast_callback')
external void _registerBroadcastCallback(
    Pointer<NativeFunction<BroadcastCallbackC>> callback);

@Native<Pointer<Utf8> Function()>(assetId: _assetId, symbol: 'get_api_base_url')
external Pointer<Utf8> _getApiBaseUrl();

@Native<Void Function(Pointer<Utf8>)>(
    assetId: _assetId, symbol: 'coroute_free_string')
external void _freeString(Pointer<Utf8> ptr);

class Bridge {
  // Keep listeners alive
  static NativeCallable<FetchRequestCallbackC>? _callbackListener;
  static NativeCallable<ViewResponseCallbackC>? _viewCallbackListener;
  static NativeCallable<BroadcastCallbackC>? _broadcastCallbackListener;

  // Broadcast stream — hub events pushed from C++ via FFI
  static final StreamController<String> _broadcastController =
      StreamController<String>.broadcast();

  /// Stream of raw JSON strings emitted by the C++ TaskHub on every broadcast.
  /// Listen to this instead of a TCP WebSocket in Flutter (client) mode.
  static Stream<String> get broadcastStream => _broadcastController.stream;

  /// WebSocket base URL derived from the configured API base URL.
  /// e.g. "http://localhost:8080" → "ws://localhost:8080"
  /// Returns null when no remote server is configured.
  static String? get wsBaseUrl {
    final base = _apiBaseUrl;
    if (base.startsWith('http://')) {
      return 'ws://${base.substring('http://'.length)}';
    }
    if (base.startsWith('https://')) {
      return 'wss://${base.substring('https://'.length)}';
    }
    return null;
  }

  // Pending view requests
  static final Map<int, Completer<String>> _pendingViews = {};
  static int _nextViewReqId = 1;

  // Simple cookie jar (Map<Name, Value>)
  static final Map<String, String> _cookies = {};

  // Base URL of the remote web server, resolved lazily on first _doFetch call
  // (App::instance() is null when initialize() runs, so we cannot read it there).
  static String _apiBaseUrl = 'http://localhost:8080';
  static bool _apiBaseUrlResolved = false;

  static void initialize() {
    // Register fetch callback bridge (C++ → Dart HTTP)
    try {
      _callbackListener =
          NativeCallable<FetchRequestCallbackC>.listener(_handleFetch);
      _registerFetchCallback(_callbackListener!.nativeFunction);
      print("Registered C++ Fetch Callback");
    } catch (e) {
      print("Fetch callback FFI not available: $e");
    }

    // Register view response callback bridge (C++ → Dart view completion)
    try {
      _viewCallbackListener =
          NativeCallable<ViewResponseCallbackC>.listener(_handleViewResponse);
      _registerViewCallback(_viewCallbackListener!.nativeFunction);
      print("Registered C++ View Callback");
    } catch (e) {
      print("View callback FFI not available: $e");
    }

    // Register broadcast callback (hub → Dart stream)
    try {
      _broadcastCallbackListener =
          NativeCallable<BroadcastCallbackC>.listener(_handleBroadcast);
      _registerBroadcastCallback(_broadcastCallbackListener!.nativeFunction);
      print("Registered C++ Broadcast Callback");
    } catch (e) {
      print("Broadcast callback FFI not available: $e");
    }
  }

  // Initialize the app
  static void initApp() {
    print("Initializing C++ app");
    _initApp();
  }

  // Handle broadcast message from C++ hub
  static void _handleBroadcast(Pointer<Utf8> jsonPtr) {
    final json = jsonPtr.toDartString();
    _freeString(jsonPtr);
    _broadcastController.add(json);
  }

  // Handle fetch request from C++
  // C++ uses strdup() for all pointer args so they are valid heap memory.
  // We must call malloc.free() on each after toDartString() copies the data.
  static void _handleFetch(
      int reqId,
      Pointer<Utf8> urlPtr,
      Pointer<Utf8> methodPtr,
      Pointer<Utf8> headersPtr,
      Pointer<Utf8> bodyPtr) {
    // Copy to Dart strings synchronously, then free the strdup'd C memory.
    final url = urlPtr.toDartString();
    final method = methodPtr.toDartString();
    final headers = headersPtr.toDartString();
    final body = bodyPtr.toDartString();

    _freeString(urlPtr);
    _freeString(methodPtr);
    _freeString(headersPtr);
    _freeString(bodyPtr);

    _doFetch(reqId, url, method, headers, body);
  }

  static final _httpClient = HttpClient()
    ..idleTimeout = const Duration(seconds: 30);

  static Future<void> _doFetch(int reqId, String url, String method,
      String headersJson, String body) async {
    var targetUrl = url;
    if (url.startsWith('/')) {
      // Resolve the API base URL lazily on the first request — App::instance()
      // is guaranteed to be alive by the time the first fetch arrives.
      if (!_apiBaseUrlResolved) {
        try {
          final ptr = _getApiBaseUrl();
          final resolved = ptr.toDartString();
          _freeString(ptr);
          if (resolved.isNotEmpty) {
            _apiBaseUrl = resolved;
            _apiBaseUrlResolved = true;
            print('[Bridge] API base URL resolved: $_apiBaseUrl');
          }
        } catch (e) {
          print('[Bridge] Could not resolve API base URL: $e');
        }
      }
      // For Android emulators 10.0.2.2 is the host loopback; honour that unless
      // an explicit non-localhost URL has been configured.
      var base = _apiBaseUrl;
      if (Platform.isAndroid &&
          (base.contains('localhost') || base.contains('127.0.0.1'))) {
        base = base.replaceFirst(RegExp(r'localhost|127\.0\.0\.1'), '10.0.2.2');
      }
      targetUrl = '$base$url';
    }

    print("--------------------------------------------------");
    print("[Dart Fetch] Request ID: $reqId");
    print("[Dart Fetch] Method:     $method");
    print("[Dart Fetch] URL:        $targetUrl");

    try {
      final uri = Uri.parse(targetUrl);
      final bodyBytes = utf8.encode(body);

      late int statusCode;
      late String responseBody;
      Map<String, String> requestHeaders = {
        HttpHeaders.contentTypeHeader: "application/json",
        "X-Requested-With": "Flutter"
      };

      // Add headers from C++
      if (headersJson.isNotEmpty) {
        try {
          final Map<String, dynamic> h = jsonDecode(headersJson);
          h.forEach((k, v) => requestHeaders[k] = v.toString());
        } catch (e) {
          print("[Dart Fetch] Error parsing headers JSON: $e");
        }
      }

      if (_cookies.isNotEmpty) {
        final cookieHeader =
            _cookies.entries.map((e) => "${e.key}=${e.value}").join("; ");
        requestHeaders[HttpHeaders.cookieHeader] = cookieHeader;
      }

      print("[Dart Fetch] Headers:    $requestHeaders");

      if (method == "POST") {
        print("[Dart Fetch] Body:       $body");

        final request = await _httpClient.postUrl(uri);
        requestHeaders
            .forEach((name, value) => request.headers.set(name, value));
        request.headers
            .set(HttpHeaders.contentLengthHeader, bodyBytes.length.toString());

        request.add(bodyBytes);
        final response = await request.close();
        statusCode = response.statusCode;
        responseBody = await response.transform(utf8.decoder).join();

        // Store cookies
        response.headers.forEach((name, values) {
          if (name.toLowerCase() == 'set-cookie') {
            for (final v in values) {
              final parts = v.split(';');
              for (final part in parts) {
                final kv = part.trim().split('=');
                if (kv.length >= 2 &&
                    (kv[0] == 'auth' ||
                        kv[0] == 'user_id' ||
                        kv[0] == 'username')) {
                  _cookies[kv[0]] = kv[1];
                }
              }
            }
          }
        });
      } else {
        final response = await http.get(uri, headers: requestHeaders);
        statusCode = response.statusCode;
        responseBody = response.body;
      }

      print("[Dart Fetch] Response:   $statusCode");
      print(
          "[Dart Fetch] Raw Body:   ${responseBody.length > 500 ? responseBody.substring(0, 500) + '...' : responseBody}");
      print("--------------------------------------------------");

      final bodyNative = responseBody.toNativeUtf8();
      _completeFetchRequest(reqId, statusCode, bodyNative);
      malloc.free(bodyNative);
    } catch (e, st) {
      print("[Dart Fetch] Error: $e\n$st");
      print("--------------------------------------------------");
      final errorNative = "{\"error\": \"$e\"}".toNativeUtf8();
      _completeFetchRequest(reqId, 500, errorNative);
      malloc.free(errorNative);
    }
  }

  // Handle view response from C++
  static void _handleViewResponse(int reqId, Pointer<Utf8> jsonPtr) {
    final jsonStr = jsonPtr.toDartString();
    _freeString(jsonPtr); // Free the memory allocated by C++ (strdup)

    String finalBody = jsonStr;
    try {
      if (jsonStr.startsWith('{"_coroute_ffi_')) {
        final parsed = jsonDecode(jsonStr);
        if (parsed is Map && parsed.containsKey('_coroute_ffi_response')) {
          final headers = parsed['headers'];
          if (headers is Map) {
            headers.forEach((k, v) {
              if (k.toString().toLowerCase() == 'set-cookie') {
                if (v is List) {
                  for (final val in v) {
                    final parts = val.toString().split(';');
                    for (final part in parts) {
                      final kv = part.trim().split('=');
                      if (kv.length >= 2 &&
                          (kv[0] == 'auth' ||
                              kv[0] == 'user_id' ||
                              kv[0] == 'username')) {
                        _cookies[kv[0]] = kv[1];
                      }
                    }
                  }
                }
              }
            });
          }
          finalBody = parsed['body'].toString();
        }
      }
    } catch (_) {
      // If parsing fails, just use the raw string
    }

    if (_pendingViews.containsKey(reqId)) {
      _pendingViews[reqId]!.complete(finalBody);
      _pendingViews.remove(reqId);
    } else {
      print("Warning: Received view response for unknown ID: $reqId");
    }
  }

  // Convert stored cookies to a header map
  static Map<String, String> _getCookieHeaders() {
    final Map<String, String> headers = {'X-Requested-With': 'Flutter'};
    if (_cookies.isNotEmpty) {
      final cookieStr =
          _cookies.entries.map((e) => '${e.key}=${e.value}').join('; ');
      headers['Cookie'] = cookieStr;
    }
    return headers;
  }

  // Request a view asynchronously
  static Future<String> requestView(String route) {
    if (_viewCallbackListener == null) {
      return Future.value("{\"error\": \"Bridge not initialized properly\"}");
    }

    final reqId = _nextViewReqId++;
    final completer = Completer<String>();
    _pendingViews[reqId] = completer;

    final routePtr = route.toNativeUtf8();
    final headers = _getCookieHeaders();
    final headersJson = jsonEncode(headers);
    final headersPtr = headersJson.toNativeUtf8();

    _requestViewAsync(reqId, routePtr, headersPtr);
    malloc.free(routePtr); // C++ makes a copy (std::string)
    malloc.free(headersPtr);

    return completer.future;
  }

  // Submit an action asynchronously (reusing view callback pattern)
  static Future<String> submitAction(String route, String jsonData) {
    if (_viewCallbackListener == null) {
      return Future.value("{\"error\": \"Bridge not initialized properly\"}");
    }

    final reqId = _nextViewReqId++;
    final completer = Completer<String>();
    _pendingViews[reqId] = completer;

    final routePtr = route.toNativeUtf8();
    final jsonPtr = jsonData.toNativeUtf8();
    final headers = _getCookieHeaders();
    final headersJson = jsonEncode(headers);
    final headersPtr = headersJson.toNativeUtf8();

    _submitActionAsync(reqId, routePtr, jsonPtr, headersPtr);

    malloc.free(routePtr);
    malloc.free(jsonPtr);
    malloc.free(headersPtr);

    return completer.future;
  }
}
