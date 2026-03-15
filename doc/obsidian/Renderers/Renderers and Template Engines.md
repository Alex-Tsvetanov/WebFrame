---
title: Renderers and Template Engines
tags:
  - coroute
  - renderers
  - templates
  - obsidian
aliases:
  - Renderer Index
  - Template Engine Index
---

# Renderers and Template Engines

> [!abstract]
> In Coroute, a "template" is not limited to HTML. The view system chooses a platform-specific presentation target through `ViewTemplates`, and each target interprets that identifier differently. This note is the hub for the current Inja web renderer, the Flutter rendering contract, and the future Kotlin Multiplatform direction.

## Shared contract

All renderer paths are built on:

- `[[include/coroute/view/view_types.hpp]]`
- `[[include/coroute/view/view_renderer.hpp]]`
- `[[include/coroute/core/app.hpp]]`

The common abstraction is:

- `ViewTemplates` names the presentation target for web, mobile, and desktop.
- `ViewResult<VM>` carries the typed model.
- `ViewResultAny` lets the runtime store and serialize the view result uniformly.

## Current renderer families

### Web / HTML rendering

The current HTML renderer is Inja-based.

See:

- [[Renderers/Inja Web Renderer]]

### Flutter / native widget rendering

Flutter does not render HTML templates. It consumes the `mobile` template identifier as a logical screen name and resolves it through `ScreenRegistry`.

See:

- [[Renderers/Flutter Renderer Contract]]

### Kotlin Multiplatform direction

The repository already reserves a package area for Kotlin Multiplatform, but there is not yet a completed adapter path equivalent to the Flutter package.

See:

- [[Renderers/Kotlin Multiplatform Renderer Roadmap]]

## Why this matters architecturally

The renderer split is what lets Coroute keep business logic and route/view-model construction in one place while swapping out the presentation technology per target.

That separation depends on three rules:

- the engine emits stable template identifiers and serialized models
- the renderer interprets those identifiers using the target platform's presentation primitives
- the renderer does not become a second business-logic layer

## Relevant examples

- `[[examples/view_example/main.cpp]]`
- `[[examples/FlutterProject/src/handlers/pages.cpp]]`
- `[[packages/Flutter/TECHNICAL_SPEC.md]]`

## Status

### Current status

- **Implemented**: Inja-based HTML rendering for web.
- **Implemented**: Flutter rendering contract based on `templates.mobile` and `ScreenRegistry`.
- **Future-facing**: Kotlin Multiplatform package area exists, but an implemented renderer/adapter contract is not yet present in this pass.

## Related notes

- [[Architecture/Project Atlas]]
- [[Architecture/View and API Abstractions]]
- [[Architecture/Framework Integration Architecture]]
- [[Architecture/Flutter Integration]]
