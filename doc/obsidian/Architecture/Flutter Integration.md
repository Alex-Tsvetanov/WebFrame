---
title: Flutter Integration
tags:
  - coroute
  - flutter
  - ffi
  - integration
aliases:
  - Flutter Adapter
  - Coroute Flutter Integration
---

# Flutter Integration

> [!abstract]
> This note documents how Coroute is embedded into Flutter through the reusable `coroute_framework` package. It covers build-time packaging, the exported native bridge, the Dart-side `Bridge`, and the route-driven view rendering contract implemented by `GenericView` and `ScreenRegistry`.

## The package boundary

The authoritative Flutter integration surface is the package:

- `[[packages/Flutter/coroute_framework/pubspec.yaml]]`

Package-level intent is documented in:

- `[[packages/Flutter/TECHNICAL_SPEC.md]]`

The example consumer app is:

- `[[examples/FlutterProject/main.dart]]`

## Build-time flow

The Flutter integration begins in CMake.

`[[cmake/CorouteApp.cmake]]`:

- builds the application logic as a shared library
- injects `[[src/bridge/bridge.cpp]]`
- renames the application entry point to `app_main`
- writes `.coroute_lib_path.<target>` into the Flutter package directory
- creates Flutter project scaffolding and run targets for mobile/desktop

That handoff is consumed by the package build hook:

- `[[packages/Flutter/coroute_framework/hook/build.dart]]`

The build hook then:

- finds the native library path from `.coroute_lib_path.*`
- copies the dynamic library into Flutter's native-assets output
- copies loader-path dependencies on Apple platforms
- copies sibling DLLs on Windows
- patches Android's manifest to ensure Internet permission when needed
- registers the bundled asset as `src/coroute_app`

## Native bridge surface

The exported bridge lives in:

- `[[src/bridge/bridge.cpp]]`

Its responsibilities include:

- registering the Dart fetch callback
- registering the Dart view-response callback
- registering the Dart broadcast callback
- starting the app via `init_app()`
- answering `request_view_async(...)`
- answering `submit_action_async(...)`
- completing suspended fetch requests
- exposing the configured API base URL

The bridge is intentionally flat and C-compatible.

## Dart-side bridge

The Dart runtime surface lives in:

- `[[packages/Flutter/coroute_framework/lib/bridge.dart]]`

Important points:

- it uses compile-time `@Native` bindings rather than runtime symbol lookup
- it registers three native callback channels:
  - fetch callback
  - view callback
  - broadcast callback
- it exposes:
  - `Bridge.initialize()`
  - `Bridge.initApp()`
  - `Bridge.requestView(...)`
  - `Bridge.submitAction(...)`
  - `Bridge.broadcastStream`
  - `Bridge.wsBaseUrl`

## View rendering flow

The standard Flutter host widget is:

- `[[packages/Flutter/coroute_framework/lib/generic_view.dart]]`

The standard screen mapping registry is:

- `[[packages/Flutter/coroute_framework/lib/screen_registry.dart]]`

The flow is:

1. Flutter starts.
2. The app calls `Bridge.initialize()`.
3. The app calls `Bridge.initApp()`.
4. The app registers screen factories in `ScreenRegistry`.
5. `GenericView(route: ...)` requests a Coroute route.
6. Coroute returns JSON containing `templates` and `model`.
7. `GenericView` reads `templates.mobile`.
8. `ScreenRegistry` maps that identifier to a widget factory.
9. Flutter renders the native widget.

## Action and fetch flow

Flutter integration is not only about rendering.

### View and action completion

Both `requestView(...)` and `submitAction(...)` use request IDs and completers so asynchronous C++ work can resolve back into Dart futures.

### C++-initiated fetch delegation

When Coroute needs an outbound HTTP request through the Flutter-hosted path:

- C++ suspends a coroutine through `CallbackFetchTransport`
- Dart receives the fetch callback
- Dart performs the HTTP request
- Dart calls back into C++ with the result
- the suspended coroutine resumes

Relevant files:

- `[[include/coroute/core/callback_fetch_transport.hpp]]`
- `[[src/bridge/bridge.cpp]]`
- `[[packages/Flutter/coroute_framework/lib/bridge.dart]]`

## Push updates and refresh model

Flutter supports two push/update channels.

### In-process broadcast channel

Coroute can push events to Flutter through the bridge callback and `Bridge.broadcastStream`.

### Cross-process WebSocket channel

`GenericView` can also connect to `Bridge.wsBaseUrl + /ws` for remote-server update scenarios.

Relevant files:

- `[[packages/Flutter/coroute_framework/lib/generic_view.dart]]`
- `[[examples/FlutterProject/src/handlers/websocket/task_hub.cpp]]`
- `[[examples/Project/src/handlers/websocket/task_hub.cpp]]`

## Example usage sites

- `[[examples/FlutterProject/main.dart]]`
- `[[examples/FlutterProject/src/handlers/pages.cpp]]`
- `[[examples/FlutterProject/pubspec.yaml]]`

## Status

### Current status

- **Implemented**: shared-library handoff from CMake into Flutter native assets.
- **Implemented**: `@Native`-based Dart bridge with request, action, fetch, and broadcast channels.
- **Implemented**: route-driven Flutter rendering through `GenericView` and `ScreenRegistry`.
- **Implemented**: remote update assist through WebSocket in the Flutter host widget.
- **Documentation anchor**: package responsibilities are already described in `[[packages/Flutter/TECHNICAL_SPEC.md]]`.

## Related notes

- [[Architecture/Project Atlas]]
- [[Architecture/Framework Integration Architecture]]
- [[Architecture/View and API Abstractions]]
- [[Renderers/Flutter Renderer Contract]]
- [[Protocols/WebSocket]]
