"""What the machine was, captured so a campaign cannot drift underneath its data.

A measurement campaign runs for weeks. In that time a kernel gets updated, a governor
gets switched back to powersave by a suspend cycle, a dependency gets rebuilt. Any of
those makes the runs before and after incomparable, and none of them announces itself.
The numbers stay plausible, which is the problem: nothing looks wrong, the graph still
has a line on it, and the conclusion is drawn from two populations that were never the
same experiment.

So the environment is captured into a fingerprint, and the driver refuses to add runs
to a campaign whose fingerprint has moved. That single mechanism is most of what makes
a multi-week campaign honest.

The interesting judgement is not how to hash. It is what belongs in the hash.
"""

from __future__ import annotations

import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


# Fields whose change invalidates comparison between runs. Anything here going into
# the fingerprint means a campaign stops rather than silently mixing populations.
#
# Chosen by asking one question of each candidate: if this changed halfway through,
# would the two halves still be one experiment? Kernel, CPU, governor and toolchain all
# fail that test. Load average and uptime pass it, so they are recorded and left out.
#
# Note what this therefore is NOT. The fingerprint answers "is this the same machine",
# not "was this run admissible", and those are different questions answered in different
# places. Power source is the clearest case: it is captured, it is not here, and it is
# checked per run by validity.check_run. That is right, because a laptop is the same
# machine plugged in or not, and fingerprinting it would refuse to append to a campaign
# for the entirely legitimate act of connecting a charger.
#
# The consequence, which is easier to write down than to rediscover: two runs can share
# a fingerprint and still differ in a way the rules refuse. A campaign file will happily
# accept the append; validity will reject the record. Demonstrated deliberately, and
# harmlessly, by the descriptor census, whose two runs on battery-with-low-power-mode and
# on mains produced the same fingerprint and byte-identical tables. For a count that is
# the correct outcome. For a timing campaign it is the whole difference between a run
# that may join the population and one that must not.
_FINGERPRINTED = (
    "machine.node",
    "machine.arch",
    "kernel.release",
    "cpu.model",
    "cpu.physical_cores",
    "cpu.logical_cores",
    "cpu.siblings",
    "cpu.governor",
    "memory.total_bytes",
    "tuning.transparent_hugepages",
    "tuning.swappiness",
    "toolchain.compiler",
    "build.type",
    "build.io_backend",
    "build.git_commit",
    "deps.openssl",
    "deps.ngtcp2",
    "deps.nghttp3",
    "deps.liburing",
)


def _run(cmd: list[str], ok: tuple[int, ...] = (0,)) -> str | None:
    """Runs a command and returns its stripped stdout, or None if it did not work.

    None rather than an exception on purpose: capture must not fail because one probe
    is unavailable. A missing field is recorded as missing and shows up in the
    fingerprint as such, which is a difference the driver will notice if it changes.

    `ok` is the set of exit codes that count as an answer. Some tools answer a yes/no
    question with the exit code, and then a non-zero exit with output is the answer no
    rather than a failure.
    """
    if shutil.which(cmd[0]) is None:
        return None
    try:
        out = subprocess.run(cmd, capture_output=True, text=True, timeout=10, check=False)
    except (OSError, subprocess.SubprocessError):
        return None
    if out.returncode not in ok:
        return None
    return out.stdout.strip() or None


def _read(path: str) -> str | None:
    try:
        return Path(path).read_text(encoding="utf-8", errors="replace").strip()
    except OSError:
        return None


def _cpu_model() -> str | None:
    text = _read("/proc/cpuinfo")
    if text:
        match = re.search(r"^model name\s*:\s*(.+)$", text, re.MULTILINE)
        if match:
            return match.group(1).strip()
    return platform.processor() or None


def _physical_cores() -> int | None:
    """Physical cores, not threads.

    Both are recorded because the difference is the SMT setting, and a campaign that
    silently gained or lost hyperthreading halfway through would otherwise look like a
    performance change in the server.
    """
    text = _read("/proc/cpuinfo")
    if not text:
        return None
    seen = set()
    physical_id = core_id = None
    for line in text.splitlines():
        if line.startswith("physical id"):
            physical_id = line.split(":", 1)[1].strip()
        elif line.startswith("core id"):
            core_id = line.split(":", 1)[1].strip()
            if physical_id is not None:
                seen.add((physical_id, core_id))
    return len(seen) or None


