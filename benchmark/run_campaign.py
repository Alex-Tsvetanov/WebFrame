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
import platform
import sys
from pathlib import Path

from benchmark.adapters import CorouteServer, LoadgenGenerator
from benchmark.harness import driver, environment, schema
from benchmark.harness.ordering import Cell, plan


REPO = Path(__file__).resolve().parents[1]

# Ryzen 5 3600: twelve logical CPUs over six physical cores, paired. 0x0FF is logical
# 0 to 7, four physical cores, for the server. 0xF00 is logical 8 to 11, two physical
# cores, for the generator. Disjoint, so the two do not take work from each other.
_WINDOWS_SERVER_AFFINITY = "0ff"
_WINDOWS_GENERATOR_AFFINITY = "f00"
GENERATOR_THREADS = 2

# macOS has no CPU affinity API for user processes. thread_policy_set with
# THREAD_AFFINITY_POLICY is a locality hint and is a documented no-op on Apple Silicon,
# so requesting a mask there would set affinity_applied false on all 250 runs and trip
# the isolation rule in validity.py on every one of them. Asking for nothing is the
# honest encoding of a platform that grants nothing: the record then says no isolation
# was requested, rather than that isolation was requested and quietly denied.
_HAS_AFFINITY = platform.system() in ("Windows", "Linux")
SERVER_AFFINITY = _WINDOWS_SERVER_AFFINITY if _HAS_AFFINITY else None
GENERATOR_AFFINITY = _WINDOWS_GENERATOR_AFFINITY if _HAS_AFFINITY else None


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


def _io_backend() -> str:
    """The backend the presets select for this host.

    Recorded rather than assumed, because it is a factor in the record and a mislabelled
    factor is worse than a missing one: it makes two different measurements look like
    repetitions of one.
    """
    return {"Darwin": "kqueue", "Linux": "io_uring"}.get(platform.system(), "iocp")


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


# h1 and h1-deep are the names to use. The windows- prefixed spellings are kept as
# aliases because the committed Windows results were produced under them and a reader
# reproducing that campaign will find those names in the commit messages; nothing about
# either design is Windows-specific, and the cells they build carry whichever system
# name and I/O backend the host implies.
DESIGNS = {
    "h1": design_windows_h1,
    "h1-deep": design_windows_h1_deep,
    "ladder": design_ladder,
    "smoke": design_smoke,
    "windows-h1": design_windows_h1,
    "windows-h1-deep": design_windows_h1_deep,
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
    ap.add_argument("--results", type=Path, default=REPO / "benchmark" / "results" / "runs.jsonl")
    ap.add_argument("--samples", type=Path, default=None,
                    help="directory for raw per-request latency samples")
    ap.add_argument("--build", type=Path,
                    default=REPO / "build" / "windows-release",
                    help="build directory holding the server and the generator")
    args = ap.parse_args(argv)

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

    env = environment.capture(repo=REPO, build_type="Release")
    args.results.parent.mkdir(parents=True, exist_ok=True)

    # Refuses to append to a campaign whose machine has changed. Mixing two populations
    # into one file is the failure this exists to prevent, and it cannot be noticed
    # afterwards from the numbers alone.
    campaign = environment.Campaign.open_or_create(
        args.results.with_suffix(".env.json"), env
    )

    cells = DESIGNS[args.design]()
    schedule = plan(cells, repetitions=args.repetitions, seed=args.seed)

    print(f"design {args.design}: {len(cells)} cells x {args.repetitions} repetitions "
          f"= {len(schedule)} runs")
    print(f"about {len(schedule) * (args.duration + args.warmup + 3) / 60:.0f} minutes")
    print(f"fingerprint {campaign.fingerprint[:12]}  virtualisation "
          f"{env.get('virtualisation') or 'none'}")
    print()

    generator = LoadgenGenerator(
        binary=gen_bin, port=args.port, threads=GENERATOR_THREADS,
        warmup_s=args.warmup, affinity_mask=GENERATOR_AFFINITY, samples_dir=args.samples,
    )

    def server_factory(cell: Cell) -> CorouteServer:
        return CorouteServer(binary=server_bin, cell=cell, port=args.port,
                             affinity_mask=SERVER_AFFINITY)

    done = {"n": 0}

    def report(record: schema.RunRecord) -> None:
        done["n"] += 1
        mark = "ok " if record.accepted else "REJ"
        detail = "" if record.accepted else "  " + "; ".join(record.rejection_reasons)[:110]
        print(f"[{done['n']:3d}/{len(schedule)}] {mark} "
              f"rate={record.offered_rate or 0:>6.0f} detect={record.protocol_detection:d} "
              f"w={record.workers} pay={record.payload_bytes:<5d} "
              f"rps={record.requests_per_second:>9.0f} "
              f"p99={record.latency_ms.get('p99', 0):>7.3f}ms{detail}")

    records = driver.run_campaign(
        schedule,
        results_path=args.results,
        server_factory=server_factory,
        generator=generator,
        environment=env,
        campaign_fingerprint=campaign.fingerprint,
        duration_s=args.duration,
        on_record=report,
    )

    summary = driver.summarise(records)
    print()
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    print(f"\nwrote {args.results}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
