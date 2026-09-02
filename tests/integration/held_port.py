"""Whether something already listens on a port.

The same probe as benchmark.adapters.refuse_held_port, which is where the reasoning
lives; that module is the source of truth and this is a copy rather than an import
because ctest runs these scripts by path, so sys.path[0] is this directory and the
repository root is not on it. One copy for both integration tests, not one each: three
versions of one socket rule is how they come to disagree.
"""

import os
import socket


def held_reason(port):
    """The reason the port cannot be used, or None if it is free.

    On Linux both backends set SO_REUSEPORT, so a server left behind by a hard-killed
    run shares the port with this one instead of failing to bind, and the probes land on
    whichever the kernel picks. A plain bind refuses that; SO_REUSEADDR keeps TIME_WAIT
    from counting on POSIX and would permit the sharing on Windows.
    """
    try:
        with socket.socket() as probe:
            if os.name != "nt":
                probe.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            probe.bind(("0.0.0.0", port))
    except OSError as exc:
        return f"port {port} is already held ({exc}); a stale server is running"
    return None
