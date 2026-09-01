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
                            port; on Linux it would even share it under SO_REUSEPORT,
                            so the adapter refuses a held port before starting and
                            every later run fails for a reason that is at least loud.

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

from benchmark.harness import environment, schema, validity
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
    # Responses over the whole process lifetime, warmup included, where requests_total
    # is the measured window. The denominator for any server-side counter that also
    # runs for the whole lifetime; None from a generator that predates it.
    requests_total_whole_run: int | None = None
    latency_ms: dict[str, float] = field(default_factory=dict)
    latency_histogram: str | None = None
    raw_samples_path: str | None = None
    # Connection establishment, which on the TLS arm includes the handshake. Separate
    # from latency_ms because a keep-alive run divides the per-connection cost by every
    # request the connection served and this does not.
    connect_ms: dict[str, float] = field(default_factory=dict)
    connections_established: int = 0
    handshake_failures: int = 0
    tls_version: str = ""
    tls_cipher: str = ""
    # The saturation check for a closed loop. A generator at full CPU is measuring
    # itself. It says nothing about an open loop, which paces by spinning.
    cpu_fraction: float | None = None
    # The saturation check for an open loop: how late the generator was, at p99, in
    # getting a request onto the socket relative to when it was due.
    pacing_p99_us: float | None = None
    # And whether it delivered the rate it was asked for.
    achieved_share: float | None = None
    # What isolation was asked for, and what the platform actually granted. Separate,
    # because on macOS they differ and the difference is not visible in any other field.
    affinity_requested: str | None = None
    affinity_applied: bool | None = None
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

    # The backend the server reported actually starting on, or None where the server
    # does not report one. Read through getattr at the call site, so an adapter for a
    # comparison framework that has no such concept needs nothing.
    effective_backend: str | None

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


@dataclass(frozen=True)
class HostProbes:
    """The readings a run takes of the machine underneath it.

    Injectable for the same reason the clock is. These were direct calls into validity,
    so a selfcheck driving run_one with a fake server and a fake generator still picked
    up the real host: on a laptop on battery the synthetic run that the check asserts
    must be accepted was correctly rejected, and the check failed. A gate that cannot
    pass on the machine it is meant to gate is not a gate.

    The live readings are still the default, so a real campaign is unchanged and
    nothing has to remember to ask for them.
    """

    cpu_mhz: Callable[[], float | str | None] = validity.current_cpu_mhz
    power_source: Callable[[], str | None] = validity.current_power_source
    speed_limit: Callable[[], int | None] = validity.current_speed_limit
    counters: Callable[[], dict[str, int]] = validity.read_counters
    governor: Callable[[], str | None] = environment._governor


LIVE_PROBES = HostProbes()

# A host that reads as clean and unremarkable. For tests only, so that a run rejected
# under these probes was rejected by its own data rather than by the machine the test
# happened to run on.
#
# Note it reports a mains desktop rather than reporting nothing. Silence is no longer
# clean: validity refuses a record whose power state it cannot establish, on the same
# reasoning that it refuses one whose virtualisation it cannot establish.
IDLE_PROBES = HostProbes(
    cpu_mhz=lambda: None,
    power_source=lambda: environment._NO_BATTERY,
    speed_limit=lambda: None,
    counters=dict,
    governor=lambda: None,
)


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
    probes: HostProbes = LIVE_PROBES,
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
        max_requests_per_connection=int(factors.get("max_requests_per_connection", 0)),
        router_arm=str(factors.get("router_arm", "")),
        route_count=int(factors.get("route_count", 0)),
        route_shape=str(factors.get("route_shape", "")),
        route_params=bool(factors.get("route_params", False)),
        route_depth=int(factors.get("route_depth", 0)),
        generator=generator.name,
        virtualisation=environment.get("virtualisation"),
        # Not bool(): None is git failing to answer, and validity refuses it as unknown.
        git_dirty=_dig(environment, "build", "git_dirty"),
        git_commit=str(_dig(environment, "build", "git_commit") or ""),
        build_type=str(_dig(environment, "build", "type") or ""),
        compiler=str(_dig(environment, "toolchain", "compiler") or ""),
        dependency_versions={k: v for k, v in (environment.get("deps") or {}).items() if v},
    )

    counters_before = probes.counters()
    record.cpu_mhz_start = probes.cpu_mhz()
    record.power_source = probes.power_source()
    record.thermal_speed_limit_start = probes.speed_limit()

    failure: str | None = None
    server: Server | None = None
    try:
        server = server_factory(cell)
        record.server_argv = list(server.argv)
        server.start()

        if not server.wait_until_ready(readiness_timeout_s):
            raise RunFailed(f"server did not become ready within {readiness_timeout_s:g}s")

        # What ran against what the cell claims ran.
        #
        # The backend is a runtime flag now, and a flag can be refused: asking for
        # io_uring on a host with kernel.io_uring_disabled=1 gets an epoll server, or on
        # a build without that arm gets whatever the build had. Either way the record
        # would carry io_backend=io_uring for a measurement of epoll, and a mislabelled
        # factor makes two different measurements look like repetitions of one. That is
        # the same failure resolve_io_backend refuses at build level, at run level.
        asked = cell.as_dict().get("io_backend")
        actually = getattr(server, "effective_backend", None)
        if asked and actually and actually != asked:
            raise RunFailed(
                f"cell asks for io_backend={asked} but the server started on {actually}"
            )

        result = generator.run(cell, duration_s)
        record.requests_total = result.requests_total
        record.requests_total_whole_run = result.requests_total_whole_run
        record.requests_non_2xx = result.requests_non_2xx
        record.socket_errors = result.socket_errors
        record.requests_per_second = result.requests_per_second
        record.bytes_per_second = result.bytes_per_second
        record.latency_ms = dict(result.latency_ms)
        record.latency_histogram = result.latency_histogram
        record.raw_samples_path = result.raw_samples_path
        record.connect_ms = dict(result.connect_ms)
        record.connections_established = result.connections_established
        record.handshake_failures = result.handshake_failures
        record.tls_version = result.tls_version
        record.tls_cipher = result.tls_cipher
        record.generator_cpu_fraction = result.cpu_fraction
        record.generator_pacing_p99_us = result.pacing_p99_us
        record.generator_achieved_share = result.achieved_share
        record.affinity_requested = result.affinity_requested
        record.affinity_applied = result.affinity_applied
        record.generator_argv = list(result.argv)

    except Exception as exc:  # noqa: BLE001 - a failed run is data, not a crash
        failure = f"{type(exc).__name__}: {exc}"

    finally:
        # In a finally block because a leaked server holds the port and every later run
        # in the campaign is then refused for a reason unrelated to the code being
        # measured. Refused rather than shared: on Linux the leak would otherwise be
        # measured under the next cell's factors.
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

    record.cpu_mhz_end = probes.cpu_mhz()
    record.thermal_speed_limit_end = probes.speed_limit()
    # After the run for the same reason as the two above: the preflight read it once,
    # and a governor that moved mid-campaign was caught only at the next invocation.
    record.governor = probes.governor()
    record.counter_deltas = validity.counter_deltas(counters_before, probes.counters())

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
    probes: HostProbes = LIVE_PROBES,
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
            probes=probes,
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
