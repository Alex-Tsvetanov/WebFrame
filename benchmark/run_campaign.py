"""Runs a campaign and writes one record per run.

    python -m benchmark.run_campaign --design h1 --repetitions 7

What this does that the previous script did not: a fresh server and a fresh generator
for every run, a randomised order that differs per repetition, an environment
fingerprint that stops the campaign if the machine changed underneath it, and a record
for every run including the rejected ones.

On the design of the Windows campaign specifically. This host has six physical cores and
must run both the server and the generator, so the two are pinned to disjoint sets and
neither gets the whole machine. That rules out absolute throughput as a claim: what it
supports is comparisons, where both arms see the same conditions and the only thing that
differs is the factor under test. The offered rates are chosen below the point where the
generator falls behind its own schedule, which was measured at about 75 thousand requests
per second with two generator threads on this host.
"""

from __future__ import annotations

import argparse
import json
import os
import platform
import sys
from pathlib import Path

from benchmark.adapters import CorouteServer, LoadgenGenerator
from benchmark.harness import driver, environment, schema
from benchmark.harness.io_report import (
    ZERO_COPY_SIZES,
    select_open_loop_rate,
    write_report,
)
from benchmark.harness.ordering import Cell, plan


REPO = Path(__file__).resolve().parents[1]

# Ryzen 5 3600: twelve logical CPUs over six physical cores, paired. 0x0FF is logical
# 0 to 7, four physical cores, for the server. 0xF00 is logical 8 to 11, two physical
# cores, for the generator. Disjoint, so the two do not take work from each other.
_WINDOWS_SERVER_AFFINITY = "0ff"
_WINDOWS_GENERATOR_AFFINITY = "f00"
GENERATOR_THREADS = 2


def _linux_affinity_masks() -> tuple[str | None, str | None]:
    """Disjoint masks for this host's CPU count.

    The Windows masks must not be reused on Linux: 0xF00 asks for CPUs that do not
    exist on a 4-CPU box, and 0x0FF collapses to every CPU, so both processes share.
    """
    n = os.cpu_count() or 0
    if n >= 4:
        return "3", "c"  # server CPUs 0-1, generator CPUs 2-3
    if n == 2:
        return "1", "2"
    return None, None


# macOS has no CPU affinity API for user processes. Asking for nothing is the honest
# encoding of a platform that grants nothing.
_SYSTEM = platform.system()
if _SYSTEM == "Windows":
    SERVER_AFFINITY = _WINDOWS_SERVER_AFFINITY
    GENERATOR_AFFINITY = _WINDOWS_GENERATOR_AFFINITY
elif _SYSTEM == "Linux":
    SERVER_AFFINITY, GENERATOR_AFFINITY = _linux_affinity_masks()
else:
    SERVER_AFFINITY = None
    GENERATOR_AFFINITY = None


def system_name() -> str:
    r"""The name every generated \R{} key is prefixed with.

    Windows keeps the bare name because the campaign already published under it and the
    thesis cites keys of the form coroute.h1.*. The other two are suffixed because the
    thesis already cites coroute-linux.* and coroute-macos.* as the measurements that are
    still to come. Deriving this from the host rather than from a flag is deliberate: a
    Mac run that inherited the Windows name would overwrite published numbers with
    numbers from a different machine, and nothing downstream could tell.
    """
    return {"Darwin": "coroute-macos", "Linux": "coroute-linux"}.get(
        platform.system(), "coroute"
    )

# Below the point where the generator stops keeping up. Measured, not chosen: above
# about 75k on this host its pacing lag at p99 goes from tens of microseconds to
# milliseconds, and validity.py refuses those runs.
OFFERED_RATES = (10_000, 25_000, 40_000, 55_000, 70_000)

# Linux I/O-portability workers match the server affinity width on this 4-CPU box.
_IO_WORKERS = 2 if (os.cpu_count() or 0) >= 4 else 1


def _io_backend() -> str:
    """The backend the presets select for this host.

    Recorded rather than assumed, because it is a factor in the record and a mislabelled
    factor is worse than a missing one: it makes two different measurements look like
    repetitions of one.
    """
    return {"Darwin": "kqueue", "Linux": "io_uring"}.get(platform.system(), "iocp")


def _default_build() -> Path:
    name = {
        "Darwin": "macos-release",
        "Linux": "linux-release",
        "Windows": "windows-release",
    }.get(platform.system(), "windows-release")
    return REPO / "build" / name


def _default_results(design: str) -> Path:
    if design.startswith("io-"):
        return REPO / "benchmark" / "results" / "linux-io-portability.jsonl"
    return REPO / "benchmark" / "results" / "runs.jsonl"


