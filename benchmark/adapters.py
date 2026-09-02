"""The two concrete things the driver needs: a server to measure and a client to drive it.

The driver deliberately knows nothing about either, so everything platform specific and
everything tool specific lives here. That separation is why the rules in validity.py can
be self-checked on a machine with neither installed.

Server-side cost is read from the operating system rather than sampled with a timer.
Sampling a process's CPU after it has stopped reports whatever the last sample happened
to be, which is how the previous harness came to plot Linux lifetime-average percent and
Windows cumulative CPU seconds into the same column.
"""

from __future__ import annotations

import ctypes
import json
import os
import platform as _platform
import re
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from benchmark.harness import environment
from benchmark.harness.driver import GeneratorResult, ResourceUsage, RunFailed
from benchmark.harness.ordering import Cell


# --------------------------------------------------------------------- Windows

if os.name == "nt":

    class _FILETIME(ctypes.Structure):
        _fields_ = [("low", ctypes.c_uint32), ("high", ctypes.c_uint32)]

        @property
        def seconds(self) -> float:
            # 100 nanosecond units since the epoch the API uses. Only differences are
            # meaningful, which is all this is used for.
            return ((self.high << 32) | self.low) / 1e7

    class _PROCESS_MEMORY_COUNTERS(ctypes.Structure):
        # The _EX layout: same as the base one plus PrivateUsage on the end. Asking for
        # the larger struct is what makes committed private bytes available, and above
        # a few thousand routes that is the only memory number that still means
        # anything: a working set is capped by how much physical memory there is, so a
        # structure larger than free RAM reports a working set the size of free RAM.
        _fields_ = [
            ("cb", ctypes.c_uint32),
            ("PageFaultCount", ctypes.c_uint32),
            ("PeakWorkingSetSize", ctypes.c_size_t),
            ("WorkingSetSize", ctypes.c_size_t),
            ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
            ("QuotaPagedPoolUsage", ctypes.c_size_t),
            ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
            ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
            ("PagefileUsage", ctypes.c_size_t),
            ("PeakPagefileUsage", ctypes.c_size_t),
            ("PrivateUsage", ctypes.c_size_t),
        ]

    def _process_cost(pid: int) -> tuple[float | None, int | None]:
        """CPU seconds and peak working set, read from the kernel before the process exits."""
        PROCESS_QUERY_INFORMATION = 0x0400
        PROCESS_VM_READ = 0x0010
        handle = ctypes.windll.kernel32.OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, False, pid
        )
        if not handle:
            return None, None
        try:
            creation, exit_t, kernel, user = _FILETIME(), _FILETIME(), _FILETIME(), _FILETIME()
            cpu = None
            if ctypes.windll.kernel32.GetProcessTimes(
                handle, ctypes.byref(creation), ctypes.byref(exit_t),
                ctypes.byref(kernel), ctypes.byref(user)
            ):
                cpu = kernel.seconds + user.seconds

            counters = _PROCESS_MEMORY_COUNTERS()
            counters.cb = ctypes.sizeof(_PROCESS_MEMORY_COUNTERS)
            peak = None
            if ctypes.windll.psapi.GetProcessMemoryInfo(
                handle, ctypes.byref(counters), counters.cb
            ):
                # The larger of the two. Peak working set is what was resident and is
                # capped by physical memory; peak pagefile usage is what the process
                # committed and is not. A route table bigger than free RAM would
                # otherwise be reported as the size of free RAM.
                peak = max(int(counters.PeakWorkingSetSize), int(counters.PeakPagefileUsage))
            return cpu, peak
        finally:
            ctypes.windll.kernel32.CloseHandle(handle)

elif _platform.system() == "Darwin":

    class _RUsageInfoV4(ctypes.Structure):
        """The prefix of rusage_info_v4 up to the two fields this needs.

        Declared in full up to those offsets rather than guessed at, because libproc
        fills the struct by size and a short declaration would be written past. Fields
        after ri_lifetime_max_phys_footprint are omitted deliberately: the buffer passed
        below is sized from the real struct, not from this view of it.
        """

        _fields_ = [
            ("ri_uuid", ctypes.c_uint8 * 16),
            ("ri_user_time", ctypes.c_uint64),
            ("ri_system_time", ctypes.c_uint64),
            ("ri_pkg_idle_wkups", ctypes.c_uint64),
            ("ri_interrupt_wkups", ctypes.c_uint64),
            ("ri_pageins", ctypes.c_uint64),
            ("ri_wired_size", ctypes.c_uint64),
            ("ri_resident_size", ctypes.c_uint64),
            ("ri_phys_footprint", ctypes.c_uint64),
            ("ri_proc_start_abstime", ctypes.c_uint64),
            ("ri_proc_exit_abstime", ctypes.c_uint64),
            ("ri_child_user_time", ctypes.c_uint64),
            ("ri_child_system_time", ctypes.c_uint64),
            ("ri_child_pkg_idle_wkups", ctypes.c_uint64),
            ("ri_child_interrupt_wkups", ctypes.c_uint64),
            ("ri_child_pageins", ctypes.c_uint64),
            ("ri_child_elapsed_abstime", ctypes.c_uint64),
            ("ri_diskio_bytesread", ctypes.c_uint64),
            ("ri_diskio_byteswritten", ctypes.c_uint64),
            ("ri_cpu_time_qos_default", ctypes.c_uint64),
            ("ri_cpu_time_qos_maintenance", ctypes.c_uint64),
            ("ri_cpu_time_qos_background", ctypes.c_uint64),
            ("ri_cpu_time_qos_utility", ctypes.c_uint64),
            ("ri_cpu_time_qos_legacy", ctypes.c_uint64),
            ("ri_cpu_time_qos_user_initiated", ctypes.c_uint64),
            ("ri_cpu_time_qos_user_interactive", ctypes.c_uint64),
            ("ri_billed_system_time", ctypes.c_uint64),
            ("ri_serviced_system_time", ctypes.c_uint64),
            ("ri_logical_writes", ctypes.c_uint64),
            ("ri_lifetime_max_phys_footprint", ctypes.c_uint64),
        ]

    _RUSAGE_INFO_V4 = 4
    _RUSAGE_BUFFER_BYTES = 1024  # comfortably larger than any published rusage_info_v*

    def _mach_timebase() -> tuple[int, int]:
        """What one unit of ri_user_time is worth, as the kernel reports it.

        proc_pid_rusage returns those times in mach absolute time units rather than in
        nanoseconds. On Intel the two coincide, because the timebase there is 1/1, which
        is how the assumption survived: on Apple Silicon it is 125/3, so reading the raw
        value as nanoseconds under-reports every server CPU figure by a factor of nearly
        forty-two. Asked of the machine rather than written down, since it is a property
        of the part and not of the architecture.
        """

        class _Timebase(ctypes.Structure):
            _fields_ = [("numer", ctypes.c_uint32), ("denom", ctypes.c_uint32)]

        try:
            tb = _Timebase()
            if ctypes.CDLL("libSystem.dylib").mach_timebase_info(ctypes.byref(tb)) != 0:
                return 1, 1
            return (tb.numer, tb.denom) if tb.numer and tb.denom else (1, 1)
        except (OSError, AttributeError):
            return 1, 1

    _TIMEBASE_NUMER, _TIMEBASE_DENOM = _mach_timebase()

    def _process_cost(pid: int) -> tuple[float | None, int | None]:
        """CPU seconds and peak footprint from libproc, read while the process is alive.

        proc_pid_rusage rather than getrusage(RUSAGE_CHILDREN): that only accounts for
        children already reaped, so a running server contributes nothing to it, and after
        the wait it is a cumulative total over every subprocess the campaign has ever
        started. Neither is the quantity the record claims.

        Times are mach absolute time units, converted through the timebase above; the
        footprint is already bytes.
        """
        buffer = (ctypes.c_uint8 * _RUSAGE_BUFFER_BYTES)()
        try:
            libc = ctypes.CDLL("libSystem.dylib", use_errno=True)
            rc = libc.proc_pid_rusage(ctypes.c_int(pid), ctypes.c_int(_RUSAGE_INFO_V4),
                                      ctypes.byref(buffer))
        except (OSError, AttributeError):
            return None, None
        if rc != 0:
            return None, None
        # _RUsageInfoV4 is the struct; _RUSAGE_INFO_V4 is the integer flavour asked of
        # libproc. Passing the latter here made ctypes.POINTER raise inside stop(), which
        # the driver recorded as a failure and every run on this platform was rejected.
        info = ctypes.cast(buffer, ctypes.POINTER(_RUsageInfoV4)).contents
        ticks = info.ri_user_time + info.ri_system_time
        cpu = ticks * _TIMEBASE_NUMER / _TIMEBASE_DENOM / 1e9
        return cpu, int(info.ri_lifetime_max_phys_footprint)

