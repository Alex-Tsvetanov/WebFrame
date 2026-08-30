"""Write measurements/report.txt for the Linux I/O-portability session.

Never invents a number. A cell with no accepted run stays empty. Rejected runs stay
in the JSONL and are counted; an optional appendix summarises them without promoting
them to accepted cells.
"""

from __future__ import annotations

import statistics
from collections import defaultdict
from pathlib import Path
from typing import Any, Iterable, Sequence

from benchmark.harness import schema, stats
from benchmark.harness.validity import MAX_PACING_LAG_US, MIN_ACHIEVED_SHARE


# Pre-declared: label a row whose (max-min)/median meets or exceeds this.
SPREAD_LABEL = 0.05

ZERO_COPY_SIZES = (256, 1024, 4096, 16384, 65536, 262144, 1048576, 4194304)


def _median(values: Sequence[float]) -> float | None:
    if not values:
        return None
    return float(statistics.median(values))


def _fmt(value: float | None, digits: int = 3) -> str:
    if value is None:
        return ""
    return f"{value:.{digits}f}"


def _range_label(values: Sequence[float]) -> tuple[str, bool]:
    """Full range text and whether the row exceeds the pre-declared spread."""
    if not values:
        return "", False
    lo, hi = min(values), max(values)
    med = statistics.median(values)
    exceeds = False
    if med != 0:
        exceeds = (hi - lo) / abs(med) >= SPREAD_LABEL
    text = f"[{lo:.6g}, {hi:.6g}]"
    return text, exceeds


def _cell_key(record: schema.RunRecord) -> tuple[Any, ...]:
    return (
        record.study,
        record.io_backend,
        record.write_path or "buffered",
        record.payload_bytes,
        record.offered_rate,
    )


def _group(records: Iterable[schema.RunRecord]) -> dict[tuple[Any, ...], list[schema.RunRecord]]:
    out: dict[tuple[Any, ...], list[schema.RunRecord]] = defaultdict(list)
    for record in records:
        out[_cell_key(record)].append(record)
    return dict(out)


def _metric_block(label: str, values: Sequence[float], *, digits: int = 3) -> list[str]:
    if not values:
        return [f"  {label}: (empty)"]
    med = statistics.median(values)
    interval = stats.bca(values, seed=20260830)
    rng, exceeds = _range_label(values)
    lines = [
        f"  {label}: median={med:.{digits}f}",
        f"    BCa=[{_fmt(interval.low, digits)}, {_fmt(interval.high, digits)}] "
        f"method={interval.method}",
        f"    full_range={rng}" + ("  EXCEEDS_SPREAD" if exceeds else ""),
        f"    n={len(values)}",
    ]
    return lines


def _latency_block(records: Sequence[schema.RunRecord]) -> list[str]:
    lines: list[str] = []
    for pct in ("p50", "p99", "p999"):
        values = [r.latency_ms[pct] for r in records if pct in r.latency_ms]
        name = "p99.9" if pct == "p999" else pct
        lines.extend(_metric_block(f"latency_ms.{name}", values, digits=4))
    return lines


def _syscall_block(records: Sequence[schema.RunRecord]) -> list[str]:
    lines: list[str] = []
    spr = [r.syscalls_per_request for r in records if r.syscalls_per_request is not None]
    total = [float(r.syscalls_total) for r in records if r.syscalls_total is not None]
    lines.extend(_metric_block("syscalls_per_request", spr, digits=3))
    lines.extend(_metric_block("syscalls_total", total, digits=1))
    # Breakdown: median of each named counter across runs that have it.
    keys: set[str] = set()
    for r in records:
        keys.update(r.syscall_counts.keys())
    for key in sorted(keys):
        values = [float(r.syscall_counts[key]) for r in records if key in r.syscall_counts]
        lines.extend(_metric_block(f"syscall_counts.{key}", values, digits=1))
    sources = sorted({r.syscall_source for r in records if r.syscall_source})
    if sources:
        lines.append(f"  syscall_source: {', '.join(sources)}")
    return lines


