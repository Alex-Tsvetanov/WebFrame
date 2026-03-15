# Flutter Package Technical Specification

## Purpose

`packages/Flutter` is the Flutter packaging area for Coroute v2.

Its job is to contain the reusable Flutter integration layer that allows a Flutter application to consume the same Coroute application logic that also runs in web-server mode. In practice, this means the Flutter package must make it possible for a Flutter app to:

- initialize the native Coroute engine
- request Coroute view routes through the C++/Dart FFI boundary
- render Flutter widgets selected by Coroute view handlers
- submit actions back to Coroute
- participate in refresh and live-update flows driven by the backend

This directory is not meant to contain a specific app's business screens or app-local domain logic. It is meant to contain the reusable package contract and its supporting documentation.

## Scope

This document serves two audiences:

- Flutter app integrators who need to know how to use the package
- Framework maintainers who need to know what the package is required to contain and which architectural boundaries it must preserve

## Authoritative Package

The authoritative Flutter package currently lives at:

```text
packages/Flutter/coroute_framework
```

This package is the adapter between Flutter and the native Coroute runtime.

## Intended Directory Responsibilities

The `packages/Flutter` directory is supposed to contain:

- package-level documentation for Flutter integration
- one or more reusable Flutter packages related to Coroute
- no app-specific generated Flutter project scaffolding as authoritative source

The `coroute_framework` package is supposed to contain only reusable framework integration code.

Transient files such as `.dart_tool`, lockfiles, generated plugin metadata, and build outputs may appear during local development, but they are not part of the architectural contract of the package itself.

## Required Contents of `coroute_framework`

### Package manifest

`pubspec.yaml` must define:

- package identity
- supported Dart and Flutter SDK constraints
- dependencies needed for Flutter integration
- native asset build participation

At the time of writing, the package depends on Flutter, `ffi`, HTTP support, and native asset tooling. That is consistent with its purpose as an FFI-backed runtime bridge.

### Public library entrypoint

`lib/coroute_framework.dart` is supposed to be the public package entrypoint.

It should re-export the supported consumer-facing API, currently:

- `Bridge`
- `GenericView`
- `ScreenRegistry`

App code is meant to import `package:coroute_framework/coroute_framework.dart` rather than deep-import package internals unless there is a deliberate reason to bypass the public surface.

### Native bridge

`lib/bridge.dart` is supposed to provide the Dart-side bridge to the native `coroute_app` library.

This file is responsible for:

- declaring compile-time native bindings via `@Native`
- registering callback entrypoints used by the C++ runtime
- requesting views from the native engine
- submitting actions to the native engine
- servicing C++-initiated fetches by performing HTTP from Dart
- receiving native broadcast events and exposing them as a Dart stream
- maintaining request correlation for async callbacks
- carrying transient request/session state needed for integration, such as in-memory cookie propagation

This file is the package's core runtime integration surface.

### Generic view host

`lib/generic_view.dart` is supposed to provide a reusable widget that can render a Coroute view route inside a Flutter application.

Its responsibilities are:

- requesting a view for a route through `Bridge.requestView()`
- decoding the returned JSON payload
- selecting the mobile template name from the Coroute response
- asking `ScreenRegistry` to build the corresponding Flutter widget
- handling loading, error, and retry states
- reacting to backend broadcast events by refreshing the current view
- optionally connecting to a server WebSocket when the app is operating against a remote server process

`GenericView` is intended to be the default host widget for route-driven, backend-selected Flutter screens.

### Screen registry

`lib/screen_registry.dart` is supposed to provide a simple registry from Coroute template names to Flutter widget factories.

Its responsibilities are:

- registering widget factories by logical template name
- building widgets from decoded model payloads
- providing the bridge between backend-selected template identifiers and concrete Flutter UI classes

This registry is intentionally small. It is not a navigation framework, a DI container, or a state-management solution.

### Native asset build hook

`hook/build.dart` is supposed to make the native `coroute_app` library available to Flutter's native asset pipeline.

Its responsibilities are:

- locating the native library built by the CMake side of the system
- copying the required dynamic library into the Flutter/native-assets output location
- copying platform-specific dependent libraries when required
- registering the bundled native asset under the asset name expected by the `@Native` declarations
- applying platform-specific packaging fixes that are part of the package contract

