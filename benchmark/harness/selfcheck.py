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
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from benchmark.harness import environment as env_mod
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
    check("the same environment hashes the same", env_mod.fingerprint(a) == env_mod.fingerprint(sample_env()))

    # The whole reason for a curated field list. If everything were hashed, every run
    # would be its own campaign and the guard would fire constantly, which is how
    # guards get switched off.
    noisy = sample_env()
    noisy["observed"] = {"load_average": 3.7, "uptime_seconds": 91234}
    check("unfingerprinted fields do not change the hash",
          env_mod.fingerprint(noisy) == env_mod.fingerprint(a))

    for field, value, label in [
        ("cpu.governor", "powersave", "governor moving to powersave"),
        ("kernel.release", "6.9.0-1-generic", "a kernel upgrade"),
        ("build.git_commit", "b" * 40, "a different commit"),
        ("deps.openssl", "3.5.9", "a dependency rebuild"),
        ("cpu.physical_cores", 4, "SMT or core count changing"),
        ("tuning.transparent_hugepages", "always", "a hugepage setting change"),
    ]:
        section, _, key = field.partition(".")
        changed = sample_env(**{field: value})
        check(f"{label} changes the hash", env_mod.fingerprint(changed) != env_mod.fingerprint(a))

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
    check("only the watched counters are returned", set(deltas) <= set(validity.ZERO_DELTA_COUNTERS))


def live_capture_check() -> None:
    print("\n== capture works on this machine ==")

    captured = env_mod.capture(build_type="Release", io_backend="io_uring")
    check("capture returns a fingerprintable dict", isinstance(env_mod.fingerprint(captured), str))
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
    live_capture_check()
    print(f"\n{PASSED} checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