def _io_base(**extra) -> dict:
    """Shared factors for the Linux I/O-portability designs."""
    base = dict(
        protocol="http1.1",
        tls=False,
        workers=_IO_WORKERS,
        connections=64,
        payload_bytes=0,
        backlog=1024,
        streams_per_connection=1,
        netem_profile="none",
        protocol_detection=True,
        write_path="buffered",
    )
    base.update(extra)
    return base


def design_windows_h1() -> list[Cell]:
    """The comparison this host can actually support.

    Three groups, each varying one factor around a fixed baseline, which is what the
    plan calls for rather than a cross product: the full factorial here would be
    thousands of cells and most of them would answer nothing.
    """
    cells: list[Cell] = []

    base = dict(
        protocol="http1.1",
        tls=False,
        io_backend=_io_backend(),
        workers=4,
        connections=64,
        payload_bytes=0,
        backlog=1024,
        streams_per_connection=1,
        netem_profile="none",
    )

    # H1: the cost of classifying after accept, both arms from one binary at identical
    # offered load. This is the hypothesis the whole dissertation turns on.
    for rate in OFFERED_RATES:
        for detect in (True, False):
            cells.append(Cell.of(system_name(), **base, protocol_detection=detect, offered_rate=rate))

    # Worker scaling at a fixed offered rate, to show where the server stops benefiting
    # from more threads on four physical cores.
    for workers in (1, 2, 4, 8):
        if workers == base["workers"]:
            continue
        cells.append(Cell.of(system_name(), **{**base, "workers": workers},
                             protocol_detection=True, offered_rate=40_000))

    # Response size, because the classification cost is per connection and the response
    # cost is per request: the ratio between them should move.
    for payload in (256, 1024, 8192):
        cells.append(Cell.of(system_name(), **{**base, "payload_bytes": payload},
                             protocol_detection=True, offered_rate=40_000))

    # Listen backlog. The default in the io context was 128, which is far too small at
    # several thousand concurrent connections, so it is a swept variable rather than a
    # constant. At this offered rate and connection count the queue is shallow and the
    # sweep is expected to show little; measuring it is how that becomes a finding
    # rather than an assumption.
    for backlog in (128, 512, 4096):
        cells.append(Cell.of(system_name(), **{**base, "backlog": backlog},
                             protocol_detection=True, offered_rate=40_000))

    return cells


def design_windows_h1_deep() -> list[Cell]:
    """Only the cells hypothesis X1 turns on, so they can be run many more times.

    The first campaign ran every cell seven times and the difference intervals came out
    between 4.7 and 11.8 percent of the median, depending on the offered rate. X1's own
    rejection threshold is five percent, so at seven repetitions the design could not
    resolve the difference it was declared to look for at four of the five rates. That is
    a fact about the design and not about the result, and the fix is repetitions: the
    interval narrows with the square root of their number.

    Ten cells rather than nineteen, because spending the same machine time on the sweeps
    would buy resolution where no claim depends on it.
    """
    base = dict(
        protocol="http1.1", tls=False, io_backend=_io_backend(), workers=4, connections=64,
        payload_bytes=0, backlog=1024, streams_per_connection=1, netem_profile="none",
    )
    return [
        Cell.of(system_name(), **base, protocol_detection=detect, offered_rate=rate)
        for rate in OFFERED_RATES
        for detect in (True, False)
    ]


def design_ladder() -> list[Cell]:
    """Where this host's generator stops keeping up, measured rather than assumed.

    The 70k ceiling in the campaigns above is a property of a Ryzen 5 3600 with the
    generator pinned to two dedicated cores. Nothing about it transfers to another
    machine, and on a host where the generator cannot be pinned it does not even
    transfer to itself under different load. Run this first on any new host: the
    campaign's top offered rate has to sit comfortably below the point where
    generator_pacing_p99_us leaves the tens of microseconds.

    One repetition per rate, because this is looking for a cliff, not a difference.
    """
    base = dict(
        protocol="http1.1", tls=False, io_backend=_io_backend(), workers=4,
        connections=64, payload_bytes=0, backlog=1024, streams_per_connection=1,
        netem_profile="none",
    )
    return [
        Cell.of(system_name(), **base, protocol_detection=True, offered_rate=rate)
        for rate in range(10_000, 130_000, 10_000)
    ]