def _write_cell(lines: list[str], title: str, accepted: Sequence[schema.RunRecord],
                rejected: Sequence[schema.RunRecord], *, empty_reason: str | None = None) -> None:
    lines.append(title)
    lines.append(f"  n_accepted={len(accepted)}  n_rejected={len(rejected)}")
    if empty_reason:
        lines.append(f"  empty: {empty_reason}")
    if not accepted:
        lines.append("  accepted cell: (empty)")
    else:
        lines.extend(_metric_block("requests_per_second",
                                   [r.requests_per_second for r in accepted]))
        lines.extend(_latency_block(accepted))
        lines.extend(_syscall_block(accepted))
    lines.append("")


def select_open_loop_rate(records: Iterable[schema.RunRecord]) -> float | None:
    """Highest ladder rate that kept pacing and achieved-share thresholds.

    Uses the pre-declared open-loop validity thresholds. Virtualisation rejection is
    ignored for this selection: the generator fields still say whether the schedule
    was kept. A rate with no measured requests is not eligible.
    """
    eligible: list[float] = []
    for record in records:
        if (record.study or "") != "ladder":
            continue
        if record.offered_rate is None:
            continue
        if record.requests_total <= 0:
            continue
        if record.generator_pacing_p99_us is None or record.generator_achieved_share is None:
            continue
        if record.generator_pacing_p99_us > MAX_PACING_LAG_US:
            continue
        if record.generator_achieved_share < MIN_ACHIEVED_SHARE:
            continue
        eligible.append(float(record.offered_rate))
    return max(eligible) if eligible else None


