"""Runs a campaign and writes one record per run.

    python -m benchmark.run_campaign --design windows-h1 --repetitions 7

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
import sys
from pathlib import Path

from benchmark.adapters import CorouteServer, LoadgenGenerator
from benchmark.harness import driver, environment, schema
from benchmark.harness.ordering import Cell, plan


REPO = Path(__file__).resolve().parents[1]

# Ryzen 5 3600: twelve logical CPUs over six physical cores, paired. 0x0FF is logical
# 0 to 7, four physical cores, for the server. 0xF00 is logical 8 to 11, two physical
# cores, for the generator. Disjoint, so the two do not take work from each other.
SERVER_AFFINITY = "0ff"
GENERATOR_AFFINITY = "f00"
GENERATOR_THREADS = 2

# Below the point where the generator stops keeping up. Measured, not chosen: above
# about 75k on this host its pacing lag at p99 goes from tens of microseconds to
# milliseconds, and validity.py refuses those runs.
OFFERED_RATES = (10_000, 25_000, 40_000, 55_000, 70_000)


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
        io_backend="iocp",
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
            cells.append(Cell.of("coroute", **base, protocol_detection=detect, offered_rate=rate))

    # Worker scaling at a fixed offered rate, to show where the server stops benefiting
    # from more threads on four physical cores.
    for workers in (1, 2, 4, 8):
        if workers == base["workers"]:
            continue
        cells.append(Cell.of("coroute", **{**base, "workers": workers},
                             protocol_detection=True, offered_rate=40_000))

    # Response size, because the classification cost is per connection and the response
    # cost is per request: the ratio between them should move.
    for payload in (256, 1024, 8192):
        cells.append(Cell.of("coroute", **{**base, "payload_bytes": payload},
                             protocol_detection=True, offered_rate=40_000))

    return cells


def design_smoke() -> list[Cell]:
    """Two cells, for checking the machinery without spending an hour on it."""
    base = dict(
        protocol="http1.1", tls=False, io_backend="iocp", workers=4, connections=64,
        payload_bytes=0, backlog=1024, streams_per_connection=1, netem_profile="none",
    )
    return [
        Cell.of("coroute", **base, protocol_detection=True, offered_rate=10_000),
        Cell.of("coroute", **base, protocol_detection=False, offered_rate=10_000),
    ]


DESIGNS = {"windows-h1": design_windows_h1, "smoke": design_smoke}


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