def _siblings(sysfs: Path = Path("/sys/devices/system/cpu")) -> list[str | None] | None:
    """thread_siblings_list per logical CPU, indexed by CPU number; None off Linux.

    The affinity masks name logical CPUs, and the claim that 0ff and f00 are four and
    two disjoint physical cores rests on siblings being enumerated in adjacent pairs.
    Linux numbers CPUs in firmware order and may interleave them, so the layout is
    recorded, and fingerprinted because nosmt or a firmware update changes what the same
    two masks mean. The list is the kernel's own string for each CPU, "0-1" or "0,6",
    which identifies the physical core without being parsed. An offline CPU has no
    topology directory and is None at its index.
    """
    try:
        dirs = [d for d in sysfs.iterdir() if re.fullmatch(r"cpu\d+", d.name)]
    except OSError:
        return None
    if not dirs:
        return None
    by_index = {int(d.name[3:]): _read(str(d / "topology" / "thread_siblings_list")) for d in dirs}
    return [by_index.get(i) for i in range(max(by_index) + 1)]


_GOVERNOR_PATH = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"


def _governor(path: str = _GOVERNOR_PATH, system: str | None = None) -> str | None:
    """The scaling governor of CPU 0.

    In the fingerprint because it is the single tuning knob most likely to move without
    anyone touching it: a suspend cycle or a power-profile daemon can put a machine back
    on powersave, and every run after that is slower for a reason that has nothing to do
    with the code being measured.

    None is Windows and macOS, which publish no governor. Linux always has one, so a
    Linux host where the file cannot be read (no cpufreq driver bound, a restricted
    sysfs) answers unchecked rather than None: None passes the validity rule, and a
    host whose governor cannot be established is not thereby on performance.
    """
    text = _read(path)
    if text is None and (system or platform.system()) == "Linux":
        return _UNCHECKED.format(f"no scaling_governor at {path}")
    return text


def _transparent_hugepages() -> str | None:
    text = _read("/sys/kernel/mm/transparent_hugepage/enabled")
    if not text:
        return None
    match = re.search(r"\[(\w+)\]", text)
    return match.group(1) if match else text


# --- macOS -------------------------------------------------------------------
#
# Everything above reads /proc or /sys, which macOS does not have, so on darwin the same
# facts come from sysctl and pmset instead. Kept in one block and reached only through
# _darwin_overlay, so Linux and Windows emit exactly the manifest they emitted before
# this branch existed.


def _sysctl(name: str) -> str | None:
    return _run(["sysctl", "-n", name])


def _sysctl_int(name: str) -> int | None:
    """A sysctl documented to hold a number, or None if it did not hold one.

    Not int() at the call site: a sysctl that exists and reports something unparseable
    would then raise out of capture and take the campaign with it, which is a worse
    outcome than one missing field.
    """
    text = _sysctl(name)
    if text is None:
        return None
    try:
        return int(text)
    except ValueError:
        return None


# Not "AC" or "mains": validity tests for the substring "battery", so anything this
# returns for a healthy host must not contain it. "no battery present" would reject the
# machine it describes.
_ON_MAINS = "mains"
_NO_BATTERY = "mains (desktop, no power source to change)"


def _cpu_mhz_linux(cpuinfo: str | None = None) -> float | None:
    """Mean current clock across cores, from /proc/cpuinfo."""
    text = cpuinfo if cpuinfo is not None else _read("/proc/cpuinfo")
    if not text:
        return None
    values = [float(m) for m in re.findall(r"^cpu MHz\s*:\s*([\d.]+)$", text, re.MULTILINE)]
    return sum(values) / len(values) if values else None


def _cpu_mhz_windows(perf: str | None = None) -> float | None:
    """Effective clock on Windows, as base frequency times the performance percentage.

    Win32_Processor.CurrentClockSpeed is not usable: it reports the nominal figure and
    does not move, so a machine that throttled to half speed still reads full. The
    formatted performance counter does move.

    Read through Win32_PerfFormattedData rather than Get-Counter because that returns
    integers. Get-Counter returns a localised decimal, and this project's own Windows
    host formats it "99,9", which float() would reject. A probe that fails on a decimal
    separator is a probe that silently returns None on somebody's machine.
    """
    if perf is None:
        perf = _run([
            "powershell", "-NoProfile", "-NonInteractive", "-Command",
            "$x = Get-CimInstance Win32_PerfFormattedData_Counters_ProcessorInformation "
            "| Where-Object { $_.Name -eq '_Total' }; "
            "'{0}|{1}' -f $x.ProcessorFrequency, $x.PercentProcessorPerformance",
        ])
    if not perf or "|" not in perf:
        return None
    base, _, pct = perf.strip().partition("|")
    try:
        return float(base) * float(pct) / 100.0
    except ValueError:
        return None


