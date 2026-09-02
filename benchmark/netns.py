"""The two-namespace pair the methodology names, created and torn down.

    python3 -m benchmark.netns up
    python3 -m benchmark.netns status
    python3 -m benchmark.netns down

Why this exists at all. A loopback run is not a network run: it skips the driver, the
queueing discipline and the interrupt path, which are exactly the fixed costs a
per-request difference has to compete with. Measuring over loopback flatters whichever
arm is slower, because the part that differs is a larger share of a smaller total. The
methodology chapter names two network namespaces joined by a veth as the accepted
arrangement on Linux, and until now nothing implemented it.

It is not a substitute for two machines. Both ends share one kernel, one scheduler and
one set of caches, and a veth pair is not a NIC: there is no PHY, no DMA and no real
interrupt moderation. What it does buy is a real qdisc, a real driver transmit path and
a real receive softirq, which is the part loopback omits and the part the numbers are
sensitive to.

Everything here runs under sudo, because creating a namespace needs CAP_NET_ADMIN and
this host does not allow unprivileged user namespaces (kernel.unprivileged_userns_clone
is 0). Nothing is written to /etc and nothing survives a reboot; `down` removes
everything `up` created.
"""

from __future__ import annotations

import argparse
import ipaddress
import shutil
import subprocess
import sys

# The two namespaces. Named rather than generated so that a campaign command line, a
# record's generator_location and a stale namespace left by a killed run all agree.
SERVER_NS = "srv"
GENERATOR_NS = "gen"

# One veth pair, one end in each namespace, named for the namespace it lives in.
SERVER_IF = "veth-srv"
GENERATOR_IF = "veth-gen"

# A /30 is exactly two usable addresses, which is exactly what a point-to-point pair
# needs, and it cannot silently host a third thing.
DEFAULT_SUBNET = "10.77.0.0/30"

# netem's default limit is 1000 packets, and it drops silently past it.
#
# That default is a trap at these rates. The queue has to hold a delay-bandwidth
# product: at 20000 requests per second and a 50 ms round trip, roughly 1000 packets are
# in flight at any instant, which is the default exactly. A campaign would then be
# measuring netem's overflow rather than the server, and the only visible symptom is a
# loss rate nobody asked for. Sized well above anything the rig offers instead.
DEFAULT_LIMIT = 100000

# Named delay, jitter and loss profiles. The value is the netem arguments for ONE
# direction; the pair is symmetric, so a 25 ms per-direction delay is a 50 ms round trip.
#
# "none" is not a zero-delay netem. It is no qdisc at all, because an unnecessary qdisc
# is still a queue: netem with no impairment still enqueues, dequeues and accounts, and
# a control arm that carries that cost is not a control.
PROFILES: dict[str, list[str] | None] = {
    "none": None,
    "lan": ["delay", "500us"],
    "wan50": ["delay", "25ms"],
    "wan100": ["delay", "50ms"],
    "jitter": ["delay", "25ms", "5ms", "distribution", "normal"],
    "loss1": ["loss", "1%"],
}


def observed_profile(text: str) -> str:
    """The impairment a `tc qdisc show dev X` output describes, normalised.

    What is installed, never what was asked for. A campaign records netem_profile as a
    factor and results2tex groups tables on it, so the string has to come from the kernel
    or it is an assertion nobody checked: every design used to hardcode "none", which
    meant a campaign run behind `netns up --profile wan100` produced records positively
    claiming an unimpaired path across a 100 ms round trip, and a wan100 file and a clean
    one pooled as repetitions of one cell.

    Named by its arguments rather than by a PROFILES key on purpose. The mapping back to
    a name is a guess about how iproute2 renders what it was given -- it does not echo
    `distribution normal`, for one -- and a guess that fails silently is what this whole
    field exists to avoid. The arguments are what the path actually carries.

    Normalised because the handle number and refcnt vary between sessions of one
    arrangement, and transport_mismatch compares this string across a campaign.

    No netem qdisc is "none", which is the truth about a veth that carries none: netns.py
    installs no qdisc at all for that profile, because netem with no impairment still
    enqueues, dequeues and accounts.
    """
    for line in text.splitlines():
        fields = line.split()
        if len(fields) < 2 or fields[0] != "qdisc" or fields[1] != "netem":
            continue
        kept: list[str] = []
        skip = False
        for token in fields[1:]:
            if skip:
                skip = False
                continue
            if token == "refcnt":
                skip = True
                continue
            if token.endswith(":"):
                continue
            kept.append(token)
        return " ".join(kept)
    return "none"


