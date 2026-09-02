"""Self-check for the harness guards.

Plain asserts and no test framework, so it runs anywhere Python does. The guards are
the part of the harness that has to work when nobody is watching, months into a
campaign, which is exactly when an unrunnable test suite is no help.

    python3 benchmark/harness/selfcheck.py
"""

from __future__ import annotations

import json
import os
import socket
import sys
import tempfile
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from benchmark.harness import environment as env_mod
from benchmark.harness import ordering
from benchmark.harness import schema
from benchmark.harness import validity


PASSED = 0


def check(name: str, condition: bool) -> None:
    global PASSED
    if not condition:
        print(f"  FAILED: {name}")
        raise SystemExit(1)
    PASSED += 1
    print(f"  ok: {name}")


def sample_env(**overrides) -> dict:
    base = {
        "machine": {"node": "bench01", "arch": "x86_64", "system": "Linux"},
        "kernel": {"release": "6.8.0-40-generic", "version": "#1 SMP"},
        "cpu": {"model": "Example CPU", "physical_cores": 8, "logical_cores": 16,
                "governor": "performance"},
        "memory": {"total_bytes": 34359738368},
        "tuning": {"transparent_hugepages": "madvise", "swappiness": "10",
                   "somaxconn": "4096"},
        "toolchain": {"compiler": "g++ 14.2.0", "python": "3.12.3"},
        "build": {"type": "Release", "io_backend": "io_uring",
                  "git_commit": "a" * 40, "git_dirty": False},
        "deps": {"openssl": "3.5.8", "ngtcp2": "1.25.0", "nghttp3": "1.18.0",
                 "liburing": "2.5"},
        "virtualisation": None,
    }
    for dotted, value in overrides.items():
        section, _, key = dotted.partition(".")
        base[section][key] = value
    return base


def fingerprint_checks() -> None:
    print("\n== the fingerprint identifies the machine, not the moment ==")

    a = sample_env()
    check("the same environment hashes the same",
          env_mod.fingerprint(a) == env_mod.fingerprint(sample_env()))

    # The whole reason for a curated field list. If everything were hashed, every run
    # would be its own campaign and the guard would fire constantly, which is how
    # guards get switched off.
    noisy = sample_env()
    noisy["observed"] = {"load_average": 3.7, "uptime_seconds": 91234}
    check("unfingerprinted fields do not change the hash",
          env_mod.fingerprint(noisy) == env_mod.fingerprint(a))

    for dotted, value, label in [
        ("cpu.governor", "powersave", "governor moving to powersave"),
        ("kernel.release", "6.9.0-1-generic", "a kernel upgrade"),
        ("build.git_commit", "b" * 40, "a different commit"),
        ("deps.openssl", "3.5.9", "a dependency rebuild"),
        ("cpu.physical_cores", 4, "SMT or core count changing"),
        ("cpu.siblings", ["0", "1", "2", "3"], "the sibling layout changing"),
        ("tuning.transparent_hugepages", "always", "a hugepage setting change"),
    ]:
        changed = sample_env(**{dotted: value})
        check(f"{label} changes the hash",
              env_mod.fingerprint(changed) != env_mod.fingerprint(a))

    diffs = env_mod.fingerprint_differences(a, sample_env(**{"cpu.governor": "powersave"}))
    check("a refusal can say which field moved",
          len(diffs) == 1 and "governor" in diffs[0] and "powersave" in diffs[0])


def campaign_checks() -> None:
    print("\n== a campaign refuses to mix populations ==")

    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "campaign.json"
        first = env_mod.Campaign.open_or_create(path, sample_env())
        check("a new campaign is created", path.exists())

        again = env_mod.Campaign.open_or_create(path, sample_env())
        check("reopening on the same machine works", again.fingerprint == first.fingerprint)

        moved = sample_env(**{"cpu.governor": "powersave"})
        message = ""
        try:
            env_mod.Campaign.open_or_create(path, moved)
            raised = False
        except env_mod.EnvironmentChanged as exc:
            raised = True
            message = str(exc)
        check("reopening after the environment moved raises", raised)
        check("the refusal names the field that moved", "governor" in message)
        check("the refusal explains the consequence", "not comparable" in message)

        forced = env_mod.Campaign.open_or_create(path, moved, force=True)
        check("force exists as a deliberate override", forced.fingerprint == first.fingerprint)

    # A key added to _FINGERPRINTED after a manifest was written moves every stored hash
    # while no field has moved. That is a harness update, not a machine change, and it
    # must not refuse every campaign on disk; a key that now has a value on one side is
    # a machine change and must.
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "campaign.json"
        first = env_mod.Campaign.open_or_create(path, sample_env())
        original = env_mod._FINGERPRINTED
        env_mod._FINGERPRINTED = original + ("cpu.a_key_from_the_future",)
        try:
            grown = env_mod.Campaign.open_or_create(path, sample_env())
            check("a fingerprint key added since, null on both sides, does not refuse the append",
                  grown.fingerprint == first.fingerprint)
            message = ""
            try:
                env_mod.Campaign.open_or_create(
                    path, sample_env(**{"cpu.a_key_from_the_future": "now known"}))
            except env_mod.EnvironmentChanged as exc:
                message = str(exc)
            check("the same key with a value on one side is refused and named",
                  "a_key_from_the_future" in message)
        finally:
            env_mod._FINGERPRINTED = original