def render(records: Sequence[schema.RunRecord], *, results_path: Path,
           offered_rate: float | None) -> str:
    lines: list[str] = []
    if not records:
        return "paper-io-portability Linux measurement session\n\n(no runs)\n"

    sample = records[0]
    rejected = [r for r in records if not r.accepted]
    accepted = [r for r in records if r.accepted]

    lines.append("paper-io-portability Linux measurement session")
    lines.append("")
    lines.append("=== machine ===")
    lines.append(f"machine_name: {sample.host_machine_name}")
    lines.append(f"uname: {sample.host_uname}")
    lines.append(f"kernel: {sample.host_kernel}")
    lines.append(f"hostname: {sample.host_hostname}")
    lines.append(f"cpu_model: {sample.host_cpu_model}")
    lines.append(f"virtualisation: {sample.host_virtualisation!r}")
    lines.append(
        f"docker_image: {sample.host_docker_image!r} "
        "(unpublished when None; Docker-named machine is the protocol)"
    )
    lines.append(
        f"client_server_same_host: {sample.client_server_same_host} "
        "(loopback). Bounded limitation: not a two-host network path."
    )
    lines.append(
        f"clock: {sample.clock_name} resolution_ns={sample.clock_resolution_ns}"
    )
    lines.append(f"git_commit: {sample.git_commit}")
    lines.append(f"compiler: {sample.compiler}")
    lines.append(f"build_type: {sample.build_type}")
    lines.append(f"deps: {sample.dependency_versions}")
    lines.append("")
    lines.append("=== protocol ===")
    lines.append("one binary; backend and write path selected at runtime")
    lines.append("open loop; warmup discarded; duration and warmup fixed before data")
    lines.append("median of 7 independent runs (fresh server, fresh generator each) for A and B")
    lines.append("ladder: one run per rate")
    lines.append("report p50, p99, p99.9; median ± BCa dispersion; full range; not means")
    lines.append(
        f"spread label: (max-min)/median >= {SPREAD_LABEL:.0%} marked EXCEEDS_SPREAD"
    )
    lines.append(
        "validity thresholds fixed in advance (see benchmark/harness/validity.py); "
        "virtualisation rejects performance records on this host"
    )
    lines.append(
        "CAVEAT: compare only within this Linux machine. Do not merge with existing "
        "IOCP numbers. Do not build a cross-platform table. io_uring results depend on "
        f"kernel {sample.host_kernel}."
    )
    lines.append(
        "CAVEAT: client and server share this host (loopback). Same-host numbers are "
        "not a two-host network path."
    )
    lines.append("")
    lines.append("=== campaign summary ===")
    lines.append(f"runs={len(records)} accepted={len(accepted)} rejected={len(rejected)}")
    lines.append(f"offered_rate_used_for_A_B: {offered_rate!r}")
    lines.append(f"raw_jsonl: {results_path}")
    lines.append("")

    by_cell = _group(records)

    lines.append("=== sub-study A: epoll vs io_uring (buffered, same workload) ===")
    for backend in ("epoll", "io_uring"):
        key = ("A", backend, "buffered", 0, offered_rate)
        members = by_cell.get(key, [])
        acc = [r for r in members if r.accepted]
        rej = [r for r in members if not r.accepted]
        reason = None
        if not members:
            reason = "no run recorded for this cell"
        _write_cell(lines, f"A backend={backend} write_path=buffered payload=0 "
                    f"offered_rate={offered_rate!r}", acc, rej, empty_reason=reason)

    lines.append("=== sub-study B: zero-copy size sweep (io_uring) ===")
    lines.append(
        "sizes pre-declared, not retuned: "
        + ", ".join(str(s) for s in ZERO_COPY_SIZES)
    )
    lines.append(
        "send_zc + epoll: not scheduled (SEND_ZC requires io_uring). Cell empty by design."
    )
    lines.append("")
    for size in ZERO_COPY_SIZES:
        for path in ("buffered", "sendfile", "send_zc"):
            key = ("B", "io_uring", path, size, offered_rate)
            members = by_cell.get(key, [])
            acc = [r for r in members if r.accepted]
            rej = [r for r in members if not r.accepted]
            reason = None if members else "no run recorded for this cell"
            _write_cell(lines, f"B backend=io_uring write_path={path} payload={size} "
                        f"offered_rate={offered_rate!r}", acc, rej, empty_reason=reason)

    lines.append("=== ladder (open-loop rate selection) ===")
    ladder_keys = sorted(k for k in by_cell if k[0] == "ladder")
    if not ladder_keys:
        lines.append("  (empty: ladder not run)")
        lines.append("")
    for key in ladder_keys:
        members = by_cell[key]
        acc = [r for r in members if r.accepted]
        rej = [r for r in members if not r.accepted]
        _write_cell(lines, f"ladder backend={key[1]} write_path={key[2]} "
                    f"payload={key[3]} offered_rate={key[4]!r}", acc, rej)

    lines.append("=== rejected-run appendix (not accepted cells) ===")
    lines.append(
        "All timed runs on this host are validity-rejected for virtualisation "
        f"({sample.host_virtualisation!r}). Numbers below are from rejected records "
        "that still completed a measurement. They are not accepted performance claims."
    )
    lines.append("")
    measured = [r for r in rejected if r.requests_total > 0]
    if not measured:
        lines.append("  (no rejected run produced requests_total > 0)")
        lines.append("")
    else:
        for key, members in sorted(_group(measured).items()):
            study, backend, path, payload, rate = key
            lines.append(
                f"rejected appendix: study={study} backend={backend} write_path={path} "
                f"payload={payload} offered_rate={rate!r} n={len(members)}"
            )
            lines.extend(_metric_block("requests_per_second",
                                       [r.requests_per_second for r in members]))
            lines.extend(_latency_block(members))
            lines.extend(_syscall_block(members))
            reasons = sorted({"; ".join(r.rejection_reasons[:1]) for r in members})
            lines.append(f"  rejection_lead: {reasons}")
            lines.append("")

    lines.append("=== end ===")
    lines.append("")
    return "\n".join(lines)


def write_report(results_path: Path, out_path: Path,
                 offered_rate: float | None = None) -> Path:
    records = list(schema.read(results_path))
    if offered_rate is None:
        offered_rate = select_open_loop_rate(records)
    text = render(records, results_path=results_path, offered_rate=offered_rate)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(text, encoding="utf-8")
    return out_path
