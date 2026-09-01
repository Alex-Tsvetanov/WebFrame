"""Runs a campaign and writes one record per run.

    python -m benchmark.run_campaign --design h1 --repetitions 7

The TLS arm, in the order it has to be done
-------------------------------------------

    python -m benchmark.make_cert
    python -m benchmark.run_campaign --design tls-smoke   --repetitions 1
    python -m benchmark.run_campaign --design tls-ladder  --repetitions 1
    python -m benchmark.run_campaign --design churn-ladder --repetitions 1

The two ladders are not optional. TLS_OFFERED_RATES and CHURN_OFFERED_RATES below are
this host's numbers, and on any other machine they are guesses: a generator that cannot
keep to its own schedule is measuring itself, and validity.py will refuse the runs after
the machine time has been spent rather than before.

Then the campaigns that carry the claim:

    --design transport   classification on and off, over cleartext and over TLS
    --design churn       the same four arms with the server closing after one request

churn is the one that can fail. Every other design serves a hundred thousand requests
over sixty-four connections, so one extra read and one comparison per connection are
divided by every request that connection went on to serve, and a null result there was
arithmetic before it was a measurement. churn divides by one.

Add --wsl-distro and --wsl-loadgen to drive the load across a network interface instead
of the loopback adapter. Both arrangements are worth having and they are not comparable
with each other, so they go in separate result files.

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

# Below the point where the generator stops keeping up. Measured, not chosen, and
# measured separately per host: the ceiling is a property of the machine and of whether
# the generator can be given cores of its own, not of the code.
#
# Windows, Ryzen 5 3600, generator pinned to two dedicated physical cores: pacing lag at
# p99 leaves the tens of microseconds above about 75k.
_WINDOWS_RATES = (10_000, 25_000, 40_000, 55_000, 70_000)
#
# macOS, M4 Pro, nothing pinned because the platform has no affinity API, so the
# generator shares performance cores with the server. The ladder found a cliff between
# 50k and 60k which is not gradual: pacing lag goes from 279us to 1,019,598us in one
# step and the achieved share from 1.000 to 0.897. Server CPU plateaus from 60k upward
# while achieved throughput falls, which is a generator that stopped keeping up rather
# than a server that saturated.
#
# So 70k is unusable here and 55k sits one step below a cliff edge. The set below keeps
# the three rates Windows also runs, which is what makes the two campaigns describe the
# same offered loads, and tops out at 50k, the highest rate the ladder measured inside
# the flat region. Faster hardware with a lower ceiling is not a contradiction: the
# ceiling is set by the isolation the platform grants, not by the silicon.
_MACOS_RATES = (10_000, 25_000, 40_000, 50_000)

OFFERED_RATES = _MACOS_RATES if platform.system() == "Darwin" else _WINDOWS_RATES


def _io_backend() -> str:
    """The backend the presets select for this host.

    Recorded rather than assumed, because it is a factor in the record and a mislabelled
    factor is worse than a missing one: it makes two different measurements look like
    repetitions of one.
    """
    return environment.default_io_backend()


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


# The TLS designs below share one baseline with the cleartext ones and differ from them
# in the transport and in nothing else, which is the only way the two can be compared.
def _base(**over) -> dict:
    return {
        **dict(
            protocol="http1.1", tls=False, io_backend=_io_backend(), workers=4,
            connections=64, payload_bytes=0, backlog=1024, streams_per_connection=1,
            netem_profile="none", max_requests_per_connection=0,
        ),
        **over,
    }


# Lower than the cleartext ceiling, and by how much is not a guess: run the tls-ladder
# design on any host before using these. A TLS record layer costs the generator work per
# request that a cleartext socket does not, so the rate at which it stops keeping to its
# own schedule is lower, and validity.py refuses runs above it.
#
# Measured on this host by tls-ladder, 5k to 60k in 5k steps, one run each. The
# generator's pacing lag at p99 stays between 81 and 143 microseconds from 5k to 35k,
# rises to 322 to 499 at 40k, 50k and 55k, and reaches 85 ms at 60k. The rejections at
# 30k and 45k sit between accepted neighbours at higher rates, so those are the host
# stalling rather than a ceiling; 60k is the ceiling.
#
# The table stops at 35k rather than at 55k, which the gate would still admit. The gate
# is 1 ms and the last three accepted rates measured within a factor of two to three of
# it, on a host that produced two isolated 2.5 and 5.2 ms stalls during the same twelve
# runs. A rate that survives one run with that little margin does not survive twenty-five,
# and a cell that loses runs to the host loses them from one arm as easily as the other.
TLS_OFFERED_RATES = (5_000, 10_000, 15_000, 25_000, 35_000)

# The churn arm offers whole connections rather than requests on existing ones, and each
# one costs a TCP handshake, an accept and on the TLS arm a full key exchange. Two orders
# of magnitude below the keep-alive rates because that is what a connection costs.
#
# Measured by churn-ladder, and the earlier guess of 50, 100, 200, 300 was wrong at half
# its entries. This host establishes TLS connections at a hard ceiling of about 330 a
# second: offering 400, 600 and 800 all delivered between 325 and 340, which is the
# accept path and the key exchange saturating rather than the generator.
#
# The admissible boundary is far below that ceiling and was found separately, because
# delivering the rate is not the same as delivering it on time. 25, 50, 100, 110, 125 and
# 150 all pace within 109 to 124 microseconds. 175 paces at 1020, which is over the gate
# by 2 percent. 200 delivers the full offered rate and paces at 9.7 ms, with the median
# establishment jumping from 1.7 ms to 11.3 ms: the system is already queueing there
# while the delivered-rate check still reads clean.
#
# So the table is the four verified rates below the boundary, a sixfold span. Every one
# was measured admissible rather than inferred from its neighbours.
CHURN_OFFERED_RATES = (25, 50, 100, 150)


def design_transport() -> list[Cell]:
    """The headline comparison, with the transport in it.

    Four arms rather than two: classification on and off, over cleartext and over TLS.
    The cleartext half repeats what the 250-run campaign already measured, and it is
    repeated rather than cited because the two halves have to be comparable to each
    other. A TLS number from this campaign against a cleartext number from that one
    would differ in the transport, in the binary and in the week, and nothing in the
    result could say which.

    Rates are the TLS ceiling in both halves, again for comparability: running cleartext
    at 70k and TLS at 40k would compare two arms at different loads.
    """
    return [
        Cell.of(system_name(), **_base(tls=tls), protocol_detection=detect, offered_rate=rate)
        for rate in TLS_OFFERED_RATES
        for tls in (False, True)
        for detect in (True, False)
    ]


def design_tls_deep() -> list[Cell]:
    """The TLS half alone, for when the cleartext half is already in hand.

    Same shape as h1-deep, so the two files can be analysed by the same code. Half the
    machine time of the transport design, and it buys a TLS on-off comparison but not a
    TLS-against-cleartext one.
    """
    return [
        Cell.of(system_name(), **_base(tls=True), protocol_detection=detect, offered_rate=rate)
        for rate in TLS_OFFERED_RATES
        for detect in (True, False)
    ]


def design_churn() -> list[Cell]:
    """Connection establishment, which is where the classification cost actually lives.

    Every other design in this file serves a hundred thousand requests over sixty-four
    connections, so the one extra read and one comparison per connection are divided by
    every request that connection went on to serve. Whatever the difference between the
    arms is, that division makes it invisible, and a null result obtained that way says
    nothing: it was arithmetic before it was a measurement.

    Here the server closes after one request, so every request pays for a fresh accept
    and a fresh classification, and connect_ms measures that directly rather than
    inferring it from throughput. This is the cell a reviewer will look for, and the one
    where the hypothesis could actually fail.
    """
    return [
        Cell.of(system_name(), **_base(tls=tls, max_requests_per_connection=1),
                protocol_detection=detect, offered_rate=rate)
        for rate in CHURN_OFFERED_RATES
        for tls in (False, True)
        for detect in (True, False)
    ]


# The same design over a network interface, where the ceiling is somewhere else
# entirely. Measured by churn-ladder with the generator inside WSL: 25, 50, 100, 200,
# 300, 400, 600 and 800 establishments a second were all admissible on the first
# attempt, with the median establishment flat between 1.50 and 1.70 ms across the whole
# range, where the loopback arrangement was inadmissible above 150 and saturated hard at
# about 330.
#
# So the loopback establishment ceiling was the arrangement rather than the server. A
# churn generator pays for a TCP handshake and an asymmetric key exchange per request
# just as the server does, and on loopback it was pinned to two physical cores of the
# same six the server was using. Moving it into the virtual machine gives it cores the
# hypervisor places, and the ceiling moves by more than a factor of five.
#
# That is a fact about what the loopback numbers can be asked to support, and it is the
# argument for running this arrangement at all rather than a detail of it.
CHURN_NET_OFFERED_RATES = (50, 150, 400, 800)


def design_churn_net() -> list[Cell]:
    """churn over a network interface, at rates loopback could not reach.

    Two of the four rates are shared with the loopback table so the two arrangements can
    be read against each other for shape. They are not comparable for magnitude and the
    results files are kept apart: the transport path is part of the environment record
    precisely so that nothing downstream can merge them by accident.
    """
    return [
        Cell.of(system_name(), **_base(tls=tls, max_requests_per_connection=1),
                protocol_detection=detect, offered_rate=rate)
        for rate in CHURN_NET_OFFERED_RATES
        for tls in (False, True)
        for detect in (True, False)
    ]


def design_tls_ladder() -> list[Cell]:
    """Where the generator stops keeping up with TLS in the path.

    The cleartext ladder found this host's ceiling at about 75k. TLS moves it, because
    the generator now encrypts every request and decrypts every response, and by how
    much is a property of the machine and the OpenSSL build rather than something to
    assume. Run this before any TLS campaign and put the top offered rate comfortably
    below where generator_pacing_p99_us leaves the tens of microseconds.
    """
    return [
        Cell.of(system_name(), **_base(tls=True), protocol_detection=True, offered_rate=rate)
        for rate in range(5_000, 65_000, 5_000)
    ]


def design_churn_ladder() -> list[Cell]:
    """The same question for whole connections rather than requests.

    Establishment is bounded by something else entirely: the accept path, the TCP
    handshake, and on this host a blocking connect in the generator. That ceiling is
    lower than the request ceiling by two orders of magnitude and has to be found before
    the churn design can claim a rate.
    """
    return [
        Cell.of(system_name(), **_base(tls=True, max_requests_per_connection=1),
                protocol_detection=True, offered_rate=rate)
        for rate in (25, 50, 100, 200, 300, 400, 600, 800)
    ]


def design_tls_smoke() -> list[Cell]:
    """Four cells: enough to prove the TLS rig works before spending a night on it."""
    return [
        Cell.of(system_name(), **_base(tls=True, max_requests_per_connection=churn),
                protocol_detection=detect, offered_rate=1_000)
        for churn in (0, 1)
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
    # The TLS half of the claim. transport and churn are the two that carry it: the
    # first says what the demultiplexer costs a connection that is already up, the
    # second what it costs to bring one up.
    "transport": design_transport,
    "tls-deep": design_tls_deep,
    "churn": design_churn,
    "churn-net": design_churn_net,
    "tls-ladder": design_tls_ladder,
    "churn-ladder": design_churn_ladder,
    "tls-smoke": design_tls_smoke,
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
    ap.add_argument("--cert", type=Path, default=REPO / "benchmark" / "certs" / "bench.crt",
                    help="certificate for the TLS designs; make it with benchmark.make_cert")
    ap.add_argument("--key", type=Path, default=REPO / "benchmark" / "certs" / "bench.key")

    # Where the load comes from. The default is loopback and on one host, which is what
    # the committed campaigns used and what keeps a new run comparable with them. The
    # alternative is the arrangement run_routing_e2e uses: the generator inside WSL,
    # reaching the server across the virtual switch, so a request passes through a
    # network interface, a driver and an interrupt instead of being handed straight up
    # the loopback adapter. Those fixed costs are what a per-connection difference has
    # to be visible against, and removing them makes any difference look larger than it
    # is.
    ap.add_argument("--host", default="127.0.0.1",
                    help="the server's address as the generator sees it")
    ap.add_argument("--wsl-distro", default=None,
                    help="run the generator inside this WSL distribution, so the load "
                         "crosses a network interface instead of the loopback adapter")
    ap.add_argument("--wsl-loadgen", default=None,
                    help="path to the Linux loadgen build, inside the distribution")
    args = ap.parse_args(argv)

    server_bin = args.build / "examples" / "Samples" / "benchmark_server" / "benchmark_server.exe"
    gen_bin = args.build / "benchmark" / "loadgen.exe"
    if not server_bin.exists():
        server_bin = server_bin.with_suffix("")
    if not gen_bin.exists():
        gen_bin = gen_bin.with_suffix("")
    if not server_bin.exists():
        print(f"not built: {server_bin}", file=sys.stderr)
        return 2

    gen_command: list[str] | None = None
    if args.wsl_distro:
        if not args.wsl_loadgen:
            print("--wsl-distro needs --wsl-loadgen", file=sys.stderr)
            return 2
        # A POSIX path handed to this script from an MSYS shell is rewritten to a
        # Windows one before Python sees it, so /home/x/loadgen arrives as
        # C:/Program Files/Git/home/x/loadgen and the generator quietly fails to start.
        if not args.wsl_loadgen.startswith("/") or ":" in args.wsl_loadgen:
            print(f"--wsl-loadgen {args.wsl_loadgen!r} is not a path inside the "
                  f"distribution. If you are running from Git Bash or MSYS, it rewrote "
                  f"the argument; prefix the command with MSYS_NO_PATHCONV=1 or run it "
                  f"from PowerShell.", file=sys.stderr)
            return 2
        gen_command = ["wsl.exe", "-d", args.wsl_distro, "--", args.wsl_loadgen]
    elif not gen_bin.exists():
        print(f"not built: {gen_bin}", file=sys.stderr)
        return 2

    loopback = args.host.startswith("127.") or args.host in ("localhost", "::1")
    if args.wsl_distro and loopback:
        # The generator would reach the WSL virtual machine's own loopback, not the
        # server, and every run would fail to connect. Caught here rather than as sixty
        # identical connection failures.
        print(f"--host {args.host} is loopback and the generator is in WSL, where that "
              f"address is the distribution itself. Pass the address the server answers "
              f"on as WSL sees it.", file=sys.stderr)
        return 2

    env = environment.capture(repo=REPO, build_type="Release",
                            io_backend=environment.default_io_backend())
    # Part of the record because two campaigns that differ only in this are not
    # comparable, and nothing else in the environment would say so.
    env["transport_path"] = {
        "host": args.host,
        "loopback": loopback,
        "generator_location": f"wsl:{args.wsl_distro}" if args.wsl_distro else "host",
    }
    args.results.parent.mkdir(parents=True, exist_ok=True)

    # Refuses to append to a campaign whose machine has changed. Mixing two populations
    # into one file is the failure this exists to prevent, and it cannot be noticed
    # afterwards from the numbers alone.
    campaign = environment.Campaign.open_or_create(
        args.results.with_suffix(".env.json"), env
    )

    cells = DESIGNS[args.design]()

    # Checked before the campaign starts rather than when the first TLS cell comes up.
    # The shuffle can put that cell an hour in, and an hour of machine time spent to
    # discover a missing file is an hour nobody gets back.
    wants_tls = any(cell.as_dict().get("tls") for cell in cells)
    if wants_tls:
        missing = [p for p in (args.cert, args.key) if not p.exists()]
        if missing:
            print(f"design {args.design} has TLS cells but {', '.join(str(m) for m in missing)} "
                  f"{'do' if len(missing) > 1 else 'does'} not exist. "
                  f"Run: python -m benchmark.make_cert", file=sys.stderr)
            return 2

    schedule = plan(cells, repetitions=args.repetitions, seed=args.seed)

    print(f"design {args.design}: {len(cells)} cells x {args.repetitions} repetitions "
          f"= {len(schedule)} runs")
    print(f"about {len(schedule) * (args.duration + args.warmup + 3) / 60:.0f} minutes")
    print(f"fingerprint {campaign.fingerprint[:12]}  virtualisation "
          f"{env.get('virtualisation') or 'none'}")
    print(f"generator {'wsl:' + args.wsl_distro if args.wsl_distro else 'host'} -> "
          f"{args.host}{'  (loopback)' if loopback else ''}")
    if wants_tls:
        print(f"tls certificate {args.cert}")
    print()

    generator = LoadgenGenerator(
        binary=gen_bin, port=args.port, threads=GENERATOR_THREADS,
        warmup_s=args.warmup,
        # No mask when the generator is in the virtual machine. Its vCPUs are placed by
        # the hypervisor and a mask over them would name cores that are not the host's,
        # which the record would then claim as isolation it does not have.
        affinity_mask=None if args.wsl_distro else GENERATOR_AFFINITY,
        samples_dir=args.samples, host=args.host, command=gen_command,
        translate_paths=bool(args.wsl_distro),
    )

    def server_factory(cell: Cell) -> CorouteServer:
        return CorouteServer(binary=server_bin, cell=cell, port=args.port,
                             affinity_mask=SERVER_AFFINITY,
                             cert_file=args.cert, key_file=args.key)

    done = {"n": 0}

    def report(record: schema.RunRecord) -> None:
        done["n"] += 1
        mark = "ok " if record.accepted else "REJ"
        detail = "" if record.accepted else "  " + "; ".join(record.rejection_reasons)[:110]
        # Establishment is shown only when there is any, which is the churn designs. On
        # a keep-alive run every connection was made during the warmup and the column
        # would be an empty field on every line.
        conn = ""
        if record.connections_established and record.connect_ms:
            conn = (f" est={record.connections_established:>5d}"
                    f" cp50={record.connect_ms.get('p50', 0):>7.3f}ms")
        print(f"[{done['n']:3d}/{len(schedule)}] {mark} "
              f"rate={record.offered_rate or 0:>6.0f} detect={record.protocol_detection:d} "
              f"tls={record.tls:d} w={record.workers} pay={record.payload_bytes:<5d} "
              f"rps={record.requests_per_second:>9.0f} "
              f"p99={record.latency_ms.get('p99', 0):>7.3f}ms{conn}{detail}")

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
