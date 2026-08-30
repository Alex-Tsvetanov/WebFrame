"""What actually happens for one run, and the order it happens in.

The driver is small on purpose. Everything it knows how to do is either enforcing a
rule from the methodology or recording something the record needs, and it knows nothing
about h2load, nginx or cgroups. Those are adapters behind two protocols, which is what
lets the rules below be checked without any of them installed.

The rules it enforces, each of which the previous harness broke:

  a run is a process        A fresh server and a fresh generator every time. The old
                            script started one server and measured it five times, so
                            n=5 was five looks at one process's page placement.

  the server always stops   Even when the generator raises. A leaked server holds the
                            port and every later run in the campaign fails for a reason
                            that has nothing to do with the code.

  every run leaves a record Including the ones that failed or were rejected. A campaign
                            with silent gaps cannot be told apart from one that was
                            never scheduled, and the rejection count has to be
                            reportable.

  guards decide, not people Validity is applied here, once, from the pre-declared rules.
"""

from __future__ import annotations

import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Callable, Iterable, Protocol

from benchmark.harness import schema, validity
from benchmark.harness.ordering import Cell, ScheduledRun


@dataclass
class GeneratorResult:
    """What a load generator observed.

    Errors are first because they are the field the old harness did not parse at all,
    which let a server that failed fast win on throughput.
    """

    requests_total: int = 0
    requests_non_2xx: int = 0
    socket_errors: int = 0
    requests_per_second: float = 0.0
    bytes_per_second: float = 0.0
    latency_ms: dict[str, float] = field(default_factory=dict)
    latency_histogram: str | None = None
    raw_samples_path: str | None = None
    # The saturation check for a closed loop. A generator at full CPU is measuring
    # itself. It says nothing about an open loop, which paces by spinning.
    cpu_fraction: float | None = None
    # The saturation check for an open loop: how late the generator was, at p99, in
    # getting a request onto the socket relative to when it was due.
    pacing_p99_us: float | None = None
    # And whether it delivered the rate it was asked for.
    achieved_share: float | None = None
    argv: list[str] = field(default_factory=list)


@dataclass
class ResourceUsage:
    """Server-side cost, from cgroup v2 rather than from ps.

    The old harness read lifetime-average percent on one platform and cumulative CPU
    seconds on the other, then plotted both into one column. These are kernel-tracked
    and each means one thing.
    """

    cpu_seconds: float | None = None
    memory_peak_bytes: int | None = None
    quic: dict[str, int] = field(default_factory=dict)


class Server(Protocol):
    """A server under test, started fresh for each run."""

    @property
    def argv(self) -> list[str]: ...

    def start(self) -> None: ...

    def wait_until_ready(self, timeout_s: float) -> bool:
        """Blocks until the server is listening.

        Without this the first moments of every run measure process startup, and that
        cost lands on whichever system happens to boot slowest rather than on anything
        being compared.
        """
        ...

    def stop(self) -> ResourceUsage: ...


class Generator(Protocol):
    """A load generator."""

    @property
    def name(self) -> str: ...

    def run(self, cell: Cell, duration_s: float) -> GeneratorResult: ...


class RunFailed(RuntimeError):
    """The run did not produce a measurement. Recorded, not swallowed."""