def cpu_mhz(system: str | None = None, raw: str | None = None) -> float | str | None:
    """The current clock, for the drift check, on the platforms that can say.

    This lived in validity.py and read /proc/cpuinfo directly, so it returned None on
    Windows and macOS. Combined with the thermal check being pmset-only, that left
    Windows with no clock-stability gate of any kind, on the platform that produced
    every measurement this project has and that runs the dispatch microbenchmark.

    macOS is still None here. Intel Macs are covered by CPU_Speed_Limit and Apple
    Silicon publishes neither, which is a gap that belongs in the limitations rather
    than in a probe that guesses.

    On the two platforms that do answer, a parser that found nothing (an aarch64
    /proc/cpuinfo with no `cpu MHz` lines, a powershell probe that timed out) is
    unchecked rather than None, and validity refuses it: the drift rule skips None,
    and a probe failure must not read as a steady clock. `raw` is the parser's text,
    for checking this from a host that is neither.
    """
    system = system or platform.system()
    if system == "Linux":
        value = _cpu_mhz_linux(raw)
    elif system == "Windows":
        value = _cpu_mhz_windows(raw)
    else:
        return None
    if value is None:
        return _UNCHECKED.format(f"no usable clock reading on {system}")
    return value


def _power_source_darwin(pmset_ps: str | None = None) -> str | None:
    """Mains or battery, as pmset names it."""
    text = pmset_ps if pmset_ps is not None else _run(["pmset", "-g", "ps"])
    if not text:
        return _UNCHECKED.format("pmset -g ps did not answer")
    match = re.search(r"Now drawing from '([^']+)'", text)
    if match is None:
        return _UNCHECKED.format("pmset -g ps printed no 'Now drawing from' line")
    return match.group(1)


def _power_source_windows(battery_status: str | None = None) -> str | None:
    """Mains, battery, or no battery at all, from Win32_Battery.

    A desktop has no Win32_Battery instance, and that is a real answer rather than a
    missing one: a machine with no battery cannot be discharging.
    """
    if battery_status is None:
        battery_status = _run([
            "powershell", "-NoProfile", "-NonInteractive", "-Command",
            "$b = @(Get-CimInstance Win32_Battery); "
            "if ($b.Count -eq 0) { 'none' } else { ($b | ForEach-Object { $_.BatteryStatus }) -join ',' }",
        ])
    if battery_status is None:
        return _UNCHECKED.format("Win32_Battery did not answer")
    text = battery_status.strip()
    if text == "none":
        return _NO_BATTERY
    # 2 is "AC connected". Everything else means running down or in an odd state, and an
    # odd state on a benchmark host is not a state to average over.
    codes = [c.strip() for c in text.split(",") if c.strip()]
    if codes and all(c == "2" for c in codes):
        return _ON_MAINS
    return f"battery (Win32_Battery BatteryStatus={text})"


def _power_source_linux(supplies: list[str] | None = None,
                        reader: "object | None" = None) -> str | None:
    """Mains, battery, or no battery at all, from /sys/class/power_supply."""
    root = Path("/sys/class/power_supply")
    if supplies is None:
        try:
            supplies = sorted(entry.name for entry in root.iterdir())
        except OSError:
            return _UNCHECKED.format("/sys/class/power_supply is not readable")
    read = reader if reader is not None else (lambda name, f: _read(str(root / name / f)))
    # A wireless mouse or keyboard registers as hidpp_battery_N with type Battery and
    # scope Device, and reports Discharging whenever it is off its cable, which would
    # put a desktop on battery and refuse every run. Only System-scoped supplies power
    # the host. The default has to be System, because ACPI's BAT0 publishes no scope
    # file at all and a laptop battery must not vanish for lacking one.
    batteries = [s for s in supplies
                 if (read(s, "type") or "").strip() == "Battery"
                 and (read(s, "scope") or "System").strip() != "Device"]
    if not batteries:
        return _NO_BATTERY
    for name in batteries:
        status = (read(name, "status") or "").strip()
        if status == "Discharging":
            return f"battery ({name} discharging)"
        if not status:
            return _UNCHECKED.format(f"{name} published no status")
    return _ON_MAINS


