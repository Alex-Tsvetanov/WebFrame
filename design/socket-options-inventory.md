# Accepted- and listening-socket options, by backend

Preparation for a shared `configure_accepted_socket`. Inventory only: this document
records what each backend does today and where a shared helper would be called from. It
does not propose which option set becomes the shared policy, because that decision changes
what every cross-arm cell measures.

Why it matters: the epoll and io_uring arms of the Linux comparison do not configure their
accepted sockets identically, so every cross-arm number is partly a comparison of socket
options rather than of I/O mechanisms. The same is true across platforms, more starkly.

## 1. Accepted sockets

| Option | io_uring multi-accept | io_uring `async_accept` | epoll (both paths) | kqueue | IOCP |
|---|---|---|---|---|---|
| `TCP_NODELAY` | yes, `uring_context.cpp:981` | **no** | yes, `epoll_context.cpp:508` | **no** | **no** |
| `TCP_QUICKACK` | yes, `uring_context.cpp:983` | **no** | **no** | n/a | n/a |
| `SO_SNDBUF` 256 KB | yes, `uring_context.cpp:987` | **no** | **no** | **no** | **no** |
| `SO_NOSIGPIPE` | n/a | n/a | n/a | yes, `kqueue_context.cpp:319` | n/a |
| `SO_UPDATE_ACCEPT_CONTEXT` | n/a | n/a | n/a | n/a | yes, `iocp_context.cpp:549` |

Three separate problems, in descending order of how much they affect published numbers.

**a. epoll against io_uring.** io_uring's multi-accept path sets `TCP_QUICKACK` and a
256 KB send buffer that epoll does not set at all. This is the pair used for every Linux
cross-arm cell, including the syscall tables and the latency ladders.

**b. io_uring against itself.** `set_tcp_opts` is called from exactly one place,
`uring_context.cpp:1023`, in the multi-accept loop. The `async_accept` path at
`uring_context.cpp:792` constructs a `UringConnection` without it, so a connection accepted
that way gets none of the three options. epoll does not have this problem, and the reason
is structural rather than deliberate: epoll sets `TCP_NODELAY` in the `EpollConnection`
constructor, which every accept path must go through, while io_uring sets its options in
one accept loop. The benchmark server uses multi-accept, so published Linux numbers are
from the configured path, but the two io_uring paths are not equivalent.

**c. kqueue and IOCP set no `TCP_NODELAY` at all.** `KqueueConnection` and
`IocpConnection` constructors set no options; `SO_NOSIGPIPE` and
`SO_UPDATE_ACCEPT_CONTEXT` are platform obligations rather than tuning. So the macOS and
Windows arms run with Nagle enabled while Linux runs with it disabled, and any
cross-platform latency comparison is partly a comparison of Nagle. Given the measured cost
of a much smaller asymmetry on Linux, this one should be assumed to matter until measured.

## 2. Listening sockets

| Option | io_uring multi | io_uring single | epoll | kqueue | IOCP |
|---|---|---|---|---|---|
| `SO_REUSEADDR` | `:212` | `:643` | `:428` | `:407` | **no** |
| `SO_REUSEPORT` | `:212-213` | **no** | `:429`, if `reuse_port_` | **no** | n/a |
| `IPV6_V6ONLY` | n/a | n/a | n/a | n/a | `:305` |
| `listen(backlog)` | `:227` | `:659` | `:448` | `:421` | `:346` |

`TCP_DEFER_ACCEPT`, `SO_LINGER`, `SO_RCVBUF` and `TCP_FASTOPEN` appear nowhere in the net
layer. backlog is a parameter everywhere and is not a per-backend difference.

Note that kqueue sets no `SO_REUSEPORT`, so its multi-accept, if it has one, cannot use the
same kernel-side load spreading the Linux backends rely on. Out of scope here, recorded
because it is the same class of unexamined per-backend difference.

## 3. Where a shared helper would live

There is no existing native-socket-handle type in `include/coroute/net/`, so one is needed:
POSIX backends hold `int`, IOCP holds `SOCKET`.

Proposed seam, matching how `src/net/io_context.cpp` already centralises backend choice:

    // include/coroute/net/socket_options.hpp
    namespace coroute::net
    {
        using NativeSocket = /* int on POSIX, SOCKET on Windows */;

        // Applies the shared accepted-socket policy. Called once per accepted
        // connection, before the connection object is handed to a handler.
        void configure_accepted_socket(NativeSocket fd);
    }

Implemented in a new `src/net/socket_options.cpp`, so the policy lives in one translation
unit and each backend keeps only its platform obligations.

Call sites, being the point in each backend where an accepted descriptor first becomes a
connection:

| Backend | Call site | Currently |
|---|---|---|
| io_uring, multi-accept | `uring_context.cpp:1023`, before `:1026` | `set_tcp_opts(op.result)` — replace |
| io_uring, `async_accept` | `uring_context.cpp:792`, before constructing | nothing — add |
| epoll | `epoll_context.cpp:507`, in `EpollConnection` ctor | `TCP_NODELAY` only — replace |
| kqueue | `kqueue_context.cpp:580`, before constructing | nothing — add, keep `SO_NOSIGPIPE` at `:319` |
| IOCP | `iocp_context.cpp:552`, before constructing | nothing — add, keep `SO_UPDATE_ACCEPT_CONTEXT` at `:549` |

epoll's constructor is the more robust shape and is worth preserving as the pattern: an
option set applied in the connection constructor cannot be missed by a new accept path,
which is exactly how io_uring's `async_accept` came to differ from its own multi-accept.

## 4. The awake control, reproduced from this text

The measurement that established CPU idle state as the mediator, recorded so it can be
repeated without reference to a script:

Server and generator as usual, both unprivileged, in the `srv` and `gen` network
namespaces over the 10.77.0.0/30 veth pair. Server:
`benchmark_server --port P --workers 4 --io-backend epoll --max-requests 0`. Generator:
`loadgen --host 10.77.0.1 --port P --connections 4 --threads 2 --duration 20 --warmup 2
--rate 100`.

The control is 16 spinners, one per logical core, started after the server is listening and
before the generator, each:

    nice -n 19 bash -c 'while :; do :; done' &

16 because the machine has 16 logical cores and the aim is that no core reaches an idle
state. `nice -n 19` so the control keeps the cores out of idle without competing for them:
the server runs at the default priority and preempts a nice-19 spinner immediately, which
is what makes the comparison a test of idleness rather than of CPU contention. Spinners are
killed by recorded PID after the generator exits, not by pattern, since a `pkill -f`
pattern matches the shell issuing it.

That the server still preempted them is visible in the result rather than assumed: with the
spinners running, the median fell from 495 µs to 74 µs, which a server starved of CPU could
not do. The cost of the control shows in the tail, p999 1748 µs and max 2756 µs against 940
and 950 without it; that is preemption against the spinners and is a property of the
control, not of the server.

Result, 100 rps, identical cells:

| arm | min | p50 | p75 | p90 | p99 | p999 | max |
|---|---|---|---|---|---|---|---|
| epoll, cores idle | 36 | 495 | 547 | 721 | 907 | 940 | 950 |
| epoll, cores awake | 54 | **74** | 78 | 85 | 102 | 1748 | 2756 |
| io_uring, cores idle | 20 | 25 | 28 | 33 | 45 | 56 | 67 |

The claim this supports is that the core was idle. Whether the recovered time is C-state
exit latency or frequency ramp is not separated, because separating them means writing to
`cpuidle` or `cpufreq`, which is a system change this rig does not make.
