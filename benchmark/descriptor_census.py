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
import re
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

from benchmark.adapters import refuse_held_port
from benchmark.harness import environment

REPO = Path(__file__).resolve().parents[1]


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


def wait_until_bound(proc: subprocess.Popen, timeout_s: float = 15.0) -> bool:
    """Waits for the listening descriptor to appear, without connecting to it.

    Exists because connecting is not free. The connection the old readiness check made
    was itself classified, armed a Deadline, and started TimerQueue's thread
    (timer_queue.hpp:24), which never stops. So every row counted afterwards held one
    thread the server would not otherwise have, and the rows labelled "no connections"
    were really "one connection ago". Measured on Windows: 5 threads before the probe,
    6 after, and 6 for ever, including after the connection closed.

    Asking the kernel which descriptors the process holds perturbs nothing, which is the
    same reason the census asks the kernel rather than the server in the first place.
    """
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            return False
        tcp, _ = count_listeners(proc.pid)
        if tcp:
            return True
        time.sleep(0.1)
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


def lsof_types(pid: int) -> list[str]:
    """Every open file's type letter for one process, from lsof field mode."""
    proc = subprocess.run(
        ["lsof", "-nP", "-w", "-p", str(pid), "-Ft"],
        capture_output=True, text=True, timeout=60,
    )
    quiet_empty_match = proc.returncode == 1 and not proc.stderr.strip()
    if proc.returncode != 0 and not quiet_empty_match:
        raise RuntimeError(f"lsof -Ft exited {proc.returncode} for pid {pid}: "
                           f"{proc.stderr.strip()}")
    return [line[1:] for line in proc.stdout.splitlines() if line.startswith("t")]


def count_event_ports(pid: int) -> int | None:
    """Event ports the process holds: kqueue descriptors, epoll fds, io_uring rings.

    The companion count to the listening descriptors. A readiness or completion backend
    needs somewhere to wait, and how many of those it needs is a property of the accept
    model rather than of the protocols served: kqueue and IOCP share one, io_uring holds
    one ring per worker.

    Returns None, never 0, where the platform cannot be asked. A zero here would read as
    "measured none" and it would be wrong in the direction that flatters the claim.
    """
    if sys.platform == "darwin":
        return sum(1 for kind in lsof_types(pid) if kind == "KQUEUE")
    if sys.platform.startswith("linux"):
        fd_dir = Path("/proc") / str(pid) / "fd"
        try:
            targets = [os.readlink(str(fd_dir / name)) for name in os.listdir(fd_dir)]
        except OSError:
            return None
        return sum(1 for target in targets
                   if "eventpoll" in target or "io_uring" in target)
    # Windows completion ports are kernel handles with no supported enumeration short of
    # NtQuerySystemInformation. Unavailable is reported as unavailable.
    return None


def count_threads(pid: int) -> int | None:
    """Threads the process holds at rest.

    "At rest" is load bearing. TimerQueue starts its thread lazily on the first scheduled
    callback, and every classified connection arms a Deadline, so this constant is not
    the constant under load.
    """
    if sys.platform.startswith("linux"):
        try:
            status = Path(f"/proc/{pid}/status").read_text(encoding="utf-8")
        except OSError:
            return None
        match = re.search(r"^Threads:\s+(\d+)$", status, re.MULTILINE)
        return int(match.group(1)) if match else None
    if sys.platform == "darwin":
        out = subprocess.run(["ps", "-M", "-p", str(pid)],
                             capture_output=True, text=True, timeout=60).stdout
        lines = [line for line in out.splitlines() if line.strip()]
        return max(len(lines) - 1, 0) or None
    if os.name == "nt":
        out = subprocess.run(
            ["powershell", "-NoProfile", "-NonInteractive", "-Command",
             f"@(Get-Process -Id {pid} -ErrorAction SilentlyContinue).Threads.Count"],
            capture_output=True, text=True, timeout=60).stdout.strip()
        return int(out) if out.isdigit() else None
    return None


def count_established(pid: int) -> int | None:
    """Established TCP connections the process holds.

    The per-connection half of the claim. If a demultiplexing wrapper cost a descriptor
    the slope of this against offered connections would exceed one.
    """
    if sys.platform == "darwin":
        return lsof_count(pid, ["-iTCP", "-sTCP:ESTABLISHED"])
    if sys.platform.startswith("linux"):
        out = subprocess.run(["ss", "-tnpH", "state", "established"],
                             capture_output=True, text=True, timeout=60).stdout
        return sum(1 for line in out.splitlines() if f"pid={pid}," in line)
    if os.name == "nt":
        out = subprocess.run(
            ["powershell", "-NoProfile", "-NonInteractive", "-Command",
             f"@(Get-NetTCPConnection -State Established -OwningProcess {pid} "
             f"-ErrorAction SilentlyContinue).Count"],
            capture_output=True, text=True, timeout=60).stdout.strip()
        return int(out) if out.isdigit() else None
    return None


