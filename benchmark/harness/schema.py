"""One row per run, and everything needed to defend it.

Two rules the old harness broke, both worth stating as rules.

Nothing lives in a filename. The previous aggregator recovered a run's identity by
parsing the name of its artefact, which is why that schema could not express protocol,
TLS or offered rate: adding a factor meant inventing a new naming convention, and a
name that does not parse is a run that silently vanishes. Every factor is a field here.

A run is a process, not a sample. The old script started one server and measured it five
times, so n=5 was really n=1: five looks at one process's page placement and allocator
luck. A record is emitted per fresh server and fresh generator, and `repetition`
distinguishes them.

Storage is JSONL rather than Parquet, which is a deliberate change from the plan.
Parquet is columnar, so appending mid-campaign means rewriting or juggling row groups,
and a campaign killed halfway leaves a file that may not open. A JSONL append under
O_APPEND is atomic per line, so an interrupted campaign loses at most the run that was
in flight and every earlier line stays readable. Parquet is the analysis format and is
produced by conversion at the end, when the data is complete and immutable, which is
when columnar storage is actually good. to_parquet() is here for that step and imports
pyarrow only when called.
"""

from __future__ import annotations

import json
import os
import uuid
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any, Iterator


# 2 added the routing factors. Bumped rather than left alone because a reader of an
# older file has to be able to tell that those columns are absent rather than null.
SCHEMA_VERSION = 2


@dataclass
class RunRecord:
    """A single measured run.

    Grouped by what each field is for, because the groups have different rules. Factors
    are what was varied and must be complete or the run cannot be placed in the design.
    Outcomes are what was observed. Validity is what decides whether the outcomes count.
    Provenance is what makes the run repeatable by someone else.
    """

    # --- Identity -----------------------------------------------------------
    run_id: str = field(default_factory=lambda: uuid.uuid4().hex)
    schema_version: int = SCHEMA_VERSION
    campaign_fingerprint: str = ""
    started_unix: float = 0.0
    # Which pass through the design this run belongs to. Runs sharing a repetition were
    # near each other in time, which is what makes thermal drift visible rather than
    # silently correlated with whichever system was measured last.
    repetition: int = 0

    # --- Factors ------------------------------------------------------------
    # What was varied. A missing factor is not a null, it is a run that cannot be placed
    # in the design, which is why they are all here rather than inferred from a path.
    system: str = ""             # coroute, nginx, h2o, actix, ...
    protocol: str = ""           # http1.1, http2, http3
    tls: bool = False
    io_backend: str = ""         # io_uring, epoll, iocp, kqueue
    protocol_detection: bool = True   # the demultiplexer A/B arm
    workers: int = 0
    connections: int = 0
    streams_per_connection: int = 1   # h2 and h3 only; the central variable there
    payload_bytes: int = 0
    backlog: int = 0
    duration_s: float = 0.0
    # None means closed loop: the generator sent as fast as it could. A number means
    # open loop at that offered rate. The distinction decides whether the latency
    # figures are service time or response time, so it cannot be left implicit.
    offered_rate: float | None = None
    netem_profile: str = "none"  # none, or a named delay/loss/jitter profile

    # --- Routing factors ----------------------------------------------------
    # Fields rather than free text in notes, on the same rule as everything above: a
    # factor recoverable only by parsing prose is a factor the analysis cannot group by.
    # router_arm is empty on runs that predate the routing experiment.
    router_arm: str = ""          # dfa, radix, regex
    route_count: int = 0
    route_shape: str = ""         # rest (shared prefix) or flat (branches immediately)
    route_params: bool = False    # routes end in a {id} capture
    route_depth: int = 0          # path segments per route

    # --- Outcomes -----------------------------------------------------------
    requests_total: int = 0
    requests_non_2xx: int = 0
    socket_errors: int = 0
    requests_per_second: float = 0.0
    bytes_per_second: float = 0.0

    # Percentiles are a summary; the histogram is the data. Kept for every run because
    # it is a few kilobytes and lossless to three significant digits, while raw
    # per-request samples are 18 million rows for a minute at 300k rps and are kept only
    # for the headline cells and every migration run.
    latency_ms: dict[str, float] = field(default_factory=dict)
    latency_histogram: str | None = None   # HdrHistogram, base64
    raw_samples_path: str | None = None

    # --- QUIC ---------------------------------------------------------------
    # forwarded_in over received is the number that decides whether kernel-side
    # connection-ID steering would be worth building.
    quic_datagrams_received: int = 0
    quic_forwarded_out: int = 0
    quic_forwarded_in: int = 0
    quic_migrations: int = 0
    quic_stateless_resets: int = 0

    # --- Resources ----------------------------------------------------------
    # From cgroup v2 rather than ps. The old harness read lifetime-average percent on
    # one platform and cumulative CPU seconds on the other, then plotted both into one
    # column on one axis. These are kernel-tracked and mean one thing.
    server_cpu_seconds: float | None = None
    server_memory_peak_bytes: int | None = None
    generator_cpu_fraction: float | None = None
    # How far behind its own schedule the generator fell, in microseconds at the
    # 99th percentile, measured from when a request was due to when it reached the
    # socket. This is the saturation signal for an open loop, where CPU is not: an
    # open loop paces by spinning and is at full CPU by construction.
    generator_pacing_p99_us: float | None = None
    # Achieved divided by offered. An open loop that could not keep up was offering
    # a different load than the one this record claims.
    generator_achieved_share: float | None = None

    # --- Validity -----------------------------------------------------------
    virtualisation: str | None = None
    git_dirty: bool = False
    cpu_mhz_start: float | None = None
    cpu_mhz_end: float | None = None
    # What isolation was asked for and what the platform granted. These are separate
    # fields because on macOS they differ: there is no user-process CPU affinity API,
    # so a mask can be requested and silently not applied. Chapter V promises the reader
    # that the run records both, so that an unpinned run is distinguishable from a
    # pinned one rather than assumed to be either.
    affinity_requested: str | None = None
    affinity_applied: bool | None = None
    # A laptop can change its power and thermal regime mid-campaign in a way the desktop
    # could not. On Apple Silicon, discharging biases scheduling toward efficiency cores
    # and caps clocks, which makes the number uncitable for the same reason a virtualised
    # host does: the machine under the measurement is not the machine being described.
    power_source: str | None = None
    thermal_speed_limit_start: int | None = None
    thermal_speed_limit_end: int | None = None
    counter_deltas: dict[str, int] = field(default_factory=dict)
    accepted: bool = True
    rejection_reasons: list[str] = field(default_factory=list)

    # --- Provenance ---------------------------------------------------------
    # Enough for someone else to rebuild the same thing. The generator's argv is
    # included verbatim because a summary of how load was applied is not the same as
    # how load was applied.
    git_commit: str = ""
    build_type: str = ""
    compiler: str = ""
    dependency_versions: dict[str, str] = field(default_factory=dict)
    generator: str = ""
    generator_argv: list[str] = field(default_factory=list)
    server_argv: list[str] = field(default_factory=list)
    notes: str = ""

    def missing_factors(self) -> list[str]:
        """Factors left unset, which make the run unplaceable in the design.

        Checked before writing rather than at analysis time. A run missing its protocol
        is not a row with a gap, it is a row nobody can interpret, and discovering that
        weeks later means the machine time is gone.
        """
        required = {
            "system": self.system,
            "protocol": self.protocol,
            "io_backend": self.io_backend,
            "workers": self.workers,
            "connections": self.connections,
            "duration_s": self.duration_s,
        }
        return [name for name, value in required.items() if not value]

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


