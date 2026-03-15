---
title: Inja Web Renderer
tags:
  - coroute
  - inja
  - web
  - renderer
aliases:
  - Web HTML Renderer
  - Inja Renderer
---

# Inja Web Renderer

> [!abstract]
> The current web renderer in Coroute is based on Inja. It interprets `ViewTemplates.web` as an HTML template identifier, converts the view model to JSON, and returns `Response::html(...)` to the browser.

## Main responsibilities

The Inja-based web renderer is responsible for:

- locating templates in the configured template directory
- validating template existence at startup when requested
- rendering a typed or type-erased view model into HTML
- reusing the application's shared template environment and template cache

Relevant files:

- `[[include/coroute/view/view_renderer.hpp]]`
- `[[include/coroute/view/web_renderer.hpp]]`
- `[[src/view/web_renderer.cpp]]`
- `[[include/coroute/core/app.hpp]]`

## Template engine ownership

The `App` class owns the Inja environment and related configuration.

Observed responsibilities in `App`:

- `set_templates(...)`
- `set_template_caching(...)`
- `render(std::string_view, json)`
- `render(filename, json)`
- `render_html(...)`
- `add_template_callback(...)`
- `clear_template_cache()`
- exposing `template_dir()` and `template_env()`

Relevant files:

- `[[include/coroute/core/app.hpp]]`

## `WebViewRenderer`

`WebViewRenderer` is the platform-specific implementation of `IViewRenderer` for the web target.

Observed behavior:

- validates that each referenced `ViewTemplates.web` file exists
- appends `.html` when no extension is provided
- calls `view_result.to_json()`
- delegates final rendering to `app_.render_html(...)`

Relevant files:

- `[[include/coroute/view/web_renderer.hpp]]`
- `[[src/view/web_renderer.cpp]]`

## Browser request path

For browser-originated requests in server mode:

- a view handler returns `ViewResultAny`
- the model is converted to JSON
- the `web` template name is resolved
- Inja renders HTML using the current template directory
- Coroute returns an HTML `Response`

Relevant files:

- `[[src/core/app.cpp]]`
- `[[src/view/web_renderer.cpp]]`
- `[[examples/view_example/main.cpp]]`
- `[[examples/Project/src/main.cpp]]`

## Relation to the shared view contract

Inja is only one consumer of the shared `ViewTemplates` contract.

- it reads `templates.web`
- it ignores the `mobile` and `desktop` identifiers
- it consumes the same serialized model that Flutter can also consume through another presenter

See also:

- [[Architecture/View and API Abstractions]]
- [[Renderers/Flutter Renderer Contract]]

## Relevant examples

- `[[examples/view_example/templates/web/login.html]]`
- `[[examples/Project/templates/pages/index.html]]`
- `[[examples/FlutterProject/templates/web/index.html]]`
- `[[README.md]]`

## Status

### Current status

- **Implemented**: Inja template environment is present in `App` behind `COROUTE_HAS_TEMPLATES`.
- **Implemented**: `WebViewRenderer` validates and renders HTML template files.
- **Implemented**: browser requests in the main server path render HTML when the client is not identified as Flutter.
- **Testing note**: I found strong tests for the shared view types in `[[tests/test_view.cpp]]`, but I did not find a dedicated `web_renderer` test file in this pass.

## Related notes

- [[Renderers/Renderers and Template Engines]]
- [[Architecture/View and API Abstractions]]
- [[Architecture/Server Runtime and OS Backends]]
- [[Protocols/HTTP-1.1]]