def run_one(
    scheduled: ScheduledRun,
    *,
    server_factory: Callable[[Cell], Server],
    generator: Generator,
    environment: dict[str, Any],
    campaign_fingerprint: str,
    duration_s: float,
    readiness_timeout_s: float = 30.0,
    now: Callable[[], float] = time.time,
) -> schema.RunRecord:
    """Performs one run and returns its record, accepted or not.

    Never raises for a failed run. A campaign that stops at the first bad cell wastes
    the machine time already spent, and a run that failed is itself a finding: the
    record carries the reason and is marked rejected.
    """
    cell = scheduled.cell
    factors = cell.as_dict()

    record = schema.RunRecord(
        campaign_fingerprint=campaign_fingerprint,
        started_unix=now(),
        repetition=scheduled.repetition,
        system=cell.system,
        protocol=str(factors.get("protocol", "")),
        tls=bool(factors.get("tls", False)),
        io_backend=str(factors.get("io_backend", "")),
        protocol_detection=bool(factors.get("protocol_detection", True)),
        workers=int(factors.get("workers", 0)),
        connections=int(factors.get("connections", 0)),
        streams_per_connection=int(factors.get("streams_per_connection", 1)),
        payload_bytes=int(factors.get("payload_bytes", 0)),
        backlog=int(factors.get("backlog", 0)),
        duration_s=duration_s,
        offered_rate=factors.get("offered_rate"),
        netem_profile=str(factors.get("netem_profile", "none")),
        router_arm=str(factors.get("router_arm", "")),
        route_count=int(factors.get("route_count", 0)),
        route_shape=str(factors.get("route_shape", "")),
        route_params=bool(factors.get("route_params", False)),
        route_depth=int(factors.get("route_depth", 0)),
        generator=generator.name,
        virtualisation=environment.get("virtualisation"),
        git_dirty=bool(_dig(environment, "build", "git_dirty")),
        git_commit=str(_dig(environment, "build", "git_commit") or ""),
        build_type=str(_dig(environment, "build", "type") or ""),
        compiler=str(_dig(environment, "toolchain", "compiler") or ""),
        dependency_versions={k: v for k, v in (environment.get("deps") or {}).items() if v},
    )

    counters_before = validity.read_counters()
    record.cpu_mhz_start = validity.current_cpu_mhz()

    failure: str | None = None
    server: Server | None = None
    try:
        server = server_factory(cell)
        record.server_argv = list(server.argv)
        server.start()

        if not server.wait_until_ready(readiness_timeout_s):
            raise RunFailed(f"server did not become ready within {readiness_timeout_s:g}s")

        result = generator.run(cell, duration_s)
        record.requests_total = result.requests_total
        record.requests_non_2xx = result.requests_non_2xx
        record.socket_errors = result.socket_errors
        record.requests_per_second = result.requests_per_second
        record.bytes_per_second = result.bytes_per_second
        record.latency_ms = dict(result.latency_ms)
        record.latency_histogram = result.latency_histogram
        record.raw_samples_path = result.raw_samples_path
        record.generator_cpu_fraction = result.cpu_fraction
        record.generator_pacing_p99_us = result.pacing_p99_us
        record.generator_achieved_share = result.achieved_share
        record.generator_argv = list(result.argv)

    except Exception as exc:  # noqa: BLE001 - a failed run is data, not a crash
        failure = f"{type(exc).__name__}: {exc}"

    finally:
        # In a finally block because a leaked server holds the port and every later run
        # in the campaign then fails for a reason unrelated to the code being measured.
        if server is not None:
            try:
                usage = server.stop()
                record.server_cpu_seconds = usage.cpu_seconds
                record.server_memory_peak_bytes = usage.memory_peak_bytes
                record.quic_datagrams_received = usage.quic.get("received", 0)
                record.quic_forwarded_out = usage.quic.get("forwarded_out", 0)
                record.quic_forwarded_in = usage.quic.get("forwarded_in", 0)
                record.quic_migrations = usage.quic.get("migrations", 0)
                record.quic_stateless_resets = usage.quic.get("stateless_resets", 0)
            except Exception as exc:  # noqa: BLE001
                # Reported rather than masking the original failure, which is the more
                # informative of the two.
                stop_note = f"stopping the server failed: {type(exc).__name__}: {exc}"
                failure = f"{failure}; {stop_note}" if failure else stop_note

    record.cpu_mhz_end = validity.current_cpu_mhz()
    record.counter_deltas = validity.counter_deltas(counters_before, validity.read_counters())

    verdict = validity.check_run(record.to_dict())
    reasons = list(verdict.reasons)
    if failure:
        reasons.insert(0, failure)
    record.rejection_reasons = reasons
    record.accepted = not reasons
    return record


def run_campaign(
    schedule: Iterable[ScheduledRun],
    *,
    results_path: Path,
    server_factory: Callable[[Cell], Server],
    generator: Generator,
    environment: dict[str, Any],
    campaign_fingerprint: str,
    duration_s: float,
    on_record: Callable[[schema.RunRecord], None] | None = None,
) -> list[schema.RunRecord]:
    """Walks the schedule, writing each record as it completes.

    Written per run rather than at the end. A campaign is hours or days, and a driver
    that batches its output loses everything if the machine reboots at hour six.
    """
    written: list[schema.RunRecord] = []
    for scheduled in schedule:
        record = run_one(
            scheduled,
            server_factory=server_factory,
            generator=generator,
            environment=environment,
            campaign_fingerprint=campaign_fingerprint,
            duration_s=duration_s,
        )
        # allow_incomplete because a run that failed before it could be described still
        # has to leave a trace. A missing record is indistinguishable from a run that
        # was never scheduled.
        schema.append(results_path, record, allow_incomplete=not record.accepted)
        written.append(record)
        if on_record is not None:
            on_record(record)
    return written


def summarise(records: Iterable[schema.RunRecord]) -> dict[str, Any]:
    """Counts and reasons, for reporting alongside the results.

    The rejection count is part of the method. A campaign that discarded a third of its
    runs is a different campaign from one that discarded none, and only saying so makes
    the difference visible.
    """
    records = list(records)
    rejected = [r for r in records if not r.accepted]
    reasons: dict[str, int] = {}
    for record in rejected:
        for reason in record.rejection_reasons:
            # Keyed on the leading phrase so "non-2xx rate 0.4%" and "non-2xx rate 0.9%"
            # count as one kind of problem rather than two.
            key = reason.split(";")[0].split("(")[0].strip()[:60]
            reasons[key] = reasons.get(key, 0) + 1
    return {
        "runs": len(records),
        "accepted": len(records) - len(rejected),
        "rejected": len(rejected),
        "rejection_reasons": dict(sorted(reasons.items(), key=lambda kv: -kv[1])),
    }


def _dig(mapping: dict[str, Any], *path: str) -> Any:
    node: Any = mapping
    for key in path:
        if not isinstance(node, dict):
            return None
        node = node.get(key)
    return node