def transport_checks() -> None:
    print("\n== a campaign refuses to mix transport paths ==")

    from benchmark.run_campaign import transport_mismatch

    # The fingerprint leaves the transport path out on purpose, so a loopback campaign
    # and a network-path one hash identically and the append is accepted. This is the
    # check that stands in for the fingerprint there.
    def env_with(loopback: bool, location: str, host: str = "10.0.0.1",
                 server_location: str = "host") -> dict:
        env = sample_env()
        env["transport_path"] = {"host": host, "loopback": loopback,
                                 "generator_location": location,
                                 "server_location": server_location}
        return env

    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "runs.env.json"
        campaign = env_mod.Campaign.open_or_create(path, env_with(False, "netns:gen"))
        check("a new campaign matches itself",
              transport_mismatch(campaign, env_with(False, "netns:gen"), "transport_path") is None)
        reopened = env_mod.Campaign.open_or_create(path, env_with(False, "netns:gen", "10.0.0.2"))
        check("the address may change between sessions of one arrangement",
              transport_mismatch(reopened, env_with(False, "netns:gen", "10.0.0.2"), "transport_path") is None)
        # Only loopback moves, so the refusal has exactly one key it can name and the
        # check is about that key rather than about whichever sorts first.
        message = transport_mismatch(reopened, env_with(True, "netns:gen"), "transport_path")
        check("a loopback run cannot join a network-path campaign", message is not None)
        check("and the refusal names the key", "loopback" in message)
        # The server's own placement, which the key list used not to reach: the generator
        # is in a namespace either way, so loopback and generator_location are identical
        # and only server_location tells the two arrangements apart.
        message = transport_mismatch(
            reopened, env_with(False, "netns:gen", server_location="sudo -n ip netns exec srv"),
            "transport_path")
        check("a namespaced server cannot join a host-server campaign", message is not None)
        check("and the refusal names the key", "server_location" in message)
        # A key the manifest carries and this run does not is refused too, which is what
        # the union comparison buys over a hand-kept list.
        current = env_with(False, "netns:gen")
        current["transport_path"].pop("server_location")
        check("a key the manifest has and the run lacks is refused",
              transport_mismatch(reopened, current, "transport_path") is not None)
        # A manifest from before the section existed says nothing about its arrangement.
        older = env_mod.Campaign.open_or_create(Path(tmp) / "old.env.json", sample_env())
        check("a manifest without the section is refused, not assumed",
              transport_mismatch(older, env_with(False, "netns:gen"), "transport_path") is not None)


def netem_checks() -> None:
    print("\n== the impairment on the path is read, not declared ==")

    from benchmark import netns

    # What `tc qdisc show dev veth-srv` prints for a pair built by netns.py, handle and
    # refcnt included, since those are what vary between sessions of one arrangement.
    wan50 = ("qdisc netem 8001: root refcnt 2 limit 100000 delay 25ms\n")
    check("a netem qdisc is named by what it carries",
          netns.observed_profile(wan50) == "netem root limit 100000 delay 25ms")
    check("the handle and refcnt are dropped, so one arrangement reads the same twice",
          netns.observed_profile(wan50)
          == netns.observed_profile("qdisc netem 8003: root refcnt 5 limit 100000 delay 25ms"))
    check("two profiles do not read alike",
          netns.observed_profile(wan50)
          != netns.observed_profile("qdisc netem 8001: root refcnt 2 limit 100000 delay 50ms"))
    check("a veth with no netem is none",
          netns.observed_profile("qdisc noqueue 0: root refcnt 2") == "none")
    check("and so is one with some other qdisc",
          netns.observed_profile("qdisc fq_codel 0: root refcnt 2 limit 10240p") == "none")


def port_checks() -> None:
    print("\n== a held port is refused before a server is started on it ==")

    from benchmark.adapters import refuse_held_port
    from benchmark.harness.driver import RunFailed

    # A real listener on the wildcard address, the way the servers bind. On BSD a probe
    # with SO_REUSEADDR could coexist with a 127.0.0.1 listener, so a loopback holder
    # would let this pass without checking anything.
    holder = socket.socket()
    if os.name != "nt":
        holder.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    holder.bind(("0.0.0.0", 0))
    holder.listen()
    port = holder.getsockname()[1]
    message = ""
    try:
        try:
            refuse_held_port(port)
            raised = False
        except RunFailed as exc:
            raised = True
            message = str(exc)
    finally:
        holder.close()
    check("a port with a listener is refused", raised)
    check("the refusal names the port", str(port) in message)
    try:
        refuse_held_port(port)
        freed = True
    except RunFailed:
        # Caught rather than allowed to propagate: an uncaught raise here aborted the
        # whole selfcheck with a traceback, so the one direction that says the probe is
        # not simply refusing everything could never be reported as a failed check.
        freed = False
    check("the same port passes once the listener is gone", freed)

    # The generator's recorded name is its kind of location. Every WSL record on disk
    # says coroute-loadgen-wsl, and a namespace must not be filed under it.
    from benchmark.adapters import LoadgenGenerator
    def named(location: str) -> str:
        return LoadgenGenerator(binary=Path("x"), port=1, threads=1, work_dir=Path("."),
                                location=location).name
    check("a host generator keeps its name", named("host") == "coroute-loadgen")
    check("a WSL generator keeps the name the records already carry",
          named("wsl:Ubuntu-24.04") == "coroute-loadgen-wsl")
    check("a namespace generator is not filed as WSL", named("netns:gen") == "coroute-loadgen-netns")

    print("\n== a server that is listening and will not name its backend says so ==")

    # A binary built before the banner carried a backend prints "(multi-accept)" alone,
    # BANNER_BACKEND never matches it, and the server is ready forever without ever
    # being startable. wait_until_ready used to return False for that, which the driver
    # files as "server did not become ready within 30s": the readiness timeout's message
    # for a server that answered the readiness connection on the first try, and the one
    # cause the rewritten failure path did not name. No timeout is long enough, so the
    # operator spends the night raising it.
    from benchmark.adapters import CorouteServer

    listener = socket.socket()
    if os.name != "nt":
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listener.bind(("127.0.0.1", 0))
    listener.listen()
    silent_port = listener.getsockname()[1]
    # _proc is None, so nothing here can be mistaken for a child that exited: this is
    # the live-and-silent case and not the died-at-startup one.
    silent = CorouteServer(binary=Path("x"),
                           cell=ordering.Cell.of("coroute", protocol="http1.1",
                                                 io_backend="io_uring"),
                           port=silent_port)
    silent._output.append(f"Server listening on port {silent_port} (multi-accept)\n")
    message = ""
    try:
        try:
            silent.wait_until_ready(0.3)
            raised = False
        except RunFailed as exc:
            raised = True
            message = str(exc)
    finally:
        listener.close()
    check("a listening server with no backend banner is refused", raised)
    check("the refusal says to rebuild, not to wait longer",
          "rebuild" in message and "multi-accept" in message)


