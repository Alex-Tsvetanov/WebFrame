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
#
# 3 added the TLS arm: the per-connection request limit as a factor, and connection
# establishment as an outcome in its own right. A file at version 2 or below has no
# connect_ms column rather than an empty one, and every run in it is tls=False because
# the harness could not produce anything else.
#
# 4 added requests_total_whole_run, the response count over the generator's whole
# lifetime, and made git_dirty nullable. A file at version 3 or below has no whole-run
# count, so a process-lifetime server counter from it cannot be normalised per request;
# and its git_dirty=false may be a tree that was read as clean or one that could not be
# read at all, which from version 4 on are distinct values. The same generator change
# moved bytes_per_second onto the measured window: at version 3 it divided the whole
# run's bytes, warmup included, by the measured wall, so a version 3 figure is higher
# than a version 4 one by about (warmup + duration) / duration and the two must not be
# pooled. The generator defines both fields, so a record's version says what the driver
# was, and requests_total_whole_run being null says the generator was older. Version 4
# also carries the governor per run; earlier files have it in the manifest only.
#
# 5 added the syscall counter: syscall_counts, the per-tracepoint totals over the
# generator's whole lifetime, and syscall_counter, a string naming the instrument and
# the privilege it ran with. Absent rather than empty at version 4 and below, which is
# the distinction that matters: an empty dict there would say a run was counted and
# found nothing, and a run that was never counted is not the same as a server that
# issued no syscalls. The counts are normalised by requests_total_whole_run, not by
# requests_total, because the counter spans the generator's warmup as well as its
# measured window and dividing a whole-lifetime count by a measured-window count would
# inflate syscalls per request by roughly (warmup + duration) / duration.
#
# 6 added generator_euid, which says who ran the load rather than who was asked to. A
# file at version 5 or below cannot answer it: the namespace arrangement asserts that the
# generator drops back to the invoking user while the server stays root, and until this
# version nothing measured either half, so a run there whose prefix silently failed to
# drop is indistinguishable in a version 5 record from one that dropped correctly.
#
# 7 added server_euid, the other half of that pair. Version 6 measured who ran the load
# but nothing measured who ran the server, so the arrangement's central claim rested on a
# free-text server_location string. This became load-bearing rather than merely tidy once
# root turned out to be exempt from a limit an unprivileged server is not: io_uring
# charges ring memory against a per-user RLIMIT_MEMLOCK budget, and a root server never
# meets the ceiling an unprivileged one hits. A record that does not say which user the
# server ran as cannot be interpreted on that axis, and a version 6 file cannot answer it
# retrospectively.
#
# Note that this reverses the arrangement described above. That paragraph says the server
# stays root, which was true while io_uring needed CAP_SYS_ADMIN on the hardened kernel
# the rig then ran. It no longer does, both ends now run as the invoking user, and a root
# server is the exception rather than the rule. From this version the driver refuses a run
# whose two euids differ unless the cell declares the asymmetry, so an accidental root
# server is a failed run rather than a quietly faster one.
# 8 added local_interface and the link's speed, duplex and MTU, read from the generator's
# own socket with getsockname after the connection is established. The arrangement was
# asserted by a route metric and nothing else. Where a host has more than one interface on
# the same subnet, which of them carries a run is decided by a number DHCP can change at a
# lease renewal, and a run that moved from a wired link to a wireless one would still
# produce a plausible figure with nothing in the record to say the medium had changed. server_location
# recorded a label, which is an intention rather than an event.
#
# Recorded per run rather than per campaign because the failure it guards against is a
# medium changing partway through, which a campaign-level field cannot express. A run
# whose interface is not the expected one is refused rather than annotated: a medium we
# did not intend is worse than one we do not know. Names and link properties only, never
# addresses or MACs, and absent rather than refused off Linux.
SCHEMA_VERSION = 8


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
    # Requests the server serves on one connection before closing it. Zero is unlimited,
    # which is what every keep-alive run uses.
    #
    # A factor rather than a constant because it decides what the run is a measurement
    # of. Classification happens once per connection, so at zero its cost is divided by
    # every request the connection went on to serve and cannot be seen. At one, every
    # request pays for a fresh accept and a fresh classification, and the difference
    # between the arms is no longer divided by anything.
    max_requests_per_connection: int = 0

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
    # Every response the generator saw, warmup included. requests_total is the measured
    # window, and dividing a counter that runs for the server's whole lifetime by it
    # inflates the ratio by (warmup + duration) / duration. None where the generator did
    # not report it.
    requests_total_whole_run: int | None = None
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

    # --- Connection establishment -------------------------------------------
    # Kept apart from latency_ms because it answers the question latency_ms cannot.
    # Request latency across a keep-alive connection divides the per-connection cost of
    # classification by every request that connection served; this divides it by
    # nothing. On the TLS arm it includes the handshake, which is the dominant term and
    # is exactly what the classification octet has to be visible against.
    #
    # Empty on a run whose connections were all made during the warmup, which is the
    # ordinary keep-alive case. connections_established says so rather than leaving an
    # empty distribution to be read as a zero.
    connect_ms: dict[str, float] = field(default_factory=dict)
    connections_established: int = 0
    handshake_failures: int = 0
    # What was negotiated, read off the connection rather than assumed from the build.
    # A record that says tls=True without saying which version and cipher describes a
    # measurement nobody can reproduce.
    tls_version: str = ""
    tls_cipher: str = ""

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
    # None when git could not say, and refused as such. The default is None so a record
    # that never set it is refused rather than read as clean.
    git_dirty: bool | None = None
    # Read after the run, not once at preflight. The manifest fingerprints the governor,
    # so a change is caught at the next invocation; a daemon or a suspend cycle that
    # flips it at run 10 of 25 would otherwise pass the remaining fifteen. None on
    # Windows and macOS, which publish none; unchecked on a Linux host that could not
    # read it, and refused as such.
    governor: str | None = None
    # A number, or the probe's unchecked string on a platform that should have read one.
    # ponytail: a str in a float column breaks to_parquet's type inference; such a file
    # is all rejections anyway, and the string is what the rejection reason cites.
    cpu_mhz_start: float | str | None = None
    cpu_mhz_end: float | str | None = None
    # What isolation was asked for and what the platform granted. These are separate
    # fields because on macOS they differ: there is no user-process CPU affinity API,
    # so a mask can be requested and silently not applied. Chapter V promises the reader
    # that the run records both, so that an unpinned run is distinguishable from a
    # pinned one rather than assumed to be either.
    affinity_requested: str | None = None
    affinity_applied: bool | None = None
    # Who ran the load, as the generator reports it. None on Windows, which has no such
    # id, and on a run whose generator predates the field. Under a launch prefix a None
    # or a zero is refused: the whole arrangement rests on the generator not being root.
    generator_euid: int | None = None
    # Who ran the server, read from /proc rather than inferred from the launch prefix.
    # Compared against generator_euid above: the two must match unless the cell declares
    # otherwise, since a root server is exempt from limits an unprivileged one is held to.
    #
    # None means the platform has no such notion -- off Linux, or an adapter predating
    # this field -- and nothing else. On Linux a /proc read that fails refuses the run
    # rather than recording None, because "the server's privilege could not be
    # established" and "this platform does not have uids" must not arrive here as the
    # same value. Under a launch prefix None is refused outright: see driver.py.
    server_euid: int | None = None
    # Why the two euids above are allowed to differ, when they do. A validity field
    # rather than a factor: it explains a run, it does not vary within a design, and
    # putting it in factors would make every declared cell a different design point from
    # its undeclared twin and break missing_factors.
    #
    # None means no asymmetry was declared, which for a run whose euids match is the
    # ordinary case. A string is the operator's stated reason, recorded verbatim so the
    # file says why rather than merely that it was allowed.
    privilege_asymmetry: str | None = None
    # The wire the run actually went over, read from the generator's socket rather than
    # from configuration. Per run rather than per campaign on purpose: the failure this
    # guards against is a medium changing underneath a campaign, which a campaign-level
    # record could not express. None off Linux and from an older generator.
    local_interface: str | None = None
    local_interface_speed_mbit: str | None = None
    local_interface_duplex: str | None = None
    local_interface_mtu: str | None = None
    # A laptop can change its power and thermal regime mid-campaign in a way the desktop
    # could not. On Apple Silicon, discharging biases scheduling toward efficiency cores
    # and caps clocks, which makes the number uncitable for the same reason a virtualised
    # host does: the machine under the measurement is not the machine being described.
    power_source: str | None = None
    thermal_speed_limit_start: int | None = None
    thermal_speed_limit_end: int | None = None
    counter_deltas: dict[str, int] = field(default_factory=dict)

    # Syscalls the server issued while the generator was running, per tracepoint, and
    # what measured them. Empty and None on a run that did not ask for counting, which
    # is every run of a timed comparison: a per-syscall tracepoint costs time roughly in
    # proportion to the syscall rate, which is the quantity being compared, so counting
    # during a comparison would change the thing it is comparing.
    syscall_counts: dict[str, int] = field(default_factory=dict)
    syscall_counter: str | None = None
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
