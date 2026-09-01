"""Level two of the routing experiment: the same three arms, through the whole server.

    python -m benchmark.run_routing_e2e --design main --repetitions 5 --build build/<preset> \
        --wsl-distro Ubuntu-24.04 --wsl-loadgen /home/alex/loadgen --host 172.28.0.1

This is the level the paper's title rests on. Level one says what a lookup costs; this
says whether that still shows once parsing, the event loop and the network are in front
of it. The answer being "it does not" is a result, and finding the route count where it
starts to is the point.

Three things about the arrangement, each of which is a methodological commitment rather
than a convenience.

The arm is a flag, never a build. All three routers are in one benchmark_server and
--router chooses. Build-to-build variation from code layout and link order is documented
at 5 to 10 percent and run-to-run variation on a controlled machine is 1 to 2, so two
binaries cannot resolve the difference this is looking for.

The load does not come over loopback. The generator runs inside WSL and reaches the
server across the virtual switch, so a request goes through a network interface, a
driver and an interrupt rather than being handed straight up the loopback adapter. Those
fixed costs are exactly what a routing difference has to be visible against; measuring
over loopback would remove them and make the difference look larger than it is. This is
still one physical machine, and that limitation is stated with the results rather than
left for a reader to notice.

The server runs one worker. The DFA matcher mutates shared repeat counters while
matching, so two threads in match_with_groups at once is a data race, and a multi-worker
DFA arm would not be a sound measurement of anything. One worker everywhere keeps the
three arms comparable and keeps the race out of the results. It also means the
throughput figures here are per worker and are not a claim about the machine.
"""

from __future__ import annotations

import argparse
import json
import shlex
import subprocess
import sys
from pathlib import Path

from benchmark.adapters import CorouteServer, LoadgenGenerator
from benchmark.harness import driver, environment, schema, validity
from benchmark.harness.ordering import Cell, plan
from benchmark.run_campaign import (GENERATOR_AFFINITY, SERVER_AFFINITY, isolation_problem,
                                    mask_cores, transport_mismatch)


REPO = Path(__file__).resolve().parents[1]

ARMS = ("dfa", "radix", "regex")

# The masks are run_campaign's, so the two Linux campaigns claim identical isolation and
# a platform that grants none asks for none. With the generator in WSL its vCPUs are
# placed by the hypervisor and it gets no mask; on the same kernel it gets its own cores,
# because otherwise it is an unpinned sibling on the cores the server is pinned to and
# nothing in the record can show the contention.

BASE = dict(
    protocol="http1.1",
    tls=False,
    io_backend=environment.default_io_backend(),
    protocol_detection=True,
    workers=1,
    connections=64,
    payload_bytes=0,
    backlog=1024,
    streams_per_connection=1,
    netem_profile="none",
    route_shape="rest",
    route_params=True,
    route_depth=5,
)


def _cell(arm: str, routes: int, rate: int, **over) -> Cell:
    return Cell.of("coroute", **{**BASE, **over}, router_arm=arm, route_count=routes,
                   offered_rate=rate)


def design_main() -> list[Cell]:
    """Route count against offered load, all three arms.

    Two rates rather than one. At the lower rate every arm should keep up at every route
    count, so the latency distributions are comparable. At the higher one the slower arms
    are close to what a single worker can serve, which is where a dispatch cost stops
    being a rounding error and starts being the service time.
    """
    return [
        _cell(arm, routes, rate)
        for routes in (10, 100, 1000)
        for rate in (1000, 4000)
        for arm in ARMS
    ]


def design_bracket() -> list[Cell]:
    """Between a hundred routes and a thousand, which is where the arms separate.

    The main grid answers "does the choice of router show end to end" with "not at ten,
    not at a hundred, yes at a thousand". That is a range, and the brief asks for a
    number. These four counts bracket it.

    One offered rate, the low one, on purpose. At the high rate the slower arms are
    close to what a single worker can serve and the tail is dominated by queueing, which
    would locate the point where the server saturates rather than the point where the
    routers become distinguishable.
    """
    return [_cell(arm, routes, 1000) for routes in (200, 300, 500, 700) for arm in ARMS]


