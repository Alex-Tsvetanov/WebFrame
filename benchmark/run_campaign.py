"""Runs a campaign and writes one record per run.

    python -m benchmark.run_campaign --design h1 --repetitions 7 --build build/<preset>

The TLS arm, in the order it has to be done
-------------------------------------------

    python -m benchmark.make_cert
    python -m benchmark.run_campaign --design tls-smoke    --repetitions 1 --build build/<preset>
    python -m benchmark.run_campaign --design tls-ladder   --repetitions 1 --build build/<preset>
    python -m benchmark.run_campaign --design churn-ladder --repetitions 1 --build build/<preset>

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

Add --wsl-distro and --wsl-loadgen (Windows) or --generator-command and
--generator-location (Linux, a network namespace) to drive the load across a network
interface instead of the loopback adapter. Both arrangements are worth having and they
are not comparable with each other, so they go in separate result files.

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
import shlex
import sys
from pathlib import Path

from benchmark.adapters import CorouteServer, LoadgenGenerator
from benchmark.harness import driver, environment, schema, validity
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


def mask_cores(mask: str, siblings: list) -> set[str]:
    """The physical cores a hexadecimal CPU mask covers, as the kernel names them.

    Raises ValueError when the mask names a CPU the host does not have online, since a
    bit the kernel ignores is isolation the record would claim and not have.
    """
    bits = int(mask, 16)
    cpus = [i for i in range(bits.bit_length()) if bits >> i & 1]
    for i in cpus:
        if i >= len(siblings) or siblings[i] is None:
            raise ValueError(f"mask {mask} names CPU {i}, which this host does not have online")
    return {siblings[i] for i in cpus}


def isolation_problem(env: dict) -> str | None:
    """Why the two masks do not give the isolation the record will claim, or None.

    The masks name logical CPUs, and the comment on them assumes siblings come in
    adjacent pairs. Linux numbers CPUs in firmware order and can interleave them, in
    which case 0ff and f00 put the generator on the SMT siblings of the server's cores
    and validity's isolation rule, which only checks that a mask was applied, would let
    the record claim isolation that does not exist. On the Windows campaign the
    generator ran inside WSL with no mask, so a Linux campaign is the first arrangement
    in which both masks are natively in force together. Nothing to check where the
    platform publishes no topology or asks for no mask; on Linux, which does publish
    one, a layout that could not be read is refused, since both masks would still be
    applied and the record would claim an isolation nothing established.
    """
    if not (SERVER_AFFINITY and GENERATOR_AFFINITY):
        return None
    siblings = (env.get("cpu") or {}).get("siblings")
    if not siblings:
        if (env.get("machine") or {}).get("system") == "Linux":
            return ("sibling layout could not be read from /sys/devices/system/cpu; "
                    f"the isolation of masks {SERVER_AFFINITY} and {GENERATOR_AFFINITY} "
                    "cannot be established")
        return None
    try:
        shared = mask_cores(SERVER_AFFINITY, siblings) & mask_cores(GENERATOR_AFFINITY, siblings)
    except ValueError as exc:
        return str(exc)
    if shared:
        return (f"server mask {SERVER_AFFINITY} and generator mask {GENERATOR_AFFINITY} share "
                f"physical core(s) {sorted(shared)}; the record would claim isolation the "
                f"sibling layout does not give")
    return None


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

# Both sets were read off ladder campaigns that this repository no longer tracks. The
# raw records are at tag results-archive-2026-09-01: benchmark/results/macos-ladder.jsonl
# for the macOS cliff described above, and churn-ladder and tls-ladder alongside it. They
# were removed when the four papers were reset to zero measurements, because a file that
# survived a reset is indistinguishable from a file that passed one. The reasoning is
# kept here rather than in the files, so the constants stay readable without them.

OFFERED_RATES = _MACOS_RATES if platform.system() == "Darwin" else _WINDOWS_RATES


def transport_mismatch(campaign: environment.Campaign, env: dict, section: str) -> str | None:
    """Why this run's arrangement cannot join the campaign on disk, or None.

    The fingerprint answers "same machine" and deliberately leaves the transport path
    out, so a loopback campaign and a network-path one hash identically and
    open_or_create accepts the append; the docstring of this module promised otherwise.
    The two populations differ in exactly the fixed cost a per-connection difference is
    measured against. Compared on the keys that name the arrangement and not on the
    address, because a virtual switch or a veth end changes address between sessions of
    the same arrangement. A file from before the section existed records nothing and is
    refused, since nothing establishes what it holds.
    """
    stored = campaign.environment.get(section) or {}
    current = env[section]
    for key in ("loopback", "generator_location"):
        if stored.get(key) != current[key]:
            return (f"{campaign.path} records {key}={stored.get(key)!r} and this run is "
                    f"{key}={current[key]!r}; the two arrangements are not comparable. "
                    f"Use a different --results.")
    return None


# The arm every cell of this campaign carries, decided in main from the build and
# --io-backend and read by the designs below. A module variable rather than an argument
# threaded through eight design functions: they are all nullary and called by name out
# of DESIGNS, and the alternative is eight signatures changed to carry one constant.
_ARM: str | None = None


def _io_backend() -> str:
    """The backend the cells of this run name.

    Recorded rather than assumed, because it is a factor in the record and a mislabelled
    factor is worse than a missing one: it makes two different measurements look like
    repetitions of one.

    A runtime arm, not a build option: CorouteServer passes whatever the cell carries
    here to --io-backend, and the driver refuses the run if the server's banner reports
    it started on a different one. So this must always name a single arm that can be
    asked for, never the compiled set: "dual" is a valid COROUTE_IO_BACKEND and would
    not be a valid answer here.

    Which arm that is comes from the build, via environment.run_io_backend, because the
    platform cannot say: on Linux both io_uring and epoll are legitimate and only the
    CMakeCache knows which one this tree has. The platform default is the fallback for a
    caller that imports a design without going through main.
    """
    return _ARM or environment.default_io_backend()


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
        for rate in sorted(set(range(5_000, 65_000, 5_000)) | set(TLS_OFFERED_RATES))
    ]


def design_churn_ladder() -> list[Cell]:
    """The same question for whole connections rather than requests.

    Establishment is bounded by something else entirely: the accept path, the TCP
    handshake, and on this host a blocking connect in the generator. That ceiling is
    lower than the request ceiling by two orders of magnitude and has to be found before
    the churn design can claim a rate.
    """
    # Built from the table it validates, so the two cannot drift apart. They had: the
    # table asks for 150 and the ladder stepped 100 to 200, which is the interval the
    # boundary lies in, so the one rate the campaign most needed checked was the one rate
    # the ladder skipped. Interpolation is not available there either, since 100 paced at
    # 83 microseconds and 200 at 3012.
    return [
        Cell.of(system_name(), **_base(tls=True, max_requests_per_connection=1),
                protocol_detection=True, offered_rate=rate)
        for rate in sorted({25, 50, 100, 125, 175, 200, 300, 400, 600, 800}
                           | set(CHURN_OFFERED_RATES) | set(CHURN_NET_OFFERED_RATES))
    ]


def design_tls_smoke() -> list[Cell]:
    """Four cells: enough to prove the TLS rig works before spending a night on it.

    The two shapes get different rates, and that is the whole correction. This design
    used to offer 1000 to both, which is nothing for a keep-alive connection and roughly
    seven times the admissible boundary for establishment, so its two churn cells were
    refused on every host and always would have been. Read without the shape in view they
    look like a host that paces at 57 microseconds in one run and eight seconds behind in
    the next, at the same offered rate, which is a machine fault nobody can find because
    it is not there.

    That is this file's own rule broken by this file: never carry an offered rate from one
    design into another. A smoke test whose failures are structural teaches the operator
    to expect failures, which is the opposite of what it is for.
    """
    return [
        Cell.of(system_name(), **_base(tls=True, max_requests_per_connection=churn),
                protocol_detection=detect, offered_rate=rate)
        for churn, rate in ((0, 1_000), (1, CHURN_OFFERED_RATES[0]))
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
# The rates the mechanism design offers, one per connection shape.
#
# Two rates rather than one, because the two shapes have very different ceilings over a
# network path and the comparison being made is between backends within a shape, not
# between shapes. Holding a single rate across both would have meant running keep-alive
# far below what it can do, or running churn above what it can do, and the second is
# what makes a ratio describe the limiting rather than the backend.
#
# Sized from what this rig has actually delivered. Keep-alive over the namespace pair
# achieved a full 10000 with the offered rate met exactly, so 10000 stands. Churn is the
# one that surprised: the feasibility probe reached about 10500 establishments a second
# over LOOPBACK, but over the pair a first attempt at 2000 delivered 84.6% and ran three
# seconds behind its own schedule, because establishment over a veth pays a driver and a
# softirq that loopback does not. The project's own network churn table runs 50 to 800
# for the same reason. 400 is half of that table's top rate and well inside what was
# just demonstrated to fail at 2000.
MECHANISM_KEEPALIVE_RATE = 10_000
MECHANISM_CHURN_RATE = 400


def design_mechanism() -> list[Cell]:
    """Syscalls per request, one connection shape against the other, one arm per run.

    The two cells differ in whether the server closes after a single request, and in the
    offered rate that shape can sustain. Everything else is held; the backend is chosen
    per invocation with --io-backend, so the four cells of the comparison are this design
    run twice.

    Cleartext and classification on, deliberately. The other designs cross TLS and
    detection because those are their questions; here they would be two more factors
    moving underneath the one being measured, and a TLS record layer in particular adds
    syscalls of its own that belong to neither backend.

    The rates differ between the two cells, so the two shapes are not comparable with
    each other for anything rate-dependent. Syscalls per request is a ratio and is the
    quantity this design exists for; throughput and latency from these cells are not.

    Meant to be run with --count-syscalls, which is why it is small: counting changes
    what a run measures, so a counted campaign buys nothing by being large.
    """
    return [
        Cell.of(system_name(), **_base(max_requests_per_connection=limit),
                protocol_detection=True, offered_rate=rate)
        for limit, rate in ((0, MECHANISM_KEEPALIVE_RATE), (1, MECHANISM_CHURN_RATE))
    ]


# Three rates, one connection shape, for testing whether a syscall count is per request
# or per second.
#
# The two-cell mechanism design gives two points, and the decomposition drawn from them
# used one keep-alive point and one churn point, so rate and shape moved together and a
# per-connection cost would land in the slope indistinguishably from a per-request one.
# Here only the rate moves. Three points also give the fit a residual, which two points
# with two unknowns cannot have.
CLOCK_LADDER_RATES = (2_000, 5_000, 10_000)


def design_clock_ladder() -> list[Cell]:
    """Keep-alive at three rates, to separate per-request syscalls from per-second ones.

    A count that is per request is flat when divided by requests and rises with rate when
    divided by time. A count that comes from a fixed-rate loop does the opposite. Neither
    can be told from the other at a single rate, which is how io_uring_enter at 4.752 was
    first read as a per-request figure when it was a poll loop, and how the timer thread's
    clock reads looked like nine per established connection.

    Counted runs, so --count-syscalls, and small for the same reason the mechanism design
    is: counting changes what a run measures.
    """
    return [
        Cell.of(system_name(), **_base(max_requests_per_connection=0),
                protocol_detection=True, offered_rate=rate)
        for rate in CLOCK_LADDER_RATES
    ]


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
    "mechanism": design_mechanism,
    "clock-ladder": design_clock_ladder,
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
    # Required rather than defaulted to one platform's preset directory. The runbook
    # passes it on every line, and a default that names a tree the host does not have
    # exits with "not built" on every other platform.
    ap.add_argument("--build", type=Path, required=True,
                    help="build directory holding the server and the generator")
    ap.add_argument("--cert", type=Path, default=REPO / "benchmark" / "certs" / "bench.crt",
                    help="certificate for the TLS designs; make it with benchmark.make_cert")
    ap.add_argument("--key", type=Path, default=REPO / "benchmark" / "certs" / "bench.key")
    # The arm of the I/O-portability comparison. Only meaningful on a dual tree, which is
    # the only build that contains a choice; elsewhere the build's one backend is the
    # answer and passing anything else is refused rather than recorded.
    ap.add_argument("--io-backend", choices=("io_uring", "epoll"), default=None,
                    help="which arm of a linux-dual build to measure; the default is "
                         "whatever the build compiled in, io_uring when it compiled both")

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
    # The Linux counterpart. The prefix is whatever enters the namespace and drops back
    # to the invoking user, e.g. "sudo -n ip netns exec gen runuser -u alex --"; the
    # build's own loadgen is appended, since the namespace shares the filesystem.
    ap.add_argument("--generator-command", default=None,
                    help="command prefix the generator is launched through, e.g. "
                         "'sudo -n ip netns exec gen runuser -u $USER --'")
    ap.add_argument("--generator-location", default=None,
                    help="where that prefix puts the generator, as a label recorded with "
                         "the campaign, e.g. netns:gen")
    # The server's counterpart. Without it the generator sits in a namespace and the
    # server sits on the host, which is not the arrangement the methodology names and
    # would only work at all because the veth address is locally owned.
    # Opt-in, and deliberately awkward to combine with a real campaign: a per-syscall
    # tracepoint costs time roughly in proportion to the syscall rate, which is exactly
    # the quantity a comparison is comparing, so a counted run is not a timed run.
    ap.add_argument("--count-syscalls", action="store_true",
                    help="count the server's syscalls with perf for each run. Changes "
                         "what is measured, so the timing from a counted run is not "
                         "comparable with an uncounted one; for the mechanism question, "
                         "not for a latency campaign")
    ap.add_argument("--server-command", default=None,
                    help="command prefix the server is launched through, e.g. "
                         "'sudo -n ip netns exec srv'; pairs with --generator-command")
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

    # A binary is not the commit the record will claim just because it sits in the
    # tree that commit is checked out at. Refused here rather than discovered later,
    # because a stale binary produces a record that looks exactly like a good one.
    stale = environment.build_staleness(args.build, REPO, (server_bin, gen_bin))
    if stale:
        for problem in stale:
            print(problem, file=sys.stderr)
        print("rebuild the tree, or point --build at one built from this source.",
              file=sys.stderr)
        return 2

    gen_command: list[str] | None = None
    location = "host"
    if args.wsl_distro:
        if not args.wsl_loadgen:
            print("--wsl-distro needs --wsl-loadgen", file=sys.stderr)
            return 2
        if args.generator_command or args.generator_location:
            print("--wsl-distro already says where the generator is; drop "
                  "--generator-command and --generator-location", file=sys.stderr)
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
        location = f"wsl:{args.wsl_distro}"
    else:
        if not gen_bin.exists():
            print(f"not built: {gen_bin}", file=sys.stderr)
            return 2
        if bool(args.generator_command) != bool(args.generator_location):
            # One without the other is a launcher nothing records, or a label nothing
            # earns, and either is a mislabelled campaign.
            print("--generator-command and --generator-location go together",
                  file=sys.stderr)
            return 2
        if args.generator_command:
            gen_command = shlex.split(args.generator_command) + [str(gen_bin)]
            location = args.generator_location

    server_prefix: list[str] = []
    if args.server_command:
        if args.wsl_distro:
            print("--wsl-distro drives the generator only; --server-command is for the "
                  "Linux namespace arrangement", file=sys.stderr)
            return 2
        if not args.generator_command:
            # A server in a namespace and a generator on the host is not the two-ended
            # arrangement either; it is a one-ended one with an extra hop.
            print("--server-command goes with --generator-command", file=sys.stderr)
            return 2
        server_prefix = shlex.split(args.server_command)

    # Derived from the launcher, not from the address alone. A generator on this host
    # reaches any locally owned address, a veth end included, through the loopback
    # interface; the address test only decides the case where the generator is elsewhere.
    loopback = gen_command is None or args.host.startswith("127.") or args.host in ("localhost", "::1")
    if gen_command is not None and loopback:
        # The generator would reach its own namespace's or virtual machine's loopback,
        # not the server, and every run would fail to connect. Caught here rather than
        # as sixty identical connection failures.
        print(f"--host {args.host} is loopback and the generator is in {location}, where "
              f"that address is not this host. Pass the address the server answers on "
              f"as the generator sees it.", file=sys.stderr)
        return 2

    # Both of these read the build's CMakeCache and both refuse rather than guess, so an
    # unconfigured --build or an arm the tree does not contain ends here in its own
    # words instead of in a traceback.
    global _ARM
    try:
        _ARM = environment.run_io_backend(args.build, args.io_backend)
        env = environment.capture(repo=REPO, build_type="Release",
                                  build_dir=args.build,
                                  io_backend=environment.resolve_io_backend(args.build))
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 2
    problem = isolation_problem(env)
    if problem:
        print(problem, file=sys.stderr)
        return 2
    # Part of the record because two campaigns that differ only in this are not
    # comparable, and nothing else in the environment would say so.
    # In the manifest, not only in the per-run records, because a counted campaign and
    # an uncounted one are different measurements of the same cells and the difference
    # has to be visible before anyone opens the numbers.
    env["syscall_counting"] = bool(args.count_syscalls)
    env["transport_path"] = {
        "host": args.host,
        "loopback": loopback,
        "generator_location": location,
        # Recorded for the same reason generator_location is: a campaign whose server
        # was in a namespace and one whose server was on the host are not the same
        # measurement, and nothing else in the record would say which this was.
        "server_location": args.server_command or "host",
    }
    # What every run would be refused for anyway, asked once before the hours are spent.
    # The governor is asked again per run by the driver; here it stops the night before
    # it starts, since a dynamic governor makes the drift gate fire on itself.
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

    args.results.parent.mkdir(parents=True, exist_ok=True)

    # Refuses to append to a campaign whose machine has changed. Mixing two populations
    # into one file is the failure this exists to prevent, and it cannot be noticed
    # afterwards from the numbers alone.
    campaign = environment.Campaign.open_or_create(
        args.results.with_suffix(".env.json"), env
    )
    mismatch = transport_mismatch(campaign, env, "transport_path")
    if mismatch:
        print(mismatch, file=sys.stderr)
        return 2

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
    if env["cpu"].get("siblings") and SERVER_AFFINITY and GENERATOR_AFFINITY:
        print(f"cores server={sorted(mask_cores(SERVER_AFFINITY, env['cpu']['siblings']))} "
              f"generator={sorted(mask_cores(GENERATOR_AFFINITY, env['cpu']['siblings']))}")
    print(f"generator {location} -> {args.host}{'  (loopback)' if loopback else ''}")
    if wants_tls:
        print(f"tls certificate {args.cert}")
    print()

    generator = LoadgenGenerator(
        binary=gen_bin, port=args.port, threads=GENERATOR_THREADS,
        work_dir=args.results.parent, warmup_s=args.warmup,
        # No mask when the generator is in the virtual machine. Its vCPUs are placed by
        # the hypervisor and a mask over them would name cores that are not the host's,
        # which the record would then claim as isolation it does not have. A namespace
        # shares the host's kernel and cores, so there the mask is real and stays.
        affinity_mask=None if args.wsl_distro else GENERATOR_AFFINITY,
        samples_dir=args.samples, host=args.host, command=gen_command,
        translate_paths=bool(args.wsl_distro), location=location,
    )

    def server_factory(cell: Cell) -> CorouteServer:
        return CorouteServer(binary=server_bin, cell=cell, port=args.port,
                             affinity_mask=SERVER_AFFINITY,
                             cert_file=args.cert, key_file=args.key,
                             launch_prefix=server_prefix,
                             count_syscalls=args.count_syscalls)

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
