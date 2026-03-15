---
title: View and API Abstractions
tags:
  - coroute
  - architecture
  - views
  - api
aliases:
  - View Contract
  - ViewModel Contract
---

# View and API Abstractions

> [!abstract]
> This note explains the shared application contract that lets Coroute serve both browser HTML and native app views from the same C++ handlers. The key building blocks are `App::view`, `ViewTemplates`, `ViewResult`, `ViewResultAny`, and the distinction between `fetch()` and `dispatch()`.

## Why this layer exists

A normal server route returns `Response`. A Coroute view route returns a typed `ViewResult<VM>` where:

- `templates` names the renderer target for each platform
- `model` contains the platform-agnostic view model

That lets one handler describe multiple presenters without cloning the business logic.

## Core types

### `ViewTemplates`

`ViewTemplates` is the cross-platform identifier bundle.

- `web` points to the HTML template name used by the web renderer.
- `mobile` points to the mobile presenter identifier, currently a Flutter screen name.
- `desktop` points to the desktop presenter identifier.

Relevant files:

- `[[include/coroute/view/view_types.hpp]]`
- `[[tests/test_view.cpp]]`
- `[[examples/view_example/main.cpp]]`
- `[[examples/FlutterProject/src/handlers/pages.cpp]]`

### `ViewResult<VM>`

`ViewResult<VM>` keeps the typed view model together with its template identifiers.

Relevant files:

- `[[include/coroute/view/view_types.hpp]]`
- `[[tests/test_view.cpp]]`

### `ViewResultAny`

`ViewResultAny` is the type-erased storage form used by the router and view-execution path.

Important behavior:

- stores the typed model in `std::any`
- stores a conversion lambda that turns the model into `nlohmann::json`
- preserves the `ViewTemplates` metadata

This is the mechanism that allows a typed handler to be stored behind a uniform runtime interface.

Relevant files:

- `[[include/coroute/view/view_types.hpp]]`
- `[[tests/test_view.cpp]]`

## Route registration

`App::view(...)` registers a GET-only view route and wraps a typed handler into a uniform `Task<ViewResultAny>` callable.

The public overloads support:

- a handler that only takes `Request&`
- a handler that takes `Request&` plus `ViewExecutionContext&`
- a handler with per-route view middleware

Relevant files:

- `[[include/coroute/core/app.hpp]]`
- `[[include/coroute/view/view_middleware.hpp]]`
- `[[tests/test_view.cpp]]`

## View execution path

A view route can resolve in two different shapes depending on who is asking.

### Browser/web path

When a normal browser request hits a view route in server mode:

- the router matches the view handler
- the handler returns `ViewResultAny`
- the model is converted to JSON
- the `web` template is rendered through Inja
- the client receives HTML

Relevant files:

- `[[src/core/app.cpp]]`
- `[[include/coroute/core/app.hpp]]`
- `[[src/view/web_renderer.cpp]]`
- `[[examples/view_example/main.cpp]]`

### Flutter/native app path

When the request comes from Flutter, Coroute looks for `X-Requested-With: Flutter` and returns the JSON view envelope instead of HTML.

That JSON envelope contains:

- `templates`
- `model`

Relevant files:

- `[[src/core/app.cpp]]`
- `[[include/coroute/core/app.hpp]]`
- `[[src/bridge/bridge.cpp]]`
- `[[packages/Flutter/coroute_framework/lib/generic_view.dart]]`

## `fetch()` vs `dispatch()`

This distinction is central to the architecture.

### `fetch()`

`fetch()` is the higher-level request path.

Behavior:

- constructs an internal request
- applies `AuthState`
- if a `FetchTransport` exists, routes the request through that transport
- otherwise performs in-process routing through Coroute
- observes the response back into `AuthState`

This is the general mechanism that allows a view handler or service to ask for another route without manually constructing transport logic.

Relevant files:

- `[[include/coroute/core/app.hpp]]`
- `[[include/coroute/core/fetch_transport.hpp]]`
- `[[include/coroute/core/callback_fetch_transport.hpp]]`

### `dispatch()`

`dispatch()` forces local in-process route execution.

Why it matters:

- in Flutter client mode, `fetch_transport_` may point to the Dart HTTP proxy
- some bridge-driven operations must stay local and must not go through that remote transport path
- `dispatch()` exists specifically to preserve the correct execution boundary in those cases

Relevant files:

- `[[include/coroute/core/app.hpp]]`
- `[[src/bridge/bridge.cpp]]`

> [!warning]
> The difference between `fetch()` and `dispatch()` is part of the integration boundary, not just a convenience overload. The Flutter bridge relies on it to avoid sending local view/action resolution through the wrong transport path.

## Router relationship

The view system uses a dedicated view matcher alongside the ordinary HTTP method router.

Observed behavior from tests and API surface:

- view routes are stored independently from regular routes
- view routes support parameter extraction
- view routes remain GET-oriented even when the API router handles multiple HTTP verbs

Relevant files:

- `[[include/coroute/core/router.hpp]]`
- `[[src/core/router.cpp]]`
- `[[tests/test_view.cpp]]`

## Example usage sites

- `[[examples/view_example/main.cpp]]`
- `[[examples/FlutterProject/src/handlers/pages.cpp]]`
- `[[examples/Project/src/main.cpp]]`

## Status

### Current status

- **Implemented**: view route registration, template selection, type erasure, JSON serialization, and distinct browser-vs-Flutter response shapes.
- **Test-backed**: `ViewTemplates`, `ViewResult`, `ViewResultAny`, and view-route matching are exercised in `[[tests/test_view.cpp]]`.
- **Architecturally important**: the `fetch()` vs `dispatch()` split is visible in the current `App` API and is essential for the Flutter path.

## Related notes

- [[Architecture/Project Atlas]]
- [[Architecture/Framework Integration Architecture]]
- [[Architecture/Flutter Integration]]
- [[Renderers/Renderers and Template Engines]]
- [[Renderers/Inja Web Renderer]]
- [[Renderers/Flutter Renderer Contract]]
