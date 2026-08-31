"""Counts the listening descriptors a running server actually holds.

The central proposition of the dissertation is about a count, not a speed:

    L = A * |T|

listening descriptors, where T is the set of transports and A is the number of accepting
descriptors the platform's accept model requires: the worker count under SO_REUSEPORT,
one under a shared-listener model such as IOCP or kqueue. Notably NOT a function of the
number of application protocols served, which is the part the work defends.

The earlier form of that line read W * |T| unconditionally, and this script is what
disproved it: Windows holds one listening descriptor at one, two, four and eight workers.
An instrument that still asserted the refuted form would be quoting the hypothesis it
exists to test.

Asking the operating system matters. The server could be made to report whatever the
author expected, and a census taken from inside the process being measured is not
independent of it. Both platforms here read the kernel's own table.

    python -m benchmark.descriptor_census <build-dir> [--workers 1,2,4,8]
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def wait_until_listening(proc: subprocess.Popen, port: int,
                         timeout_s: float = 15.0) -> bool:
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        # A child that has already exited will never answer, so sitting out the rest of
        # the timeout only delays the report and discards nothing useful.
        if proc.poll() is not None:
            return False
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.25):
                return True
        except OSError:
            time.sleep(0.05)
    return False


def lsof_count(pid: int, selector: list[str]) -> int:
    """Files matching one lsof selector for one process, in field mode."""
    # -a intersects -p with -i. Without it lsof ORs them and reports every socket on the
    # machine. -w drops warnings, -nP skips host and port name resolution.
    proc = subprocess.run(
        ["lsof", "-nP", "-w", "-a", "-p", str(pid), *selector, "-Fn"],
        capture_output=True, text=True, timeout=60,
    )
    # lsof exits 1 both when the selectors simply matched nothing, which is a real zero,
    # and when lsof itself failed. -w has silenced the warnings, so anything left on
    # stderr marks the second case, where the empty output is not a count.
    quiet_empty_match = proc.returncode == 1 and not proc.stderr.strip()
    if proc.returncode != 0 and not quiet_empty_match:
        raise RuntimeError(
            f"lsof exited {proc.returncode} for pid {pid} {' '.join(selector)}: "
            f"{proc.stderr.strip()}"
        )
    return sum(1 for line in proc.stdout.splitlines() if line.startswith("n"))


def counting_command() -> str:
    """Named in failure messages so a zero census points at the tool that produced it."""
    if os.name == "nt":
        return "Get-NetTCPConnection -State Listen / Get-NetUDPEndpoint"
    if sys.platform == "darwin":
        return "lsof -nP -w -a -p <pid> -iTCP -sTCP:LISTEN -Fn"
    return "ss -lntupH"


def count_listeners(pid: int) -> tuple[int, int]:
    """Listening TCP endpoints and bound UDP endpoints for one process."""
    if os.name == "nt":
        # Get-NetTCPConnection and Get-NetUDPEndpoint read the kernel's tables directly.
        # netstat would work too and is slower and harder to parse reliably.
        script = (
            f"$t = @(Get-NetTCPConnection -State Listen -OwningProcess {pid} "
            f"-ErrorAction SilentlyContinue).Count; "
            f"$u = @(Get-NetUDPEndpoint -OwningProcess {pid} "
            f"-ErrorAction SilentlyContinue).Count; "
            f"Write-Output \"$t $u\""
        )
        out = subprocess.run(
            ["powershell", "-NoProfile", "-NonInteractive", "-Command", script],
            capture_output=True, text=True, timeout=60,
        ).stdout.strip()
        parts = out.split()
        return (int(parts[0]), int(parts[1])) if len(parts) == 2 else (0, 0)

    if sys.platform == "darwin":
        # ss is iproute2 and does not exist here. lsof reads the same kernel file table
        # the process itself is described by, so the census stays independent of it.
        return lsof_count(pid, ["-iTCP", "-sTCP:LISTEN"]), lsof_count(pid, ["-iUDP"])

    # Linux: count sockets in the listening state owned by this pid. Nothing catches a
    # missing ss, because a machine without the counting tool has produced no census and
    # a returned zero would be indistinguishable from a measured one.
    tcp = udp = 0
    out = subprocess.run(
        ["ss", "-lntupH"], capture_output=True, text=True, timeout=60
    ).stdout
    for line in out.splitlines():
        if f"pid={pid}," not in line:
            continue
        if line.startswith("tcp"):
            tcp += 1
        elif line.startswith("udp"):
            udp += 1
    return tcp, udp


def census(server_bin: Path, port: int, workers: int, detect: bool) -> dict:
    args = [
        str(server_bin), "--port", str(port), "--workers", str(workers),
        "--max-requests", "0",
    ]
    if not detect:
        args.append("--no-detect")

    # When the server fails to bind, its own output is the only diagnosis there is, so it
    # goes to a file rather than to DEVNULL. A pipe would deadlock once its buffer filled,
    # since nothing reads it while the server runs.
    log = tempfile.TemporaryFile()
    proc = subprocess.Popen(args, stdout=log, stderr=subprocess.STDOUT)
    try:
        if not wait_until_listening(proc, port):
            log.seek(0)
            output = log.read().decode("utf-8", "replace").strip() or "(no output)"
            if proc.poll() is not None:
                raise RuntimeError(
                    f"server with {workers} workers exited with code {proc.returncode} "
                    f"before listening on port {port}:\n{output}"
                )
            raise RuntimeError(
                f"server with {workers} workers never answered on port {port}. On macOS "
                f"an unanswered firewall prompt on binding INADDR_ANY leaves the process "
                f"alive and unreachable exactly like this:\n{output}"
            )
        # A moment after the port answers, so every worker has had time to bind. Counting
        # too early would report the first descriptor and call it the total.
        time.sleep(1.0)
        tcp, udp = count_listeners(proc.pid)
        return {
            "workers": workers,
            "protocol_detection": int(detect),
            "tcp_listeners": tcp,
            "udp_listeners": udp,
            "total": tcp + udp,
        }
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()
        log.close()
        # Windows keeps the port in TIME_WAIT briefly; a fresh port per run would work
        # too but would make the table harder to reproduce by hand.
        time.sleep(0.5)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("build", type=Path)
    ap.add_argument("--workers", default="1,2,4,8")
    ap.add_argument("--port", type=int, default=18200)
    # Windows keeps the unsuffixed name the committed census and sec:census already use.
    # Every other platform writes beside it, so a second run cannot overwrite the first.
    default_out = Path("doc/thesis/data") / (
        "descriptors.csv" if os.name == "nt" else f"descriptors-{sys.platform}.csv"
    )
    ap.add_argument("--out", type=Path, default=default_out)
    args = ap.parse_args(argv)

    server_bin = (args.build / "examples" / "Samples" / "benchmark_server"
                  / "benchmark_server.exe")
    if not server_bin.exists():
        server_bin = server_bin.with_suffix("")
    if not server_bin.exists():
        print(f"not built: {server_bin}", file=sys.stderr)
        return 2

    rows = []
    for detect in (True, False):
        for workers in [int(w) for w in args.workers.split(",")]:
            row = census(server_bin, args.port, workers, detect)
            rows.append(row)
            print(f"workers={row['workers']:<2} detect={row['protocol_detection']} "
                  f"tcp={row['tcp_listeners']} udp={row['udp_listeners']} "
                  f"total={row['total']}")

    # Every row here answered a connection before it was counted, so it held at least one
    # listening TCP descriptor. A zero is therefore the counting tool failing, never a
    # result, and a table of zeros would compare equal to itself and read as the
    # proposition confirmed. Refuse before that table is written or believed.
    if any(row["tcp_listeners"] == 0 for row in rows):
        raise RuntimeError(
            f"counted zero listening TCP descriptors on {sys.platform} for a server that "
            f"had already answered a connection. The counting command was "
            f"'{counting_command()}'. This is instrumentation failure, not a measurement."
        )

    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open("w", encoding="utf-8", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    print(f"\nwrote {args.out}")

    # The proposition says the count is independent of the protocols served. It says
    # nothing about proportionality to workers, which is what A absorbs. Stated here as
    # something the data can contradict.
    on = {r["workers"]: r["tcp_listeners"] for r in rows if r["protocol_detection"]}
    off = {r["workers"]: r["tcp_listeners"] for r in rows if not r["protocol_detection"]}
    print("\nTCP listeners by worker count:")
    print("  with classification   ", json.dumps(on))
    print("  without classification", json.dumps(off))
    if on == off:
        print("  identical: serving three protocols costs no extra descriptor")
    else:
        print("  NOT identical: the proposition does not hold as stated here")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
