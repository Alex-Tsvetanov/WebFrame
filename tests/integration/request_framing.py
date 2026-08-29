"""Where does this server think a request ends?

Each case below is a way two parsers can disagree about that, which is the
precondition for a desync between this server and anything in front of it. What is
checked is not that the answer is polite but that there is exactly one of it, and that
the status says which rule was applied.

The four failing cases all used to answer 404 with the declared body silently dropped,
because field names were compared case-sensitively, a repeated Content-Length
overwrote the earlier one in a map, Transfer-Encoding was not read at all, and every
parse failure was reported as 400.

Usage:
    tests/integration/request_framing.py <port>

Expects a server on that port serving GET / and nothing else. The benchmark server
will do: benchmark_server --port <port>
"""

import socket
import sys

CASES = []


def case(name, raw, expect):
    CASES.append((name, raw, expect))


# The body of a smuggled request, used as the payload in the framing cases. If the
# server drops it rather than consuming it, nothing here executes, and if the server
# executes it there are two responses instead of one.
SMUGGLED = b"GET /smuggled HTTP/1.1\r\nHost: x\r\n\r\n"

# Field names are case-insensitive (RFC 9110 section 5.1), and HTTP/2 mandates
# lowercase, so a gateway translating down to HTTP/1.1 emits exactly this. Missing the
# Content-Length means never reading the body it declares.
case("lowercase content-length",
     b"POST / HTTP/1.1\r\nHost: x\r\ncontent-length: " + str(len(SMUGGLED)).encode()
     + b"\r\n\r\n" + SMUGGLED,
     ["404"])

# Two lengths that disagree. RFC 9112 section 6.3 requires rejecting the message
# rather than picking one, because whichever one is picked, something else may pick
# the other.
case("conflicting content-length",
     b"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 0\r\nContent-Length: "
     + str(len(SMUGGLED)).encode() + b"\r\n\r\n" + SMUGGLED,
     ["400"])

# Chunked framing, which the server decodes. The terminating chunk ends the body, so
# what follows is not part of this request and must not become another one. The
# connection is closed after a chunked request precisely so it cannot.
case("transfer-encoding chunked",
     b"POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n" + SMUGGLED,
     ["404"])

# A coding this server cannot apply. RFC 9112 section 6.1 says answer 501 rather than
# ignore it, and picking out the part of a list it recognises would be worse than both.
case("unsupported transfer coding",
     b"POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: gzip, chunked\r\n\r\n0\r\n\r\n",
     ["501"])

# Both framings at once, which two parties may resolve differently.
case("both framings at once",
     b"POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\nContent-Length: 5\r\n\r\n0\r\n\r\n",
     ["400"])

# A body larger than the server accepts, rejected on the header before anything is
# read or allocated.
case("oversized content-length",
     b"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 99999999\r\n\r\n",
     ["413"])

# And a request that was always framed correctly, so a fix that answers everything
# with an error would not pass.
case("ordinary request",
     b"GET / HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n",
     ["200"])


def statuses_for(port, raw):
    s = socket.create_connection(("127.0.0.1", port), timeout=5.0)
    s.sendall(raw)
    received = b""
    try:
        while True:
            chunk = s.recv(65536)
            if not chunk:
                break
            received += chunk
    except (socket.timeout, ConnectionResetError):
        pass
    finally:
        s.close()
    return [line.split(b" ")[1].decode() for line in received.split(b"\r\n")
            if line.startswith(b"HTTP/1.1 ")]


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2

    port = int(sys.argv[1])
    failures = 0

    for name, raw, expect in CASES:
        got = statuses_for(port, raw)
        ok = got == expect
        failures += 0 if ok else 1
        print(f"{'PASS' if ok else 'FAIL'}  {name:28s} expected={expect} got={got}")

    print(f"\n{len(CASES) - failures} of {len(CASES)} passed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
