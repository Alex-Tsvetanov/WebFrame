"""Driver checks, against fakes rather than real servers.

The rules the driver enforces are the ones the old harness broke, and every one of them
can be checked without h2load, nginx or a network. Fakes here are not a compromise
forced by the machine: a real server would make these checks slower, flakier and worse
at telling you which rule was violated.

Imported and run by selfcheck.py.
"""

from __future__ import annotations

import functools
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Callable

from benchmark.harness import driver, ordering, schema

# Everything here runs against a fake server and a fake generator, so it reads a fake
# host too. Without this the check picked up the real machine underneath it and the run
# it asserts must be accepted was rejected for being on battery, which made the gate
# unusable on exactly the kind of host that most needs gating before a campaign.
_run_one = functools.partial(driver.run_one, probes=driver.IDLE_PROBES)
_run_campaign = functools.partial(driver.run_campaign, probes=driver.IDLE_PROBES)


@dataclass
class FakeServer:
    """Counts what happened to it, so the driver's discipline is observable."""

    cell: ordering.Cell
    log: list[str]
    ready: bool = True
    fail_on_stop: bool = False
    # What a real adapter raises when the child died before listening.
    died: str | None = None
    argv_value: list[str] = field(default_factory=lambda: ["fake-server"])

    @property
    def argv(self) -> list[str]:
        return self.argv_value

    def start(self) -> None:
        self.log.append("start")

    def wait_until_ready(self, timeout_s: float) -> bool:
        self.log.append("ready-check")
        if self.died:
            raise driver.RunFailed(f"server exited -6 (Abort trap: 6) before listening: {self.died}")
        return self.ready

    def stop(self) -> driver.ResourceUsage:
        self.log.append("stop")
        if self.fail_on_stop:
            raise OSError("could not stop")
        return driver.ResourceUsage(cpu_seconds=12.5, memory_peak_bytes=1 << 26,
                                    quic={"received": 100, "forwarded_in": 3})


@dataclass
class FakeGenerator:
    name_value: str = "fake"
    raises: bool = False
    result: driver.GeneratorResult = field(
        default_factory=lambda: driver.GeneratorResult(
            requests_total=100_000, requests_total_whole_run=115_000, requests_non_2xx=0,
            requests_per_second=3333.0, latency_ms={"p50": 1.2, "p99": 4.5},
            cpu_fraction=0.4, argv=["fake", "-c", "256"],
        )
    )

    @property
    def name(self) -> str:
        return self.name_value

    def run(self, cell: ordering.Cell, duration_s: float) -> driver.GeneratorResult:
        if self.raises:
            raise RuntimeError("generator exploded")
        return self.result


def _env() -> dict[str, Any]:
    return {
        "virtualisation": None,
        "build": {"git_dirty": False, "git_commit": "c" * 40, "type": "Release"},
        "toolchain": {"compiler": "g++ 14.2.0"},
        "deps": {"openssl": "3.5.8", "ngtcp2": "1.25.0"},
    }


def _cell(**factors: Any) -> ordering.Cell:
    base = dict(protocol="http1.1", io_backend="io_uring", workers=6,
                connections=256, tls=False)
    base.update(factors)
    return ordering.Cell.of("coroute", **base)


def _scheduled(cell: ordering.Cell | None = None, repetition: int = 0) -> ordering.ScheduledRun:
    return ordering.ScheduledRun(repetition=repetition, index_in_repetition=0,
                                 cell=cell or _cell())