def read_qdisc(prefix: list[str], iface: str) -> str | None:
    """The impairment on one end of the pair, or None when it cannot be read.

    None rather than "none": a tc that could not run says nothing about the queue, and a
    campaign that files an unreadable path as a clean one is the falsy default this
    project refuses everywhere else.
    """
    probe = subprocess.run([*prefix, "tc", "qdisc", "show", "dev", iface],
                           capture_output=True, text=True)
    if probe.returncode != 0 or not probe.stdout.strip():
        return None
    return observed_profile(probe.stdout)


def _run(args: list[str], check: bool = True) -> subprocess.CompletedProcess:
    """Runs a privileged command, non-interactively.

    -n throughout: this must never sit waiting for a password. A rig that blocks on a
    prompt at three in the morning has failed in the least useful way available.
    """
    return subprocess.run(["sudo", "-n", *args], capture_output=True, text=True, check=check)


def namespace_exists(name: str) -> bool:
    result = subprocess.run(["ip", "netns", "list"], capture_output=True, text=True)
    return any(line.split()[0] == name for line in result.stdout.splitlines() if line.strip())


def addresses(subnet: str) -> tuple[str, str, int]:
    """The server address, the generator address and the prefix length."""
    net = ipaddress.ip_network(subnet, strict=True)
    hosts = list(net.hosts())
    if len(hosts) < 2:
        raise ValueError(f"{subnet} has fewer than two usable addresses")
    return str(hosts[0]), str(hosts[1]), net.prefixlen


def expected_tokens(netem: list[str] | None, limit: int) -> set[str]:
    """What observed_profile reports for a freshly applied profile.

    A set rather than a string, because iproute2's dump order is its own and refusing a
    pair over the order it printed its arguments in would be a gate that cannot be
    satisfied, which is a gate that gets removed. `distribution <table>` is dropped
    because tc does not echo it back.
    """
    if netem is None:
        return {"none"}
    kept: list[str] = []
    skip = False
    for token in netem:
        if skip:
            skip = False
            continue
        if token == "distribution":
            skip = True
            continue
        kept.append(token)
    return {"netem", "root", "limit", str(limit), *kept}


def configured(ns: str, iface: str, ip_addr: str,
               netem: list[str] | None, limit: int) -> bool:
    """Whether one end of the pair is the end this profile asks for.

    Address, link state and qdisc, because the existence of a namespace is not evidence
    that anything inside it was set up. Anything unreadable is not configured: a pair
    nobody could inspect is not a pair anybody should measure over.
    """
    addr = _run(["ip", "-n", ns, "-br", "addr", "show", iface], check=False)
    if addr.returncode != 0 or ip_addr not in addr.stdout or "UP" not in addr.stdout.split():
        return False
    qdisc = _run(["ip", "netns", "exec", ns, "tc", "qdisc", "show", "dev", iface],
                 check=False)
    if qdisc.returncode != 0:
        return False
    return set(observed_profile(qdisc.stdout).split()) == expected_tokens(netem, limit)