def _power_source(pmset_ps: str | None = None) -> str | None:
    """Whether the host can change its own power policy mid-campaign.

    A laptop on battery is a different machine. Discharging biases the scheduler toward
    the efficiency cores and caps the clocks, so the run measures the power policy rather
    than the server.

    This used to be macOS-only and returned None everywhere else, and None reads as
    clean, which is the same fail-open shape the virtualisation gate had: a host that
    could not be asked was treated as a host that answered well. A desktop with no
    battery is genuinely clean and now says so; a host that could not be asked now says
    that instead, and is refused.
    """
    system = platform.system()
    if system == "Darwin":
        return _power_source_darwin(pmset_ps)
    if system == "Windows":
        return _power_source_windows()
    if system == "Linux":
        return _power_source_linux()
    return _UNCHECKED.format(f"no power probe for {system or 'this platform'}")


def _low_power_mode(pmset_live: str | None = None) -> bool | None:
    """Whether Low Power Mode is on, under either spelling pmset has used.

    macOS 26 renamed the setting: there is no lowpowermode line any more, and the
    replacement is powermode, which is tri-state rather than boolean. 0 is automatic,
    1 is low power, 2 is high power. Reading only the old name is the same defect as
    reading a counting tool that is not installed: it returns None for ever and the
    absence looks like hardware without the feature rather than like a probe that
    stopped working.

    None still means not established, which is the honest answer for a machine that
    reports neither key. Recording it as off would be a claim nothing measured.
    """
    text = pmset_live if pmset_live is not None else _run(["pmset", "-g"])
    if not text:
        return None
    match = re.search(r"^\s*powermode\s+(\d+)\s*$", text, re.MULTILINE)
    if match:
        return match.group(1) == "1"
    match = re.search(r"^\s*lowpowermode\s+(\d+)\s*$", text, re.MULTILINE)
    return match.group(1) != "0" if match else None


def _cpu_speed_limit(pmset_therm: str | None = None) -> int | None:
    """CPU_Speed_Limit as a percentage of full speed, which is how macOS says throttled.

    `pmset -g therm` prints the fields read here without elevated privileges. When the
    kernel has published no limit it prints a note instead, and the field stays None.
    That is not the same fact as 100 and is not recorded as 100.
    """
    text = pmset_therm if pmset_therm is not None else _run(["pmset", "-g", "therm"])
    if not text:
        return None
    match = re.search(r"CPU_Speed_Limit\s*=\s*(\d+)", text)
    return int(match.group(1)) if match else None


def _set_if_probed(section: dict[str, Any], key: str, value: Any) -> None:
    """Writes the sysctl answer, or leaves what capture already found if there was none.

    os.cpu_count() already reports the logical cores correctly on macOS, and that is a
    fingerprinted field. Assigning unconditionally would let one sysctl that failed to
    answer replace a known value with None and move the fingerprint, which stops the
    campaign over the probe rather than over the machine.
    """
    if value is not None:
        section[key] = value


def _darwin_overlay(env: dict[str, Any]) -> None:
    """Fills in from sysctl and pmset what /proc fills in on Linux.

    The perflevel counts and the power block have no Linux or Windows equivalent, so they
    are absent there rather than present and None. None of them joins _FINGERPRINTED: a
    new fingerprinted key adds itself as null to every manifest already captured and moves
    every existing hash, which would end the Windows campaign this branch exists to match.
    """
    _set_if_probed(env["cpu"], "model", _sysctl("machdep.cpu.brand_string"))
    _set_if_probed(env["cpu"], "physical_cores", _sysctl_int("hw.physicalcpu"))
    _set_if_probed(env["cpu"], "logical_cores", _sysctl_int("hw.logicalcpu"))
    # perflevel0 is the fastest core class and perflevel1 the next one down, so here they
    # are the P and E counts. Nothing else in the manifest tells the two apart, and on a
    # heterogeneous chip which kind of core served the load is most of the result. A
    # uniform part publishes no perflevel1, so there the count stays None.
    env["cpu"]["performance_cores"] = _sysctl_int("hw.perflevel0.physicalcpu")
    env["cpu"]["efficiency_cores"] = _sysctl_int("hw.perflevel1.physicalcpu")
    _set_if_probed(env["memory"], "total_bytes", _sysctl_int("hw.memsize"))
    # macOS clamps listen() to somaxconn and ships it at 128, while the design sweeps
    # backlog over 128, 512, 1024 and 4096. Unrecorded, three of those cells measure one
    # clamped queue while each record claims a different backlog.
    _set_if_probed(env["tuning"], "somaxconn", _sysctl("kern.ipc.somaxconn"))
    env["power"] = {
        "source": _power_source(),
        "low_power_mode": _low_power_mode(),
        "cpu_speed_limit": _cpu_speed_limit(),
    }


