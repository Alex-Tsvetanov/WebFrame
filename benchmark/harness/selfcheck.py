"""Self-check for the harness guards.

Plain asserts and no test framework, so it runs anywhere Python does. The guards are
the part of the harness that has to work when nobody is watching, months into a
campaign, which is exactly when an unrunnable test suite is no help.

    python3 benchmark/harness/selfcheck.py
"""

from __future__ import annotations

import json
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


def validity_checks() -> None:
    print("\n== the pre-declared rejection criteria ==")

    good = {
        "virtualisation": None, "git_dirty": False,
        "requests_total": 1_000_000, "requests_non_2xx": 0,
        "generator_cpu_fraction": 0.42,
        "cpu_mhz_start": 4200.0, "cpu_mhz_end": 4190.0,
        "counter_deltas": {"UdpRcvbufErrors": 0, "TcpExtListenOverflows": 0},
    }
    check("a clean run is accepted", validity.check_run(good).valid)

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

    dropped = dict(good, counter_deltas={"UdpRcvbufErrors": 17, "TcpExtListenOverflows": 0})
    verdict = validity.check_run(dropped)
    check("a run where the kernel dropped datagrams is refused", not verdict.valid)
    check("the reason names the counter", any("UdpRcvbufErrors" in r for r in verdict.reasons))

    virt = dict(good, virtualisation="kvm")
    check("a run under virtualisation is refused", not validity.check_run(virt).valid)

    dirty = dict(good, git_dirty=True)
    check("a run from a dirty tree is refused", not validity.check_run(dirty).valid)

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
    check("a UDP counter is found by name", before.get("RcvbufErrors") == 0)
    check("a TcpExt counter is prefixed", before.get("TcpExtListenOverflows") == 0)

    after = validity.read_counters(
        snmp=snmp.replace("Udp: 100 1 0 90 0 0", "Udp: 200 1 0 190 42 0"),
        netstat=netstat,
    )
    deltas = validity.counter_deltas(
        {"UdpRcvbufErrors": before.get("RcvbufErrors", 0), "TcpExtListenOverflows": 0},
        {"UdpRcvbufErrors": after.get("RcvbufErrors", 0), "TcpExtListenOverflows": 0},
    )
    check("a delta is computed", deltas["UdpRcvbufErrors"] == 42)

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
    fingerprint_checks()
    campaign_checks()
    validity_checks()
    counter_checks()
    schema_checks()
    ordering_checks()
    live_capture_check()
    print(f"\n{PASSED} checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