def topology_checks() -> None:
    print("\n== the affinity masks are checked against the sibling layout ==")

    from benchmark import run_campaign

    # One directory for the three fake trees, removed on the way out: mkdtemp left
    # three behind per run, and this file uses TemporaryDirectory everywhere else.
    with tempfile.TemporaryDirectory() as tmp:
        def fake_sysfs(name: str, layout: list[str]) -> Path:
            root = Path(tmp) / name / "cpu"
            for i, siblings in enumerate(layout):
                (root / f"cpu{i}" / "topology").mkdir(parents=True)
                (root / f"cpu{i}" / "topology" / "thread_siblings_list").write_text(siblings + "\n")
            # Files the real directory also holds and the probe must not read as CPUs.
            (root / "cpufreq").mkdir()
            (root / "online").write_text("0-11\n")
            return root

        # Zen and the Windows enumeration: siblings adjacent, 0ff and f00 disjoint.
        paired = ["0-1", "0-1", "2-3", "2-3", "4-5", "4-5", "6-7", "6-7",
                  "8-9", "8-9", "10-11", "10-11"]
        siblings = env_mod._siblings(fake_sysfs("paired", paired))
        check("siblings are read in CPU order, not lexicographic", siblings == paired)
        check("adjacent pairs keep the masks disjoint",
              run_campaign.mask_cores("0ff", siblings).isdisjoint(run_campaign.mask_cores("f00", siblings)))

        # Cores first, then their SMT threads: the same two masks now share four cores.
        interleaved = ["0,6", "1,7", "2,8", "3,9", "4,10", "5,11",
                       "0,6", "1,7", "2,8", "3,9", "4,10", "5,11"]
        siblings = env_mod._siblings(fake_sysfs("interleaved", interleaved))
        check("an interleaved layout is caught",
              run_campaign.mask_cores("0ff", siblings) & run_campaign.mask_cores("f00", siblings)
              == {"2,8", "3,9", "4,10", "5,11"})
        # With the masks the Linux campaign uses, whatever this platform asks for.
        masks = run_campaign.SERVER_AFFINITY, run_campaign.GENERATOR_AFFINITY
        run_campaign.SERVER_AFFINITY, run_campaign.GENERATOR_AFFINITY = "0ff", "f00"
        try:
            problem = run_campaign.isolation_problem({"cpu": {"siblings": siblings}})
            check("and refused with the shared cores named", problem is not None and "2,8" in problem)
            check("the paired layout passes",
                  run_campaign.isolation_problem({"cpu": {"siblings": paired}}) is None)
            check("a mask over CPUs the host does not have is refused",
                  "does not have" in str(run_campaign.isolation_problem({"cpu": {"siblings": paired[:8]}})))
            # Windows publishes no topology and the masks are applied unchecked, as every
            # record on disk was. Linux publishes one, so failing to read it is a refusal:
            # the masks would still be applied and the record would claim disjoint cores.
            check("no topology to publish leaves the masks alone",
                  run_campaign.isolation_problem(
                      {"machine": {"system": "Windows"}, "cpu": {"siblings": None}}) is None)
            check("a linux layout that could not be read is refused",
                  "could not be read" in str(run_campaign.isolation_problem(
                      {"machine": {"system": "Linux"}, "cpu": {"siblings": None}})))
        finally:
            run_campaign.SERVER_AFFINITY, run_campaign.GENERATOR_AFFINITY = masks
        check("no sysfs is None, not an empty layout", env_mod._siblings(Path(tmp) / "absent") is None)


def validity_checks() -> None:
    print("\n== the pre-declared rejection criteria ==")

    good = {
        "virtualisation": None, "git_dirty": False,
        "requests_total": 1_000_000, "requests_non_2xx": 0,
        "generator_cpu_fraction": 0.42,
        "cpu_mhz_start": 4200.0, "cpu_mhz_end": 4190.0,
        "counter_deltas": {"UdpRcvbufErrors": 0, "TcpExtListenOverflows": 0},
        "power_source": "mains",
    }
    check("a clean run is accepted", validity.check_run(good).valid)

    # power_source had to be added to the record above for it to pass, which is the
    # point: it was absent, absent was falsy, and the rule never fired. A desktop is
    # clean because it says it has no battery, not because it said nothing.
    check("a desktop with no battery is accepted",
          validity.check_run(dict(good, power_source=env_mod._NO_BATTERY)).valid)
    check("a host that could not be asked is refused",
          not validity.check_run(dict(good, power_source=env_mod._UNCHECKED.format("no probe"))).valid)
    check("a record with no power state at all is refused",
          not validity.check_run({k: v for k, v in good.items() if k != "power_source"}).valid)

    # The criterion that matters most. Without it a server that collapses under load
    # wins on throughput, because failing is cheaper than serving.
    failing = dict(good, requests_non_2xx=5_000)
    verdict = validity.check_run(failing)
    check("a run with 0.5% errors is refused", not verdict.valid)
    check("the reason names the error rate", any("non-2xx" in r for r in verdict.reasons))

    borderline = dict(good, requests_non_2xx=1_000)  # exactly 0.1%
    check("exactly at the threshold is still accepted", validity.check_run(borderline).valid)

    saturated = dict(good, generator_cpu_fraction=0.91)
    verdict = validity.check_run(saturated)
    check("a saturated generator is refused", not verdict.valid)
    check("the reason says the client was measured",
          any("load generator" in r for r in verdict.reasons))

    throttled = dict(good, cpu_mhz_end=3800.0)
    check("a thermally throttled run is refused", not validity.check_run(throttled).valid)
    # The drift rule skipped None, and a failed probe on Linux or Windows returned None,
    # so a clock nobody read passed as a clock that held. Unchecked is refused; None is
    # still macOS, where the speed limit stands in.
    check("a clock the probe could not read is refused",
          any("could not read the clock" in r for r in validity.check_run(
              dict(good, cpu_mhz_end=env_mod._UNCHECKED.format("no reading"))).reasons))
    check("no clock to read is left to the speed limit",
          validity.check_run(dict(good, cpu_mhz_start=None, cpu_mhz_end=None)).valid)

    # The drift rule compares two instants, so under a dynamic governor it fires on the
    # governor and calls it thermal. Only Linux publishes one; None stays clean because
    # Windows and macOS have nothing to read and a value would be invented.
    check("a dynamic governor is refused",
          any("governor" in r for r in validity.check_run(dict(good, governor="schedutil")).reasons))
    check("the performance governor is accepted", validity.check_run(dict(good, governor="performance")).valid)
    check("no governor to read is left to the other rules", validity.check_run(dict(good, governor=None)).valid)
    # Linux always has a governor, so a Linux host that cannot read it is unchecked and
    # refused; the probe used to return the same None a Windows host does.
    check("a governor that could not be read is refused",
          any("governor" in r for r in validity.check_run(
              dict(good, governor=env_mod._UNCHECKED.format("no scaling_governor"))).reasons))
    missing = str(Path(tempfile.mkdtemp()) / "scaling_governor")
    check("an unreadable governor on linux is unchecked",
          "unchecked" in str(env_mod._governor(missing, system="Linux")))
    check("and stays None on a platform with none to read",
          env_mod._governor(missing, system="Windows") is None)
    present = Path(tempfile.mkdtemp()) / "scaling_governor"
    present.write_text("performance\n")
    check("a readable governor is its stripped value",
          env_mod._governor(str(present), system="Linux") == "performance")

    dropped = dict(good, counter_deltas={"UdpRcvbufErrors": 17, "TcpExtListenOverflows": 0})
    verdict = validity.check_run(dropped)
    check("a run where the kernel dropped datagrams is refused", not verdict.valid)
    check("the reason names the counter", any("UdpRcvbufErrors" in r for r in verdict.reasons))

    virt = dict(good, virtualisation="kvm")
    check("a run under virtualisation is refused", not validity.check_run(virt).valid)

    dirty = dict(good, git_dirty=True)
    check("a run from a dirty tree is refused", not validity.check_run(dirty).valid)
    # git that could not answer used to be folded to False on its way into the record.
    check("a tree whose state could not be read is refused, not clean",
          any("unknown is not clean" in r for r in validity.check_run(dict(good, git_dirty=None)).reasons))
    check("a record with no tree state at all is refused",
          not validity.check_run({k: v for k, v in good.items() if k != "git_dirty"}).valid)

    # Slow is a result, not a fault. A criterion that rejected slow runs would be
    # rejecting the finding.
    slow = dict(good, p99_ms=980.0, requests_per_second=12.0)
    check("a merely slow run is accepted", validity.check_run(slow).valid)

    multi = dict(good, virtualisation="kvm", generator_cpu_fraction=0.99, git_dirty=True)
    check("every reason is reported, not just the first",
          len(validity.check_run(multi).reasons) == 3)


