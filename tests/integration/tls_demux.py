#!/usr/bin/env python3
"""One descriptor serving TLS and cleartext, asserted rather than measured.

    python tls_demux.py <server-binary> --cert bench.crt --key bench.key [--port N]

The dissertation's central claim is that a single TCP descriptor serves TLS and
cleartext HTTP/1.1, chosen by the first octet. Chapter VI measures what that costs.
Nothing measures whether it is true, and a cost measured for a thing that does not work
is worse than no measurement: the arms would differ in what they served rather than in
how they decided.

So this asserts the claim itself. Three server configurations, and the interesting one
is the third.

    detection on, TLS configured    both transports answer on one port. This is the
                                    claim, and no other server here can do it: nginx
                                    cannot put ssl and non-ssl on one listen directive.

    detection off, TLS configured   the dedicated TLS listener the comparison is against,
                                    which is nginx's `listen 443 ssl`. TLS answers,
                                    cleartext does not.

    detection off, no TLS           the dedicated cleartext listener. The mirror image.

The second and third are what make the A/B a comparison of one factor. If detection off
served both, the arms would differ in nothing and the measurement would be of noise; if
it served neither, they would differ in everything.
"""

import argparse
import os
import socket
import ssl
import subprocess
import sys
import time

HOST = "127.0.0.1"
TIMEOUT = 5.0

GET_CLOSE = b"GET / HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n"


class Failure(Exception):
    pass


def check(name, condition, detail=""):
    status = "PASS" if condition else "FAIL"
    print(f"  [{status}] {name}" + (f"  {detail}" if detail and not condition else ""))
    if not condition:
        raise Failure(name)


def client_context():
    """A client that completes a handshake and verifies nothing.

    The rig's certificate is self-signed, so verification would fail for a reason that
    has nothing to do with what is being tested. The load generator makes the same
    choice for the same reason and both say so.
    """
    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    ctx.check_hostname = False
    ctx.verify_mode = ssl.CERT_NONE
    return ctx


def tls_get(port):
    """A full handshake and one request. Returns the response, or None if refused."""
    try:
        with socket.create_connection((HOST, port), timeout=TIMEOUT) as raw:
            with client_context().wrap_socket(raw) as sock:
                sock.settimeout(TIMEOUT)
                sock.sendall(GET_CLOSE)
                return sock.recv(256), sock.version()
    except (ssl.SSLError, OSError):
        return None, None


def cleartext_get(port):
    try:
        with socket.create_connection((HOST, port), timeout=TIMEOUT) as sock:
            sock.settimeout(TIMEOUT)
            sock.sendall(GET_CLOSE)
            return sock.recv(256)
    except OSError:
        return b""


def start(server, port, extra, wait=15.0):
    # On Linux both backends set SO_REUSEPORT, so a server left behind by a hard-killed
    # run shares the port with this one instead of failing to bind, and the probes below
    # land on whichever the kernel picks. A plain bind refuses that; SO_REUSEADDR keeps
    # TIME_WAIT from counting on POSIX and would permit the sharing on Windows.
    try:
        with socket.socket() as probe:
            if os.name != "nt":
                probe.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            probe.bind(("0.0.0.0", port))
    except OSError as exc:
        raise Failure(f"port {port} is already held ({exc}); a stale server is running")
    proc = subprocess.Popen(
        [server, "--port", str(port), "--workers", "2", "--max-requests", "0", *extra],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    deadline = time.time() + wait
    while time.time() < deadline:
        if proc.poll() is not None:
            out = proc.stdout.read().decode(errors="replace")
            raise Failure(f"server exited early with {' '.join(extra)}:\n{out}")
        try:
            socket.create_connection((HOST, port), timeout=0.25).close()
            return proc
        except OSError:
            time.sleep(0.05)
    proc.kill()
    raise Failure(f"server never listened with {' '.join(extra)}")


def stop(proc):
    proc.kill()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        pass


def test_unified(server, port, cert, key):
    """Both transports on one descriptor. The claim."""
    proc = start(server, port, ["--tls", cert, key])
    try:
        body, version = tls_get(port)
        check("TLS is served", bool(body) and body.startswith(b"HTTP/1.1 200"),
              repr(body)[:60] if body else "no response")
        print(f"         negotiated {version}")
        plain = cleartext_get(port)
        check("cleartext is served on the same port",
              plain.startswith(b"HTTP/1.1 200"), repr(plain[:40]))
    finally:
        stop(proc)


def test_dedicated_tls(server, port, cert, key):
    """The control arm: a TLS listener that does not classify."""
    proc = start(server, port, ["--no-detect", "--tls", cert, key])
    try:
        body, _ = tls_get(port)
        check("dedicated TLS listener serves TLS", bool(body) and body.startswith(b"HTTP/1.1 200"),
              repr(body)[:60] if body else "no response")
        plain = cleartext_get(port)
        # A cleartext request reaches a server that is waiting for a ClientHello. It
        # must not be answered: an HTTP response here would mean the arm was still
        # classifying, and the comparison would have no control.
        check("dedicated TLS listener does not answer cleartext",
              not plain.startswith(b"HTTP/"), repr(plain[:40]))
    finally:
        stop(proc)


def test_dedicated_cleartext(server, port):
    """The other control arm, and the one the committed campaigns already used."""
    proc = start(server, port, ["--no-detect"])
    try:
        plain = cleartext_get(port)
        check("dedicated cleartext listener serves HTTP/1.1",
              plain.startswith(b"HTTP/1.1 200"), repr(plain[:40]))
        body, _ = tls_get(port)
        check("dedicated cleartext listener does not complete a handshake",
              body is None, repr(body)[:60] if body else "")
    finally:
        stop(proc)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("server")
    ap.add_argument("--cert", required=True)
    ap.add_argument("--key", required=True)
    ap.add_argument("--port", type=int, default=18120)
    args = ap.parse_args()

    print("one descriptor, both transports:")
    tests = [
        ("unified", lambda: test_unified(args.server, args.port, args.cert, args.key)),
        ("dedicated_tls", lambda: test_dedicated_tls(args.server, args.port + 1,
                                                     args.cert, args.key)),
        ("dedicated_cleartext", lambda: test_dedicated_cleartext(args.server, args.port + 2)),
    ]
    failed = 0
    for name, fn in tests:
        try:
            fn()
        except Failure as exc:
            print(f"  [FAIL] {name}  {exc}")
            failed += 1
        except OSError as exc:
            print(f"  [FAIL] {name}  {exc}")
            failed += 1

    print(f"{len(tests) - failed}/{len(tests)} passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