This build hook is part of the package's required functionality, not an optional convenience.

## Public API Contract

The package is meant to expose three primary consumer-level concepts.

### `Bridge`

`Bridge` is the application's runtime gateway to the native Coroute engine.

It is intended to support this usage pattern:

- `Bridge.initialize()` during startup to register callbacks
- `Bridge.initApp()` after initialization to initialize the native app
- `Bridge.requestView(route)` to ask Coroute for a view payload
- `Bridge.submitAction(route, jsonData)` to send an action payload back to Coroute
- `Bridge.broadcastStream` to observe backend-driven events
- `Bridge.wsBaseUrl` for remote WebSocket connectivity when relevant

`Bridge` owns the FFI boundary. Application code should not define its own parallel FFI integration for the same responsibilities.

### `GenericView`

`GenericView` is meant to be the standard consumer widget for rendering Coroute-driven views in Flutter.

Usage contract:

- the app passes a Coroute route such as `/login` or `/dashboard`
- `GenericView` requests the route from the backend
- Coroute returns a JSON payload containing templates and model data
- the mobile template identifier is resolved through `ScreenRegistry`
- the registered Flutter screen renders the model

### `ScreenRegistry`

`ScreenRegistry` is meant to be populated by the application at startup.

Usage contract:

- register each Flutter widget with the logical mobile template name emitted by the backend
- provide a factory with the signature `Widget Function(Map<String, dynamic>)`
- keep template names stable across the C++ and Flutter boundary

If the backend emits `DashboardScreen`, the Flutter app must register a factory for `DashboardScreen`.

## Intended Runtime Architecture

The Flutter package participates in the Coroute v2 architecture as the Flutter-specific presentation adapter.

The intended flow is:

1. Flutter app starts.
2. The app initializes `Bridge` and the native Coroute app.
3. The app registers Flutter screen factories in `ScreenRegistry`.
4. A `GenericView` asks Coroute for a route.
5. The native Coroute runtime resolves that route using the same view/business logic used by web mode.
6. Coroute returns JSON containing:
   - platform-specific template identifiers
   - serialized model data
7. `GenericView` selects the mobile template.
8. `ScreenRegistry` maps that template to a Flutter widget.
9. The widget renders the supplied model.

This allows the business/view-model layer to remain in C++ while Flutter remains the native mobile or desktop presentation layer.

## Relation to the Coroute v2 Cross-Platform Model

This package exists to support the Coroute v2 design where the same backend application can serve:

- HTML in web-server mode
- JSON view payloads for Flutter in app mode

The architectural intent is:

- route selection and business logic stay in Coroute
- platform-specific rendering stays in the target UI layer
- view handlers choose platform templates through `ViewTemplates`
- Flutter consumes the `mobile` template identifier
- the web renderer consumes the `web` template identifier

The package must preserve that separation. It should not move business logic from Coroute into Flutter just to make the integration work.

## Required Response Shape

For `GenericView` and `ScreenRegistry` to work correctly, the Flutter-side contract expects a JSON payload with the conceptual shape:

```json
{
  "templates": {
    "web": "index.html",
    "mobile": "DashboardScreen",
    "desktop": "DashboardScreen"
  },
  "model": {
  }
}
```

The exact model fields are application-specific.

The important package-level contract is:

- `templates.mobile` identifies the Flutter widget to build
- `model` contains the serialized view model consumed by that widget

If the native side wraps responses in an FFI transport envelope, `Bridge` is responsible for unwrapping that transport detail before the result reaches consumer widgets.

## Fetch and Action Semantics

The package is meant to support two complementary flows.

### View retrieval

`Bridge.requestView(route)` requests a view route from Coroute and resolves asynchronously with the resulting JSON payload.

### Action submission

`Bridge.submitAction(route, jsonData)` submits application actions back to Coroute and resolves asynchronously with the response payload.

These methods are meant to provide a high-level app API over the lower-level FFI callback mechanism.

## Transport Responsibilities

The package's transport behavior is intentional.

### Embedded FFI mode