def counter_checks() -> None:
    print("\n== counters are parsed by name, not by column ==")

    # Real shape, trimmed. Parsed by pairing header names with values rather than by
    # index, because the columns differ between kernel versions and a fixed index would
    # silently read a different counter.
    snmp = (
        "Udp: InDatagrams NoPorts InErrors OutDatagrams RcvbufErrors SndbufErrors\n"
        "Udp: 100 1 0 90 0 0\n"
    )
    netstat = (
        "TcpExt: SyncookiesSent ListenOverflows ListenDrops\n"
        "TcpExt: 0 0 0\n"
    )
    before = validity.read_counters(snmp=snmp, netstat=netstat)
    # Under the name the watch list uses. This used to expect the bare key, and the
    # delta below was built by hand with the prefixed one, so the check passed while the
    # real path never produced the counter the gate watches.
    check("a UDP counter is found by name", before.get("UdpRcvbufErrors") == 0)
    check("a TcpExt counter is prefixed", before.get("TcpExtListenOverflows") == 0)

    after = validity.read_counters(
        snmp=snmp.replace("Udp: 100 1 0 90 0 0", "Udp: 200 1 0 190 42 0"),
        netstat=netstat,
    )
    deltas = validity.counter_deltas(before, after)
    check("a delta is computed", deltas["UdpRcvbufErrors"] == 42)
    check("and the gate fires on it",
          any("UdpRcvbufErrors" in r for r in validity.check_run(
              {"counter_deltas": deltas, "power_source": env_mod._NO_BATTERY}).reasons))

    # A counter that moved but is not watched must not appear. Otherwise every run has
    # something to point at and the criteria stop meaning anything.
    check("only the watched counters are returned",
          set(deltas) <= set(validity.ZERO_DELTA_COUNTERS))


def schema_checks() -> None:
    print("\n== a run record carries everything needed to place it ==")

    def complete(**kw):
        base = dict(system="coroute", protocol="http1.1", io_backend="io_uring",
                    workers=6, connections=256, duration_s=30.0)
        base.update(kw)
        return schema.RunRecord(**base)

    check("a complete record has no missing factors", not complete().missing_factors())

    # A run missing its protocol is not a row with a gap, it is a row nobody can
    # interpret, and finding out weeks later means the machine time is gone.
    incomplete = complete(protocol="")
    check("a record missing a factor says which", incomplete.missing_factors() == ["protocol"])

    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "runs.jsonl"

        message = ""
        try:
            schema.append(path, incomplete)
            raised = False
        except ValueError as exc:
            raised = True
            message = str(exc)
        check("writing an unplaceable run is refused", raised)
        check("the refusal names the factor", "protocol" in message)

        for repetition in range(3):
            schema.append(path, complete(repetition=repetition, requests_total=1000))
        records = list(schema.read(path))
        check("three runs round-trip", len(records) == 3)
        check("repetition distinguishes them",
              sorted(r.repetition for r in records) == [0, 1, 2])
        check("run ids are distinct", len({r.run_id for r in records}) == 3)

        # What a campaign killed mid-write leaves behind: a line with no terminating
        # newline. Refusing to read the file because of it would throw away everything
        # that did complete.
        with path.open("a", encoding="utf-8") as handle:
            handle.write('{"system": "coroute", "protoc')
        check("a torn trailing line is skipped, not fatal", len(list(schema.read(path))) == 3)

        # And the next append must not join itself onto the fragment. Without the
        # newline guard the torn run takes a good one down with it, which is a second
        # run lost to a crash that only interrupted one.
        schema.append(path, complete(repetition=99, requests_total=1))
        check("a later append survives a torn line", len(list(schema.read(path))) == 4)
        check("the recovered run is the new one",
              any(r.repetition == 99 for r in schema.read(path)))

        # Rejected runs stay in the file: they are the evidence for the rejection count
        # that has to be reported next to the results.
        schema.append(path, complete(accepted=False,
                                     rejection_reasons=["virtualisation detected"]))
        all_runs = list(schema.read(path))
        check("rejected runs are kept", len(all_runs) == 5)
        check("but excluded from analysis", len(schema.accepted_only(iter(all_runs))) == 4)

        # A file written by a later schema must still read.
        with path.open("a", encoding="utf-8") as handle:
            handle.write(json.dumps({"system": "coroute", "protocol": "http3",
                                     "io_backend": "epoll", "workers": 1,
                                     "connections": 1, "duration_s": 1.0,
                                     "a_field_from_the_future": 42}) + "\n")
        check("an unknown field does not break reading", len(list(schema.read(path))) == 6)

    # None means closed loop, a number means open loop, and the difference decides
    # whether the latency figures are service time or response time.
    check("offered_rate defaults to closed loop", complete().offered_rate is None)


