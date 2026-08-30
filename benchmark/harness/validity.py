"""Whether a run is allowed to become a data point.

Every threshold here is declared before any data is collected, and that ordering is the
whole point. Rejection criteria chosen after seeing the results are indistinguishable
from choosing the results: there is always a defensible-sounding reason to drop the run
that spoils the trend.

So these are fixed in advance, they are few, and each one names a specific mechanical
fault rather than an opinion about whether a number looks right. A run that is merely
surprising is kept. The count of rejections is reported alongside the results, because
a method that silently discards a third of its runs is a different method from one that
discards none.

None of these is about latency being high. Slow is a result.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from benchmark.harness import environment


# --- Pre-declared thresholds -------------------------------------------------
#
# Fixed before data collection. Changing one is a change to the method and belongs in
# the methodology chapter, not in a commit that also touches the results.

# Above this share of non-2xx responses the server was not serving, it was failing, and
# a failure is cheap. This is the criterion that matters most: without it a server that
# collapses under load wins on throughput, because errors are faster than work.
MAX_ERROR_RATE = 0.001  # 0.1%

# Above this the load generator is the bottleneck and the measurement is of the client.
# The most common way a framework benchmark is wrong.
#
# Applied to closed loop runs only. An open loop paces by spinning, because no sleep
# is accurate to the tens of microseconds a fixed rate needs, so it sits at full CPU
# whether or not it is keeping up. Judging it by CPU would refuse every valid open
# loop run and accept none.
MAX_GENERATOR_CPU = 0.85  # 85%

# What an open loop is judged by instead: how late the generator was in getting a
# request onto the socket, relative to when that request was due. A generator that
# fell milliseconds behind was offering a different load than the record claims, and
# the resulting latency is partly its own queueing.
MAX_PACING_LAG_US = 1000.0  # 1 ms at p99

# And whether it delivered the rate it promised at all.
MIN_ACHIEVED_SHARE = 0.99

# Sustained clock has to stay close to what the run started at. Thermal throttling
# midway produces a slowdown that belongs to the room, not to the code.
MAX_FREQUENCY_DRIFT = 0.02  # 2%

# Kernel counters that must not move during a run. Each means the machine dropped work
# before the server saw it, so the server cannot be held responsible for the result and
# cannot be credited with it either.
ZERO_DELTA_COUNTERS = ("UdpRcvbufErrors", "TcpExtListenOverflows")


@dataclass
class Verdict:
    """Why a run was refused, or nothing if it was accepted."""

    reasons: list[str] = field(default_factory=list)

    @property
    def valid(self) -> bool:
        return not self.reasons

    def __bool__(self) -> bool:
        return self.valid


def check_run(record: dict[str, Any]) -> Verdict:
    """Applies every pre-declared criterion to one run.

    All of them, rather than stopping at the first. A run can be invalid for more than
    one reason and knowing all of them is what tells you whether the rig is broken or
    the run was unlucky.
    """
    verdict = Verdict()

    virt = record.get("virtualisation")
    if virt:
        # Refused outright rather than warned about. Scheduling and timer behaviour
        # under a hypervisor differ enough that tail latency is not the machine's, and
        # a warning in a log is not a mechanism.
        verdict.reasons.append(
            f"virtualisation detected ({virt}); performance records must come from bare metal"
        )

    if record.get("git_dirty"):
        # The commit hash would not describe the binary that ran, which is worse than
        # having no hash: it looks authoritative and is not.
        verdict.reasons.append("working tree was dirty; the recorded commit does not describe the binary")

    total = record.get("requests_total")
    non_2xx = record.get("requests_non_2xx")
    if total and non_2xx is not None:
        rate = non_2xx / total
        if rate > MAX_ERROR_RATE:
            verdict.reasons.append(
                f"non-2xx rate {rate:.4%} exceeds {MAX_ERROR_RATE:.2%}; "
                "the server was failing rather than serving"
            )

    # Which rule applies depends on the loop, because the two fail differently.
    # offered_rate is set for an open loop and None for a closed one.
    open_loop = record.get("offered_rate") is not None

    if not open_loop:
        generator_cpu = record.get("generator_cpu_fraction")
        if generator_cpu is not None and generator_cpu > MAX_GENERATOR_CPU:
            verdict.reasons.append(
                f"generator CPU {generator_cpu:.1%} exceeds {MAX_GENERATOR_CPU:.0%}; "
                "the measurement is of the load generator"
            )
    else:
        lag = record.get("generator_pacing_p99_us")
        if lag is not None and lag > MAX_PACING_LAG_US:
            verdict.reasons.append(
                f"generator was {lag:.0f}us behind its own schedule at p99, above "
                f"{MAX_PACING_LAG_US:.0f}us; the offered load was not the load recorded"
            )
        share = record.get("generator_achieved_share")
        if share is not None and share < MIN_ACHIEVED_SHARE:
            verdict.reasons.append(
                f"generator delivered {share:.1%} of the offered rate, below "
                f"{MIN_ACHIEVED_SHARE:.0%}; it could not sustain the schedule"
            )

    start = record.get("cpu_mhz_start")
    end = record.get("cpu_mhz_end")
    if start and end:
        drift = abs(end - start) / start
        if drift > MAX_FREQUENCY_DRIFT:
            verdict.reasons.append(
                f"CPU frequency drifted {drift:.1%} (from {start:.0f} to {end:.0f} MHz), "
                f"over the {MAX_FREQUENCY_DRIFT:.0%} limit"
            )

    counters = record.get("counter_deltas") or {}
    for name in ZERO_DELTA_COUNTERS:
        value = counters.get(name)
        if value:
            verdict.reasons.append(
                f"{name} increased by {value} during the run; "
                "the kernel dropped work before the server saw it"
            )

    # A socket error is an absent response, not a slow one, and the non-2xx rate above
    # cannot see it: a connection that failed produced no status code to classify.
    # Without this, a run where a third of the connections died reports a clean error
    # rate over the survivors.
    socket_errors = record.get("socket_errors")
    if socket_errors:
        verdict.reasons.append(
            f"{socket_errors} socket errors during the run; "
            "these are absent responses and the non-2xx rate does not count them"
        )

    # A handshake that failed is a connection that never carried a request, and unlike a
    # socket error it can fail silently in the client's favour: the connections that did
    # complete are the fast ones, so a run where a tenth of the handshakes were refused
    # reports a clean and flattering establishment distribution over the survivors.
    handshake_failures = record.get("handshake_failures")
    if handshake_failures:
        verdict.reasons.append(
            f"{handshake_failures} TLS handshakes failed during the run; "
            "the establishment distribution is over the connections that survived"
        )

    # A run recorded as TLS whose generator never negotiated anything did not measure
    # TLS. This is the check for the failure the whole arm exists to avoid: a client
    # that quietly connected in cleartext while the record said otherwise.
    if record.get("tls") and not record.get("tls_version"):
        verdict.reasons.append(
            "record claims tls but the generator reported no negotiated version; "
            "nothing establishes that this run was encrypted"
        )

    # Isolation asked for and not granted. The comparison itself survives an unpinned
    # host, since both arms meet the same scheduler, but the run must not be recorded as
    # though it had the isolation the design specifies. A campaign on a platform with no
    # affinity API asks for no mask at all, so this fires on the mistake it is for:
    # running the pinned design somewhere pinning does not exist.
    requested = record.get("affinity_requested")
    if requested and record.get("affinity_applied") is False:
        verdict.reasons.append(
            f"affinity mask {requested} was requested and the platform did not apply it; "
            "this run did not have the isolation the design specifies"
        )

    # A laptop on battery is what a virtualised host is to a server: the number describes
    # something other than the code. On a chip with heterogeneous cores, discharging
    # shifts placement toward the efficiency cores and caps clocks, and neither shows up
    # in any criterion above.
    power = record.get("power_source")
    if power and "battery" in power.lower():
        verdict.reasons.append(
            f"host was on {power} rather than mains; "
            "clocks and core placement are not the ones this campaign describes"
        )

    # The macOS counterpart of the frequency drift check, which reads a sysfs path that
    # does not exist there. CPU_Speed_Limit is a percentage and sits at 100 on an
    # unthrottled machine, so anything lower at either end means the run and the machine
    # disagree about what was executing.
    for label, value in (("start", record.get("thermal_speed_limit_start")),
                         ("end", record.get("thermal_speed_limit_end"))):
        if value is not None and value < 100:
            verdict.reasons.append(
                f"CPU speed limit was {value}% at {label} of run; the host was throttled"
            )

    return verdict


def read_counters(snmp: str | None = None, netstat: str | None = None) -> dict[str, int]:
    """Pulls the watched counters out of /proc/net/snmp and /proc/net/netstat.

    Both files are the same shape: a header line of names and a value line beneath it,
    repeated per protocol. Parsed by pairing the two lines rather than by column index,
    because the columns differ between kernel versions and a fixed index would read the
    wrong counter without ever failing.
    """
    out: dict[str, int] = {}
    for text in (snmp if snmp is not None else _read("/proc/net/snmp"),
                 netstat if netstat is not None else _read("/proc/net/netstat")):
        if not text:
            continue
        lines = text.splitlines()
        for header, values in zip(lines[::2], lines[1::2]):
            prefix_h, _, rest_h = header.partition(":")
            prefix_v, _, rest_v = values.partition(":")
            if prefix_h != prefix_v:
                continue
            names = rest_h.split()
            numbers = rest_v.split()
            for name, number in zip(names, numbers):
                key = name if prefix_h in ("Udp", "Tcp", "Ip") else f"{prefix_h}{name}"
                try:
                    out[key] = int(number)
                except ValueError:
                    continue
    return out


def counter_deltas(before: dict[str, int], after: dict[str, int]) -> dict[str, int]:
    """Change in the watched counters across a run.

    Only the watched ones. The rest are recorded elsewhere; carrying every counter into
    the validity check would invite reading meaning into whichever one happened to move.
    """
    return {
        name: after.get(name, 0) - before.get(name, 0)
        for name in ZERO_DELTA_COUNTERS
        if name in before or name in after
    }


def _read(path: str) -> str | None:
    try:
        return Path(path).read_text(encoding="utf-8", errors="replace")
    except OSError:
        return None


def current_cpu_mhz() -> float | None:
    """Mean current clock across cores, for the drift check."""
    text = _read("/proc/cpuinfo")
    if not text:
        return None
    values = [float(m) for m in re.findall(r"^cpu MHz\s*:\s*([\d.]+)$", text, re.MULTILINE)]
    return sum(values) / len(values) if values else None


def current_power_source() -> str | None:
    """Mains or battery, on hosts where that can change during a campaign.

    Delegates to environment.py, which owns the parser and has recorded-output checks
    over it in selfcheck.py. A second parser here would be a second chance to disagree
    with the manifest that describes the same run.
    """
    return environment._power_source()


def current_speed_limit() -> int | None:
    """CPU_Speed_Limit as a percentage, 100 on an unthrottled Mac.

    Stands in for the frequency drift check above, which reads a sysfs path macOS does
    not have. Without it a laptop that throttled halfway through a three hour campaign
    satisfies every other rule in this file.
    """
    return environment._cpu_speed_limit()
