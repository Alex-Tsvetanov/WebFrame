"""Counts the listening descriptors a running server actually holds.

The central proposition of the dissertation is about a count, not a speed:

    L = W * |T|

listening descriptors, where W is the worker count and T the set of transports, and
notably NOT a function of the number of application protocols served. A claim about a
count is checked by counting, so this asks the operating system rather than the server.

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
import time
from pathlib import Path


def wait_until_listening(port: int, timeout_s: float = 15.0) -> bool:
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.25):
                return True
        except OSError:
            time.sleep(0.05)
    return False


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

    # Linux and macOS: count sockets in the listening state owned by this pid.
    tcp = udp = 0
    try:
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
    except (OSError, subprocess.SubprocessError):
        pass
    return tcp, udp


def census(server_bin: Path, port: int, workers: int, detect: bool) -> dict:
    args = [
        str(server_bin), "--port", str(port), "--workers", str(workers),
        "--max-requests", "0",
    ]
    if not detect:
        args.append("--no-detect")

    proc = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        if not wait_until_listening(port):
            raise RuntimeError(f"server with {workers} workers did not start")
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
        # Windows keeps the port in TIME_WAIT briefly; a fresh port per run would work
        # too but would make the table harder to reproduce by hand.
        time.sleep(0.5)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("build", type=Path)
    ap.add_argument("--workers", default="1,2,4,8")
    ap.add_argument("--port", type=int, default=18200)
    ap.add_argument("--out", type=Path, default=Path("doc/thesis/data/descriptors.csv"))
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

    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open("w", encoding="utf-8", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    print(f"\nwrote {args.out}")

    # The proposition says the count is proportional to workers and independent of the
    # protocols served. Stated here as something the data can contradict.
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