def ordering_checks() -> None:
    print("\n== run order interleaves systems so drift does not pick a side ==")

    cells = [
        ordering.Cell.of("coroute", protocol="http1.1", connections=256),
        ordering.Cell.of("nginx", protocol="http1.1", connections=256),
        ordering.Cell.of("h2o", protocol="http1.1", connections=256),
        ordering.Cell.of("coroute", protocol="http2", connections=256),
    ]

    plan = ordering.plan(cells, repetitions=5, seed=1234)
    check("every cell runs once per repetition", len(plan) == len(cells) * 5)

    counts = Counter(run.cell for run in plan)
    check("no cell is favoured", set(counts.values()) == {5})

    # The property that matters. All of A then all of B would give A a mean position of
    # 0 and B of 1, so every warm-up effect would land on B and be read as B being slow.
    means = ordering.position_summary(plan)
    spread = max(means.values()) - min(means.values())
    check(f"systems sit at similar mean positions (spread {spread:.2f})", spread < 1.0)

    check("the same seed gives the same order",
          [r.cell for r in ordering.plan(cells, 5, 1234)] == [r.cell for r in plan])
    check("a different seed gives a different order",
          [r.cell for r in ordering.plan(cells, 5, 9999)] != [r.cell for r in plan])

    # Passes must not be identical to each other, or a cell keeps its slot and is always
    # measured on an equally cold or equally hot machine.
    first = [r.cell for r in plan if r.repetition == 0]
    second = [r.cell for r in plan if r.repetition == 1]
    check("repetitions are shuffled differently from each other", first != second)

    # Seeding from seed+repetition would make campaign 1 pass 2 identical to campaign 2
    # pass 1, colliding two campaigns that are meant to be independent.
    a = [r.cell for r in ordering.plan(cells, 3, 100) if r.repetition == 1]
    b = [r.cell for r in ordering.plan(cells, 3, 101) if r.repetition == 0]
    check("campaign seeds do not collide across repetitions", a != b)

    # Cells built with the same factors in a different keyword order are one cell, so
    # the design cannot silently contain a duplicate.
    check("factor order does not create a second cell",
          ordering.Cell.of("x", a=1, b=2) == ordering.Cell.of("x", b=2, a=1))
    try:
        ordering.plan([cells[0], cells[0]], 1, 1)
        raised = False
    except ValueError:
        raised = True
    check("a duplicated cell is refused", raised)


def darwin_parser_checks() -> None:
    print("\n== the macOS probes are parsed, not guessed ==")

    # Fed recorded command output so they are checked from any platform. A regex that
    # stopped matching would otherwise report None forever, and a field that is quietly
    # always missing is the failure this whole module exists to prevent.
    check("mains is read from pmset -g ps",
          env_mod._power_source_darwin("Now drawing from 'AC Power'\n") == "AC Power")
    check("battery is read from the same line",
          env_mod._power_source_darwin("Now drawing from 'Battery Power'\n") == "Battery Power")
    check("an unrecognised pmset -g ps is unknown, not mains",
          "unchecked" in str(env_mod._power_source_darwin("")))

    check("low power mode off is read", env_mod._low_power_mode(" lowpowermode         0\n") is False)
    check("low power mode on is read", env_mod._low_power_mode(" lowpowermode         1\n") is True)
    check("hardware without the setting is unknown, not off",
          env_mod._low_power_mode(" powernap             1\n") is None)
    # macOS 26 renamed the key and made it tri-state. Both spellings are checked, because
    # a probe that knows only the retired one returns None for ever, and that absence is
    # indistinguishable from hardware which has no such mode.
    check("high power mode is not low power mode",
          env_mod._low_power_mode(" powermode            2\n") is False)
    check("powermode 1 is low power mode",
          env_mod._low_power_mode(" powermode            1\n") is True)
    check("powermode 0, automatic, is not low power mode",
          env_mod._low_power_mode(" powermode            0\n") is False)

    check("a throttled machine reports its speed limit",
          env_mod._cpu_speed_limit("\tCPU_Speed_Limit \t= 70\n") == 70)
    check("no published limit is not 100",
          env_mod._cpu_speed_limit("Note: No thermal warning level has been recorded") is None)

    known = {"logical_cores": 14}
    env_mod._set_if_probed(known, "logical_cores", None)
    check("a sysctl that did not answer leaves the known core count alone",
          known["logical_cores"] == 14)

    check("a non-darwin capture grows no macOS fields",
          sys.platform == "darwin"
          or "power" not in env_mod.capture(build_type="Release"))


def io_backend_checks() -> None:
    print("\n== on Linux the build says which backend, not the platform ==")

    def resolve(system: str, cache_value: str | None) -> object:
        """resolve_io_backend with the platform and the CMakeCache both pinned."""
        real_system = env_mod.platform.system
        real_read = env_mod.io_backend_from_build
        env_mod.platform.system = lambda: system
        env_mod.io_backend_from_build = lambda _build: cache_value
        try:
            return env_mod.resolve_io_backend(Path("build/pretend"))
        except ValueError as exc:
            return exc
        finally:
            env_mod.platform.system = real_system
            env_mod.io_backend_from_build = real_read

    # The one that used to be refused. linux-epoll is a supported preset, so a tree
    # configured with it is a legitimate single-backend build and not a disagreement
    # with the host.
    check("a Linux epoll tree resolves to epoll", resolve("Linux", "epoll") == "epoll")
    check("a Linux io_uring tree resolves to io_uring", resolve("Linux", "io_uring") == "io_uring")
    check("a Linux dual tree records dual", resolve("Linux", "dual") == "dual")

    # No cache is no answer on Linux, because the platform cannot supply one.
    check("a Linux tree with no readable cache is refused",
          isinstance(resolve("Linux", None), ValueError))

    # Elsewhere the platform does decide, so a cache that disagrees is a real conflict.
    check("macOS agreeing with its only backend is fine", resolve("Darwin", "kqueue") == "kqueue")
    check("macOS reading io_uring is refused", isinstance(resolve("Darwin", "io_uring"), ValueError))
    check("Windows with no cache falls back to the platform", resolve("Windows", None) == "iocp")

    print("\n== a cell names one arm, and only one the build actually contains ==")

    def arm(system: str, cache_value: str | None, requested: str | None) -> object:
        """run_io_backend with the platform and the CMakeCache both pinned."""
        real_system = env_mod.platform.system
        real_read = env_mod.io_backend_from_build
        env_mod.platform.system = lambda: system
        env_mod.io_backend_from_build = lambda _build: cache_value
        try:
            return env_mod.run_io_backend(Path("build/pretend"), requested)
        except ValueError as exc:
            return exc
        finally:
            env_mod.platform.system = real_system
            env_mod.io_backend_from_build = real_read

    # The default on a dual tree is io_uring, which is what every campaign already on
    # disk recorded: adding the flag must not move a run that does not pass it.
    check("a dual tree defaults to io_uring", arm("Linux", "dual", None) == "io_uring")
    check("a dual tree can be asked for epoll", arm("Linux", "dual", "epoll") == "epoll")

    # The one that used to make every cell of every campaign die at server start: an
    # epoll build was told to run io_uring, because the arm came from the platform.
    check("an epoll tree measures epoll without being asked",
          arm("Linux", "epoll", None) == "epoll")
    check("an epoll tree cannot be asked for io_uring",
          isinstance(arm("Linux", "epoll", "io_uring"), ValueError))
    check("an io_uring tree cannot be asked for epoll",
          isinstance(arm("Linux", "io_uring", "epoll"), ValueError))

    # Where the platform has one backend there is no arm to choose, and asking for one
    # is a mislabelled run rather than a harmless flag.
    check("macOS answers kqueue and refuses io_uring",
          arm("Darwin", "kqueue", None) == "kqueue"
          and isinstance(arm("Darwin", "kqueue", "io_uring"), ValueError))
    check("Windows answers iocp", arm("Windows", None, None) == "iocp")