def append(path: Path, record: RunRecord, *, allow_incomplete: bool = False) -> None:
    """Appends one record as a line of JSON.

    Refuses a record missing a factor unless told otherwise, because the cheapest moment
    to notice is now. One write of one line, so a campaign interrupted here loses this
    run and keeps every earlier one.
    """
    missing = record.missing_factors()
    if missing and not allow_incomplete:
        raise ValueError(
            f"run {record.run_id} is missing factors: {', '.join(missing)}. "
            "A run that cannot be placed in the design is not a data point."
        )

    path.parent.mkdir(parents=True, exist_ok=True)
    line = json.dumps(record.to_dict(), sort_keys=True, separators=(",", ":"))

    # A crash mid-write leaves a line with no terminating newline. Appending straight
    # onto that would join the fragment and this record into one unparseable line, so
    # the torn run would take a good one down with it. Writing a newline first keeps the
    # damage to the run that was actually interrupted.
    if _ends_mid_line(path):
        line = "\n" + line

    # O_APPEND so concurrent writers cannot interleave a partial line.
    with os.fdopen(os.open(path, os.O_WRONLY | os.O_CREAT | os.O_APPEND, 0o644), "a",
                   encoding="utf-8") as handle:
        handle.write(line + "\n")


def _ends_mid_line(path: Path) -> bool:
    """Whether the file exists, is non-empty, and does not end with a newline."""
    try:
        size = path.stat().st_size
    except OSError:
        return False
    if size == 0:
        return False
    with path.open("rb") as handle:
        handle.seek(-1, os.SEEK_END)
        return handle.read(1) != b"\n"


def read(path: Path, *, skip_malformed: bool = True) -> Iterator[RunRecord]:
    """Reads records back.

    A malformed trailing line is skipped rather than fatal by default: it is what a
    campaign killed mid-write leaves behind, and refusing to read the whole file because
    of it would throw away everything that did complete.
    """
    if not path.exists():
        return
    fields = {f for f in RunRecord.__dataclass_fields__}
    with path.open(encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if not line:
                continue
            try:
                raw = json.loads(line)
            except json.JSONDecodeError:
                if skip_malformed:
                    continue
                raise
            # Unknown keys are dropped rather than fatal, so a file written by a later
            # schema still reads. Missing keys keep their defaults.
            yield RunRecord(**{k: v for k, v in raw.items() if k in fields})


def accepted_only(records: Iterator[RunRecord]) -> list[RunRecord]:
    """The runs that passed the validity rules.

    Rejected runs stay in the file. They are the evidence for the rejection count that
    has to be reported alongside the results: a method that silently discards a third of
    its runs is a different method from one that discards none.
    """
    return [r for r in records if r.accepted]


def to_parquet(path: Path, out: Path) -> None:
    """Converts a finished campaign to Parquet for analysis.

    Imports pyarrow here rather than at module scope, so collecting data does not
    require the analysis stack. The measurement machine should carry as little as
    possible; the machine doing the plotting can carry more.
    """
    try:
        import pyarrow as pa
        import pyarrow.parquet as pq
    except ImportError as exc:  # pragma: no cover - depends on the host
        raise RuntimeError(
            "pyarrow is needed to write Parquet. The JSONL log is the authority and is "
            "readable without it; install pyarrow on the analysis machine."
        ) from exc

    rows = [r.to_dict() for r in read(path)]
    if not rows:
        raise ValueError(f"no records in {path}")
    pq.write_table(pa.Table.from_pylist(rows), out)