def up(subnet: str, profile: str, limit: int) -> int:
    server_ip, generator_ip, prefix = addresses(subnet)

    if profile not in PROFILES:
        print(f"unknown profile {profile!r}; one of {', '.join(sorted(PROFILES))}",
              file=sys.stderr)
        return 2
    netem = PROFILES[profile]
    ends = ((SERVER_NS, SERVER_IF, server_ip), (GENERATOR_NS, GENERATOR_IF, generator_ip))

    if namespace_exists(SERVER_NS) and namespace_exists(GENERATOR_NS):
        # Idempotent on purpose. A campaign script that calls this before every run must
        # not fail because the pair it needs is already there.
        #
        # Idempotent about the pair, though, not about the two names. This used to return
        # 0 the moment both namespaces existed, before the profile was so much as looked
        # at, so `up --profile wan50` against a pair built as none printed success and
        # left no qdisc, and a pair whose veth step failed -- leaving both namespaces
        # behind -- reported itself up forever after.
        if all(configured(ns, iface, ip_addr, netem, limit) for ns, iface, ip_addr in ends):
            print(f"pair already up: {SERVER_NS}={server_ip} {GENERATOR_NS}={generator_ip} "
                  f"profile={profile}")
            return 0
        print(f"{SERVER_NS} and {GENERATOR_NS} exist but are not the pair being asked "
              f"for (profile={profile}, limit={limit}): an address, a link state or a "
              f"qdisc does not match, or could not be read. Run 'down' first.",
              file=sys.stderr)
        return 2
    if namespace_exists(SERVER_NS) or namespace_exists(GENERATOR_NS):
        # Half a pair is not a pair, and guessing which half to repair is how a rig ends
        # up in a state nobody can describe.
        print(f"only one of {SERVER_NS}/{GENERATOR_NS} exists; run 'down' first",
              file=sys.stderr)
        return 2

    try:
        return _build(server_ip, generator_ip, prefix, profile, netem, limit)
    except subprocess.CalledProcessError:
        # Undone rather than left. Every command here is check=True and the two
        # namespaces are created first, so a failure anywhere after them used to leave a
        # pair that the test above then called up: veth-less, so every run failed to
        # connect, or -- worse, because it is silent -- impaired on one direction and not
        # the other, which no record would have carried either.
        for name in (SERVER_NS, GENERATOR_NS):
            _run(["ip", "netns", "del", name], check=False)
        # The end created but never moved into a namespace, which `down` cannot reach
        # because it deletes namespaces. Left behind it makes `ip link add` fail with
        # "File exists" on every later attempt, after the two adds have already
        # succeeded, which is precisely the state that made the pair report itself up.
        _run(["ip", "link", "del", SERVER_IF], check=False)
        raise


def _build(server_ip: str, generator_ip: str, prefix: int, profile: str,
           netem: list[str] | None, limit: int) -> int:
    _run(["ip", "netns", "add", SERVER_NS])
    _run(["ip", "netns", "add", GENERATOR_NS])

    # Created in the default namespace, then each end moved into place.
    _run(["ip", "link", "add", SERVER_IF, "type", "veth", "peer", "name", GENERATOR_IF])
    _run(["ip", "link", "set", SERVER_IF, "netns", SERVER_NS])
    _run(["ip", "link", "set", GENERATOR_IF, "netns", GENERATOR_NS])

    _run(["ip", "-n", SERVER_NS, "addr", "add", f"{server_ip}/{prefix}", "dev", SERVER_IF])
    _run(["ip", "-n", GENERATOR_NS, "addr", "add", f"{generator_ip}/{prefix}", "dev", GENERATOR_IF])
    _run(["ip", "-n", SERVER_NS, "link", "set", SERVER_IF, "up"])
    _run(["ip", "-n", GENERATOR_NS, "link", "set", GENERATOR_IF, "up"])

    # A namespace starts with lo down. Anything that talks to itself inside one, and
    # that includes a server binding 0.0.0.0 and then being probed locally, needs it up.
    _run(["ip", "-n", SERVER_NS, "link", "set", "lo", "up"])
    _run(["ip", "-n", GENERATOR_NS, "link", "set", "lo", "up"])

    if netem is not None:
        # Per direction, because netem shapes egress only. One qdisc on each end's
        # transmit side makes the impairment symmetric and, more to the point, makes it
        # explicit which direction each half applies to.
        for ns, iface in ((SERVER_NS, SERVER_IF), (GENERATOR_NS, GENERATOR_IF)):
            _run(["ip", "netns", "exec", ns, "tc", "qdisc", "add", "dev", iface,
                  "root", "netem", *netem, "limit", str(limit)])

    print(f"pair up: {SERVER_NS}={server_ip} {GENERATOR_NS}={generator_ip} "
          f"profile={profile}" + (f" limit={limit}" if netem is not None else " (no qdisc)"))
    print(f"server address:    {server_ip}")
    print(f"generator address: {generator_ip}")
    return 0