def _compiler_version() -> str | None:
    """The first line of `c++ --version`, which is the identifying one.

    Only the first line: the rest is licence boilerplate that would go into the
    fingerprint and change on a distribution rebuild that did not change the compiler.
    """
    text = _run(["c++", "--version"])
    return text.splitlines()[0] if text else None


def _pkgconfig_version(name: str) -> str | None:
    return _run(["pkg-config", "--modversion", name])


def _git_commit(repo: Path) -> str | None:
    try:
        out = subprocess.run(
            ["git", "-C", str(repo), "rev-parse", "HEAD"],
            capture_output=True, text=True, timeout=10, check=False,
        )
    except (OSError, subprocess.SubprocessError):
        return None
    return out.stdout.strip() if out.returncode == 0 else None


def _git_dirty(repo: Path) -> bool | None:
    """Whether the working tree has uncommitted changes.

    Recorded and checked, but not fingerprinted: a dirty tree is not a different
    environment, it is an untrustworthy one, so it belongs in the validity rules rather
    than in the identity. A commit hash that does not describe the binary is worse than
    no hash, because it looks authoritative.
    """
    try:
        out = subprocess.run(
            ["git", "-C", str(repo), "status", "--porcelain"],
            capture_output=True, text=True, timeout=10, check=False,
        )
    except (OSError, subprocess.SubprocessError):
        return None
    if out.returncode != 0:
        return None
    return bool(out.stdout.strip())


# Strings that appear in SMBIOS identity fields under a hypervisor.
#
# Deliberately NOT "is a hypervisor present". On Windows, enabling WSL2 sets
# Win32_ComputerSystem.HypervisorPresent to true on bare metal, so that flag would
# refuse the machine that produced most of this project's data. Identity strings say
# what the firmware claims to be, which is the question actually being asked.
#
# "microsoft corporation" is absent on purpose: Surface hardware reports it as
# manufacturer on real metal. Hyper-V guests are caught by the model, "Virtual Machine".
_UNCHECKED = "unchecked ({})"

_VM_SIGNATURES = (
    "vmware", "virtualbox", "innotek", "virtual machine", "virtual platform",
    "qemu", "kvm", "xen", "parallels", "bochs", "bhyve",
    "amazon ec2", "google compute engine", "openstack", "alibaba cloud",
)


def _virtualisation_from_identity(fields: str | None) -> str | None:
    """The signature matched by a machine's SMBIOS identity strings, or None.

    None means checked and nothing matched. The caller decides what a probe that could
    not run at all means, which is a different thing and must not collapse into this one.
    """
    if not fields:
        return None
    haystack = fields.lower()
    for signature in _VM_SIGNATURES:
        if signature in haystack:
            return signature
    return None


def _virtualisation_darwin(hv_vmm_present: str | None = None) -> str | None:
    """Darwin's own answer, from kern.hv_vmm_present.

    1 when running under a hypervisor, 0 on metal. Native, cheap, needs no privileges,
    and it exists precisely because the question has no portable answer.
    """
    if hv_vmm_present is None:
        hv_vmm_present = _run(["sysctl", "-n", "kern.hv_vmm_present"])
    if hv_vmm_present is None:
        return _UNCHECKED.format("sysctl kern.hv_vmm_present did not answer")
    return "hypervisor (kern.hv_vmm_present=1)" if hv_vmm_present.strip() == "1" else None


def _virtualisation_windows(identity: str | None = None) -> str | None:
    """Windows, from the SMBIOS identity WMI reports rather than the hypervisor flag."""
    if identity is None:
        identity = _run([
            "powershell", "-NoProfile", "-NonInteractive", "-Command",
            "$c = Get-CimInstance Win32_ComputerSystem; "
            "$b = Get-CimInstance Win32_BIOS; "
            "'{0}|{1}|{2}' -f $c.Manufacturer, $c.Model, $b.Manufacturer",
        ])
    if identity is None:
        return _UNCHECKED.format("Win32_ComputerSystem did not answer")
    return _virtualisation_from_identity(identity)


