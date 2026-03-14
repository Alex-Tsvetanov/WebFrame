---
trigger: always_on
description: Always build all targets and run all tests after every change
---

# Build All Targets & Regression Rule

After every code change — including bug fixes, refactors, feature additions, and documentation edits — **both** of the following conditions must hold before the task is considered complete:

## 1. All Targets Must Compile
- **Never build only the focused target.** A change that compiles cleanly in isolation may break a sibling target or the overall project.
- Always use a full build with no `--target` restriction:
    - ✅ `cmake --build .` (no target filter)
    - ❌ `cmake --build . --target foo` (as the sole build step)
- Every target must compile and link without errors or warnings on all supported platforms (Linux, macOS, Windows, Android, iOS).

## 2. All Tests Must Pass (No Regressions)
- After every change, run the full test suite to confirm no regressions have been introduced.
- A task is **not done** until all tests pass. Do not mark work as complete with failing or skipped tests.
- Every bug fix must include a regression test that reproduces the original failure and verifies it is resolved before the fix is considered complete.

## Enforcement
- CI pipelines and local verification commands must reflect this policy.
- Do not merge or close a task with suppressed, disabled, or weakened tests to satisfy the build.
