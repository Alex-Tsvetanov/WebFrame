---
trigger: always_on
---

# Architecture and Code Quality Preference Rules

When choosing between implementation approaches, always default to the clean, idiomatic, maintainable solution — even when it requires a larger diff.

## 1. Clean Over Minimal Diff
- **Never choose a fragile workaround over a proper structural fix** simply because it is smaller. A one-time patch that breaks on re-scaffolding, regeneration, or a new machine is worse than a larger change that is correct everywhere.
- **Never ask the user** to choose between "clean but larger" and "minimal but fragile" — always default to the clean solution.

## 2. Idiomatic Language and Framework APIs
- Use the **canonical, platform-endorsed API** for each concern rather than approximations or workarounds invented outside the intended tool.
- Use **compile-time bindings and declarations** over runtime dynamic lookup wherever the language/framework supports it (e.g., compile-time FFI annotations, static type-checked bindings).
- When refactoring, **never leave mixed patterns** in the same file — complete the migration fully.

## 3. Structural Fixes Over Runtime Workarounds
- **Fix the build system, not the runtime.** If a library path, permission, or generated file needs to be in the right place, ensure the build layer puts it there — not a runtime try/catch or manual post-build step.
- **Build automation is preferred** over manual steps for anything that must work automatically across all platforms and all developer machines.

## 4. Compile-Time Guarantees Over Runtime Checks
- Prefer solutions that catch errors at **compile time** (type system, concepts, static assertions) over those that only fail at runtime.
- Prefer **static configuration and code generation** over dynamic discovery when both are viable.

## 5. Platform-Canonical Build Tools
- Use the canonical build tool endorsed by each platform for each concern (e.g., native build hooks for native assets, platform package managers for dependencies) rather than workarounds invented in shell scripts or other build systems.
- Cross-platform reliability requires each platform to be handled through its own idiomatic mechanism — do not paper over platform differences with a single generic script.