else:

    def _process_cost(pid: int) -> tuple[float | None, int | None]:
        """CPU seconds and peak RSS from /proc, which is the same kernel accounting.

        cgroup v2 is what the methodology asks for and is what a Linux campaign should
        use. This is the fallback for a process not placed in its own scope.
        """
        try:
            with open(f"/proc/{pid}/stat", encoding="ascii") as fh:
                fields = fh.read().rsplit(")", 1)[1].split()
            ticks = os.sysconf("SC_CLK_TCK")
            cpu = (int(fields[11]) + int(fields[12])) / ticks
        except (OSError, IndexError, ValueError):
            cpu = None
        peak = None
        try:
            with open(f"/proc/{pid}/status", encoding="ascii") as fh:
                for line in fh:
                    if line.startswith("VmHWM:"):
                        peak = int(line.split()[1]) * 1024
                        break
        except OSError:
            pass
        return cpu, peak


# ---------------------------------------------------------------------- server


# Matched rather than assumed, because IoBackend::Default resolves against the host: a
# cell that asked for io_uring on a machine that refuses it must not be recorded as an
# io_uring run. Shared with the descriptor census; see environment.BANNER_BACKEND.
_BANNER_BACKEND = environment.BANNER_BACKEND

# Enough to hold the banner and a startup failure, not enough to grow without bound.
_KEPT_OUTPUT_LINES = 200


def describe_signal(number: int) -> str:
    """The signal's name, or its number where the platform has no name for it."""
    try:
        return signal.strsignal(number) or f"signal {number}"
    except ValueError:
        return f"signal {number}"


def ss_listening_pids(text: str, port: int) -> list[int]:
    """The pids `ss -ltnpH` reports listening on one port.

    Every one of them, not the first. Both Linux backends set SO_REUSEPORT, so several
    processes can hold the same listener and a caller stopping only one leaves the rest
    holding the port.
    """
    pids: list[int] = []
    for line in text.splitlines():
        fields = line.split()
        # Same field as refuse_held_port reads: the local address, port last.
        if len(fields) >= 4 and fields[3].rsplit(":", 1)[-1] == str(port):
            pids.extend(int(found) for found in re.findall(r"pid=(\d+)", line))
    return pids


def refuse_held_port(port: int, launch_prefix: list[str] | None = None) -> None:
    """Refuses to start a server on a port something already listens on.

    With a launch prefix the port that matters is not this host's. A server behind
    `ip netns exec srv` binds inside that namespace, where a local bind probe can say
    nothing at all: it would succeed against a stale server in the namespace and the
    gate would pass every time it was most needed. The check is run through the same
    prefix in that case, so it looks where the server is about to look.

    On Windows and macOS a second server on a held port fails to bind and the run fails
    loudly. On Linux both backends set SO_REUSEPORT, so a leftover benchmark_server and
    the new one bind side by side and the kernel splits the generator's connections
    between two servers with different factors, under one record, and the readiness
    probe is satisfied by whichever answers.

    The probe is a throwaway bind without SO_REUSEPORT, which Linux refuses against a
    reuseport group exactly as it refuses it against any other listener. SO_REUSEADDR is
    set on POSIX only, so the previous run's connections in TIME_WAIT are not read as a
    holder; on Windows that option would instead permit binding over a live listener,
    and a plain bind there is what the IOCP server itself does. That server is a
    dual-stack listener on [::], which holds the IPv4 wildcard port as well, so an IPv4
    probe conflicts with it on every platform.
    """
    if launch_prefix:
        # ss inside the namespace rather than a bind here. Reading a listener is enough
        # to refuse, and it needs no socket in a namespace this process is not in.
        probe = subprocess.run([*launch_prefix, "ss", "-ltnH"],
                               capture_output=True, text=True)
        if probe.returncode != 0:
            raise RunFailed(
                f"could not check port {port} through {' '.join(launch_prefix)}: "
                f"{(probe.stderr or probe.stdout).strip()[:200]}"
            )
        for line in probe.stdout.splitlines():
            fields = line.split()
            if len(fields) >= 4 and fields[3].rsplit(":", 1)[-1] == str(port):
                raise RunFailed(
                    f"port {port} is already held inside the launch namespace "
                    f"({line.strip()}); a stale server is running"
                )
        return

    try:
        with socket.socket() as probe_sock:
            if os.name != "nt":
                probe_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            probe_sock.bind(("0.0.0.0", port))
    except OSError as exc:
        raise RunFailed(
            f"port {port} is already held ({exc.strerror or exc}); a stale server is running"
        ) from exc