def design_bracket_low() -> list[Cell]:
    """Between ten routes and a hundred, the other end of the same question.

    At ten routes the three arms' repetition ranges overlap completely and no choice is
    visible. At a hundred they no longer overlap. The crossing is somewhere in here, and
    it is the number the brief asks for, so it is measured rather than interpolated.
    """
    return [_cell(arm, routes, 1000) for routes in (20, 50) for arm in ARMS]


def design_large() -> list[Cell]:
    """Ten thousand routes.

    Run apart from the grid. Level one measured the DFA arm's table at roughly a
    megabyte a route, so at ten thousand it does not fit in this machine's memory, and a
    cell that pushes the host into its pagefile would contaminate whatever the shuffle
    put after it.
    """
    return [_cell(arm, 10000, 1000) for arm in ARMS]


def design_smoke() -> list[Cell]:
    return [_cell(arm, 100, 1000) for arm in ARMS]


DESIGNS = {
    "main": design_main,
    "bracket": design_bracket,
    "bracket-low": design_bracket_low,
    "large": design_large,
    "smoke": design_smoke,
}


def dump_route_paths(server_bin: Path, out_dir: Path, cells: list[Cell]) -> dict[tuple, Path]:
    """Writes one request-path file per distinct table, from the server's own generator.

    From the server binary rather than from a copy of the rule, so the client cannot end
    up asking for a table the server did not register. A mismatch there would show up as
    a uniform 404 rate, which is a fast response, and the arm that failed hardest would
    look quickest.
    """
    out_dir.mkdir(parents=True, exist_ok=True)
    files: dict[tuple, Path] = {}
    for cell in cells:
        f = cell.as_dict()
        key = (f["route_count"], f["route_shape"], bool(f["route_params"]), f["route_depth"])
        if key in files:
            continue
        path = out_dir / ("paths-{}-{}-p{}-d{}.txt".format(
            key[0], key[1], 1 if key[2] else 0, key[3]))
        proc = subprocess.run(
            [str(server_bin),
             "--routes", str(key[0]),
             "--route-shape", key[1],
             "--route-params", "1" if key[2] else "0",
             "--route-depth", str(key[3]),
             "--dump-routes", str(path)],
            capture_output=True, text=True, timeout=1800,
        )
        if proc.returncode != 0 or not path.exists():
            raise RuntimeError(
                f"could not dump paths for {key}: exit {proc.returncode} "
                f"{(proc.stderr or proc.stdout).strip()[:200]}"
            )
        files[key] = path
    return files


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--design", choices=sorted(DESIGNS), default="smoke")
    ap.add_argument("--repetitions", type=int, default=5)
    ap.add_argument("--duration", type=float, default=20.0)
    ap.add_argument("--warmup", type=float, default=3.0)
    ap.add_argument("--seed", type=int, default=20260830)
    ap.add_argument("--port", type=int, default=18080)
    ap.add_argument("--threads", type=int, default=2, help="generator threads")
    ap.add_argument("--results", type=Path,
                    default=REPO / "benchmark" / "results" / "routing-e2e")
    ap.add_argument("--build", type=Path, required=True,
                    help="build directory holding a benchmark_server with the router arms "
                         "compiled in, and the generator; the bench preset does both")
    ap.add_argument("--readiness-timeout", type=float, default=180.0,
                    help="a large DFA table takes seconds to build before the port opens")

    ap.add_argument("--host", required=True,
                    help="the server's address as the generator sees it; must not be a "
                         "loopback address")
    ap.add_argument("--wsl-distro", default=None,
                    help="run the generator inside this WSL distribution")
    ap.add_argument("--wsl-loadgen", default=None,
                    help="path to the Linux loadgen build, inside the distribution")
    ap.add_argument("--generator-command", default=None,
                    help="command prefix the generator is launched through, e.g. "
                         "'sudo -n ip netns exec gen runuser -u $USER --'")
    ap.add_argument("--generator-location", default=None,
                    help="where that prefix puts the generator, as a label recorded with "
                         "the campaign, e.g. netns:gen")
    ap.add_argument("--allow-loopback", action="store_true",
                    help="permit a loopback --host; the results then measure a path that "
                         "never reaches a network interface and must say so")
    args = ap.parse_args(argv)

    server_bin = args.build / "examples" / "Samples" / "benchmark_server" / "benchmark_server.exe"
    if not server_bin.exists():
        server_bin = server_bin.with_suffix("")
    if not server_bin.exists():
        print(f"not built: {server_bin}", file=sys.stderr)
        return 2

    gen_command: list[str] | None = None
    location = "host"
    gen_binary = args.build / "benchmark" / "loadgen.exe"
    if args.wsl_distro:
        if not args.wsl_loadgen:
            print("--wsl-distro needs --wsl-loadgen", file=sys.stderr)
            return 2
        if args.generator_command or args.generator_location:
            print("--wsl-distro already says where the generator is; drop "
                  "--generator-command and --generator-location", file=sys.stderr)
            return 2
        # Checked because a POSIX path handed to this script from an MSYS shell
        # (Git Bash) is rewritten to a Windows one before Python ever sees it, and
        # /home/x/loadgen arrives as C:/Program Files/Git/home/x/loadgen. The
        # generator then cannot start, and the failure is quiet. Refused here so it
        # is loud instead.
        if not args.wsl_loadgen.startswith("/") or ":" in args.wsl_loadgen:
            print(f"--wsl-loadgen {args.wsl_loadgen!r} is not a path inside the "
                  f"distribution. If you are running from Git Bash or MSYS, it rewrote "
                  f"the argument; prefix the command with MSYS_NO_PATHCONV=1 or run it "
                  f"from PowerShell.", file=sys.stderr)
            return 2
        gen_command = ["wsl.exe", "-d", args.wsl_distro, "--", args.wsl_loadgen]
        location = f"wsl:{args.wsl_distro}"
    else:
        if not gen_binary.exists():
            gen_binary = gen_binary.with_suffix("")
        if not gen_binary.exists():
            print(f"not built: {gen_binary}", file=sys.stderr)
            return 2
        if bool(args.generator_command) != bool(args.generator_location):
            print("--generator-command and --generator-location go together",
                  file=sys.stderr)
            return 2
        if args.generator_command:
            gen_command = shlex.split(args.generator_command) + [str(gen_binary)]
            location = args.generator_location

    # Whether the load crosses a network interface is a property of where the generator
    # runs, not of the address string: a host-side generator reaches any locally owned
    # address, a veth end included, through the loopback interface.
    address_is_loopback = args.host.startswith("127.") or args.host in ("localhost", "::1")
    loopback = gen_command is None or address_is_loopback
    if gen_command is not None and address_is_loopback:
        # Nothing to override here: that address is the namespace's or the virtual
        # machine's own loopback, and the generator would connect to nothing.
        print(f"--host {args.host} is loopback and the generator is in {location}, where "
              f"that address is not this host.", file=sys.stderr)
        return 2
    # Refused rather than warned about. A loopback end-to-end run answers a different
    # question from the one this level exists to ask, and a warning in a log is not a
    # mechanism.
    if loopback and not args.allow_loopback:
        print(f"the generator is on this host, so --host {args.host} is reached over "
              f"loopback; this level is not measured over loopback. Pass --allow-loopback "
              f"to override, and say so in the paper.", file=sys.stderr)
        return 2

    out_dir = args.results / args.design
    out_dir.mkdir(parents=True, exist_ok=True)
    results_path = out_dir / "runs.jsonl"

    env = environment.capture(repo=REPO, build_type="Release",
                            io_backend=environment.resolve_io_backend(args.build))
    problem = isolation_problem(env)
    if problem:
        print(problem, file=sys.stderr)
        return 2
    env["routing_e2e"] = {
        "host": args.host,
        "loopback": loopback,
        "generator_location": location,
    }
    # Same preflight as the other two entry points, for the same reason: every run would
    # be refused for these anyway, and hours are cheaper to lose here than at run one.
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
    campaign = environment.Campaign.open_or_create(out_dir / "campaign.env.json", env)
    mismatch = transport_mismatch(campaign, env, "routing_e2e")
    if mismatch:
        print(mismatch, file=sys.stderr)
        return 2

    cells = DESIGNS[args.design]()
    schedule = plan(cells, repetitions=args.repetitions, seed=args.seed)

    print("dumping the request paths from the server's own table generator")
    path_files = dump_route_paths(server_bin, out_dir / "paths", cells)
    for key, path in sorted(path_files.items()):
        print(f"  {key}: {sum(1 for _ in path.open())} paths -> {path.name}")
    print()

    print(f"design {args.design}: {len(cells)} cells x {args.repetitions} repetitions "
          f"= {len(schedule)} runs")
    print(f"about {len(schedule) * (args.duration + args.warmup + 6) / 60:.0f} minutes")
    print(f"fingerprint {campaign.fingerprint[:12]}  virtualisation "
          f"{env.get('virtualisation') or 'none'}  git {env['build']['git_commit'][:12]}"
          f"{' DIRTY' if env['build']['git_dirty'] else ''}")
    if env["cpu"].get("siblings") and SERVER_AFFINITY and GENERATOR_AFFINITY:
        print(f"cores server={sorted(mask_cores(SERVER_AFFINITY, env['cpu']['siblings']))} "
              f"generator={sorted(mask_cores(GENERATOR_AFFINITY, env['cpu']['siblings']))}")
    print(f"generator {location} -> {args.host}{'  (loopback)' if loopback else ''}")
    print()

    samples_dir = out_dir / "samples"

    # One generator object per table, because the paths file is part of how it is
    # invoked. Built up front so a missing file fails before the campaign starts rather
    # than in run 43 of 90.
    generators = {}
    for key, paths in path_files.items():
        generators[key] = LoadgenGenerator(
            binary=gen_binary,
            port=args.port,
            threads=args.threads,
            work_dir=out_dir,
            warmup_s=args.warmup,
            affinity_mask=None if args.wsl_distro else GENERATOR_AFFINITY,
            samples_dir=samples_dir,
            host=args.host,
            command=gen_command,
            translate_paths=bool(args.wsl_distro),
            paths_file=paths,
            location=location,
        )

    def key_of(cell: Cell) -> tuple:
        f = cell.as_dict()
        return (f["route_count"], f["route_shape"], bool(f["route_params"]), f["route_depth"])

    def server_factory(cell: Cell) -> CorouteServer:
        return CorouteServer(binary=server_bin, cell=cell, port=args.port,
                             affinity_mask=SERVER_AFFINITY)

    records = []
    done = 0
    for scheduled in schedule:
        done += 1
        record = driver.run_one(
            scheduled,
            server_factory=server_factory,
            generator=generators[key_of(scheduled.cell)],
            environment=env,
            campaign_fingerprint=campaign.fingerprint,
            duration_s=args.duration,
            readiness_timeout_s=args.readiness_timeout,
        )
        schema.append(results_path, record, allow_incomplete=not record.accepted)
        records.append(record)

        mark = "ok " if record.accepted else "REJ"
        detail = "" if record.accepted else "  " + "; ".join(record.rejection_reasons)[:110]
        print(f"[{done:3d}/{len(schedule)}] {mark} {record.router_arm:<5s} "
              f"routes={record.route_count:>6d} rate={record.offered_rate or 0:>6.0f} "
              f"rps={record.requests_per_second:>8.0f} "
              f"p50={record.latency_ms.get('p50', 0):>8.3f}ms "
              f"p99={record.latency_ms.get('p99', 0):>8.3f}ms "
              f"p99.9={record.latency_ms.get('p999', 0):>9.3f}ms{detail}")

    print()
    print(json.dumps(driver.summarise(records), indent=2, ensure_ascii=False))
    print(f"\nwrote {results_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