When Flutter and the Coroute runtime are in the same process, the package uses FFI callbacks for:

- view completion
- backend broadcast delivery
- C++-initiated fetch delegation into Dart HTTP

In this mode, the package acts as the in-process transport adapter.

### Remote server assist mode

When a remote server process is involved, `GenericView` may also connect to a WebSocket endpoint derived from the configured API base URL.

This supports cross-process backend push events in scenarios where a pure in-process broadcast is not sufficient.

The package therefore supports both:

- same-process native integration
- remote-backed refresh/update integration

## Build and Packaging Contract

The package relies on a coordinated contract with the CMake side of the repository.

### Native asset identity

The Dart code expects the bundled asset identifier:

```text
package:coroute_framework/src/coroute_app
```

The native asset build hook must register the native library under this asset name so that the compile-time `@Native` bindings resolve correctly.

### CMake handoff

`CorouteApp.cmake` writes per-target configuration files named like:

```text
.coroute_lib_path.<target>
```

inside `packages/Flutter/coroute_framework`.

These files are the handoff point from the native build system to the Flutter package build hook. Their primary purpose is to tell the package where the built native library lives so the hook can bundle it for Flutter.

### Platform packaging duties

The build hook is expected to handle platform-specific packaging requirements that are necessary for the package to work across supported targets. Current examples include:

- copying loader-path dependencies on Apple platforms
- copying sibling DLLs on Windows
- ensuring Android networking permission support when needed by the integration contract

These responsibilities belong in build automation, not in manual developer steps.

## Expected Consumer Usage

A Flutter application is meant to use the package in this order.

### 1. Add the dependency

A consuming app adds `coroute_framework` as a dependency, commonly as a path dependency within this monorepo.

### 2. Initialize the bridge at startup

On startup, the app should:

- call `Bridge.initialize()`
- call `Bridge.initApp()`

If the current platform or execution mode does not expose the native bridge, the app may choose to handle initialization failure gracefully, but the primary intended mode is native bridge availability.

### 3. Register screens

The app must register Flutter widgets whose names match backend-emitted mobile templates.

Examples of expected registrations:

- `LoginScreen`
- `DashboardScreen`
- `TaskDetailScreen`

### 4. Render through `GenericView`

The app should use `GenericView(route: ...)` as the host for Coroute-managed routes.

### 5. Keep screen factories dumb about transport

Registered screen widgets should focus on rendering model data and triggering actions. They should not reimplement the package's FFI, template dispatch, or transport wiring.

## Maintainer Boundaries and Non-Goals

The package is not supposed to become:

- a second business-logic layer separate from Coroute
- a Flutter routing framework
- a state-management framework
- a long-term credential store
- a replacement for the native Coroute app runtime
- a place for app-specific screen implementations that belong in consuming apps

A maintainer should preserve the package as a thin but complete integration layer.

## Stability Expectations

The following should be treated as stable package concepts:

- `Bridge` as the consumer gateway to native runtime functions
- `GenericView` as the backend-driven route host
- `ScreenRegistry` as the mapping from template name to widget factory
- the expectation that Coroute returns template metadata plus serialized model data
- the native asset name used by the `@Native` bindings

Internal implementation details may evolve, but these concepts define the intended package identity.

## Current Example of Intended Use

A canonical app startup sequence looks like this:

1. Import `package:coroute_framework/coroute_framework.dart`.
2. Call `Bridge.initialize()`.
3. Call `Bridge.initApp()`.
4. Register all Flutter screens in `ScreenRegistry`.
5. Start the app with `GenericView(route: '/login')` or another Coroute route.

That is the expected integration model for consumers of this package.

## Summary

`packages/Flutter/coroute_framework` is supposed to be the reusable Flutter adapter for Coroute v2.

It must contain:

- a package manifest
- a public package entrypoint
- a Dart/native bridge
- a generic route-driven view host
- a template-to-widget registry
- a native asset build hook

It is meant to be used by initializing the native bridge, registering Flutter screens that correspond to backend-selected mobile templates, and rendering Coroute routes through `GenericView`.

Its core architectural role is to let Flutter act as the platform-native renderer for the same Coroute application logic that also powers the web-server path.
