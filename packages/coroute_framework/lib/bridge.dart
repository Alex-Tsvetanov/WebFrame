import 'dart:async';
import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';
import 'package:http/http.dart' as http;

// FFI signature for init_app
typedef InitAppC = Void Function();
typedef InitAppDart = void Function();

// FFI signature for request_view_async
typedef RequestViewAsyncC = Void Function(Int32 reqId, Pointer<Utf8> route);
typedef RequestViewAsyncDart = void Function(int reqId, Pointer<Utf8> route);

// FFI signature for submit_action
// FFI signature for submit_action_async
typedef SubmitActionAsyncC = Void Function(Int32 reqId, Pointer<Utf8> route, Pointer<Utf8> json);
typedef SubmitActionAsyncDart = void Function(int reqId, Pointer<Utf8> route, Pointer<Utf8> json);

// FFI signature for fetch callback
typedef FetchRequestCallbackC = Void Function(Int32 reqId, Pointer<Utf8> url, Pointer<Utf8> method, Pointer<Utf8> headers, Pointer<Utf8> body);

// FFI signature for register_fetch_callback
typedef RegisterFetchCallbackC = Void Function(Pointer<NativeFunction<FetchRequestCallbackC>> callback);
typedef RegisterFetchCallbackDart = void Function(Pointer<NativeFunction<FetchRequestCallbackC>> callback);

// FFI signature for complete_fetch_request
typedef CompleteFetchRequestC = Void Function(Int32 reqId, Int32 status, Pointer<Utf8> body);
typedef CompleteFetchRequestDart = void Function(int reqId, int status, Pointer<Utf8> body);

// FFI signature for view callback
typedef ViewResponseCallbackC = Void Function(Int32 reqId, Pointer<Utf8> json);

// FFI signature for register_view_callback
typedef RegisterViewCallbackC = Void Function(Pointer<NativeFunction<ViewResponseCallbackC>> callback);
typedef RegisterViewCallbackDart = void Function(Pointer<NativeFunction<ViewResponseCallbackC>> callback);


class Bridge {
  static late DynamicLibrary _lib;
  static late InitAppDart _initApp;
  static late RequestViewAsyncDart _requestViewAsync;
  static late SubmitActionAsyncDart _submitActionAsync;
  static late RegisterFetchCallbackDart _registerFetchCallback;
  static late CompleteFetchRequestDart _completeFetchRequest;
  static late RegisterViewCallbackDart _registerViewCallback;
  
  // Keep listeners alive
  static NativeCallable<FetchRequestCallbackC>? _callbackListener;
  static NativeCallable<ViewResponseCallbackC>? _viewCallbackListener;

  // Pending view requests
  static final Map<int, Completer<String>> _pendingViews = {};
  static int _nextViewReqId = 1;

  // Simple cookie jar (Map<Name, Value>)
  static final Map<String, String> _cookies = {};

  static void initialize() {
    // Load the library based on platform
    if (Platform.isAndroid) {
      _lib = DynamicLibrary.open('libcoroute_app.so');
    } else if (Platform.isIOS) {
      _lib = DynamicLibrary.process(); // Statically linked on iOS
    } else if (Platform.isMacOS) {
      // Try absolute path for local development (bypasses SIP/DYLD issues)
      try {
        _lib = DynamicLibrary.open('/Users/Alex.Tsvetanov/GitHub/CoroutinesInWebServers/examples/view_example/.flutter/libcoroute_app.dylib');
      } catch (e00) {
        try {
            _lib = DynamicLibrary.open('/Users/Alex.Tsvetanov/GitHub/CoroutinesInWebServers/examples/view_example/libcoroute_app.dylib');
        } catch (e0) {
            try {
                _lib = DynamicLibrary.open('/Users/Alex.Tsvetanov/GitHub/CoroutinesInWebServers/build_mobile/libcoroute_app.dylib');
            } catch (e) {
                try {
                   _lib = DynamicLibrary.open('/Users/Alex.Tsvetanov/GitHub/CoroutinesInWebServers/build_mobile/examples/view_example/flutter_app/libcoroute_app.dylib');
                } catch (e2) {
                    print("Failed to load absolute path: $e00, $e0, $e, $e2");
                    // Fallback to strict name (for release/bundle)
                    _lib = DynamicLibrary.open('libcoroute_app.dylib');
                }
            }
        }
      }
    } else if (Platform.isWindows) {
      _lib = DynamicLibrary.open('coroute_app.dll');
    } else {
      _lib = DynamicLibrary.open('./libcoroute_app.so');
    }

    // Lookup functions
    _initApp = _lib.lookupFunction<InitAppC, InitAppDart>('init_app');
    _requestViewAsync = _lib.lookupFunction<RequestViewAsyncC, RequestViewAsyncDart>('request_view_async');
    _submitActionAsync = _lib.lookupFunction<SubmitActionAsyncC, SubmitActionAsyncDart>('submit_action_async');
    
    // Optional functions (fetch bridge)
    try {
        _registerFetchCallback = _lib.lookupFunction<RegisterFetchCallbackC, RegisterFetchCallbackDart>('register_fetch_callback');
        _completeFetchRequest = _lib.lookupFunction<CompleteFetchRequestC, CompleteFetchRequestDart>('complete_fetch_request');
        
        // Setup fetch listener
        _callbackListener = NativeCallable<FetchRequestCallbackC>.listener(_handleFetch);
        _registerFetchCallback(_callbackListener!.nativeFunction);
        print("Registered C++ Fetch Callback");
    } catch (e) {
        print("Fetch callback FFI not available: $e");
    }

    // Optional functions (view bridge async)
    try {
        _registerViewCallback = _lib.lookupFunction<RegisterViewCallbackC, RegisterViewCallbackDart>('register_view_callback');
        
        // Setup view listener (must be on main thread for Completer?)
        // NativeCallable.listener invokes on the thread that created it (Main Isolate).
        _viewCallbackListener = NativeCallable<ViewResponseCallbackC>.listener(_handleViewResponse);
        _registerViewCallback(_viewCallbackListener!.nativeFunction);
        print("Registered C++ View Callback");
    } catch (e) {
        print("View callback FFI not available: $e");
    }
  }

