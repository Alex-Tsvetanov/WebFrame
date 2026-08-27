#!/usr/bin/env python3
"""Protocol demultiplexing over a single listening socket.

Drives a real server with raw sockets, because the thing under test is what the
server does with the first octets of a connection and no unit test can reach that.
Nothing in the Catch2 suite covers src/core/app.cpp at all.

Usage:
    python protocol_demux.py <path-to-server-binary> [--port N]

The server binary must accept --port and --workers, which benchmark_server does.
"""

import argparse
import socket
import subprocess
import sys
import time

PREFACE = b"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
# SETTINGS frame: length 0, type 0x04, flags 0, stream id 0.
SETTINGS = bytes([0, 0, 0, 4, 0, 0, 0, 0, 0])

HOST = "127.0.0.1"
TIMEOUT = 4.0


class Failure(Exception):
    pass


def connect(port):
    sock = socket.create_connection((HOST, port), timeout=TIMEOUT)
    sock.settimeout(TIMEOUT)
    return sock


def recv(sock, size=256):
    try:
        return sock.recv(size)
    except (socket.timeout, ConnectionResetError):
        return b""


def is_settings_frame(data):
    # 9 byte frame header, type is the fourth octet.
    return len(data) >= 9 and data[3] == 0x04


def check(name, condition, detail=""):
    status = "PASS" if condition else "FAIL"
    print(f"  [{status}] {name}" + (f"  {detail}" if detail and not condition else ""))
    if not condition:
        raise Failure(name)


def test_http11(port):
    with connect(port) as sock:
        sock.sendall(b"GET / HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")
        data = recv(sock)
        check("HTTP/1.1 request is served", data.startswith(b"HTTP/1.1 200"), repr(data[:40]))


def test_prior_knowledge_h2(port):
    # Before the unified listener this path was unreachable: is_http2_preface existed
    # but nothing on the accept path ever called it, so a prior-knowledge client got
    # an HTTP/1.1 400 instead of a SETTINGS frame.
    with connect(port) as sock:
        sock.sendall(PREFACE + SETTINGS)
        data = recv(sock)
        check("prior-knowledge HTTP/2 gets a SETTINGS frame", is_settings_frame(data), repr(data[:40]))


def test_preface_dribbled(port):
    # The classic first-octet classifier bug: read once, see three bytes, decide.
    with connect(port) as sock:
        for byte in PREFACE:
            sock.sendall(bytes([byte]))
        sock.sendall(SETTINGS)
        data = recv(sock)
        check("preface split one byte per segment still detected", is_settings_frame(data), repr(data[:40]))


def test_request_line_split(port):
    # The request line arrives across segments straddling the classification
    # boundary. If the replayed bytes are dropped the server sees a truncated line.
    with connect(port) as sock:
        for chunk in (b"GE", b"T / HT", b"TP/1.1\r\nHost: x\r\n", b"Connection: close\r\n\r\n"):
            sock.sendall(chunk)
            time.sleep(0.01)
        data = recv(sock)
        check("request line split across segments is reassembled", data.startswith(b"HTTP/1.1 200"), repr(data[:40]))


def test_junk_rejected(port):
    with connect(port) as sock:
        sock.sendall(bytes([0x00, 0xFF, 0xFE, 0x01]))
        data = recv(sock)
        check("binary junk is dropped, not answered", data == b"", repr(data[:40]))


def test_lowercase_method_rejected(port):
    # HTTP method tokens are uppercase. A lowercase verb is not a valid request and
    # must not be classified as cleartext.
    with connect(port) as sock:
        sock.sendall(b"get / HTTP/1.1\r\n\r\n")
        data = recv(sock)
        check("lowercase method is rejected", data == b"", repr(data[:40]))


def test_tls_byte(port, tls_enabled):
    with connect(port) as sock:
        sock.sendall(bytes([0x16, 0x03, 0x01, 0x00, 0x2A]))
        data = recv(sock)
        if tls_enabled:
            # A truncated ClientHello: the server may alert or close, but must never
            # answer in cleartext.
            check("TLS byte is not answered in cleartext", not data.startswith(b"HTTP/"), repr(data[:40]))
        else:
            check("TLS byte with TLS disabled closes the connection", data == b"", repr(data[:40]))


def test_silent_client(port):
    # Connect and say nothing. The server must not answer, and must not wedge the
    # worker: the following request has to still be served.
    quiet = connect(port)
    try:
        test_http11(port)
        check("a silent connection does not block other clients", True)
    finally:
        quiet.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("server", help="path to a server binary accepting --port/--workers")
    parser.add_argument("--port", type=int, default=18080)
    parser.add_argument("--tls", action="store_true", help="server was built with TLS enabled")
    args = parser.parse_args()

    proc = subprocess.Popen(
        [args.server, "--port", str(args.port), "--workers", "2"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

    # Wait for the port rather than sleeping a fixed amount.
    deadline = time.time() + 10
    while time.time() < deadline:
        if proc.poll() is not None:
            out = proc.stdout.read().decode(errors="replace")
            print(f"server exited early:\n{out}", file=sys.stderr)
            return 1
        try:
            connect(args.port).close()
            break
        except OSError:
            time.sleep(0.1)
    else:
        proc.kill()
        print("server never accepted a connection", file=sys.stderr)
        return 1

    print("protocol demultiplexing over one listening socket:")
    failed = 0
    tests = [
        ("http11", lambda: test_http11(args.port)),
        ("prior_knowledge_h2", lambda: test_prior_knowledge_h2(args.port)),
        ("preface_dribbled", lambda: test_preface_dribbled(args.port)),
        ("request_line_split", lambda: test_request_line_split(args.port)),
        ("junk_rejected", lambda: test_junk_rejected(args.port)),
        ("lowercase_method", lambda: test_lowercase_method_rejected(args.port)),
        ("tls_byte", lambda: test_tls_byte(args.port, args.tls)),
        ("silent_client", lambda: test_silent_client(args.port)),
    ]

    try:
        for name, fn in tests:
            try:
                fn()
            except Failure:
                failed += 1
            except OSError as exc:
                print(f"  [FAIL] {name}  {exc}")
                failed += 1
    finally:
        proc.kill()
        proc.wait(timeout=5)

    print(f"{len(tests) - failed}/{len(tests)} passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
