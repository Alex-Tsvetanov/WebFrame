---
trigger: model_decision
description: Avoid modifying gitignored files; fix generators and build system instead
---

# Gitignored File Preservation Rules

## Core Principle

When investigating or fixing a bug or issue, always prioritize solutions that do **not** touch `.gitignore`d files or directories. Manual edits to gitignored paths are a last resort.

## Investigation Order

1. **Identify root cause in tracked files first.** Before touching any generated or ignored path, confirm the bug cannot be fixed in source-controlled files (e.g., `CMakeLists.txt`, source headers, Dart/C++ source files, build scripts).

2. **Use the build system.** If a fix requires changing something inside a gitignored folder (e.g., a generated file, a build artifact, a Flutter plugin directory), find the appropriate build system hook instead:
   - For CMake projects: use `CMakeLists.txt`, `cmake/` modules, or `ExternalProject`/`FetchContent` directives.
   - For Flutter/Dart: use `pubspec.yaml`, `flutter pub` commands, or plugin configuration in `pubspec.yaml`.
   - For Xcode projects: use `xcconfig` files or build phases in `CMakeLists.txt`, not direct edits to `.xcodeproj/` contents if those are gitignored.

3. **Prefer code generation over manual patching.** If a generated file is wrong, fix the generator or template, not the output.

## When Manual Edits to Gitignored Files Are Considered

Only modify a gitignored file directly if **all** of the following are true:
- No tracked source file, build script, or configuration can produce the correct result.
- The change cannot be expressed as a build system step or post-generation patch.
- The manual change is documented with a comment explaining why it cannot be automated.

Even then, always note in the conversation that the edit targets a gitignored path and recommend a follow-up to automate it.

## Examples

| Situation | Preferred approach |
|---|---|
| Generated Flutter plugin file has wrong content | Fix `pubspec.yaml` or the plugin source; re-run `flutter pub get` |
| Xcode `project.pbxproj` (gitignored) needs a flag | Add the flag via a checked-in `.xcconfig` file or CMake build phase |
| Build artifact in `build/` is stale | Clean and rebuild via `cmake --build` rather than editing the artifact |
| A `.flutter/` generated file is incorrect | Trace back to the Flutter toolchain command or template that produced it |