def virtualisation_checks() -> None:
    print("\n== a host that cannot answer is not a clean host ==")

    # The gate existed and only ever worked on Linux. systemd-detect-virt is Linux-only,
    # _run maps an absent binary to None, and None read as bare metal, so every Windows
    # and macOS record this project holds was stamped bare metal unchecked.
    check("a Hyper-V guest is caught by its model",
          env_mod._virtualisation_from_identity(
              "Microsoft Corporation|Virtual Machine|American Megatrends") == "virtual machine")
    check("VMware is caught",
          env_mod._virtualisation_from_identity(
              "VMware, Inc.|VMware Virtual Platform|Phoenix") == "vmware")
    # Both of these would be false positives on real hardware, and one of them is the
    # machine that produced most of this project's data.
    check("a DIY desktop is not a false positive",
          env_mod._virtualisation_from_identity(
              "System manufacturer|System Product Name|American Megatrends Inc.") is None)
    check("a Surface is not a false positive, despite the manufacturer",
          env_mod._virtualisation_from_identity(
              "Microsoft Corporation|Surface Laptop Studio|Microsoft Corporation") is None)

    check("darwin reads its own sysctl: 1 is a hypervisor",
          "hypervisor" in str(env_mod._virtualisation_darwin("1")))
    check("darwin: 0 is metal", env_mod._virtualisation_darwin("0") is None)
    check("linux prefers systemd-detect-virt", env_mod._virtualisation_linux("docker") == "docker")
    check("linux: none is metal", env_mod._virtualisation_linux("none") is None)
    # None is the function's "go and ask" sentinel, not "there is nothing to ask", so
    # this case has to make the probe genuinely absent. Passing None asserted nothing on
    # a host that has systemd-detect-virt: the call answered `none`, returned before the
    # fallback the case is about, and the check failed on every Linux machine while
    # passing on this one.
    _real_run = env_mod._run
    env_mod._run = lambda cmd, **kw: None
    try:
        check("linux without systemd falls back to DMI",
              env_mod._virtualisation_linux(identity="QEMU Standard PC") == "qemu")
        check("linux with neither systemd nor DMI is unchecked, not clean",
              str(env_mod._virtualisation_linux(identity="  ")).startswith("unchecked"))
    finally:
        env_mod._run = _real_run
    # systemd-detect-virt says `none` with exit 1, so the bare-metal answer used to be
    # read as no answer and the verdict came from the SMBIOS heuristic instead.
    if sys.platform != "win32":
        check("exit 1 with output is an answer when the probe says so",
              env_mod._run(["sh", "-c", "echo none; exit 1"], ok=(0, 1)) == "none")
        check("exit 1 is still a failure by default",
              env_mod._run(["sh", "-c", "echo none; exit 1"]) is None)

    # The whole point. A probe that could not run must be distinguishable from a probe
    # that ran and found nothing, and must fail closed.
    real_run = env_mod._run
    env_mod._run = lambda cmd, **kw: None
    try:
        check("a windows probe that does not answer is unchecked, not clean",
              "unchecked" in str(env_mod._virtualisation_windows()))
        check("a darwin probe that does not answer is unchecked, not clean",
              "unchecked" in str(env_mod._virtualisation_darwin()))
        check("linux with neither probe is unchecked, not clean",
              "unchecked" in str(env_mod._virtualisation_linux(None, "")))
    finally:
        env_mod._run = real_run

    # The rule is a truthiness test, so the sentinel has to be truthy. It is, and this
    # is what makes fail-closed a mechanism rather than an intention.
    unchecked = validity.check_run({"virtualisation": env_mod._UNCHECKED.format("no probe"),
                                    "requests_total": 1, "requests_non_2xx": 0})
    check("an unchecked host is refused by validity", not unchecked.valid)
    check("and the reason names it", any("unchecked" in r for r in unchecked.reasons))


