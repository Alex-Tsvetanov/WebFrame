# Documentation Update Rules

Maintaining accurate and up-to-date documentation is a core requirement for the WebFrame / Coroute v2 project. Code changes must always be reflected in the relevant documentation interfaces to prevent architectural drift and ensure that the framework's behavior is transparent to developers.

## Mandatory Documentation Synchronization
After modifying the codebase (such as adding new routing abstractions, altering middleware behavior, tweaking the Flutter FFI interface, or introducing new C++20 structures):

1. **Update `doc/v2/`:** You MUST review and update any relevant markdown documents or architectural overviews inside the `doc/v2/` directory that correspond to the changed feature. Ensure that code examples, structural explanations, and endpoint behaviors are perfectly aligned with your new implementations.
2. **Update `README.md`:** If the change affects the framework’s top-level usage, integration instructions (like `CorouteApp.cmake`), high-level architecture overview, or CMake configuration options, the repository's root `README.md` MUST be explicitly updated alongside the core code.

## Enforcement
Any agentic task completion, code generation, or architectural modification must verify that both `doc/v2/` and `README.md` are accurately synchronized before marking the implementation phase as complete. No functional change should be considered "done" without its corresponding documentation.
