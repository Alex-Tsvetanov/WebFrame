---
trigger: glob
globs: **/*.cpp,**/*.hpp,**/*.dart,**/*.md
description: Sync doc/v2/ and README.md after any code change
---

# Documentation Update Rules

Maintaining accurate and up-to-date documentation is a core requirement for the WebFrame / Coroute v2 project. Code changes must always be reflected in the relevant documentation to prevent architectural drift.

## Mandatory Documentation Synchronization

After modifying the codebase (new routing abstractions, altered middleware behavior, changes to the Flutter FFI interface, new C++23 structures, build system changes):

1. **Update `doc/v2/` and `doc/obsidian/`:** Review and update any relevant markdown documents or architectural overviews inside `doc/v2/` and `doc/obsidian/` that correspond to the changed feature. Ensure code examples, structural explanations, and endpoint behaviors are perfectly aligned with the new implementation. Using wikilinks and callouts in Obsidian documents is preferred.
2. **Update `README.md`:** If the change affects the framework's top-level usage, integration instructions (e.g., `CorouteApp.cmake`), high-level architecture overview, or CMake configuration options, the root `README.md` **must** be updated alongside the core code.

## Enforcement

- No functional change is considered "done" without its corresponding documentation update across all relevant platforms (`doc/v2/`, `doc/obsidian/`, `README.md`).
- Agentic task completion must verify that `doc/v2/`, `doc/obsidian/` and `README.md` are accurately synchronized before marking the implementation phase complete.
- Documentation examples must compile and run correctly — stale code examples are bugs.