def _virtualisation_linux(detect_virt: str | None = None,
                          identity: str | None = None) -> str | None:
    """Linux, preferring systemd-detect-virt and falling back to DMI without systemd.

    systemd-detect-virt exits 0 only when it detects something and prints `none` with
    exit 1 on bare metal, so accepting exit 0 alone read every bare-metal answer as no
    answer and took the verdict from the SMBIOS heuristic below, which this module
    documents as spoofable, while claiming it came from systemd. A genuine failure still
    prints nothing to stdout and stays None.
    """
    if detect_virt is None:
        detect_virt = _run(["systemd-detect-virt"], ok=(0, 1))
    if detect_virt is not None:
        return None if detect_virt.strip() == "none" else detect_virt.strip()
    if identity is None:
        identity = " ".join(filter(None, (
            _read("/sys/class/dmi/id/sys_vendor"),
            _read("/sys/class/dmi/id/product_name"),
        )))
    if not identity.strip():
        return _UNCHECKED.format("no systemd-detect-virt and no DMI identity")
    return _virtualisation_from_identity(identity)


def detect_virtualisation() -> str | None:
    """The virtualisation technology in use, None on bare metal, or a refusal.

    The validity rules refuse a performance record when this is set, which is meant to
    make publishing numbers from a VM by accident mechanically impossible rather than a
    matter of remembering.

    It did not do that. The only probe was systemd-detect-virt, which exists on Linux
    and nowhere else, and _run maps an absent binary to None so one missing probe cannot
    fail a capture. So on Windows and macOS this returned None unconditionally, and None
    reads as bare metal. Every record this project holds from those two platforms was
    stamped bare metal without anything ever having checked. The gate fired exactly once
    in the project's history, on the Linux runs in Docker, which is the one platform
    where the probe existed.

    So a probe that could not run now returns a string beginning "unchecked", which is
    truthy and therefore refused by the same rule. Unknown is not clean. A machine that
    cannot answer the question is not thereby a bare-metal machine.

    The Windows and Linux-without-systemd paths read SMBIOS identity strings, which a
    hypervisor can be configured to spoof. That is a weaker guarantee than Darwin's
    sysctl or systemd-detect-virt, and it is stated rather than glossed: the check
    stops an accident, not an adversary.
    """
    system = platform.system()
    if system == "Darwin":
        return _virtualisation_darwin()
    if system == "Windows":
        return _virtualisation_windows()
    if system == "Linux":
        return _virtualisation_linux()
    return _UNCHECKED.format(f"no probe for {system or 'this platform'}")


# "Server listening on port 18080 (multi-accept, backend io_uring)", and the same
# without the multi-accept clause on the fallback accept path.
#
# Here rather than in adapters.py because two entry points read it now, the campaign
# driver and the descriptor census, and a second copy of this pattern is a second thing
# that can drift from what App actually prints.
BANNER_BACKEND = re.compile(r"Server listening on port \d+ \([^)]*backend (\w+)\)")


def default_io_backend() -> str:
    """The backend the presets compile in on this host.

    One source of truth, because build.io_backend is a fingerprinted key: it is what
    stops a kqueue run being compared against an IOCP run as if the two were
    repetitions of one measurement. Two entry points disagreeing about the name would
    defeat the fingerprint as thoroughly as not recording it at all.

    Note this is the backend the CMake presets select by platform, and on Linux it is
    also the arm a run defaults to. It is no longer the only arm available: a tree
    configured with COROUTE_IO_BACKEND=dual contains both Linux backends and picks
    between them per process with --io-backend, so what a given run actually used comes
    from the server's own banner and not from here. This stays the name of the default.
    """
    return {"Darwin": "kqueue", "Linux": "io_uring"}.get(platform.system(), "iocp")


def io_backend_from_build(build_dir: Path | None) -> str | None:
    """The backend a build tree was actually configured with, read from its CMakeCache.

    A reading, where default_io_backend is a guess. The guess is wrong on Linux, which
    is the platform this matters on: both epoll and io_uring are selectable at
    configure time, so the operating system does not determine the answer, and a record
    that said io_uring for an epoll build is precisely the mislabelled factor this key
    exists to prevent.

    Returns None when there is no cache to read, so the caller can decide whether to
    fall back or refuse.
    """
    if build_dir is None:
        return None
    cache = Path(build_dir) / "CMakeCache.txt"
    try:
        text = cache.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return None
    match = re.search(r"^COROUTE_IO_BACKEND:[A-Z]+=(.+)$", text, re.MULTILINE)
    return match.group(1).strip() if match else None