def down() -> int:
    present = [n for n in (SERVER_NS, GENERATOR_NS) if namespace_exists(n)]
    if not present:
        print("pair already down")
        return 0
    for name in present:
        # Deleting the namespace takes its veth end with it, and deleting one end of a
        # pair deletes the other, so there is nothing else to clean up.
        _run(["ip", "netns", "del", name])
    print(f"removed: {', '.join(present)}")
    return 0


def status(subnet: str) -> int:
    server_ip, generator_ip, _ = addresses(subnet)
    up_now = namespace_exists(SERVER_NS) and namespace_exists(GENERATOR_NS)
    print(f"{SERVER_NS}: {'up' if namespace_exists(SERVER_NS) else 'absent'}")
    print(f"{GENERATOR_NS}: {'up' if namespace_exists(GENERATOR_NS) else 'absent'}")
    if not up_now:
        return 1
    for ns, iface in ((SERVER_NS, SERVER_IF), (GENERATOR_NS, GENERATOR_IF)):
        addr = _run(["ip", "-n", ns, "-br", "addr", "show", iface], check=False)
        qdisc = _run(["ip", "netns", "exec", ns, "tc", "qdisc", "show", "dev", iface], check=False)
        print(f"  {ns}: {addr.stdout.strip() or '(no address)'}")
        print(f"        qdisc: {qdisc.stdout.strip() or '(none)'}")
    print(f"server address:    {server_ip}")
    print(f"generator address: {generator_ip}")
    # Non-zero for a pair whose ends are not addressed and up, so a script that gates a
    # night on this exit status does not start over a hollow pair. The impairment is not
    # checked here because status is not told which profile was asked for; up is.
    for ns, iface, ip_addr in ((SERVER_NS, SERVER_IF, server_ip),
                               (GENERATOR_NS, GENERATOR_IF, generator_ip)):
        addr = _run(["ip", "-n", ns, "-br", "addr", "show", iface], check=False)
        if addr.returncode != 0 or ip_addr not in addr.stdout or "UP" not in addr.stdout.split():
            print(f"{ns} exists but {iface} is not up at {ip_addr}", file=sys.stderr)
            return 1
    return 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("action", choices=("up", "down", "status"))
    ap.add_argument("--subnet", default=DEFAULT_SUBNET,
                    help=f"point-to-point subnet, default {DEFAULT_SUBNET}")
    ap.add_argument("--profile", default="none", choices=sorted(PROFILES),
                    help="netem profile applied to each direction; 'none' adds no qdisc")
    ap.add_argument("--limit", type=int, default=DEFAULT_LIMIT,
                    help=f"netem queue limit in packets, default {DEFAULT_LIMIT}; the "
                         f"netem default of 1000 overflows silently at campaign rates")
    args = ap.parse_args(argv)

    if sys.platform != "linux":
        print("network namespaces are Linux only", file=sys.stderr)
        return 2
    for tool in ("ip", "tc"):
        if shutil.which(tool) is None:
            print(f"{tool} not found; install iproute2", file=sys.stderr)
            return 2
    if subprocess.run(["sudo", "-n", "true"], capture_output=True).returncode != 0:
        # Said once, here, rather than as a failure inside whichever ip command ran
        # first. Creating a namespace needs CAP_NET_ADMIN and this host does not allow
        # unprivileged user namespaces.
        print("sudo -n does not work; namespace setup needs passwordless sudo",
              file=sys.stderr)
        return 2

    try:
        if args.action == "up":
            return up(args.subnet, args.profile, args.limit)
        if args.action == "down":
            return down()
        return status(args.subnet)
    except subprocess.CalledProcessError as exc:
        print(f"{' '.join(exc.cmd)} failed: {(exc.stderr or exc.stdout or '').strip()}",
              file=sys.stderr)
        return 1
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
