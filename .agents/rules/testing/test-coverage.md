---
trigger: always_on
description: 100% coverage requirement and mandatory test execution after every change
---

# Test Coverage Rules

## 100% Code Coverage Requirement

All production code in the project must be covered by unit and/or integration tests. The target is **100% line, branch, and function coverage** across the codebase.

### Scope

- Every new function, method, or class introduced must be accompanied by corresponding tests before the implementation is considered complete.
- Every bug fix must include a regression test that reproduces the original failure and verifies it is resolved.
- Platform-specific backends (e.g., `src/net/io_uring/`, `src/net/iocp/`, `src/net/kqueue/`) must each have dedicated tests covering their own code paths — shared logic coverage in one backend does not substitute for coverage in another.

### Test Placement

- Unit tests belong in `tests/` and must be registered in `tests/CMakeLists.txt`.
- Integration tests that exercise multi-component behavior (e.g., FFI boundary, WebSocket hub, coroutine pipelines) must also reside under `tests/` with clearly scoped names.

### Mandatory Test Execution After Every Change

- After every code change, the full test suite **must be run** before marking the task as done.
- A task is **not complete** if any test fails, is skipped, or is disabled. All tests must pass.
- Coverage must be verified with an instrumented build (e.g., via `gcov`/`llvm-cov`) before marking any implementation task done.

### Enforcement

- A pull request or task is not considered complete if it reduces overall coverage below 100% for the lines it touches.
- Do not merge code that bypasses, disables, or weakens existing tests to make coverage numbers pass.
- Test failures are never acceptable collateral damage — if a refactor breaks a test, fix the code or update the test with a documented rationale.
