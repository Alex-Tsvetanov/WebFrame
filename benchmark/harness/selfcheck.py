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
    def env_with(loopback: bool, location: str, host: str = "10.0.0.1") -> dict:
        env = sample_env()
        env["transport_path"] = {"host": host, "loopback": loopback,
                                 "generator_location": location}
        return env

    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "runs.env.json"
        campaign = env_mod.Campaign.open_or_create(path, env_with(False, "netns:gen"))
        check("a new campaign matches itself",
              transport_mismatch(campaign, env_with(False, "netns:gen"), "transport_path") is None)
        reopened = env_mod.Campaign.open_or_create(path, env_with(False, "netns:gen", "10.0.0.2"))
        check("the address may change between sessions of one arrangement",
              transport_mismatch(reopened, env_with(False, "netns:gen", "10.0.0.2"), "transport_path") is None)
        message = transport_mismatch(reopened, env_with(True, "host"), "transport_path")
        check("a loopback run cannot join a network-path campaign", message is not None)
        check("and the refusal names the key", "loopback" in message)
        # A manifest from before the section existed says nothing about its arrangement.
        older = env_mod.Campaign.open_or_create(Path(tmp) / "old.env.json", sample_env())
        check("a manifest without the section is refused, not assumed",
              transport_mismatch(older, env_with(False, "netns:gen"), "transport_path") is not None)


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
    refuse_held_port(port)
    check("the same port passes once the listener is gone", True)

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


def topology_checks() -> None:
    print("\n== the affinity masks are checked against the sibling layout ==")

    from benchmark import run_campaign

    def fake_sysfs(layout: list[str]) -> Path:
        root = Path(tempfile.mkdtemp()) / "cpu"
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
    siblings = env_mod._siblings(fake_sysfs(paired))
    check("siblings are read in CPU order, not lexicographic", siblings == paired)
    check("adjacent pairs keep the masks disjoint",
          run_campaign.mask_cores("0ff", siblings).isdisjoint(run_campaign.mask_cores("f00", siblings)))

    # Cores first, then their SMT threads: the same two masks now share four cores.
    interleaved = ["0,6", "1,7", "2,8", "3,9", "4,10", "5,11",
                   "0,6", "1,7", "2,8", "3,9", "4,10", "5,11"]
    siblings = env_mod._siblings(fake_sysfs(interleaved))
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
        check("no topology to read leaves the masks alone",
              run_campaign.isolation_problem({"cpu": {"siblings": None}}) is None)
    finally:
        run_campaign.SERVER_AFFINITY, run_campaign.GENERATOR_AFFINITY = masks
    check("no sysfs is None, not an empty layout", env_mod._siblings(Path(tempfile.mkdtemp()) / "x") is None)


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
    check("linux without systemd falls back to DMI",
          env_mod._virtualisation_linux(None, "QEMU Standard PC") == "qemu")
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
    check("linux clock is the mean across cores",
          env_mod._cpu_mhz_linux("cpu MHz\t: 3600.5\ncpu MHz\t: 3599.5\n") == 3600.0)
    check("windows clock is base times performance percentage",
          env_mod._cpu_mhz_windows("3950|99") == 3950 * 0.99)
    # Get-Counter returns a localised decimal and this project's own Windows host
    # formats it "99,9". Reading the integer counter avoids a probe that works
    # everywhere except on a comma-decimal machine, which is where it runs.
    check("a windows reading that is not two numbers is None, not a guess",
          env_mod._cpu_mhz_windows("99,9") is None)
    check("an empty windows reading is None", env_mod._cpu_mhz_windows("") is None)

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


def main() -> int:
    from benchmark.harness import selfcheck_driver, selfcheck_results

    fingerprint_checks()
    campaign_checks()
    transport_checks()
    port_checks()
    topology_checks()
    validity_checks()
    counter_checks()
    schema_checks()
    ordering_checks()
    selfcheck_driver.run(check)
    selfcheck_results.run(check)
    darwin_parser_checks()
    virtualisation_checks()
    power_checks()
    live_capture_check()
    print(f"\n{PASSED} checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
