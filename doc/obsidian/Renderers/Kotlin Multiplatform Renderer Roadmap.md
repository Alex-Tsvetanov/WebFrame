---
title: Kotlin Multiplatform Renderer Roadmap
tags:
  - coroute
  - kotlin
  - multiplatform
  - roadmap
aliases:
  - KMP Renderer Roadmap
  - Kotlin Integration Roadmap
---

# Kotlin Multiplatform Renderer Roadmap

> [!abstract]
> This note documents the current repository state and the likely architectural place for a future Kotlin Multiplatform adapter. It is intentionally a roadmap note, not a claim that a finished KMP renderer already exists.

## Current repository state

The repository contains:

- `packages/KotlinMultiPlatform/`

During this documentation pass, I did **not** find an implemented adapter package in that directory that is comparable to:

- `[[packages/Flutter/coroute_framework/pubspec.yaml]]`

That means KMP is currently best understood as a reserved integration direction rather than a completed runtime path.

## Why KMP fits the architecture

Coroute's existing architecture already has the pieces needed for another host framework:

- a shared engine with route and view-model logic in C++
- a flat exported bridge surface
- a cross-platform `ViewTemplates` contract
- a build-system layer that can package the engine for host runtimes

Relevant files:

- `[[src/bridge/bridge.cpp]]`
- `[[include/coroute/view/view_types.hpp]]`
- `[[cmake/CorouteApp.cmake]]`
- `[[Architecture/Framework Integration Architecture]]`

## Likely responsibilities of a future KMP adapter

A future Kotlin Multiplatform adapter would likely need to provide the same categories of functionality the Flutter adapter already provides.

### 1. Native bridge bindings

It would need to bind to the exported bridge entry points rather than duplicating Coroute logic in Kotlin.

### 2. View host abstraction

It would need an equivalent of `GenericView` that:

- requests a Coroute route
- receives `{ templates, model }`
- resolves the target template identifier into native Kotlin UI code

### 3. Screen/template registry

It would need a registry or equivalent mapping layer from backend-emitted template names to Kotlin-side UI constructors.

### 4. Packaging/build handoff

It would need a build pipeline equivalent to the current Flutter native-asset handoff.

## How it should align with the current view model contract

The clean architectural path is not to invent a second view protocol.

A KMP adapter should reuse the same conceptual contract already used by Flutter:

- Coroute returns platform-specific template identifiers plus model JSON
- the host-side adapter interprets the target identifier for its platform
- the host-side UI renders the model using native widgets/components

Relevant files:

- `[[include/coroute/view/view_types.hpp]]`
- `[[packages/Flutter/TECHNICAL_SPEC.md]]`
- `[[Renderers/Flutter Renderer Contract]]`

## Non-goals for a future adapter

A future KMP adapter should not:

- move business logic from C++ into Kotlin just to make integration possible
- replace the bridge with a parallel ad hoc transport layer
- bypass the shared view-model contract
- become a second routing system that competes with Coroute routing

## Status

### Current status

- **Present as repository direction**: `packages/KotlinMultiPlatform/` exists.
- **Not yet documented as implemented**: I did not find a concrete KMP runtime adapter package or renderer implementation in this pass.
- **Architecturally feasible**: the current engine/bridge/view contract already provides the right extension points for a future KMP adapter.

## Related notes

- [[Renderers/Renderers and Template Engines]]
- [[Architecture/Framework Integration Architecture]]
- [[Architecture/Flutter Integration]]
- [[Renderers/Flutter Renderer Contract]]
