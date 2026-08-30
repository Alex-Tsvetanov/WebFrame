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
import socket
import subprocess
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any

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

    def _process_cost(pid: int) -> tuple[float | None, int | None]:
        """CPU seconds and peak footprint from libproc, read while the process is alive.

        proc_pid_rusage rather than getrusage(RUSAGE_CHILDREN): that only accounts for
        children already reaped, so a running server contributes nothing to it, and after
        the wait it is a cumulative total over every subprocess the campaign has ever
        started. Neither is the quantity the record claims.

        Times are nanoseconds and the footprint is already bytes, so unlike ru_maxrss
        there is no unit that differs between Darwin and Linux to get wrong.
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
        info = ctypes.cast(buffer, ctypes.POINTER(_RUSAGE_INFO_V4)).contents
        cpu = (info.ri_user_time + info.ri_system_time) / 1e9
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
    _proc: subprocess.Popen | None = None
    _cost: tuple[float | None, int | None] = (None, None)
    _stats_path: Path | None = None

    @property
    def pid(self) -> int | None:
        return None if self._proc is None else self._proc.pid

    def _ensure_stats_path(self) -> Path:
        if self._stats_path is None:
            self._stats_path = Path(tempfile.gettempdir()) / (
                f"coroute-iostats-{self.port}-{os.getpid()}-{time.time_ns()}.bin"
            )
            try:
                self._stats_path.unlink()
            except FileNotFoundError:
                pass
        return self._stats_path

    @property
    def argv(self) -> list[str]:
        factors = self.cell.as_dict()
        args = [
            str(self.binary),
            "--port", str(self.port),
            "--workers", str(factors.get("workers", 4)),
            "--backlog", str(factors.get("backlog", 1024)),
            "--payload", str(factors.get("payload_bytes", 0)),
            # Unlimited, because a limit forces a reconnect and turns a keep-alive
            # measurement into a measurement of accept.
            "--max-requests", "0",
        ]
        if not factors.get("protocol_detection", True):
            args.append("--no-detect")
        backend = factors.get("io_backend")
        if backend:
            args += ["--io-backend", str(backend)]
        write_path = factors.get("write_path")
        if write_path:
            args += ["--write-path", str(write_path)]
        # Linux only: the shared-memory layout is Linux mmap. Leaving the flag off on
        # other platforms keeps their argv identical to the campaigns already on disk.
        if _platform.system() == "Linux":
            args += ["--io-stats", str(self._ensure_stats_path())]
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
        if _platform.system() == "Linux":
            self._ensure_stats_path()
        self._proc = subprocess.Popen(
            self.argv,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
        )

    def wait_until_ready(self, timeout_s: float) -> bool:
        """Connects until it can, so no run measures process startup."""
        deadline = time.monotonic() + timeout_s
        while time.monotonic() < deadline:
            if self._proc is not None and self._proc.poll() is not None:
                return False
            try:
                with socket.create_connection(("127.0.0.1", self.port), timeout=0.25):
                    return True
            except OSError:
                time.sleep(0.05)
        return False

    def stop(self) -> ResourceUsage:
        if self._proc is None:
            return ResourceUsage()

        # Read the cost and the I/O counters while the process still exists. After it
        # exits the handle reports nothing and a munmap'd stats file is empty.
        cpu, peak = _process_cost(self._proc.pid)
        io_stats = None
        if self._stats_path is not None:
            from benchmark.harness.syscalls import read_io_stats
            io_stats = read_io_stats(self._stats_path)

        self._proc.terminate()
        try:
            self._proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            self._proc.kill()
            self._proc.wait(timeout=10)
        self._proc = None
        if self._stats_path is not None:
            try:
                self._stats_path.unlink()
            except OSError:
                pass
            self._stats_path = None
        return ResourceUsage(cpu_seconds=cpu, memory_peak_bytes=peak, io_stats=io_stats)


# ------------------------------------------------------------------- generator


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
    loopback end-to-end number would flatter whichever arm was slower.
    """

    binary: Path
    port: int
    threads: int
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

    @property
    def name(self) -> str:
        return "coroute-loadgen-wsl" if self.command else "coroute-loadgen"

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
        out = Path(tempfile.gettempdir()) / f"loadgen-{self.port}.json"

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

        lat = data.get("latency_us", {})
        # Reported in milliseconds, because that is what the record and every figure
        # use. Converted once, here, rather than in each consumer.
        latency_ms = {k: v / 1000.0 for k, v in lat.items() if k != "samples"}

        offered = float(data.get("offered_rate", 0.0))
        achieved = float(data.get("rps", 0.0))

        result = GeneratorResult(
            requests_total=int(data.get("completed", 0)) + int(data.get("non_2xx", 0)),
            requests_non_2xx=int(data.get("non_2xx", 0)),
            socket_errors=int(data.get("socket_errors", 0)),
            requests_per_second=float(data.get("rps", 0.0)),
            bytes_per_second=float(data.get("bytes_read", 0)) / max(float(data.get("duration_s", 1)), 1e-9),
            latency_ms=latency_ms,
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
                                and str(mask_hex).strip("0") else None),
            # loadgen emits an unquoted JSON boolean for affinity_applied (field_s with
            # quote=false). Accept both the boolean and the string form.
            affinity_applied=(
                None if "affinity_applied" not in data else (
                    bool(data["affinity_applied"])
                    if isinstance(data["affinity_applied"], bool)
                    else str(data["affinity_applied"]).lower() == "true"
                )
            ),
            argv=list(argv),
        )
        return result