def power_checks() -> None:
    print("\n== a host that cannot say it is on mains is not on mains ==")

    check("darwin mains", env_mod._power_source_darwin("Now drawing from 'AC Power'") == "AC Power")
    check("darwin battery",
          "battery" in env_mod._power_source_darwin("Now drawing from 'Battery Power'").lower())
    check("darwin pmset that says nothing useful is unchecked",
          "unchecked" in str(env_mod._power_source_darwin("no such line")))

    check("windows desktop has no Win32_Battery instance and says so",
          env_mod._power_source_windows("none") == env_mod._NO_BATTERY)
    check("windows on AC", env_mod._power_source_windows("2") == env_mod._ON_MAINS)
    check("windows discharging is refused",
          "battery" in env_mod._power_source_windows("1").lower())
    check("windows WMI that does not answer is unchecked",
          "unchecked" in str(env_mod._power_source_windows(None)) or sys.platform == "win32")

    # The case the macOS session flagged for the Linux box: on many desktop builds
    # /sys/class/power_supply exists and is EMPTY rather than missing. An empty
    # directory must land on the desktop answer, not on a silent pass, because a silent
    # pass is the exact shape of the three fail-opens already found.
    check("an empty /sys/class/power_supply is a desktop, not an unknown",
          env_mod._power_source_linux([], lambda n, f: None) == env_mod._NO_BATTERY)
    check("a mains supply with no battery device is also a desktop",
          env_mod._power_source_linux(["AC"], lambda n, f: "Mains" if f == "type" else "x")
          == env_mod._NO_BATTERY)
    check("a charging laptop is on mains",
          env_mod._power_source_linux(
              ["BAT0"], lambda n, f: {"type": "Battery", "status": "Charging"}.get(f))
          == env_mod._ON_MAINS)
    check("a discharging laptop is refused",
          "battery" in env_mod._power_source_linux(
              ["BAT0"], lambda n, f: {"type": "Battery", "status": "Discharging"}.get(f)).lower())
    check("a battery that publishes no status is unchecked, not clean",
          "unchecked" in str(env_mod._power_source_linux(
              ["BAT0"], lambda n, f: "Battery" if f == "type" else "")))
    # A wireless mouse is a Battery of scope Device and is Discharging whenever it is
    # off its cable. It does not power the host and must not put a desktop on battery.
    hidpp = {"type": "Battery", "scope": "Device", "status": "Discharging"}
    check("a peripheral's battery is not the host's",
          env_mod._power_source_linux(["hidpp_battery_0"], lambda n, f: hidpp.get(f))
          == env_mod._NO_BATTERY)
    # BAT0 publishes no scope file, so the default must be System or a discharging
    # laptop with a wireless mouse would read as a desktop.
    bat0 = {"type": "Battery", "status": "Discharging"}
    check("a laptop battery without a scope file is still the host's",
          "battery" in env_mod._power_source_linux(
              ["BAT0", "hidpp_battery_0"],
              lambda n, f: (bat0 if n == "BAT0" else hidpp).get(f)).lower())

    # The trap in the value itself: validity tests for the substring "battery", so a
    # healthy host's string must not contain it. "no battery present" would have refused
    # the machine it describes.
    # Both clock gates were dead on Windows: the drift check read /proc/cpuinfo and the
    # thermal check read pmset, so the platform that produced every measurement in this
    # project had no clock-stability check of any kind, on an rdtsc microbenchmark.
    check("linux clock is the fastest core, not the average one",
          env_mod._cpu_mhz_linux("cpu MHz\t: 3600.5\ncpu MHz\t: 400.0\n") == 3600.5)
    check("windows clock is base times performance percentage",
          env_mod._cpu_mhz_windows("3950|99") == 3950 * 0.99)
    # Get-Counter returns a localised decimal and this project's own Windows host
    # formats it "99,9". Reading the integer counter avoids a probe that works
    # everywhere except on a comma-decimal machine, which is where it runs.
    check("a windows reading that is not two numbers is None, not a guess",
          env_mod._cpu_mhz_windows("99,9") is None)
    check("an empty windows reading is None", env_mod._cpu_mhz_windows("") is None)
    # And what the wrapper makes of that None on each platform.
    check("a linux cpuinfo without cpu MHz lines is unchecked",
          "unchecked" in str(env_mod.cpu_mhz("Linux", "processor\t: 0\n")))
    check("a windows probe that did not answer is unchecked",
          "unchecked" in str(env_mod.cpu_mhz("Windows", "")))
    check("a linux reading is passed through",
          env_mod.cpu_mhz("Linux", "cpu MHz\t: 3600.0\n") == 3600.0)
    check("darwin has no clock probe and says None", env_mod.cpu_mhz("Darwin") is None)

    check("the desktop string does not trip the rule it is read by",
          "battery" not in env_mod._NO_BATTERY.lower())
    check("and a desktop is accepted end to end",
          validity.check_run({"power_source": env_mod._NO_BATTERY, "git_dirty": False,
                              "requests_total": 1, "requests_non_2xx": 0}).valid)


def live_capture_check() -> None:
    print("\n== capture works on this machine ==")

    captured = env_mod.capture(build_type="Release", io_backend="io_uring")
    check("capture returns a fingerprintable dict",
          isinstance(env_mod.fingerprint(captured), str))
    check("capture is stable across calls",
          env_mod.fingerprint(captured)
          == env_mod.fingerprint(env_mod.capture(build_type="Release", io_backend="io_uring")))
    check("it serialises", isinstance(json.dumps(captured), str))

    virt = captured.get("virtualisation")
    print(f"     this machine reports virtualisation: {virt!r}")
    if virt:
        print("     so it is correctly refused as a source of performance records")


def clocksource_checks() -> None:
    """Which kernel clock served the run, because it decides what reading one costs.

    With the TSC the vDSO answers clock_gettime in userspace. With HPET it cannot, so
    every call is a syscall plus an MMIO read: 1931 ns against 5 ns on the host where
    this was found, and one syscall per call where a TSC host makes none. That lands in
    every latency figure and every syscall count, and until this key existed two hosts
    differing only in it would pool into one campaign without a word.
    """
    from benchmark.harness import environment as env_mod

    print("\n== the kernel clock is part of the machine, and is fingerprinted ==")
    check("clocksource is fingerprinted", "tuning.clocksource" in env_mod._FINGERPRINTED)

    base = {
        "machine": {"node": "h", "arch": "x86_64"},
        "kernel": {"release": "6.1"},
        "cpu": {"model": "m", "physical_cores": 8, "logical_cores": 16,
                "governor": "performance"},
        "memory": {"total_bytes": 1},
        "tuning": {"transparent_hugepages": None, "swappiness": None,
                   "clocksource": "tsc"},
        "toolchain": {"compiler": "g++"},
        "build": {"type": "Release", "io_backend": "epoll", "git_commit": "abc"},
        "deps": {},
    }
    other = json.loads(json.dumps(base))
    other["tuning"]["clocksource"] = "hpet"
    check("two hosts differing only in the clock are different campaigns",
          env_mod.fingerprint(base) != env_mod.fingerprint(other))

    same = json.loads(json.dumps(base))
    check("and one that matches is the same campaign",
          env_mod.fingerprint(base) == env_mod.fingerprint(same))