# The values a Linux tree can legitimately be configured with.
_LINUX_IO_BACKENDS = ("io_uring", "epoll", "dual")


def resolve_io_backend(build_dir: Path | None) -> str:
    """The backend to record, preferring what the build says over what the host implies.

    Refuses rather than choosing when the two disagree. A campaign whose cells claim one
    backend against a binary built with another is not a degraded measurement, it is a
    mislabelled one, and a mislabelled factor makes two different measurements look like
    repetitions of a single one.

    On Linux the reading is the answer and there is nothing to disagree with. The
    platform does not determine the backend there: io_uring, epoll and dual are all
    legitimate, and epoll in particular is what the linux-epoll preset builds, so
    refusing it as a disagreement refused a supported configuration. It follows that a
    Linux tree whose CMakeCache cannot be read has no answer at all rather than a
    default, because the operating system alone cannot supply one; that is raised.

    Elsewhere the platform does determine it, one backend per system, so a cache that
    says otherwise is a real disagreement and is still refused.
    """
    guessed = default_io_backend()
    read = io_backend_from_build(build_dir)

    if platform.system() == "Linux":
        if read in _LINUX_IO_BACKENDS:
            # "dual" included, and recorded as "dual": the binary really does contain
            # both backends and no single name for the compiled set is more accurate. It
            # says nothing about which arm a given run used, and it must not. That is a
            # per-run factor carried by the cell and confirmed against the server's
            # banner.
            return read
        raise ValueError(
            f"cannot read COROUTE_IO_BACKEND from {build_dir}. On Linux the build "
            f"decides the backend and the host cannot be asked instead, so there is no "
            f"safe default to record. Point --build at a configured build tree."
        )

    if read and read != guessed:
        raise ValueError(
            f"the build in {build_dir} was configured with COROUTE_IO_BACKEND={read}, "
            f"but this host implies {guessed}. Recording either would mislabel the run. "
            f"Point --build at a tree configured for {guessed}."
        )
    return read or guessed


def run_io_backend(build_dir: Path | None, requested: str | None = None) -> str:
    """The single arm a run will ask the server for, from the build and the flag.

    resolve_io_backend answers what the binary contains, which for a dual tree is two
    backends and not an arm. A cell has to name one: it is passed to --io-backend and
    checked against the server's banner. That made io_uring the only arm any campaign
    could ever measure on Linux, and on a linux-epoll tree it made every cell ask for a
    backend the binary does not have, which benchmark_server refuses with exit 2.

    So the arm is chosen here. With no flag it is the first the build offers, which for
    dual is io_uring and for every single-backend tree is the one thing it compiled, so
    the campaigns already on disk record exactly what they recorded before. With a flag
    it is the flag, and an arm the build does not contain is refused rather than
    recorded: a cell claiming epoll against an io_uring-only binary is a mislabelled
    measurement, not a degraded one.
    """
    compiled = resolve_io_backend(build_dir)
    arms = ("io_uring", "epoll") if compiled == "dual" else (compiled,)
    arm = requested or arms[0]
    if arm not in arms:
        raise ValueError(
            f"the build in {build_dir} was configured with COROUTE_IO_BACKEND="
            f"{compiled}, which does not contain {arm}. The linux-dual preset builds "
            f"both Linux arms and lets --io-backend choose between them; linux-release "
            f"builds io_uring alone and linux-epoll builds epoll alone."
        )
    return arm


def capture(repo: Path | None = None, build_type: str | None = None,
            io_backend: str | None = None) -> dict[str, Any]:
    """Everything worth knowing about the machine, in one nested dict."""
    repo = repo or Path(__file__).resolve().parents[2]
    uname = platform.uname()

    meminfo = _read("/proc/meminfo") or ""
    mem_match = re.search(r"^MemTotal:\s+(\d+) kB$", meminfo, re.MULTILINE)

    env = {
        "machine": {
            "node": uname.node or None,
            "arch": uname.machine or None,
            "system": uname.system or None,
        },
        "kernel": {
            "release": uname.release or None,
            "version": uname.version or None,
        },
        "cpu": {
            "model": _cpu_model(),
            "physical_cores": _physical_cores(),
            "logical_cores": os.cpu_count(),
            "siblings": _siblings(),
            "governor": _governor(),
        },
        "memory": {
            "total_bytes": int(mem_match.group(1)) * 1024 if mem_match else None,
        },
        "tuning": {
            "transparent_hugepages": _transparent_hugepages(),
            "swappiness": _read("/proc/sys/vm/swappiness"),
            "somaxconn": _read("/proc/sys/net/core/somaxconn"),
        },
        "toolchain": {
            "compiler": _compiler_version(),
            "python": platform.python_version(),
        },
        "build": {
            "type": build_type,
            "io_backend": io_backend,
            "git_commit": _git_commit(repo),
            "git_dirty": _git_dirty(repo),
        },
        "deps": {
            "openssl": _pkgconfig_version("openssl"),
            "ngtcp2": _pkgconfig_version("libngtcp2"),
            "nghttp3": _pkgconfig_version("libnghttp3"),
            "liburing": _pkgconfig_version("liburing"),
        },
        "virtualisation": detect_virtualisation(),
    }

    if sys.platform == "darwin":
        _darwin_overlay(env)
    return env


