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
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


# Fields whose change invalidates comparison between runs. Anything here going into
# the fingerprint means a campaign stops rather than silently mixing populations.
#
# Chosen by asking one question of each candidate: if this changed halfway through,
# would the two halves still be one experiment? Kernel, CPU, governor and toolchain all
# fail that test. Load average and uptime pass it, so they are recorded and left out.
_FINGERPRINTED = (
    "machine.node",
    "machine.arch",
    "kernel.release",
    "cpu.model",
    "cpu.physical_cores",
    "cpu.logical_cores",
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


def _run(cmd: list[str]) -> str | None:
    """Runs a command and returns its stripped stdout, or None if it did not work.

    None rather than an exception on purpose: capture must not fail because one probe
    is unavailable. A missing field is recorded as missing and shows up in the
    fingerprint as such, which is a difference the driver will notice if it changes.
    """
    if shutil.which(cmd[0]) is None:
        return None
    try:
        out = subprocess.run(cmd, capture_output=True, text=True, timeout=10, check=False)
    except (OSError, subprocess.SubprocessError):
        return None
    if out.returncode != 0:
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


def _governor() -> str | None:
    """The scaling governor of CPU 0.

    In the fingerprint because it is the single tuning knob most likely to move without
    anyone touching it: a suspend cycle or a power-profile daemon can put a machine back
    on powersave, and every run after that is slower for a reason that has nothing to do
    with the code being measured.
    """
    return _read("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor")


def _transparent_hugepages() -> str | None:
    text = _read("/sys/kernel/mm/transparent_hugepage/enabled")
    if not text:
        return None
    match = re.search(r"\[(\w+)\]", text)
    return match.group(1) if match else text


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


def detect_virtualisation() -> str | None:
    """The virtualisation technology in use, or None on bare metal.

    Returns a string such as "kvm", "wsl" or "docker". The validity rules refuse to
    accept a performance record when this is set, which makes publishing numbers from a
    VM by accident mechanically impossible rather than a matter of remembering.
    """
    result = _run(["systemd-detect-virt"])
    if result in (None, "none"):
        return None
    return result


def capture(repo: Path | None = None, build_type: str | None = None,
            io_backend: str | None = None) -> dict[str, Any]:
    """Everything worth knowing about the machine, in one nested dict."""
    repo = repo or Path(__file__).resolve().parents[2]
    uname = platform.uname()

    meminfo = _read("/proc/meminfo") or ""
    mem_match = re.search(r"^MemTotal:\s+(\d+) kB$", meminfo, re.MULTILINE)

    return {
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
            raise EnvironmentChanged(
                path=path,
                differences=fingerprint_differences(stored["environment"], environment),
            )
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