def design_smoke() -> list[Cell]:
    """Two cells, for checking the machinery without spending an hour on it."""
    base = dict(
        protocol="http1.1", tls=False, io_backend=_io_backend(), workers=4, connections=64,
        payload_bytes=0, backlog=1024, streams_per_connection=1, netem_profile="none",
    )
    return [
        Cell.of(system_name(), **base, protocol_detection=True, offered_rate=10_000),
        Cell.of(system_name(), **base, protocol_detection=False, offered_rate=10_000),
    ]


def design_io_ladder() -> list[Cell]:
    """Where this Linux host's generator stops keeping up, for the I/O session.

    One repetition per rate. Uses io_uring buffered as the probe arm; the cliff is a
    property of the generator on this machine, not of the backend under comparison.
    """
    base = _io_base(io_backend="io_uring", study="ladder")
    return [
        Cell.of(system_name(), **base, offered_rate=rate)
        for rate in range(10_000, 130_000, 10_000)
    ]


def design_io_a(offered_rate: float) -> list[Cell]:
    """Sub-study A: epoll against io_uring. Same binary, same workload, runtime flag."""
    cells: list[Cell] = []
    for backend in ("epoll", "io_uring"):
        cells.append(Cell.of(
            system_name(),
            **_io_base(io_backend=backend, study="A", write_path="buffered",
                       payload_bytes=0, offered_rate=offered_rate),
        ))
    return cells


def design_io_b(offered_rate: float) -> list[Cell]:
    """Sub-study B: sendfile vs SEND_ZC vs buffered across pre-declared sizes.

    io_uring only. send_zc+epoll is not scheduled: SEND_ZC is an io_uring opcode.
    Sizes are fixed in advance; they are not retuned after seeing numbers.
    """
    cells: list[Cell] = []
    for size in ZERO_COPY_SIZES:
        for path in ("buffered", "sendfile", "send_zc"):
            cells.append(Cell.of(
                system_name(),
                **_io_base(io_backend="io_uring", study="B", write_path=path,
                           payload_bytes=size, offered_rate=offered_rate),
            ))
    return cells


