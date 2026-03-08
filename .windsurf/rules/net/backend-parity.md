---
trigger: glob
globs: src/net/**/*.cpp,src/net/**/*.h,include/coroute/core/**/*.hpp
description: Apply equivalent fixes across all three net I/O backends (io_uring, IOCP, kqueue)
---

# Network Backend Parity Rules

## Cross-Backend Bug Fix Policy

When a bug fix is applied to any one of the three network I/O backend implementations:

- `src/net/io_uring/` (Linux io_uring)
- `src/net/iocp/` (Windows IOCP)
- `src/net/kqueue/` (macOS/BSD kqueue)

You **must** investigate whether the same bug (or an equivalent manifestation) exists in the other two backends. If it does, implement the corresponding fix before closing the task.

## Investigation Checklist

1. Identify the **root cause** in the affected backend — not just the symptom.
2. Locate the **analogous code path** in each of the other two backends.
3. Determine whether the same logical error can occur given each backend's API semantics.
4. If yes — apply an equivalent fix. If no — document briefly in the commit/PR why the other backend is not affected.

## Rationale

The three backends are parallel implementations of the same abstraction. A logic error in one (e.g., incorrect error handling, missing cancellation, off-by-one in buffer accounting) is very likely a copy-propagated or design-level mistake present in all three. Fixing only one backend creates silent divergence and deferred bugs on other platforms.

## New Backend Features

When adding a new capability to one backend, the same capability must be implemented — or explicitly stubbed with a `static_assert(false, "not yet implemented on <platform>")` — in the other two backends before the feature is considered complete. Partial platform support must never be silently absent.
