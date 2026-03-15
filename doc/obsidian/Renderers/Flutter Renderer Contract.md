---
title: Flutter Renderer Contract
tags:
  - coroute
  - flutter
  - renderer
  - templates
aliases:
  - Flutter Screen Contract
  - Mobile Renderer Contract
---

# Flutter Renderer Contract

> [!abstract]
> Flutter is a renderer target for Coroute, but it does not render HTML templates. Instead, Coroute emits a JSON envelope containing `templates` and `model`, and Flutter interprets `templates.mobile` as a logical screen identifier resolved through `ScreenRegistry`.

## What Flutter consumes

The Flutter rendering contract expects a payload conceptually shaped like:

- `templates.web`
- `templates.mobile`
- `templates.desktop`
- `model`

From the Flutter side, the key field is:

- `templates.mobile`

That value is expected to match a registered Flutter screen factory.

Relevant files:

- `[[include/coroute/view/view_types.hpp]]`
- `[[packages/Flutter/TECHNICAL_SPEC.md]]`
- `[[packages/Flutter/coroute_framework/lib/generic_view.dart]]`
- `[[packages/Flutter/coroute_framework/lib/screen_registry.dart]]`

## Runtime components

### `Bridge`

`Bridge` owns the FFI-facing runtime gateway.

Responsibilities:

- initialize native callbacks
- start the native app
- request views
- submit actions
- surface broadcast events
- support remote WebSocket base URL derivation

Relevant files:

- `[[packages/Flutter/coroute_framework/lib/bridge.dart]]`
- `[[src/bridge/bridge.cpp]]`

### `GenericView`

`GenericView` is the standard Flutter host widget for route-driven views.

Observed responsibilities:

- request a route through `Bridge.requestView(...)`
- decode JSON into `templates` and `model`
- read `templates.mobile`
- ask `ScreenRegistry` to build the widget
- handle loading, error, retry, and refresh behavior
- listen to both bridge broadcasts and optional server WebSocket updates

Relevant files:

- `[[packages/Flutter/coroute_framework/lib/generic_view.dart]]`

### `ScreenRegistry`

`ScreenRegistry` is the mapping layer from backend-selected logical screen names to concrete Flutter widget factories.

Relevant files:

- `[[packages/Flutter/coroute_framework/lib/screen_registry.dart]]`

## How Coroute produces the Flutter payload

The C++ side can return the Flutter-oriented JSON view envelope through two closely related paths:

- the server path when the request includes `X-Requested-With: Flutter`
- the bridge/local dispatch path for embedded app mode

In both cases the logical idea is the same:

- execute the view handler
- serialize the model through `ViewResultAny`
- return `{ templates, model }` instead of rendered HTML

Relevant files:

- `[[src/core/app.cpp]]`
- `[[include/coroute/core/app.hpp]]`
- `[[src/bridge/bridge.cpp]]`

## Example template mappings

Observed examples in the repository include:

- `LoginScreen`
- `DashboardScreen`
- `TaskDetailScreen`
- `UserScreen`
- `ListingScreen`

Relevant files:

- `[[examples/FlutterProject/src/handlers/pages.cpp]]`
- `[[examples/view_example/main.cpp]]`
- `[[examples/FlutterProject/main.dart]]`

## What this note calls a renderer

In the Flutter path, the term "renderer" means:

- the C++ engine selects a presentation identifier
- Flutter maps that identifier to a widget
- the widget renders the supplied model in native UI

This differs from Inja, where the renderer is an HTML template engine.

## Status

### Current status

- **Implemented**: Coroute emits `templates` plus serialized `model` for Flutter-facing view requests.
- **Implemented**: the Flutter package resolves the mobile template identifier through `ScreenRegistry`.
- **Implemented**: `GenericView` acts as the default route-driven host widget.
- **Documentation-backed**: package-level contract is explicitly described in `[[packages/Flutter/TECHNICAL_SPEC.md]]`.
- **Test note**: I did not find a dedicated Dart test suite for `coroute_framework` during this pass.

## Related notes

- [[Renderers/Renderers and Template Engines]]
- [[Architecture/Flutter Integration]]
- [[Architecture/View and API Abstractions]]
- [[Renderers/Inja Web Renderer]]
