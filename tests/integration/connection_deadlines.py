"""Do the two connection deadlines actually fire, and do they leave live traffic alone?

Chapter III states measured times for both. This is where those numbers come from, so
they are reproducible rather than remembered.

Two windows, and they close different things:

  handshake   from accept until the protocol is known. A peer that connects and says
              nothing parks a coroutine here, and nothing else in the stack would ever
              wake it: Connection::set_timeout is stored by all four backends and
              enforced by none.

  keep-alive  an established HTTP/1.1 connection that has gone quiet between requests.

The third case is the one that would catch an over-eager fix: a connection that is being
used must not be closed, so the check serves several requests down one connection and
requires all of them to succeed.

Usage:
    tests/integration/connection_deadlines.py <build-dir> [port]

It starts and stops the server itself, because the deadlines are per-run configuration
and a server left over from another test would have the defaults.
"""

import json
import socket
import subprocess
import sys
import time
from pathlib import Path

HANDSHAKE_MS = 1000
KEEP_ALIVE_MS = 1500
# Wide enough that a slow machine does not fail the check, narrow enough that a deadline
# which never fires cannot pass it.
TOLERANCE_S = 1.5


def wait_until_listening(port, timeout_s=15.0):
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.25):
                return True
        except OSError:
            time.sleep(0.05)
    return False


def silent_peer(port, wait_s):
    """Connect and say nothing. Returns seconds until the server gave up, or None."""
    s = socket.create_connection(("127.0.0.1", port), timeout=wait_s + 1.0)
    s.settimeout(wait_s)
    started = time.monotonic()
    try:
        data = s.recv(1)
    except socket.timeout:
        return None
    except ConnectionResetError:
        # close() on a socket with a read still pending sends RST rather than FIN. Still
        # a close, and still the deadline firing.
        return time.monotonic() - started
    finally:
        s.close()
    return time.monotonic() - started if data == b"" else None


def idle_after_request(port, wait_s):
    """Complete one request, then go quiet. Returns seconds until close, or None."""
    s = socket.create_connection(("127.0.0.1", port), timeout=5.0)
    s.sendall(b"GET / HTTP/1.1\r\nHost: x\r\nConnection: keep-alive\r\n\r\n")
    reply = s.recv(4096)
    if b" 200 " not in reply.split(b"\r\n", 1)[0]:
        s.close()
        raise AssertionError(f"expected 200 for the first request, got {reply[:60]!r}")

    s.settimeout(wait_s)
    started = time.monotonic()
    try:
        s.recv(1)
    except socket.timeout:
        return None
    except ConnectionResetError:
        return time.monotonic() - started
    finally:
        s.close()
    return time.monotonic() - started


def requests_on_one_connection(port, count):
    """A connection in use must not be closed by either deadline."""
    s = socket.create_connection(("127.0.0.1", port), timeout=5.0)
    served = 0
    try:
        for _ in range(count):
            s.sendall(b"GET / HTTP/1.1\r\nHost: x\r\nConnection: keep-alive\r\n\r\n")
            reply = s.recv(4096)
            if b" 200 " in reply.split(b"\r\n", 1)[0]:
                served += 1
            else:
                break
    finally:
        s.close()
    return served


def run(server_bin, port):
    results = {}
    failures = []

    def check(name, ok, detail):
        results[name] = {"pass": bool(ok), "detail": detail}
        print(f"{'PASS' if ok else 'FAIL'}  {name:34s} {detail}")
        if not ok:
            failures.append(name)

    # --- both deadlines short, so the run takes seconds rather than a minute ---
    proc = subprocess.Popen(
        [str(server_bin), "--port", str(port), "--workers", "2",
         "--handshake-ms", str(HANDSHAKE_MS), "--keep-alive-ms", str(KEEP_ALIVE_MS)],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
    )
    try:
        if not wait_until_listening(port):
            print("server did not start", file=sys.stderr)
            return 2, results

        t = silent_peer(port, wait_s=HANDSHAKE_MS / 1000.0 + TOLERANCE_S + 2)
        check("handshake deadline fires",
              t is not None and abs(t - HANDSHAKE_MS / 1000.0) < TOLERANCE_S,
              f"closed after {t:.2f}s, expected about {HANDSHAKE_MS/1000:.2f}s"
              if t is not None else "never closed")

        t = idle_after_request(port, wait_s=KEEP_ALIVE_MS / 1000.0 + TOLERANCE_S + 2)
        check("keep-alive deadline fires",
              t is not None and abs(t - KEEP_ALIVE_MS / 1000.0) < TOLERANCE_S,
              f"closed after {t:.2f}s, expected about {KEEP_ALIVE_MS/1000:.2f}s"
              if t is not None else "never closed")

        served = requests_on_one_connection(port, 3)
        check("a connection in use is not closed", served == 3,
              f"{served} of 3 requests served on one connection")
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()

    # --- both deadlines off, to show the closes above come from them and not from
    #     something else that happens to close connections ---
    proc = subprocess.Popen(
        [str(server_bin), "--port", str(port + 1), "--workers", "2",
         "--handshake-ms", "0", "--keep-alive-ms", "0"],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
    )
    try:
        if not wait_until_listening(port + 1):
            print("server did not start with deadlines disabled", file=sys.stderr)
            return 2, results

        t = silent_peer(port + 1, wait_s=4.0)
        check("silent peer survives with the deadline off", t is None,
              "still open after 4.0s" if t is None else f"closed after {t:.2f}s")

        t = idle_after_request(port + 1, wait_s=5.0)
        check("idle peer survives with the deadline off", t is None,
              "still open after 5.0s" if t is None else f"closed after {t:.2f}s")
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()

    return (1 if failures else 0), results


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    build = Path(sys.argv[1])
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 18190

    server_bin = build / "examples" / "Samples" / "benchmark_server" / "benchmark_server.exe"
    if not server_bin.exists():
        server_bin = server_bin.with_suffix("")
    if not server_bin.exists():
        print(f"not built: {server_bin}", file=sys.stderr)
        return 2

    code, results = run(server_bin, port)
    passed = sum(1 for r in results.values() if r["pass"])
    print(f"\n{passed} of {len(results)} passed")

    out = build / "connection_deadlines.json"
    out.write_text(json.dumps(results, indent=2), encoding="utf-8")
    print(f"wrote {out}")
    return code


if __name__ == "__main__":
    raise SystemExit(main())