# The tracepoints the syscall counter asks for.
#
# raw_syscalls:sys_enter first and always: it is the total, and it is the one the
# non-zero assertion is made against. The rest are the named breakdown.
#
# clock_gettime and epoll_ctl are in the list because a probe without them left 61% of
# an epoll server's syscalls unattributed, and strace -c showed those two accounting for
# most of it at about two of each per request. A named set that misses the majority of
# the total is a list that looks like a breakdown and is not one.
SYSCALL_TRACEPOINTS = (
    "raw_syscalls:sys_enter",
    "syscalls:sys_enter_epoll_wait",
    "syscalls:sys_enter_epoll_pwait",
    "syscalls:sys_enter_epoll_ctl",
    "syscalls:sys_enter_io_uring_enter",
    "syscalls:sys_enter_accept4",
    "syscalls:sys_enter_recvfrom",
    "syscalls:sys_enter_read",
    "syscalls:sys_enter_sendto",
    "syscalls:sys_enter_write",
    "syscalls:sys_enter_close",
    "syscalls:sys_enter_clock_gettime",
)

TOTAL_TRACEPOINT = "raw_syscalls:sys_enter"


def _is_sudo_wrapper(path: str | None) -> bool:
    """Whether the tool on PATH is a shell script that re-invokes itself under sudo.

    A real arrangement on this rig, not a hypothetical one. /usr/local/bin/perf is two
    lines long and reads `exec /usr/bin/sudo -n "/usr/bin/perf" "$@"`, and the same
    pattern covers ip, sysctl, bpftool, bpftrace, cpupower and pacman. It makes an
    unprivileged-looking command run as root, which is convenient and is exactly the
    kind of thing a record must not describe as unprivileged: the counts are then of a
    root process, and whether a rig needs root is a question about the rig.
    """
    if not path:
        return False
    try:
        head = Path(path).read_bytes()[:512]
    except OSError:
        return False
    return head.startswith(b"#!") and b"sudo" in head


def perf_binary() -> tuple[str, bool]:
    """The perf to exec directly, and whether this harness has to add sudo to it.

    Never the PATH name when that is a sudo wrapper. The counter has to know perf's own
    pid so it can interrupt it, and it learns that by having a shell write $$ and then
    exec perf. Exec-ing a wrapper puts sudo at that pid instead: the SIGINT goes to
    sudo, perf is never told to stop, it writes no counts, and the run comes back with a
    zero total. That is what happened the first time this ran on this rig.

    Resolving to the wrapped binary makes the pid perf's again and puts the privilege
    decision back where it can be recorded.
    """
    path = shutil.which("perf")
    if path and _is_sudo_wrapper(path):
        # Two lines of shell whose last word before "$@" is the real binary.
        for token in Path(path).read_text().split():
            candidate = token.strip('"\'')
            if candidate.endswith("/perf") and candidate != path:
                return candidate, True
        return "/usr/bin/perf", True
    return path or "perf", False


def perf_privilege() -> str | None:
    """How perf can read tracepoints here, or None when it cannot at all.

    Asked rather than assumed, and recorded with the counts, because the answer decides
    what the number describes and it is not stable even on one machine. What can grant
    it, and they are not interchangeable:

      root                    invoked under sudo by this harness
      root via PATH wrapper   the perf on PATH is a script that adds sudo itself
      file capabilities       cap_perfmon or cap_sys_admin on the perf binary, which
                              some distributions set; note that these are NOT sufficient
                              on their own when /sys/kernel/tracing is mode 0700, since
                              perf reads the tracepoint id out of tracefs before it ever
                              calls perf_event_open
      relaxed tracefs         /sys/kernel/tracing/events readable

    The wrapper case is checked first and reported plainly because it is invisible
    otherwise: `perf stat` succeeds, `id -u` inside it prints 0, and a probe that only
    looks at the exit status concludes the host allows unprivileged counting when it
    does not. This host reported exactly that for a while, and /usr/bin/perf invoked
    directly still fails with "can't access trace events".
    """
    perf_path = shutil.which("perf")
    probe = ["perf", "stat", "-e", TOTAL_TRACEPOINT, "-x,", "true"]

    if subprocess.run(probe, capture_output=True).returncode == 0:
        if _is_sudo_wrapper(perf_path):
            return f"root via PATH wrapper ({perf_path})"
        caps = ""
        if perf_path:
            got = subprocess.run(["getcap", perf_path], capture_output=True, text=True)
            if got.returncode == 0 and "=" in got.stdout:
                caps = got.stdout.rsplit("=", 1)[0].split()[-1]
        if os.access("/sys/kernel/tracing/events", os.R_OK):
            return "unprivileged, tracefs readable"
        if caps:
            return f"unprivileged, perf has file capabilities ({caps})"
        return "unprivileged"

    if subprocess.run(["sudo", "-n", *probe], capture_output=True).returncode == 0:
        return "root"
    return None


def parse_perf_counts(text: str) -> dict[str, int]:
    """perf stat -x, output to a mapping. Unparsed and <not counted> lines are dropped.

    Dropping them is safe only because the caller asserts the total is present and
    non-zero afterwards; on its own this would turn a failed attach into an empty dict
    that reads like a quiet server.
    """
    counts: dict[str, int] = {}
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        fields = line.split(",")
        if len(fields) >= 3 and fields[0].isdigit():
            counts[fields[2]] = int(fields[0])
    return counts