  // Initialize the app
  static void initApp() {
    print("Initializing C++ app");
    _initApp();
  }

  // Handle fetch request from C++
  static void _handleFetch(int reqId, Pointer<Utf8> urlPtr, Pointer<Utf8> methodPtr, Pointer<Utf8> headersPtr, Pointer<Utf8> bodyPtr) async {
    final url = urlPtr.toDartString();
    final method = methodPtr.toDartString();
    final body = bodyPtr.toDartString(); // Only if not empty? Assume C++ sends ""
    
    print("[Dart Fetch] $method $url");
    
    var targetUrl = url;
    if (url.startsWith("/")) {
        final host = Platform.isAndroid ? "10.0.2.2:8080" : "127.0.0.1:8080";
        targetUrl = "http://$host$url";
    }
    
    try {
        final uri = Uri.parse(targetUrl);
        http.Response response;
        
        // Prepare headers with cookies
        final Map<String, String> headers = {
            "Content-Type": "application/json"
        };
        
        if (_cookies.isNotEmpty) {
            final cookieHeader = _cookies.entries.map((e) => "${e.key}=${e.value}").join("; ");
            headers["Cookie"] = cookieHeader;
        }

        if (method == "GET") {
            response = await http.get(uri, headers: headers);
        } else if (method == "POST") {
             response = await http.post(uri, body: body, headers: headers);
        } else {
             response = http.Response("Method not supported", 405);
        }
        
        // Debug headers
        // print("[Dart Fetch] Response Headers: ${response.headers}");

        final bodyNative = response.body.toNativeUtf8();

        // Parse Set-Cookie (Simple implementation for 'auth' token)
        final setCookie = response.headers['set-cookie'];
        if (setCookie != null) {
             // Extract "auth=..."
             final parts = setCookie.split(';');
             for (var part in parts) {
                 final kv = part.trim().split('=');
                 if (kv.length >= 2 && kv[0] == "auth") {
                     _cookies["auth"] = kv[1];
                 }
             }
        }
        _completeFetchRequest(reqId, response.statusCode, bodyNative);
        malloc.free(bodyNative);
        
    } catch (e) {
        print("[Dart Fetch] Error: $e");
        final errorNative = "{\"error\": \"$e\"}".toNativeUtf8();
        _completeFetchRequest(reqId, 500, errorNative);
        malloc.free(errorNative);
    }
  }

  // Handle view response from C++
  static void _handleViewResponse(int reqId, Pointer<Utf8> jsonPtr) {
    final jsonStr = jsonPtr.toDartString();
    malloc.free(jsonPtr); // Free the memory allocated by C++ (strdup)

    if (_pendingViews.containsKey(reqId)) {
        _pendingViews[reqId]!.complete(jsonStr);
        _pendingViews.remove(reqId);
    } else {
        print("Warning: Received view response for unknown ID: $reqId");
    }
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
    _requestViewAsync(reqId, routePtr);
    malloc.free(routePtr); // C++ makes a copy (std::string)
    
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
    
    // We reuse the _submitAction FFI signature but map it to our new async C function
    // Wait, we need to update the typedefs if C signature changed.
    // Cpp change: void submit_action_async(int32_t req_id, const char* route, const char* json_data)
    
    _submitActionAsync(reqId, routePtr, jsonPtr);
    
    malloc.free(routePtr);
    malloc.free(jsonPtr);
    
    return completer.future;
  }
}
