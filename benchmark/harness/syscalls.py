"""Shared-memory I/O counters written by the server, read by the harness.

Layout matches include/coroute/net/io_stats.hpp. Only operations that are actual
syscalls on that backend contribute to syscalls_total: an io_uring SQE is not a
syscall and must not invent equality with epoll.
"""

from __future__ import annotations

import struct
from pathlib import Path
from typing import Any


MAGIC = 0x434F524F55544501
# magic + 8 counters
_LAYOUT = struct.Struct("<9Q")
_NAMES = (
    "epoll_wait",
    "epoll_ctl",
    "io_uring_enter",
    "accept",
    "read",
    "write",
    "sendfile",
    "send_zc",
)

# Counters that are real syscalls on the backend that issues them.
_SYSCALL_KEYS = (
    "epoll_wait",
    "epoll_ctl",
    "io_uring_enter",
    "accept",
    "read",
    "write",
    "sendfile",
)


def read_io_stats(path: Path) -> dict[str, int] | None:
    """Reads the mmap file. None when missing, short, or magic-mismatched."""
    try:
        data = path.read_bytes()
    except OSError:
        return None
    if len(data) < _LAYOUT.size:
        return None
    values = _LAYOUT.unpack_from(data)
    if values[0] != MAGIC:
        return None
    return {name: int(values[i + 1]) for i, name in enumerate(_NAMES)}


def syscalls_total(counts: dict[str, int]) -> int:
    """Sum of backend-issued I/O syscalls. send_zc is excluded: it is an SQE."""
    return sum(counts.get(name, 0) for name in _SYSCALL_KEYS)


def read_proc_io(pid: int) -> dict[str, int] | None:
    """Kernel read/write syscall counters from /proc/<pid>/io, or None."""
    try:
        text = Path(f"/proc/{pid}/io").read_text(encoding="ascii")
    except OSError:
        return None
    out: dict[str, int] = {}
    for line in text.splitlines():
        if ":" not in line:
            continue
        key, _, value = line.partition(":")
        try:
            out[key.strip()] = int(value.strip())
        except ValueError:
            continue
    return out or None


def summarise(
    *,
    backend_counts: dict[str, int] | None,
    proc_io_before: dict[str, int] | None,
    proc_io_after: dict[str, int] | None,
    requests_total: int,
) -> dict[str, Any]:
    """Fields for a RunRecord. Empty totals when nothing could be counted."""
    out: dict[str, Any] = {
        "syscalls_total": None,
        "syscalls_per_request": None,
        "syscall_counts": {},
        "syscall_source": "unavailable",
    }

    counts: dict[str, int] = {}
    source_parts: list[str] = []

    if backend_counts:
        counts.update({f"backend_{k}": v for k, v in backend_counts.items() if v})
        total = syscalls_total(backend_counts)
        out["syscalls_total"] = total
        source_parts.append("backend_io_stats")

    if proc_io_before and proc_io_after:
        for key in ("syscr", "syscw"):
            if key in proc_io_before and key in proc_io_after:
                delta = proc_io_after[key] - proc_io_before[key]
                if delta >= 0:
                    counts[f"proc_{key}"] = delta
        source_parts.append("proc_io")

    if counts:
        out["syscall_counts"] = counts
        out["syscall_source"] = "+".join(source_parts)
        if out["syscalls_total"] is not None and requests_total > 0:
            out["syscalls_per_request"] = out["syscalls_total"] / requests_total

    return out