@dataclass
class CorouteServer:
    """One benchmark_server process, started fresh and stopped for certain.

    Every factor the cell carries becomes a command line flag rather than a build
    option, so both arms of a comparison are the same binary. Build to build variation
    from code layout and link order is documented at 5 to 10 percent, well above the
    run to run noise this is trying to resolve.
    """

    binary: Path
    cell: Cell
    port: int
    affinity_mask: str | None = None
    # What the server is launched through, if anything: the Linux counterpart of the
    # generator's --generator-command. For the namespace rig this is
    # ["sudo", "-n", "ip", "netns", "exec", "srv"], which puts the server on the far end
    # of the veth instead of on loopback.
    #
    # Deliberately NOT dropped back to the invoking user the way the generator's prefix
    # is. io_uring needs CAP_SYS_ADMIN on a host with kernel.io_uring_disabled=1, so an
    # io_uring cell has to stay root; and if only that arm were root, the epoll arm
    # would differ from it in scheduling class as well as in backend, which is the one
    # confound this comparison cannot carry. Both arms run the same way.
    launch_prefix: list[str] = field(default_factory=list)
    # Opt in only. See RunRecord.syscall_counts for why this is never on during a
    # timed comparison.
    count_syscalls: bool = False
    syscall_counter: str | None = None
    _perf: subprocess.Popen | None = None
    _perf_out: Path | None = None
    _perf_err: Path | None = None
    _perf_pidfile: Path | None = None
    _perf_privilege: str | None = None
    _perf_workdir: Path | None = None
    # Where the rig's self-signed certificate lives. Only consulted when the cell asks
    # for TLS, so a cleartext campaign runs on a machine that has none.
    cert_file: Path | None = None
    key_file: Path | None = None
    _proc: subprocess.Popen | None = None
    _cost: tuple[float | None, int | None] = (None, None)
    # The backend the server said it actually started on, read back from its banner.
    # None until the banner has been seen. See _drain.
    effective_backend: str | None = None
    _output: list[str] = field(default_factory=list)

    @property
    def argv(self) -> list[str]:
        factors = self.cell.as_dict()
        args = [
            *self.launch_prefix,
            str(self.binary),
            "--port", str(self.port),
            "--workers", str(factors.get("workers", 4)),
            "--backlog", str(factors.get("backlog", 1024)),
            "--payload", str(factors.get("payload_bytes", 0)),
            # Zero is unlimited. A limit forces a reconnect and turns a keep-alive
            # measurement into a measurement of accept, which for most cells is the
            # wrong measurement and for the churn cells is the whole point.
            "--max-requests", str(factors.get("max_requests_per_connection", 0)),
        ]
        # The arm of the I/O-portability comparison, passed rather than built in. Only
        # when the cell carries it, so a campaign that predates the factor produces the
        # command line it always did and stays comparable with the runs already on disk.
        #
        # Only the two names --io-backend accepts. The cell carries io_backend on every
        # platform, so on macOS this used to pass "--io-backend kqueue" and on Windows
        # "--io-backend iocp", which parse_io_backend refuses with exit 2: every cell of
        # every campaign died at server start on the two platforms that have no choice
        # to make. Where the platform has one backend there is nothing to select, and
        # the banner cross-check in the driver still confirms which one ran.
        if factors.get("io_backend") in ("io_uring", "epoll"):
            args += ["--io-backend", str(factors["io_backend"])]
        if not factors.get("protocol_detection", True):
            args.append("--no-detect")
        if factors.get("tls"):
            if self.cert_file is None or self.key_file is None:
                # Refused rather than falling back to cleartext. A server started
                # without its certificate would answer every request and the record
                # would say tls=True, which is a full set of plausible numbers for an
                # experiment that never happened.
                raise RunFailed(
                    "cell asks for tls but no certificate was configured; "
                    "run benchmark/make_cert.py and pass --cert and --key"
                )
            args += ["--tls", str(self.cert_file), str(self.key_file)]
        # Only when the cell carries them, so a campaign that is not about routing
        # produces the same command line it always did and stays comparable with the
        # runs already on disk.
        if factors.get("router_arm"):
            args += [
                "--router", str(factors["router_arm"]),
                "--routes", str(factors.get("route_count", 0)),
                "--route-shape", str(factors.get("route_shape", "rest")),
                "--route-params", "1" if factors.get("route_params", True) else "0",
                "--route-depth", str(factors.get("route_depth", 5)),
            ]
        if self.affinity_mask:
            args += ["--affinity", self.affinity_mask]
        return args

    def start(self) -> None:
        refuse_held_port(self.port, self.launch_prefix)
        self._proc = subprocess.Popen(
            self.argv,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        self._output = []
        self.effective_backend = None
        # A thread rather than a read in wait_until_ready, for two reasons. It keeps the
        # pipe drained, so a server that talks more than expected cannot block on a full
        # buffer half way through a measurement, which discarding stdout used to prevent.
        # And it never blocks the caller: a readline against a silent process would hang
        # the campaign, and select() on a pipe is not portable to the Windows hosts this
        # harness also runs on.
        reader = threading.Thread(target=self._drain, daemon=True)
        reader.start()

    def _drain(self) -> None:
        proc = self._proc
        if proc is None or proc.stdout is None:
            return
        for raw in proc.stdout:
            line = raw.decode(errors="replace") if isinstance(raw, bytes) else raw
            # Capped. The pipe still has to be drained for the whole life of the
            # process or the server blocks on a full buffer, but only the first lines
            # are ever read back: the banner is among them, and a run that logged for
            # an hour would otherwise hold every line of it in memory.
            if len(self._output) < _KEPT_OUTPUT_LINES:
                self._output.append(line)
            match = _BANNER_BACKEND.search(line)
            if match:
                self.effective_backend = match.group(1)

    def wait_until_ready(self, timeout_s: float) -> bool:
        """Connects until it can, so no run measures process startup.

        A child that exited before listening raises rather than returning False, so the
        record says what killed it. Returning False filed an OOM-killed server, a missing
        certificate, a bad_alloc and a refused --io-backend under the driver's one
        reason, that the server did not become ready within the timeout, and the stderr
        that named the cause was piped and never read.
        """
        deadline = time.monotonic() + timeout_s
        connected = False
        while time.monotonic() < deadline:
            if self._proc is not None and self._proc.poll() is not None:
                raise RunFailed(self._startup_failure())
            if not connected:
                if self._connect_once():
                    connected = True
                else:
                    time.sleep(0.05)
                    continue
            # Listening and having said which backend it is listening on are not the
            # same instant: the banner is written just after the socket comes up. The
            # run is only startable once both have happened, because the backend is
            # what the driver checks the cell against.
            if self.effective_backend is not None:
                return True
            time.sleep(0.01)
        if connected:
            # Listening, answering, and silent about its backend. The only other answer
            # available here is False, which the driver files as "server did not become
            # ready", and that sends the operator to the readiness timeout when the
            # cause is a binary from before the banner named a backend: it prints
            # "(multi-accept)" alone, BANNER_BACKEND never matches, and no timeout is
            # long enough. The banner it did print is quoted rather than described,
            # because that is what tells this apart from a genuinely slow start.
            raise RunFailed(
                f"server on port {self.port} is listening but printed no backend banner "
                f"within {timeout_s:.0f}s; a binary older than the banner never will, so "
                f"rebuild the tree passed to --build. It printed: "
                f"{''.join(self._output).strip()[:500] or '(nothing)'}"
            )
        return False

    def _connect_once(self) -> bool:
        """One readiness connection, made from wherever the server is.

        Without a prefix that is this host's loopback, as before. With one it is not,
        and connecting here would be connecting to the wrong namespace: a server behind
        `ip netns exec srv` holds no port this host can reach, so every poll fails and
        every run is filed as a 30 second readiness timeout no matter how healthy the
        server is. That is precisely what the first namespace campaign did.

        The connection is still a connection rather than a listening check, because
        answering is what readiness means here and a bound socket that never accepts
        would otherwise pass. Inside the namespace 127.0.0.1 is that namespace's own
        loopback, which netns.py brings up, and the server binds the wildcard address.
        """
        if not self.launch_prefix:
            try:
                with socket.create_connection(("127.0.0.1", self.port), timeout=0.25):
                    return True
            except OSError:
                return False

        probe = subprocess.run(
            [*self.launch_prefix, sys.executable, "-c",
             "import socket,sys;"
             "s=socket.create_connection(('127.0.0.1',int(sys.argv[1])),timeout=0.25);"
             "s.close()", str(self.port)],
            capture_output=True,
        )
        return probe.returncode == 0

    def server_pid(self) -> int | None:
        """The benchmark_server process, not whatever was used to launch it.

        Without a prefix this is simply Popen's pid. With one it is not: Popen holds
        sudo, which holds ip, which holds the server, and anything that reads the wrong
        one gets a process asleep in wait(). That failure is silent in both places it
        matters. perf reports `<not counted>` rather than an error, and the resource
        cost comes back as a few milliseconds of sudo.

        Resolved by walking children rather than by matching /proc/PID/comm, because
        TASK_COMM_LEN truncates comm to 15 characters and "benchmark_server" is 16, so
        an exact match on the name never fires and the search silently finds nothing.
        The first argv entry is compared instead, which is not truncated.
        """
        if self._proc is None:
            return None
        if not self.launch_prefix:
            return self._proc.pid

        target = str(self.binary)
        seen: set[int] = set()
        frontier = [self._proc.pid]
        while frontier:
            pid = frontier.pop()
            if pid in seen:
                continue
            seen.add(pid)
            try:
                argv0 = Path(f"/proc/{pid}/cmdline").read_bytes().split(b"\0")[0].decode()
            except OSError:
                argv0 = ""
            if argv0 == target:
                return pid
            try:
                children = Path(f"/proc/{pid}/task/{pid}/children").read_text().split()
            except OSError:
                children = []
            frontier.extend(int(c) for c in children)

        # The walk saw nothing, which is not the same as there being nothing. It swallows
        # OSError on both files it reads, and task/PID/children does not exist without
        # CONFIG_PROC_CHILDREN and is unreadable under hidepid -- neither is transient, so
        # a host with either would answer None on every run. ss inside the namespace is
        # the second opinion, and it is the mechanism refuse_held_port already trusts for
        # the same question one stack lower.
        found = self._listening_pids()
        return found[0] if found else None

    def _listening_pids(self) -> list[int]:
        """Every pid holding this server's port inside the launch namespace."""
        if not self.launch_prefix:
            return []
        probe = subprocess.run([*self.launch_prefix, "ss", "-ltnpH"],
                               capture_output=True, text=True)
        if probe.returncode != 0:
            return []
        return ss_listening_pids(probe.stdout, self.port)

    def start_syscall_count(self) -> None:
        """Attaches perf to the server for the generator's run, and only for it.

        Called by the driver immediately before generator.run and stopped immediately
        after, so the window is the generator's lifetime rather than a fixed sleep that
        overhangs it. The feasibility probe used `-- sleep 13` against a 10 second run
        and folded three seconds of idle server into every count.

        That window includes the generator's warmup, which is why the counts are
        normalised by requests_total_whole_run and not by requests_total.
        """
        if not self.count_syscalls or self._proc is None:
            return
        pid = self.server_pid()
        if pid is None:
            raise RunFailed("cannot count syscalls: the server process could not be found")

        privilege = perf_privilege()
        if privilege is None:
            raise RunFailed(
                "cannot count syscalls: perf cannot read tracepoints here, as root or "
                "otherwise. Check the permissions on /sys/kernel/tracing."
            )

        # A private directory, and the files inside it created by perf rather than here.
        #
        # mkstemp would create them as this user, and on a hardened kernel root then
        # cannot write to them: fs.protected_regular is 1, which forbids writing an
        # existing file owned by someone else in a sticky, world-writable directory, and
        # /tmp is exactly that. perf failed with "failed to create output file:
        # Permission denied", wrote nothing, and the run came back with a zero total
        # that looked like a quiet server rather than a blocked write.
        #
        # mkdtemp's directory is 0700 and neither sticky nor world-writable, so the rule
        # does not apply inside it and root can create what it needs.
        workdir = Path(tempfile.mkdtemp(prefix="coroute-perf-"))
        out = workdir / "counts.csv"
        # A file rather than a pipe. perf's stderr is read only when the counts come back
        # unusable, so a perf that wrote more than a pipe buffer would block forever with
        # nobody draining it, and a read of that pipe after perf has been killed blocks
        # until the last writer closes. A file is readable whether or not perf still
        # exists, which is exactly the case the read is for. The parent opens it, so the
        # inherited descriptor is unaffected by what root may create.
        errfile = workdir / "perf.err"
        pidfile = workdir / "perf.pid"
        binary, wrapped = perf_binary()
        perf = [binary, "stat", "-e", ",".join(SYSCALL_TRACEPOINTS),
                "-p", str(pid), "-x,", "-o", str(out)]
        # perf's own pid is written by the shell before it execs, because Popen would
        # otherwise hold whatever wraps perf and a signal sent there never reaches perf.
        # perf writes its counts on SIGINT and discards them on SIGTERM, so stopping the
        # right process is the whole game.
        #
        # Only the bare "root" case adds sudo. A PATH wrapper has already added it, and
        # nesting sudo inside sudo would work but would put another process between the
        # signal and perf.
        launcher = ["sudo", "-n"] if (wrapped or privilege == "root") else []
        argv = [*launcher, "sh", "-c", f'echo $$ > {pidfile}; exec ' + " ".join(perf)]

        with errfile.open("wb") as sink:
            self._perf = subprocess.Popen(argv, stdout=subprocess.DEVNULL, stderr=sink)
        self._perf_privilege = privilege
        self._perf_out = out
        self._perf_err = errfile
        self._perf_pidfile = pidfile
        self._perf_workdir = workdir
        self.syscall_counter = (
            f"perf stat -e {len(SYSCALL_TRACEPOINTS)} tracepoints, -p server, "
            f"privilege={privilege}"
        )
        # perf needs a moment to open the events; starting the load before it has
        # attached would leave the first requests uncounted.
        time.sleep(0.3)

    def stop_syscall_count(self) -> dict[str, int]:
        """Stops perf, parses its counts, and refuses a count that cannot be true.

        The assertion is the point. Both ways this went wrong during the feasibility
        probe produced a confident zero rather than an error: perf attached to the sudo
        wrapper reported `<not counted>`, and a pid resolved by matching a truncated
        /proc/PID/comm was never found at all. A run whose server issued no syscalls
        while serving tens of thousands of requests did not happen, so it is refused
        here rather than averaged into a table later.
        """
        if self._perf is None:
            return {}

        def perf_said() -> str:
            """perf's own words, from a file that can be read after it is gone."""
            try:
                return (self._perf_err.read_text(errors="replace").strip()
                        if self._perf_err else "")
            except OSError:
                return ""

        def signal_perf(sig: str) -> subprocess.CompletedProcess:
            # By the pid the shell recorded, never through the Popen handle: that holds
            # sudo whenever perf runs as root, and a signal sent there never reaches
            # perf. Root either way, whether this harness added the sudo or a PATH
            # wrapper did, so the signal needs privilege either way.
            argv = ["kill", sig, pid_text]
            if self._perf_privilege and self._perf_privilege.startswith("root"):
                argv = ["sudo", "-n", *argv]
            return subprocess.run(argv, capture_output=True)

        try:
            try:
                pid_text = self._perf_pidfile.read_text().strip() if self._perf_pidfile else ""
            except OSError:
                # No pidfile means the shell that writes it never ran, which is what a
                # refused `sudo -n` looks like from here. Unguarded this propagated as a
                # bare FileNotFoundError through the finally, so perf was never signalled
                # and sudo's own explanation was never read.
                pid_text = ""
            if not pid_text:
                raise RunFailed(
                    f"syscall counting never learned perf's own pid, so perf was never "
                    f"told to write its counts and nothing it measured is recoverable. "
                    f"perf said: {perf_said()[:400] or '(nothing)'}"
                )
            # SIGINT, not SIGTERM: perf writes its counts on interrupt and discards
            # them on terminate.
            interrupted = signal_perf("-INT")
            try:
                self._perf.wait(timeout=10)
            except subprocess.TimeoutExpired:
                # Through the same privileged killer, for the same reason. self._perf is
                # sudo on a root count, and SIGKILL can be neither caught nor forwarded,
                # so killing the handle would leave a root perf attached to the server.
                signal_perf("-9")
                try:
                    self._perf.kill()
                except OSError:
                    # An unprivileged parent cannot signal its own setuid child.
                    pass
                try:
                    self._perf.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    pass

            text = self._perf_out.read_text() if self._perf_out else ""
            counts = parse_perf_counts(text)
            total = counts.get(TOTAL_TRACEPOINT, 0)
            # Every requested tracepoint, not only the total. parse_perf_counts drops any
            # row whose count field is not a number -- `<not counted>`, `<not supported>`
            # -- and says in its own docstring that doing so is safe only because the
            # caller asserts afterwards; the caller asserted about one of twelve. perf
            # marks a single evsel unsupported and carries on with the rest, so a run
            # could come back with a plausible total and eleven absent keys, which the
            # schema documents as "the per-tracepoint totals" and which hands any reader
            # doing counts.get(name, 0) the falsy default this project refuses. The named
            # set exists to attribute the total; a set that quietly lost most of itself
            # makes the unattributed remainder look like the thing it was added to
            # prevent. A tracepoint that counted zero is a key with the value 0, so
            # absence means the event never opened.
            missing = [name for name in SYSCALL_TRACEPOINTS if name not in counts]
            if total <= 0 or missing:
                # perf's own words, not a guess at them. Piping its stderr and never
                # reading it is how the server's startup failures used to become a
                # thirty second timeout with no reason attached; the same mistake here
                # would leave "counted zero" with nothing to act on.
                #
                # The interrupt's exit status is quoted too, because a sudoers policy
                # that permits perf and not kill fails silently there and is what makes
                # the timeout branch above reachable at all.
                sent = "" if interrupted.returncode == 0 else (
                    f" The interrupt itself failed ({interrupted.returncode}): "
                    f"{(interrupted.stderr or b'').decode(errors='replace').strip()[:200]}."
                )
                raise RunFailed(
                    f"syscall counting is not usable: {TOTAL_TRACEPOINT}={total} and "
                    f"{len(missing)} of {len(SYSCALL_TRACEPOINTS)} tracepoints never "
                    f"opened ({', '.join(missing) or 'none'}). A zero total cannot be "
                    f"true of a server that served this run, and a breakdown recorded as "
                    f"a whole one attributes what it lost to nothing; the run is refused "
                    f"rather than recorded as quiet.{sent} "
                    f"perf said: {perf_said()[:400] or '(nothing)'}"
                )
            return counts
        finally:
            # Ours to remove: mkdtemp's directory is 0700 and owned by this user, and
            # unlinking needs write on the directory rather than on the files, so what
            # root created inside it comes out without sudo. It used to go through
            # `sudo -n rm -rf`, which raises on a host where counting is legitimately
            # unprivileged and sudo is not installed -- and a raise here replaces the
            # counts the try just returned, or the RunFailed that carries perf's own
            # words, with an errno about sudo. Cleanup must never be the thing that
            # decides a run.
            if self._perf_workdir is not None:
                shutil.rmtree(self._perf_workdir, ignore_errors=True)
            self._perf = None
            self._perf_out = None
            self._perf_err = None
            self._perf_pidfile = None
            self._perf_privilege = None
            self._perf_workdir = None

    def _startup_failure(self) -> str:
        """Why the server is already gone, in the words it used.

        Raised rather than reported as "not ready". A server that refused its
        --io-backend exits in milliseconds with the reason on stderr, and returning
        False turned that into "did not become ready within 30s" with the reason never
        read at all. The caller sits in `except Exception` and records the failure
        string, so this is the only route by which the real message reaches the record.
        """
        proc = self._proc
        code = proc.returncode if proc is not None else None
        # A negative code is a signal on POSIX, and "server exited -6" alone says
        # nothing; "Abort trap: 6" is what tells an OOM kill from an assertion.
        sig = f" ({describe_signal(-code)})" if code is not None and code < 0 else ""
        detail = ""
        if proc is not None and proc.stderr is not None:
            # Popen was not opened with text=True, so this is bytes.
            raw = proc.stderr.read()
            if isinstance(raw, bytes):
                raw = raw.decode(errors="replace")
            detail = (raw or "").strip()
        if not detail:
            # Nothing on stderr does not mean nothing was said: the backend refusal and
            # the banner both go to stdout, which the drain thread is holding.
            detail = "".join(self._output).strip()
        return f"server exited {code}{sig} before listening: {detail[:500] or '(no output)'}"

    def stop(self) -> ResourceUsage:
        if self._proc is None:
            return ResourceUsage()

        pid = self.server_pid()
        if self.launch_prefix and pid is None:
            # Neither the /proc walk nor ss could name the server, and both fallbacks
            # that used to stand in for it are wrong in the same direction. `or
            # self._proc.pid` charged sudo's few milliseconds to server_cpu_seconds and
            # its VmHWM to the memory peak -- a plausible small number, not a null, that
            # no validity rule looks at and results2tex typesets. And terminate() on that
            # handle signals sudo, which does not pass anything on to a process it
            # launched through `ip netns exec`, so the server outlives the run holding
            # the port. The handle is killed so nothing is left waiting here, and the run
            # is refused: it cannot be stopped for certain and its cost is not its own.
            self._proc.kill()
            try:
                self._proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                pass
            self._proc = None
            raise RunFailed(
                f"the server behind {' '.join(self.launch_prefix)} could not be located, "
                f"in /proc or as a listener on port {self.port}. It cannot be stopped for "
                f"certain and its resource cost would be the launcher's, so the run is "
                f"refused rather than recorded with someone else's numbers."
            )

        # Read the cost while the process still exists. After it exits the handle
        # reports nothing and the run would silently carry no server-side numbers.
        cpu, peak = _process_cost(pid)

        # With a prefix, terminate() reaches sudo, which does not pass the signal on to
        # a process it launched through ip netns exec. The server would outlive the run,
        # hold the port, and be found by the next run's held-port gate if it is lucky or
        # split the next run's connections through SO_REUSEPORT if it is not.
        if self.launch_prefix:
            subprocess.run(["sudo", "-n", "kill", str(pid)], capture_output=True)
        else:
            self._proc.terminate()
        try:
            self._proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            if self.launch_prefix:
                subprocess.run(["sudo", "-n", "kill", "-9", str(pid)], capture_output=True)
            self._proc.kill()
            self._proc.wait(timeout=10)
        self._proc = None
        return ResourceUsage(cpu_seconds=cpu, memory_peak_bytes=peak)


# ------------------------------------------------------------------- generator


def _as_bool(value: Any) -> bool:
    """A flag the generator wrote, whether it wrote it as JSON or as text.

    The generator emits affinity_applied unquoted, so it parses as a JSON boolean, and
    this used to compare it against the string "true". That is False for every run in
    which the mask was in fact applied, and the isolation rule in validity.py then
    refuses the run for not having the isolation it does have.

    It had never fired: the affinity fields arrived in schema version 2, after the last
    campaign that used a host-side generator with a mask, and the campaigns since drove
    their load from a virtual machine and asked for no mask at all. It fired on the
    first TLS run and rejected all four.

    Both spellings are accepted rather than one being chosen, because a result file
    written by either version of the generator has to keep meaning what it says.
    """
    return value is True or (isinstance(value, str) and value.strip().lower() == "true")


def to_wsl_path(path: Path) -> str:
    """C:\\Users\\x\\y as WSL sees it: /mnt/c/Users/x/y.

    Needed because the generator and the harness are on opposite sides of the WSL
    boundary but have to agree on one result file. Translated rather than copied: a copy
    would be a second thing that can be stale.
    """
    text = str(Path(path).resolve())
    drive, _, rest = text.partition(":")
    if len(drive) == 1 and rest:
        return "/mnt/" + drive.lower() + rest.replace("\\", "/")
    return text.replace("\\", "/")


@dataclass
class LoadgenGenerator:
    """The load generator from benchmark/generator, invoked once per run.

    It reports its own admissibility through the exit code as well as through the JSON,
    but the verdict is not taken from here: validity.py decides, from the record, once.
    Two places deciding the same thing is how they come to disagree.

    `command` exists so the generator can live somewhere other than this machine's
    filesystem. For the routing experiment it is a Linux build inside WSL, invoked
    through wsl.exe, so that requests leave a network interface instead of going round
    the loopback adapter. Loopback skips the driver and the interrupt path entirely, and
    those are exactly the fixed costs the routing difference has to compete with, so a
    loopback end-to-end number would flatter whichever arm was slower. On Linux the same
    role is played by a network namespace, entered with `ip netns exec`.
    """

    binary: Path
    port: int
    threads: int
    # Where the generator writes its result file. A directory the harness owns rather
    # than the system temp dir: a generator entered through sudo leaves a root-owned
    # file in sticky /tmp, and from the second run on the unprivileged harness cannot
    # unlink it. In a directory the user owns, unlinking needs nothing of the file.
    work_dir: Path
    warmup_s: float = 2.0
    affinity_mask: str | None = None
    samples_dir: Path | None = None
    host: str = "127.0.0.1"
    # How to invoke the generator. None means "run self.binary here".
    command: list[str] | None = None
    # Whether the paths in the command line have to be rewritten for the other side of
    # the WSL boundary.
    translate_paths: bool = False
    # Request paths, one per line, written by the server from the table it registered.
    paths_file: Path | None = None
    # Where the generator runs, as a label: "host", "wsl:<distro>", "netns:<name>". The
    # same string the campaign environment records as generator_location, so the record
    # and the manifest cannot disagree about where the load came from.
    location: str = "host"

    @property
    def name(self) -> str:
        # The kind alone. Every WSL record already on disk says coroute-loadgen-wsl, and
        # the distribution or namespace name is in generator_argv and in the manifest.
        kind = self.location.partition(":")[0]
        return "coroute-loadgen" if kind == "host" else f"coroute-loadgen-{kind}"

    def _path(self, path: Path) -> str:
        return to_wsl_path(path) if self.translate_paths else str(path)

    def _argv(self, cell: Cell, duration_s: float, out: Path, samples: Path | None) -> list[str]:
        factors = cell.as_dict()
        args = list(self.command) if self.command else [str(self.binary)]
        args += [
            "--host", self.host,
            "--port", str(self.port),
            "--connections", str(factors.get("connections", 64)),
            "--threads", str(self.threads),
            "--duration", f"{duration_s:g}",
            "--warmup", f"{self.warmup_s:g}",
            "--out", self._path(out),
        ]
        if factors.get("tls"):
            args.append("--tls")
        # One request per connection, and the client rather than the server decides when
        # the connection ends. Letting the server close at its own limit races the next
        # request against the close, which the stack answers with a reset and the record
        # counts as a socket error. The server is still configured with the same limit,
        # so it agrees rather than being surprised.
        if int(factors.get("max_requests_per_connection", 0)) == 1:
            args.append("--reconnect")
        rate = factors.get("offered_rate")
        if rate:
            args += ["--rate", f"{float(rate):g}"]
        if self.affinity_mask:
            args += ["--affinity", self.affinity_mask]
        if samples is not None:
            args += ["--samples", self._path(samples)]
        if self.paths_file is not None:
            args += ["--paths", self._path(self.paths_file)]
        return args

    def run(self, cell: Cell, duration_s: float) -> GeneratorResult:
        self.work_dir.mkdir(parents=True, exist_ok=True)
        out = self.work_dir / f"loadgen-{self.port}.json"

        # Removed before the run, not just checked for afterwards.
        #
        # The check used to be "did a file appear", which a file left behind by an
        # earlier campaign passes. A generator that failed to start then produced a full
        # set of plausible numbers belonging to a different experiment, and nothing in
        # the record said so: three arms came back with identical percentiles taken from
        # one week-old file. Deleting first makes a generator that did not run look like
        # a generator that did not run.
        try:
            out.unlink()
        except FileNotFoundError:
            pass
        except OSError as exc:
            raise RunFailed(f"could not clear the previous result file {out}: {exc}") from exc

        samples = None
        if self.samples_dir is not None:
            self.samples_dir.mkdir(parents=True, exist_ok=True)
            factors = cell.as_dict()
            stem = "-".join(
                str(factors.get(k, ""))
                for k in ("router_arm", "route_count", "route_shape",
                          "workers", "connections", "offered_rate")
                if factors.get(k, "") != ""
            )
            samples = self.samples_dir / f"{cell.system}-{stem}-{int(time.time()*1000)}.txt"

        argv = self._argv(cell, duration_s, out, samples)
        # A generator that hangs would hold the campaign forever, so it is bounded by
        # the run it was asked for plus a wide margin for startup and teardown.
        proc = subprocess.run(
            argv, capture_output=True, text=True,
            timeout=duration_s + self.warmup_s + 120,
        )

        # Only the missing file is a failure here, not the exit code. The generator
        # exits non-zero when it judges its own run inadmissible, and that verdict
        # belongs to validity.py, once, from the record. What this catches is the
        # generator not running at all, which is a different thing and is not a verdict.
        if not out.exists():
            raise RunFailed(
                f"generator produced no result file (exit {proc.returncode}): "
                f"{(proc.stderr or proc.stdout).strip()[:300]}"
            )

        data: dict[str, Any] = json.loads(out.read_text(encoding="utf-8"))
        # Everything the record keeps has been read; the file would otherwise sit next
        # to runs.jsonl and be committed with it.
        out.unlink()

        lat = data.get("latency_us", {})
        # Reported in milliseconds, because that is what the record and every figure
        # use. Converted once, here, rather than in each consumer.
        latency_ms = {k: v / 1000.0 for k, v in lat.items() if k != "samples"}

        # Same conversion, same reason. Absent from a generator built before the TLS
        # arm existed, which is why it is a get with a default rather than an index: a
        # campaign re-run against an older binary should fail its validity rules, not
        # its JSON parsing.
        conn = data.get("connect_us", {})
        connect_ms = {k: v / 1000.0 for k, v in conn.items() if k != "samples"}

        offered = float(data.get("offered_rate", 0.0))
        achieved = float(data.get("rps", 0.0))

        whole_run = data.get("responses_total")
        result = GeneratorResult(
            requests_total=int(data.get("completed", 0)) + int(data.get("non_2xx", 0)),
            # None rather than zero from a generator built before the counter existed:
            # a zero would read as measured.
            requests_total_whole_run=int(whole_run) if whole_run is not None else None,
            requests_non_2xx=int(data.get("non_2xx", 0)),
            socket_errors=int(data.get("socket_errors", 0)),
            requests_per_second=float(data.get("rps", 0.0)),
            bytes_per_second=float(data.get("bytes_read", 0)) / max(float(data.get("duration_s", 1)), 1e-9),
            latency_ms=latency_ms,
            connect_ms=connect_ms,
            connections_established=int(data.get("connections_established", 0)),
            handshake_failures=int(data.get("handshake_failures", 0)),
            tls_version=str(data.get("tls_version", "")),
            tls_cipher=str(data.get("tls_cipher", "")),
            raw_samples_path=str(samples) if samples is not None else None,
            cpu_fraction=float(data.get("generator_cpu_fraction", -1.0)),
            # The open loop rules in validity.py are stated in terms of these two.
            pacing_p99_us=float(data.get("pacing_us", {}).get("p99", 0.0)),
            achieved_share=(achieved / offered) if offered > 0 else None,
            # Emitted by the generator since it learned it cannot pin itself on macOS.
            # Read here rather than inferred from the mask we passed, because the whole
            # point is that asking and getting are two different things. The server's
            # requested mask is recoverable from server_argv; it has no reporting channel
            # of its own, which is why the macOS design asks for neither.
            affinity_requested=(mask_hex if (mask_hex := data.get("affinity_mask"))
                                and mask_hex.strip("0") else None),
            affinity_applied=_as_bool(data.get("affinity_applied"))
            if "affinity_applied" in data else None,
            # Absent on Windows and from a generator built before it reported one, which
            # is why this is None rather than a number the driver could compare blindly.
            euid=int(euid) if (euid := data.get("euid")) is not None else None,
            argv=list(argv),
        )
        return result