# h1 and h1-deep are the names to use. The windows- prefixed spellings are kept as
# aliases because the committed Windows results were produced under them and a reader
# reproducing that campaign will find those names in the commit messages; nothing about
# either design is Windows-specific, and the cells they build carry whichever system
# name and I/O backend the host implies.
#
# io-ladder / io-a / io-b / io-portability are the Linux I/O-portability session.
# io-a and io-b need --offered-rate. io-portability runs the ladder then A and B.
DESIGNS = {
    "h1": design_windows_h1,
    "h1-deep": design_windows_h1_deep,
    "ladder": design_ladder,
    "smoke": design_smoke,
    "windows-h1": design_windows_h1,
    "windows-h1-deep": design_windows_h1_deep,
    "io-ladder": design_io_ladder,
    "io-a": lambda: design_io_a(0),  # placeholder; main rebuilds with --offered-rate
    "io-b": lambda: design_io_b(0),
    "io-portability": design_io_ladder,  # orchestrated specially in main
}


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--design", choices=sorted(DESIGNS), default="smoke")
    ap.add_argument("--repetitions", type=int, default=7,
                    help="runs per cell; the median and its interval come from these")
    ap.add_argument("--duration", type=float, default=20.0, help="measured seconds per run")
    ap.add_argument("--warmup", type=float, default=3.0, help="discarded seconds per run")
    ap.add_argument("--seed", type=int, default=20260829)
    ap.add_argument("--port", type=int, default=18080)
    ap.add_argument("--results", type=Path, default=None)
    ap.add_argument("--samples", type=Path, default=None,
                    help="directory for raw per-request latency samples")
    ap.add_argument("--build", type=Path, default=None,
                    help="build directory holding the server and the generator")
    ap.add_argument("--offered-rate", type=float, default=None,
                    help="open-loop rate for io-a / io-b (required unless io-portability)")
    ap.add_argument("--report", type=Path,
                    default=REPO / "measurements" / "report.txt",
                    help="write the I/O-portability report here after io-* designs")
    args = ap.parse_args(argv)

    if args.build is None:
        args.build = _default_build()
    if args.results is None:
        args.results = _default_results(args.design)

    server_bin = args.build / "examples" / "Samples" / "benchmark_server" / "benchmark_server.exe"
    gen_bin = args.build / "benchmark" / "loadgen.exe"
    if not server_bin.exists():
        server_bin = server_bin.with_suffix("")
    if not gen_bin.exists():
        gen_bin = gen_bin.with_suffix("")
    for binary in (server_bin, gen_bin):
        if not binary.exists():
            print(f"not built: {binary}", file=sys.stderr)
            return 2

    # Dual Linux backends share one fingerprint. Per-run backend is a schema field.
    io_backend_fp = "epoll+io_uring" if args.design.startswith("io-") else None
    env = environment.capture(repo=REPO, build_type="Release", io_backend=io_backend_fp)
    args.results.parent.mkdir(parents=True, exist_ok=True)

    campaign = environment.Campaign.open_or_create(
        args.results.with_suffix(".env.json"), env
    )

    generator = LoadgenGenerator(
        binary=gen_bin, port=args.port, threads=GENERATOR_THREADS,
        warmup_s=args.warmup, affinity_mask=GENERATOR_AFFINITY, samples_dir=args.samples,
    )

    def server_factory(cell: Cell) -> CorouteServer:
        return CorouteServer(binary=server_bin, cell=cell, port=args.port,
                             affinity_mask=SERVER_AFFINITY)

    all_records: list[schema.RunRecord] = []

    def run_cells(cells: list[Cell], repetitions: int, seed: int, label: str) -> list[schema.RunRecord]:
        schedule = plan(cells, repetitions=repetitions, seed=seed)
        print(f"design {label}: {len(cells)} cells x {repetitions} repetitions "
              f"= {len(schedule)} runs")
        print(f"about {len(schedule) * (args.duration + args.warmup + 3) / 60:.0f} minutes")
        print(f"fingerprint {campaign.fingerprint[:12]}  virtualisation "
              f"{env.get('virtualisation') or 'none'}")
        print(f"affinity server={SERVER_AFFINITY!r} generator={GENERATOR_AFFINITY!r}")
        print()

        done = {"n": 0}

        def on_record(record: schema.RunRecord) -> None:
            done["n"] += 1
            mark = "ok " if record.accepted else "REJ"
            detail = "" if record.accepted else "  " + "; ".join(record.rejection_reasons)[:110]
            print(f"[{done['n']:3d}/{len(schedule)}] {mark} "
                  f"study={record.study or '-':7s} "
                  f"io={record.io_backend:8s} wp={record.write_path or '-':8s} "
                  f"rate={record.offered_rate or 0:>6.0f} "
                  f"pay={record.payload_bytes:<7d} "
                  f"rps={record.requests_per_second:>9.0f} "
                  f"p99={record.latency_ms.get('p99', 0):>7.3f}ms"
                  f"{detail}")

        records = driver.run_campaign(
            schedule,
            results_path=args.results,
            server_factory=server_factory,
            generator=generator,
            environment=env,
            campaign_fingerprint=campaign.fingerprint,
            duration_s=args.duration,
            on_record=on_record,
        )
        all_records.extend(records)
        return records

    offered = args.offered_rate

    if args.design == "io-portability":
        run_cells(design_io_ladder(), repetitions=1, seed=args.seed, label="io-ladder")
        offered = select_open_loop_rate(schema.read(args.results))
        print(f"\nselected offered_rate={offered!r}")
        if offered is None:
            print("no ladder rate kept pacing/achieved-share thresholds; A and B cells stay empty")
        else:
            run_cells(design_io_a(offered), repetitions=args.repetitions,
                      seed=args.seed + 1, label="io-a")
            run_cells(design_io_b(offered), repetitions=args.repetitions,
                      seed=args.seed + 2, label="io-b")
        write_report(args.results, args.report, offered_rate=offered)
        print(f"wrote report {args.report}")
    elif args.design == "io-a":
        if offered is None:
            print("--offered-rate is required for io-a", file=sys.stderr)
            return 2
        run_cells(design_io_a(offered), repetitions=args.repetitions,
                  seed=args.seed, label="io-a")
        write_report(args.results, args.report, offered_rate=offered)
    elif args.design == "io-b":
        if offered is None:
            print("--offered-rate is required for io-b", file=sys.stderr)
            return 2
        run_cells(design_io_b(offered), repetitions=args.repetitions,
                  seed=args.seed, label="io-b")
        write_report(args.results, args.report, offered_rate=offered)
    elif args.design == "io-ladder":
        run_cells(design_io_ladder(), repetitions=1, seed=args.seed, label="io-ladder")
        offered = select_open_loop_rate(schema.read(args.results))
        write_report(args.results, args.report, offered_rate=offered)
        print(f"selected offered_rate={offered!r}")
    else:
        cells = DESIGNS[args.design]()
        run_cells(cells, repetitions=args.repetitions, seed=args.seed, label=args.design)

    summary = driver.summarise(all_records)
    print()
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    print(f"\nwrote {args.results}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