def clock_probe_checks() -> None:
    """The clock reading must move when the machine is throttled and not when it idles.

    The average across cores fails that: a parked core sits at a few hundred megahertz, so
    on a lightly loaded machine the mean reports how many cores were asleep at the instant
    it was read. The drift rule then refused every run one host produced, at 26 percent
    when the offered load was low and 2 to 4 percent when it rose, which is the wrong
    direction for thermal throttling and the right one for an average full of idle cores.
    """
    from benchmark.harness import environment as env_mod

    print("\n== the clock reading follows the package, not the idle cores ==")

    def cpuinfo(*mhz: float) -> str:
        return "".join(f"processor\t: {i}\ncpu MHz\t\t: {v}\n"
                       for i, v in enumerate(mhz))

    loaded = env_mod._cpu_mhz_linux(cpuinfo(*[4000.0] * 16))
    parked = env_mod._cpu_mhz_linux(cpuinfo(*([400.0] * 12 + [4000.0] * 4)))
    throttled = env_mod._cpu_mhz_linux(cpuinfo(*[2500.0] * 16))

    check("a machine with cores asleep reads the same as a busy one", parked == loaded)
    check("a throttled package reads lower", throttled is not None and throttled < loaded)
    check("and by enough for the rule to see it",
          abs(throttled - loaded) / loaded > 0.02)
    check("a /proc/cpuinfo with no clock lines is unchecked, not clean",
          str(env_mod.cpu_mhz("Linux", "processor\t: 0\n")).startswith("unchecked"))


def ladder_coverage_checks() -> None:
    """A ladder that does not test the rates it validates is not a ladder.

    The campaign's offered rates are chosen from a ladder run on the host, and the rule
    that lets a campaign start is that every rate in the table was accepted in its ladder.
    That rule is unsatisfiable for a rate the ladder never visits, and the gap does not
    announce itself: the campaign simply offers a rate nobody measured. It happened, at
    the one rate whose tested neighbours paced at 83 microseconds and at 3012.

    The second half is the same rule from the other side. One design, one shape, one
    ceiling: a rate that is nothing for a reused connection can be several times what
    establishment sustains, and a design that offers one rate to both shapes fails half
    its cells on every host for ever.
    """
    from benchmark import run_campaign as rc

    print("\n== a ladder visits the rates it exists to validate ==")
    for design, table in (
        ("churn-ladder", set(rc.CHURN_OFFERED_RATES) | set(rc.CHURN_NET_OFFERED_RATES)),
        ("tls-ladder", set(rc.TLS_OFFERED_RATES)),
    ):
        rates = {cell.as_dict()["offered_rate"] for cell in rc.DESIGNS[design]()}
        check(f"{design} visits every rate it validates", table <= rates)

    ceiling = max(set(rc.CHURN_OFFERED_RATES) | set(rc.CHURN_NET_OFFERED_RATES))
    for design in ("tls-smoke", "churn", "churn-net", "transport", "h1-deep"):
        offered = {
            cell.as_dict()["offered_rate"]
            for cell in rc.DESIGNS[design]()
            if int(cell.as_dict().get("max_requests_per_connection", 0)) == 1
        }
        check(
            f"{design} offers establishment no rate above what a ladder admitted",
            not offered or max(offered) <= ceiling,
        )


def staleness_checks() -> None:
    """A binary that does not contain the source its record will claim.

    Both halves are here because each caught a real case on one machine in one night: a
    tree that could not rebuild at all, whose binaries were three days behind, and a tree
    reconfigured and never rebuilt, whose cache and executable described different source.
    The fallback path is exercised too, since a machine without ninja is the one where the
    coarse comparison decides.
    """
    from benchmark.harness import environment as env_mod

    print("\n== a binary is not the commit the tree is checked out at ==")
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        repo = root / "repo"
        (repo / "src").mkdir(parents=True)
        source = repo / "src" / "app.cpp"
        source.write_text("int main() { return 0; }\n")

        build = root / "build"
        build.mkdir()
        cache = build / "CMakeCache.txt"
        cache.write_text("COROUTE_IO_BACKEND:STRING=epoll\n")
        exe = build / "server"
        exe.write_text("binary")

        # No build.ninja, so this exercises the file-comparison fallback throughout.
        os.utime(source, (1_000, 1_000))
        os.utime(cache, (1_000, 1_000))
        os.utime(exe, (2_000, 2_000))
        check("a binary newer than its source and its cache passes",
              env_mod.build_staleness(build, repo, (exe,)) == [])

        os.utime(source, (3_000, 3_000))
        problems = env_mod.build_staleness(build, repo, (exe,))
        check("a binary older than a compiled source is refused",
              len(problems) == 1 and "older than" in problems[0])

        os.utime(source, (1_000, 1_000))
        os.utime(cache, (3_000, 3_000))
        problems = env_mod.build_staleness(build, repo, (exe,))
        check("a tree reconfigured and not rebuilt is refused",
              len(problems) == 1 and "CMakeCache" in problems[0])

        check("an executable that does not exist is not a staleness problem",
              env_mod.build_staleness(build, repo, (build / "absent",)) == [])

        empty = root / "empty"
        (empty).mkdir()
        os.utime(cache, (1_000, 1_000))
        problems = env_mod.build_staleness(build, empty, (exe,))
        check("a tree with no compiled source at all refuses rather than passes",
              len(problems) == 1 and problems[0].startswith("unchecked"))

    # The matcher commit is a dependency version and is read from the build tree, because
    # it is fetched by commit at configure time and pkg-config never sees it.
    with tempfile.TemporaryDirectory() as tmp:
        build = Path(tmp)
        (build / "CMakeCache.txt").write_text(
            "COROUTE_IO_BACKEND:STRING=dual\n"
            "COROUTE_URL_MATCHER_TAG:STRING=d16f30a81158d620e5a5514087b175a2251e4fa3\n"
        )
        check("the matcher commit is read from the build tree",
              env_mod.cache_value(build, "COROUTE_URL_MATCHER_TAG")
              == "d16f30a81158d620e5a5514087b175a2251e4fa3")
        check("a key the cache does not carry reads as absent",
              env_mod.cache_value(build, "COROUTE_NOT_A_KEY") is None)
        check("a build tree that cannot be read reads as absent",
              env_mod.cache_value(Path(tmp) / "nope", "COROUTE_URL_MATCHER_TAG") is None)


def main() -> int:
    from benchmark.harness import selfcheck_driver, selfcheck_results

    fingerprint_checks()
    campaign_checks()
    transport_checks()
    netem_checks()
    port_checks()
    topology_checks()
    validity_checks()
    counter_checks()
    schema_checks()
    ordering_checks()
    ladder_coverage_checks()
    clock_probe_checks()
    clocksource_checks()
    staleness_checks()
    selfcheck_driver.run(check)
    selfcheck_results.run(check)
    darwin_parser_checks()
    virtualisation_checks()
    io_backend_checks()
    power_checks()
    live_capture_check()
    print(f"\n{PASSED} checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