def run(check: Callable[[str, bool], None]) -> None:
    print("\n== the driver enforces the rules the old harness broke ==")

    # --- a run is a process --------------------------------------------------
    logs: list[list[str]] = []

    def factory(cell: ordering.Cell) -> FakeServer:
        log: list[str] = []
        logs.append(log)
        return FakeServer(cell=cell, log=log)

    cells = [_cell(), _cell(protocol="http2")]
    plan = ordering.plan(cells, repetitions=3, seed=7)

    with tempfile.TemporaryDirectory() as tmp:
        results = Path(tmp) / "runs.jsonl"
        records = _run_campaign(
            plan, results_path=results, server_factory=factory,
            generator=FakeGenerator(), environment=_env(),
            campaign_fingerprint="fp", duration_s=30.0,
        )

        # Six scheduled runs, six servers. The old script would have made one.
        check("a fresh server per run", len(logs) == 6)
        check("every server was started and stopped",
              all(log == ["start", "ready-check", "stop"] for log in logs))
        check("a record per run", len(records) == 6)
        check("all were accepted", all(r.accepted for r in records))

        # Written as they complete, not batched: a campaign is hours, and a reboot at
        # hour six must not cost the first five.
        on_disk = list(schema.read(results))
        check("records reach disk during the campaign", len(on_disk) == 6)
        check("run ids are unique", len({r.run_id for r in on_disk}) == 6)
        check("the campaign fingerprint is on every record",
              all(r.campaign_fingerprint == "fp" for r in on_disk))
        check("factors survive into the record",
              {r.protocol for r in on_disk} == {"http1.1", "http2"})
        check("provenance is carried",
              all(r.git_commit == "c" * 40 and r.compiler for r in on_disk))
        check("server-side resource usage is recorded",
              all(r.server_cpu_seconds == 12.5 for r in on_disk))
        # The denominator for a process-lifetime server counter is the whole-run count,
        # not the measured window; both are carried so neither has to be inferred.
        check("the whole-run response count is carried beside the measured one",
              all(r.requests_total_whole_run == 115_000 and r.requests_total == 100_000
                  for r in on_disk))
        check("QUIC counters are carried",
              all(r.quic_forwarded_in == 3 for r in on_disk))

    # --- the server always stops --------------------------------------------
    log: list[str] = []
    record = _run_one(
        _scheduled(),
        server_factory=lambda cell: FakeServer(cell=cell, log=log),
        generator=FakeGenerator(raises=True), environment=_env(),
        campaign_fingerprint="fp", duration_s=30.0,
    )
    # A leaked server holds the port, and then every later run in the campaign fails
    # for a reason that has nothing to do with the code being measured.
    check("the server is stopped even when the generator raises", "stop" in log)
    check("a failed run is still a record", isinstance(record, schema.RunRecord))
    check("and is marked rejected", not record.accepted)
    check("with the reason kept",
          any("generator exploded" in r for r in record.rejection_reasons))

    # --- a server that never comes up ---------------------------------------
    log = []
    record = _run_one(
        _scheduled(),
        server_factory=lambda cell: FakeServer(cell=cell, log=log, ready=False),
        generator=FakeGenerator(), environment=_env(),
        campaign_fingerprint="fp", duration_s=30.0, readiness_timeout_s=0.1,
    )
    check("a server that never listens is rejected", not record.accepted)
    check("the reason says so", any("ready" in r for r in record.rejection_reasons))
    check("it is still stopped", "stop" in log)
    # Otherwise the first moments of every run measure process startup, and that cost
    # lands on whichever system boots slowest.
    check("the generator never ran", record.requests_total == 0)

    # A child that died is not a child that was slow. The two used to share one reason,
    # so an OOM-killed server read as a readiness timeout.
    log = []
    record = _run_one(
        _scheduled(),
        server_factory=lambda cell: FakeServer(cell=cell, log=log, died="std::bad_alloc"),
        generator=FakeGenerator(), environment=_env(),
        campaign_fingerprint="fp", duration_s=30.0, readiness_timeout_s=0.1,
    )
    check("a server that died before listening is rejected", not record.accepted)
    check("with the exit and its stderr, not a timeout",
          any("before listening" in r and "bad_alloc" in r for r in record.rejection_reasons)
          and not any("within" in r for r in record.rejection_reasons))
    check("and is still stopped", "stop" in log)

    # --- a failure while stopping does not hide the real one ----------------
    record = _run_one(
        _scheduled(),
        server_factory=lambda cell: FakeServer(cell=cell, log=[], fail_on_stop=True),
        generator=FakeGenerator(raises=True), environment=_env(),
        campaign_fingerprint="fp", duration_s=30.0,
    )
    joined = " ".join(record.rejection_reasons)
    check("the original failure is reported first", joined.index("exploded") < joined.index("stop"))
    check("and the stop failure is reported too", "could not stop" in joined)

    # --- guards decide -------------------------------------------------------
    record = _run_one(
        _scheduled(),
        server_factory=lambda cell: FakeServer(cell=cell, log=[]),
        generator=FakeGenerator(result=driver.GeneratorResult(
            requests_total=1_000_000, requests_non_2xx=50_000,
            requests_per_second=9999.0, cpu_fraction=0.3)),
        environment=_env(), campaign_fingerprint="fp", duration_s=30.0,
    )
    # Failing is cheaper than serving, so without this the collapsing server wins.
    check("a run with too many errors is rejected by the guards", not record.accepted)
    check("even though the generator reported a high rate",
          record.requests_per_second == 9999.0)

    virtualised = dict(_env(), virtualisation="kvm")
    record = _run_one(
        _scheduled(),
        server_factory=lambda cell: FakeServer(cell=cell, log=[]),
        generator=FakeGenerator(), environment=virtualised,
        campaign_fingerprint="fp", duration_s=30.0,
    )
    check("a run under virtualisation is rejected", not record.accepted)

    # --- rejected runs are kept and counted ---------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        results = Path(tmp) / "mixed.jsonl"
        good_plan = ordering.plan([_cell()], repetitions=2, seed=1)
        _run_campaign(good_plan, results_path=results,
                            server_factory=lambda c: FakeServer(cell=c, log=[]),
                            generator=FakeGenerator(), environment=_env(),
                            campaign_fingerprint="fp", duration_s=30.0)
        _run_campaign(good_plan, results_path=results,
                            server_factory=lambda c: FakeServer(cell=c, log=[]),
                            generator=FakeGenerator(raises=True), environment=_env(),
                            campaign_fingerprint="fp", duration_s=30.0)

        everything = list(schema.read(results))
        check("failed runs are on disk too", len(everything) == 4)

        summary = driver.summarise(everything)
        check("the summary counts what was rejected",
              summary["runs"] == 4 and summary["accepted"] == 2 and summary["rejected"] == 2)
        # A campaign that discarded a third of its runs is a different campaign from one
        # that discarded none, and only reporting it makes the difference visible.
        check("and groups the reasons", len(summary["rejection_reasons"]) == 1)