def census(server_bin: Path, port: int, workers: int, detect: bool,
           tls: tuple[Path, Path] | None = None, connections: int = 0) -> dict:
    args = [
        str(server_bin), "--port", str(port), "--workers", str(workers),
        "--max-requests", "0",
    ]
    if not detect:
        args.append("--no-detect")
    if tls is not None:
        args += ["--tls", str(tls[0]), str(tls[1])]
    if connections:
        # Silent parked connections are the point: they measure what a connection costs
        # in descriptors before it has said anything, which is where a demultiplexing
        # wrapper would show up if it cost one.
        #
        # Both deadlines have to be off. The handshake deadline reaps a connection that
        # has sent no first octet, so with the default a parked connection is gone
        # before it is counted and the slope comes out below one. That is a control, not
        # a detail: it would have quietly produced the answer the claim wants.
        args += ["--handshake-ms", "0", "--keep-alive-ms", "0"]

    # When the server fails to bind, its own output is the only diagnosis there is, so it
    # goes to a file rather than to DEVNULL. A pipe would deadlock once its buffer filled,
    # since nothing reads it while the server runs.
    # On Linux a stale server would share the port under SO_REUSEPORT and be counted
    # under this row's factors; refused before anything is started.
    refuse_held_port(port)
    log = tempfile.TemporaryFile()
    proc = subprocess.Popen(args, stdout=log, stderr=subprocess.STDOUT)
    def fail(what: str) -> RuntimeError:
        log.seek(0)
        output = log.read().decode("utf-8", "replace").strip() or "(no output)"
        if proc.poll() is not None:
            return RuntimeError(
                f"server with {workers} workers exited with code {proc.returncode} "
                f"before {what}:\n{output}"
            )
        return RuntimeError(
            f"server with {workers} workers never {what}. On macOS an unanswered "
            f"firewall prompt on binding INADDR_ANY leaves the process alive and "
            f"unreachable exactly like this:\n{output}"
        )

    try:
        # Two-stage readiness, and the order is the whole point. Wait for the
        # descriptor to appear by asking the kernel, which perturbs nothing, and take
        # the at-rest thread count there. Only then connect, which validates that the
        # server really answers and is what makes a later count of zero
        # instrumentation failure rather than a result.
        #
        # The connection is not free and cannot be taken back. It is classified, arms
        # a Deadline, and starts TimerQueue's thread (timer_queue.hpp:24), which never
        # stops. Before this split, every row was counted after that had happened, so
        # the rows labelled "no connections" were really "one connection ago".
        if not wait_until_bound(proc):
            raise fail(f"bound a listening descriptor on port {port}")
        time.sleep(1.0)
        threads_at_rest = count_threads(proc.pid)

        if not wait_until_listening(proc, port):
            raise fail(f"answered on port {port}")
        # A moment after the port answers, so every worker has had time to bind.
        # Counting too early would report the first descriptor and call it the total.
        time.sleep(1.0)

        held = []
        try:
            for _ in range(connections):
                sock = socket.create_connection(("127.0.0.1", port), timeout=5.0)
                held.append(sock)
            if connections:
                # After the connections, so they have been accepted and are counted, and
                # long enough that a reaping deadline would have fired if one were armed.
                time.sleep(1.0)
            tcp, udp = count_listeners(proc.pid)
            row = {
                "workers": workers,
                "protocol_detection": int(detect),
                "tls": int(tls is not None),
                "connections_offered": connections,
                # Recorded because the per-connection rows vary it and the at-rest rows
                # do not, which confounds the thread column between the two groups.
                # TimerQueue starts its thread on first use (timer_queue.hpp:24), and
                # with both deadlines at zero nothing ever arms one, so a server with
                # connections held reports one thread FEWER than the same server at
                # rest. That is the timer thread, not a connection effect. Comparing
                # threads across the two groups would read it backwards.
                "deadlines": 0 if connections else 1,
                "tcp_listeners": tcp,
                "udp_listeners": udp,
                "total": tcp + udp,
                "event_ports": count_event_ports(proc.pid),
                # Two readings, because they are two server states and the difference
                # between them is one thread that a single connection starts for ever.
                "threads_at_rest": threads_at_rest,
                "threads": count_threads(proc.pid),
                "established": count_established(proc.pid),
            }
        finally:
            for sock in held:
                sock.close()
        return row
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
    ap.add_argument("--cert", type=Path, help="server certificate; adds the TLS arm")
    ap.add_argument("--key", type=Path, help="server private key; adds the TLS arm")
    ap.add_argument("--connections", default="",
                    help="comma separated connection counts for the per-connection census, "
                         "e.g. 0,1,10,50. Loopback, and a count rather than a timing.")
    args = ap.parse_args(argv)

    # The census writes a table asserting what a build holds. Until now it wrote no
    # provenance at all: no machine, no commit, no dirty flag, so a CSV could not say
    # which binary produced it. Every campaign entry point in this harness records that;
    # this one is a measurement too.
    env = environment.capture(repo=REPO, build_type="Release",
                              io_backend=environment.resolve_io_backend(args.build))
    if env["build"]["git_dirty"]:
        print("working tree is dirty; the recorded commit would not describe the binary "
              "this census counted", file=sys.stderr)
        return 2
    if env.get("virtualisation"):
        print(f"virtualisation detected ({env['virtualisation']}); a descriptor count "
              f"from a guest describes the guest", file=sys.stderr)
        return 2

    if bool(args.cert) != bool(args.key):
        print("--cert and --key go together", file=sys.stderr)
        return 2
    tls_pair = (args.cert, args.key) if args.cert else None
    if tls_pair and not (args.cert.exists() and args.key.exists()):
        # A server started without its certificate answers every request in cleartext
        # while the record says tls=1, which is a full set of plausible numbers
        # describing the wrong thing.
        print(f"certificate or key missing: {args.cert}, {args.key}", file=sys.stderr)
        return 2

    server_bin = (args.build / "examples" / "Samples" / "benchmark_server"
                  / "benchmark_server.exe")
    if not server_bin.exists():
        server_bin = server_bin.with_suffix("")
    if not server_bin.exists():
        print(f"not built: {server_bin}", file=sys.stderr)
        return 2

    worker_list = [int(w) for w in args.workers.split(",")]
    conn_list = [int(c) for c in args.connections.split(",") if c.strip()]
    tls_options = [None, tls_pair] if tls_pair else [None]

    def show(row: dict) -> None:
        print(f"workers={row['workers']:<2} detect={row['protocol_detection']} "
              f"tls={row['tls']} conns={row['connections_offered']:<3} "
              f"tcp={row['tcp_listeners']} udp={row['udp_listeners']} "
              f"kq={row['event_ports']} thr={row['threads']} "
              f"est={row['established']}")

    rows = []
    # Listener census. The transport set is fixed at one, TCP, in every arm here: TLS and
    # cleartext are both TCP, so this varies the PROTOCOL set and not |T|. Writing it up
    # as |T|=2 would be a different and unsupported claim.
    for detect in (True, False):
        for tls in tls_options:
            for workers in worker_list:
                row = census(server_bin, args.port, workers, detect, tls)
                rows.append(row)
                show(row)

    # Per-connection census, if asked for. One worker, because the question is what a
    # connection costs and not how connections are distributed.
    for detect, tls in [(True, None), (False, None)] + ([(False, tls_pair)] if tls_pair else []):
        for count in conn_list:
            row = census(server_bin, args.port, 1, detect, tls, connections=count)
            rows.append(row)
            show(row)

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

    # The control the per-connection census turns on. A connection that has sent no
    # first octet is reaped by the handshake deadline, so if that deadline were armed
    # the parked connections would be gone before they were counted and the slope would
    # come out below one, which is the answer the claim would like. Refuse instead.
    for row in rows:
        offered, seen = row["connections_offered"], row["established"]
        if offered and seen is not None and seen != offered:
            raise RuntimeError(
                f"offered {offered} connections and the server held {seen}. Either the "
                f"handshake deadline reaped the parked ones or the counter is wrong. "
                f"Either way the per-connection slope this would produce is not a "
                f"measurement."
            )

    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open("w", encoding="utf-8", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    manifest = args.out.with_suffix(".env.json")
    manifest.write_text(json.dumps(
        {"environment": env, "fingerprint": environment.fingerprint(env),
         "argv": sys.argv[1:], "counting_command": counting_command()},
        indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"\nwrote {args.out}")
    print(f"wrote {manifest}")

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
