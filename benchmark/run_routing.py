"""Level one of the routing experiment: dispatch cost, isolated.

    python -m benchmark.run_routing --design main --repetitions 5 --build build/<preset>

Same discipline as run_campaign, for the same reasons, on a design that needs no server
and no network:

  a run is a process        A fresh route_bench every time. Measuring one process five
                            times is five looks at one heap, one page placement and one
                            branch predictor state.

  order is interleaved      Every repetition visits every cell once, shuffled from the
                            campaign seed. Machines warm up; measuring all of one arm
                            and then all of another attributes the warming to the arm.

  every run leaves a record Including the ones that failed. A cell whose router will not
                            fit in memory is a result, and a campaign with a silent gap
                            cannot be told from one that was never scheduled.

The arm is a flag on one binary, never a build. Build-to-build variation from code
layout and link order is documented at 5 to 10 percent, and the differences here are
often smaller than that.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import time
from pathlib import Path

from benchmark.adapters import describe_signal
from benchmark.harness import environment, validity
from benchmark.harness.ordering import Cell, plan


REPO = Path(__file__).resolve().parents[1]

ARMS = ("dfa", "radix", "regex")


def _cell(arm: str, routes: int, shape: str, params: int, depth: int,
          path_set: int = 0) -> Cell:
    # path_set is a factor rather than a campaign-wide argument, so the scheduler
    # interleaves it the way it interleaves arm and route count. Run as three blocked
    # campaigns instead, one per value, the machine's warming would be attributed to the
    # path-set condition, which is the confound this control exists to remove; and it
    # would land hardest on whichever value ran first. 0 is the whole table.
    return Cell.of(arm, routes=routes, shape=shape, params=params, depth=depth,
                   path_set=path_set)


def design_main() -> list[Cell]:
    """Route count and table shape, which is what the claim is stated in terms of.

    Two shapes rather than one. Under `rest` every route hangs off /api/v1/, the way an
    API is actually laid out, so the structures carry all of them down a shared spine
    before branching. Under `flat` the first segment differs from its first character.
    A paper that reported only one of these would be reporting an accident of the table
    it happened to generate.
    """
    cells = []
    for routes in (10, 100, 1000):
        for shape in ("rest", "flat"):
            for arm in ARMS:
                cells.append(_cell(arm, routes, shape, params=1, depth=5))
    return cells


def design_scaling() -> list[Cell]:
    """The whole route-count sweep in one campaign, all three arms.

    Separate from `main` because `main` splits ten thousand routes off into designs of
    their own: the DFA's table did not fit in memory at that size and a cell that pushed
    the machine into its pagefile would have contaminated whatever the shuffle placed
    after it. It fits now, so the sweep can be one interleaved design again, which is
    what makes the arms at different route counts comparable with each other.
    """
    return [
        _cell(arm, routes, "rest", params=1, depth=5)
        for routes in (10, 100, 1000, 10000)
        for arm in ARMS
    ]


def design_static() -> list[Cell]:
    """The same grid without captures, which isolates what parameter extraction costs.

    Routes with a {id} are the realistic case and the expensive one. Subtracting this
    from the main design is the only way to say how much of the cost is the parameter
    rather than the lookup.
    """
    return [
        _cell(arm, routes, "rest", params=0, depth=5)
        for routes in (10, 100, 1000)
        for arm in ARMS
    ]


def design_depth() -> list[Cell]:
    """Path depth at a fixed table size.

    In the design because the claim says a radix tree grows with path depth and a DFA
    does not. At one depth that is untestable either way.
    """
    return [
        _cell(arm, 1000, "rest", params=1, depth=depth)
        for depth in (3, 8, 12)
        for arm in ARMS
    ]


def design_large() -> list[Cell]:
    """Ten thousand routes, run as a pass of its own.

    Separated because the DFA arm at this size needs memory measured in gigabytes, and a
    cell that pushes the machine into its pagefile would contaminate every cell that
    followed it in a shuffled order. Within this pass the three arms are still
    interleaved, so the comparison at ten thousand is made the same way as the others.
    """
    return [
        _cell(arm, 10000, shape, params=params, depth=5)
        for shape in ("rest", "flat")
        for params in (1, 0)
        for arm in ARMS
    ]


def design_large_cheap() -> list[Cell]:
    """The ten thousand route cells that fit, repeated properly.

    Split out after the first pass measured what the DFA arm's parameterised table costs
    to hold at this size. Repeating a cell that does not fit in memory would spend hours
    paging and would still produce no distribution, while the cells that do fit deserve
    the same five repetitions as everything else. The cells left out are in `large` and
    their outcome is recorded there.
    """
    cells = [
        _cell(arm, 10000, shape, params=params, depth=5)
        for shape in ("rest", "flat")
        for params in (1, 0)
        for arm in ("radix", "regex")
    ]
    cells += [_cell("dfa", 10000, shape, params=0, depth=5) for shape in ("rest", "flat")]
    return cells


def design_large_dfa() -> list[Cell]:
    """Only the cells that may not fit, so a run that pages cannot spoil its neighbours."""
    return [_cell("dfa", 10000, shape, params=1, depth=5) for shape in ("rest", "flat")]


def design_smoke() -> list[Cell]:
    return [_cell(arm, 100, "rest", params=1, depth=5) for arm in ARMS]



# The pools the working-set control compares. 0 is the unbounded ring, which is what every
# cell before this design used and what the confound looks like; 10 and 100 hold the
# queried set still while the table grows.
PATH_SETS = (0, 10, 100)


def design_path_set() -> list[Cell]:
    """Table size against working-set size, which no previous design separates.

    The ring draws with replacement from the whole table, so the number of distinct paths
    it holds rises with the route count and the two grow together in every scaling cell.
    A dispatch cost that rises across the sweep cannot then be told from a cache and TLB
    effect, which is the open question the routing paper carries.

    Three pools per cell rather than three campaigns. Interleaved, so the machine warming
    up is not charged to whichever pool ran first, which matters here more than usual
    because the unbounded pool is the slow one and running it first would flatter the
    control.

    Both arms, because the question is about the automaton and the tree separately: if
    bounding the pool moves them by the same proportion the effect is the memory
    hierarchy, and if it moves one and not the other it is the structure.
    """
    return [
        _cell(arm, routes, "rest", params=1, depth=5, path_set=ps)
        for routes in (1000, 10000)
        for ps in PATH_SETS
        for arm in ("dfa", "radix")
    ]

DESIGNS = {
    "main": design_main,
    "scaling": design_scaling,
    "static": design_static,
    "depth": design_depth,
    "large": design_large,
    "large-cheap": design_large_cheap,
    "large-dfa": design_large_dfa,
    "path-set": design_path_set,
    "smoke": design_smoke,
}


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--design", choices=sorted(DESIGNS), default="smoke")
    ap.add_argument("--repetitions", type=int, default=5)
    ap.add_argument("--seed", type=int, default=20260830)
    ap.add_argument("--lookups", type=int, default=200000)
    ap.add_argument("--warmup", type=int, default=20000)
    # A factor of the measurement rather than a convenience, and one this campaign could
    # not express until route_bench took the flag. Set externally instead, a mask governs
    # every cell and appears in no record: the run is pinned and nothing says so, which is
    # the failure this harness exists to prevent. Empty means unpinned, recorded as such.
    # An instrument rather than a treatment: every cell takes both readings and the value
    # does not vary across cells, so it is a campaign argument and not a factor. It is
    # still recorded per row, because a cell must say which instrument it quotes and the
    # two do not measure the same quantity.
    ap.add_argument("--batch", type=int, default=0,
                    help="time K lookups between one pair of clock reads and divide; 0 "
                         "off. Required on hosts with no cycle counter, where the "
                         "per-lookup histogram does not exist")
    ap.add_argument("--affinity", default="",
                    help="hex CPU mask for route_bench; empty runs unpinned. Which core is\n"
                         "chosen moves both the median and the spread, so it belongs in the\n"
                         "record beside the numbers it produced")
    ap.add_argument(
        "--max-seconds",
        type=float,
        default=20.0,
        help="per-run cap on the timed window; the record says how many samples it got",
    )
    ap.add_argument(
        "--timeout",
        type=float,
        default=600.0,
        help="wall clock per run before it is killed and recorded as failed",
    )
    ap.add_argument("--results", type=Path, default=REPO / "benchmark" / "results" / "routing")
    ap.add_argument("--build", type=Path, required=True,
                    help="build directory holding route_bench, configured with "
                         "-DCOROUTE_ROUTER_ARMS=ON; the bench preset does")
    args = ap.parse_args(argv)

    binary = args.build / "benchmark" / "route_bench.exe"
    if not binary.exists():
        binary = binary.with_suffix("")
    if not binary.exists():
        print(f"not built: {binary}", file=sys.stderr)
        print("configure with -DCOROUTE_ROUTER_ARMS=ON", file=sys.stderr)
        return 2

    # A binary is not the commit the record will claim just because it sits in the tree
    # that commit is checked out at. Refused here rather than discovered later, because a
    # stale binary produces a record that looks exactly like a good one.
    stale = environment.build_staleness(args.build, REPO, (binary,))
    if stale:
        for problem in stale:
            print(problem, file=sys.stderr)
        print("rebuild the tree, or point --build at one built from this source.",
              file=sys.stderr)
        return 2

    out_dir = args.results / args.design
    raw_dir = out_dir / "raw"
    raw_dir.mkdir(parents=True, exist_ok=True)
    records_path = out_dir / "runs.jsonl"

    env = environment.capture(repo=REPO, build_type="Release",
                            build_dir=args.build,
                            io_backend=environment.resolve_io_backend(args.build))
    campaign = environment.Campaign.open_or_create(out_dir / "campaign.env.json", env)

    # The three arms have to agree on the table before they are compared on speed.
    # Checked here, at the top of the campaign, rather than trusted: if they disagree,
    # the fast arm is fast because it is answering a different question.
    print("checking the arms resolve the same tables")
    for routes, shape, params, depth in (
        (200, "rest", 1, 5),
        (200, "flat", 1, 5),
        (200, "rest", 0, 5),
        (100, "rest", 1, 12),
    ):
        proc = subprocess.run(
            [
                str(binary), "--verify",
                "--routes", str(routes),
                "--shape", shape,
                "--params", str(params),
                "--depth", str(depth),
            ],
            capture_output=True, text=True, timeout=600,
        )
        sys.stdout.write(proc.stdout)
        if proc.returncode != 0:
            print("the arms do not agree; refusing to measure them", file=sys.stderr)
            return 1
    print()

    # Level one used to gate on nothing. It printed the virtualisation and DIRTY state
    # and then measured anyway, so its `accepted` field meant "the subprocess exited 0"
    # while the same word elsewhere in this harness means "the method allows this run".
    # That is worse than an absent field: dispatch is an rdtsc microbenchmark, the
    # measurement in this project most sensitive to clock and thermal state, and it was
    # the one level with no check on either.
    blocking = validity.check_run({
        "virtualisation": env.get("virtualisation"),
        "git_dirty": env["build"]["git_dirty"],
        "power_source": validity.current_power_source(),
        "governor": env["cpu"]["governor"],
    }).reasons
    if blocking:
        for reason in blocking:
            print(f"refusing to measure: {reason}", file=sys.stderr)
        return 1

    cells = DESIGNS[args.design]()
    schedule = plan(cells, repetitions=args.repetitions, seed=args.seed)

    print(f"design {args.design}: {len(cells)} cells x {args.repetitions} repetitions "
          f"= {len(schedule)} runs")
    print(f"fingerprint {campaign.fingerprint[:12]}  virtualisation "
          f"{env.get('virtualisation') or 'none'}  git {(env['build']['git_commit'] or '?')[:12]}"
          f"{' DIRTY' if env['build']['git_dirty'] else ''}")
    print()

    accepted = 0
    failed = 0
    with records_path.open("a", encoding="utf-8") as sink:
        for n, scheduled in enumerate(schedule, start=1):
            factors = scheduled.cell.as_dict()
            arm = factors["system"]
            stem = (f"{arm}-{factors['routes']}-{factors['shape']}-p{factors['params']}"
                    f"-d{factors['depth']}-r{scheduled.repetition}")
            json_path = raw_dir / f"{stem}.json"
            hist_path = raw_dir / f"{stem}.hist.csv"

            argv_run = [
                str(binary),
                "--arm", arm,
                "--routes", str(factors["routes"]),
                "--shape", factors["shape"],
                "--params", str(factors["params"]),
                "--depth", str(factors["depth"]),
                "--lookups", str(args.lookups),
                "--warmup", str(args.warmup),
                "--max-seconds", f"{args.max_seconds:g}",
                "--seed", str(args.seed + scheduled.repetition),
                "--out", str(json_path),
                "--hist", str(hist_path),
            ]
            if args.affinity:
                argv_run += ["--affinity", args.affinity]
            if args.batch:
                argv_run += ["--batch", str(args.batch)]
            # 0 is the unbounded pool and is the default in the binary, so it is passed
            # only when set. The record carries it either way, from the factor below.
            if factors.get("path_set"):
                argv_run += ["--path-set", str(factors["path_set"])]

            power_before = validity.current_power_source()

            record = {
                "campaign_fingerprint": campaign.fingerprint,
                "started_unix": time.time(),
                "repetition": scheduled.repetition,
                "index_in_repetition": scheduled.index_in_repetition,
                "arm": arm,
                **{k: v for k, v in factors.items() if k != "system"},
                "argv": argv_run,
                "git_commit": env["build"]["git_commit"],
                "git_dirty": env["build"]["git_dirty"],
                "virtualisation": env.get("virtualisation"),
                "power_source": power_before,
                "thermal_speed_limit_start": validity.current_speed_limit(),
                # The drift rule keys on these two and the record never carried them,
                # so an rdtsc microbenchmark had no clock gate on any platform: the
                # speed limit below is pmset-only and None everywhere else.
                "cpu_mhz_start": validity.current_cpu_mhz(),
                # Requested here; whether it took is in route_bench's own --out JSON,
                # beside the measurement it qualifies. Empty string means no mask was
                # asked for, which is a different record from one that was asked for and
                # refused.
                "affinity_requested": args.affinity or None,
                # Which instrument this row may be quoted from. The per-lookup histogram
                # and the batch reading measure different quantities and must not be
                # pooled, so a row that does not name its instrument is a row nobody can
                # place. 0 means the per-lookup histogram alone.
                "batch": args.batch,
            }

            try:
                proc = subprocess.run(
                    argv_run, capture_output=True, text=True, timeout=args.timeout
                )
                if proc.returncode != 0 or not json_path.exists():
                    record["accepted"] = False
                    # A negative code is a signal, and -9 with an empty stderr is the
                    # OOM killer; named so the failure column says so.
                    rc = proc.returncode
                    sig = f" ({describe_signal(-rc)})" if rc < 0 else ""
                    record["failure"] = (
                        f"exit {rc}{sig}: {(proc.stderr or proc.stdout).strip()[:300]}"
                    )
                else:
                    record["accepted"] = True
                    record["result"] = json.loads(json_path.read_text(encoding="utf-8"))
                    record["raw_json"] = str(json_path.relative_to(out_dir))
                    record["raw_histogram"] = str(hist_path.relative_to(out_dir))
            except subprocess.TimeoutExpired:
                record["accepted"] = False
                record["failure"] = f"exceeded the {args.timeout:g}s wall clock and was killed"
            except MemoryError as exc:  # pragma: no cover - the driver itself is small
                record["accepted"] = False
                record["failure"] = f"{type(exc).__name__}: {exc}"

            # Taken after the run, because a laptop that throttled during a cell was
            # not throttled when it started. Intel Macs publish CPU_Speed_Limit and
            # Apple Silicon does not, so this fires on some hosts and is inert on
            # others. Which one a number came from belongs in the record.
            record["thermal_speed_limit_end"] = validity.current_speed_limit()
            record["cpu_mhz_end"] = validity.current_cpu_mhz()
            # Per run, like the driver: the preflight above read it once, and the drift
            # rule means nothing under a governor that moved at cell ten.
            record["governor"] = environment._governor()
            verdict = validity.check_run(record)
            record["admission_rules"] = validity.ADMISSION_RULES
            if verdict.reasons:
                record["accepted"] = False
                record["rejection_reasons"] = verdict.reasons

            sink.write(json.dumps(record, ensure_ascii=False) + "\n")
            sink.flush()

            if record["accepted"]:
                accepted += 1
                r = record["result"]
                # Which instrument produced the line, because on a host with no cycle
                # counter the per-lookup histogram is absent rather than zero and
                # printing its empty percentiles reads as a run that measured nothing.
                # The record was always right; the console was not.
                head = (f"[{n:3d}/{len(schedule)}] ok  {arm:<5s} "
                        f"routes={factors['routes']:>6d} {factors['shape']:<4s} "
                        f"p={factors['params']} d={factors['depth']:<2d} "
                        f"pool={factors.get('path_set', 0):>5d}")
                if r.get("per_lookup_histogram", True):
                    print(f"{head} p50={r['ns']['p50']:>10.1f}ns "
                          f"p99={r['ns']['p99']:>11.1f}ns p99.9={r['ns']['p999']:>11.1f}ns "
                          f"n={r['lookups']:>7d} build={r['build_ms']:>9.1f}ms "
                          f"rss={r['rss_delta_bytes']/1048576:>8.1f}MB")
                elif r.get("batch_picoseconds_per_lookup"):
                    b = r["batch_picoseconds_per_lookup"]
                    print(f"{head} batch p50={b['p50']/1000.0:>10.1f}ns "
                          f"p90={b['p90']/1000.0:>11.1f}ns p99={b['p99']/1000.0:>11.1f}ns "
                          f"n={r.get('batch_lookups', 0):>7d} build={r['build_ms']:>9.1f}ms "
                          f"rss={r['rss_delta_bytes']/1048576:>8.1f}MB")
                else:
                    # Accepted by the runner and carrying no measurement from either
                    # instrument. The record keeps it; the console must not imply a
                    # number was taken.
                    print(f"{head} NO MEASUREMENT: neither instrument produced a reading")
            else:
                failed += 1
                print(f"[{n:3d}/{len(schedule)}] FAIL {arm:<5s} routes={factors['routes']:>6d} "
                      f"{factors['shape']:<4s} p={factors['params']} d={factors['depth']:<2d} "
                      f"{record['failure'][:120]}")

    print()
    print(json.dumps({"runs": len(schedule), "accepted": accepted, "failed": failed}, indent=2))
    print(f"\nwrote {records_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