def _lookup(env: dict[str, Any], dotted: str) -> Any:
    node: Any = env
    for part in dotted.split("."):
        if not isinstance(node, dict) or part not in node:
            return None
        node = node[part]
    return node


def fingerprint(env: dict[str, Any]) -> str:
    """A stable hash over the fields whose change invalidates comparison.

    Deliberately not a hash of the whole dict. Load average, uptime and free memory move
    every second, so hashing everything would make every run its own campaign and the
    check would be worthless: a guard that always fires is a guard that gets disabled.
    """
    material = {key: _lookup(env, key) for key in _FINGERPRINTED}
    canonical = json.dumps(material, sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(canonical.encode("utf-8")).hexdigest()


def fingerprint_differences(before: dict[str, Any], after: dict[str, Any]) -> list[str]:
    """Which fingerprinted fields moved, so a refusal can say why.

    A guard that reports only "the environment changed" gets overridden out of
    frustration. One that says the governor went from performance to powersave gets
    acted on.
    """
    out = []
    for key in _FINGERPRINTED:
        old, new = _lookup(before, key), _lookup(after, key)
        if old != new:
            out.append(f"{key}: {old!r} -> {new!r}")
    return out


@dataclass
class Campaign:
    """A set of runs that are comparable with each other, and the proof that they are.

    Opening a campaign whose environment has moved raises. That is the point: the
    alternative is a directory of results that look like one experiment and are two.
    """

    path: Path
    environment: dict[str, Any]
    fingerprint: str = field(default="")

    def __post_init__(self) -> None:
        if not self.fingerprint:
            self.fingerprint = fingerprint(self.environment)

    def save(self) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.path.write_text(
            json.dumps(
                {"fingerprint": self.fingerprint, "environment": self.environment},
                indent=2, sort_keys=True,
            ),
            encoding="utf-8",
        )

    @classmethod
    def open_or_create(cls, path: Path, environment: dict[str, Any],
                       force: bool = False) -> Campaign:
        """Loads an existing campaign, or starts one.

        Raises EnvironmentChanged if the machine no longer matches what the campaign was
        started on, unless force is set. force exists because there are legitimate
        reasons to continue anyway, but it is an argument someone has to type, which is
        the difference between a considered decision and an accident.
        """
        current = fingerprint(environment)
        if not path.exists():
            campaign = cls(path=path, environment=environment, fingerprint=current)
            campaign.save()
            return campaign

        stored = json.loads(path.read_text(encoding="utf-8"))
        if stored["fingerprint"] != current and not force:
            differences = fingerprint_differences(stored["environment"], environment)
            # The hash and the differences are computed over the same key list, so a hash
            # that moved while no field moved means only one thing: the list grew since
            # the manifest was written, and the new key is null on both sides. That is
            # the same machine, and a manifest must not be refused for a harness update.
            # A new key that has a value on one side is listed above and refused.
            if differences:
                raise EnvironmentChanged(path=path, differences=differences)
        return cls(path=path, environment=stored["environment"],
                   fingerprint=stored["fingerprint"])


class EnvironmentChanged(RuntimeError):
    """The machine is not the one this campaign was started on."""

    def __init__(self, path: Path, differences: list[str]) -> None:
        self.path = path
        self.differences = differences
        detail = "\n  ".join(differences) if differences else "(no field-level detail)"
        super().__init__(
            f"environment no longer matches campaign {path}:\n  {detail}\n"
            "Runs recorded before and after this change are not comparable. "
            "Start a new campaign, or pass force=True if you have decided otherwise."
        )
