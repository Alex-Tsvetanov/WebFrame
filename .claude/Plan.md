# PhD-WebFrame: from library to PhD dissertation

## Context

`D:\GitHub\PhD-WebFrame` holds `coroute`, a C++20 web framework (~18.5k LOC across `include/` + `src/`),
with a Bulgarian **bachelor** diploma thesis in `doc/v1/bg/` (3548 lines of LaTeX). The goal is to turn
both into a **PhD dissertation**: raise the artifact to a C++23 multi-protocol server whose research claim
is socket-minimal protocol demultiplexing, produce thesis-grade measurements for it, and rewrite the
document at PhD depth under TU-Sofia styling.

What the code already has, verified:

| Capability | State |
| --- | --- |
| HTTP/1.1 | Complete. Hand-rolled parser, `src/core/app.cpp:509-743`. |
| HTTP/2 | Complete, ~1950 lines in `src/http2/`. Hand-written framing, only nghttp2's HPACK used. Server push refused with GOAWAY (`src/http2/connection.cpp:295`), contradicting the README. |
| WebSocket | Complete RFC 6455. Handlers key on an exact path in an `unordered_map` (`include/coroute/core/app.hpp:286`), so no route params. |
| TLS + ALPN | Working. Two live bugs, see below. |
| io_uring / kqueue / IOCP | All three, behind one 158-line seam: `include/coroute/net/io_context.hpp`. |
| DFA routing | `matcher::RegexMatcher<size_t>`, 8 matchers (`include/coroute/core/router.hpp:77-86`). The cited TELECOM 2024 paper. |
| API vs UI route split | **Already exists.** `view_routes_` + `view_matcher_`, `App::view<VM>()`. |
| Server-side API invocation | **Already exists.** `App::fetch()` dispatches in-process, no socket (`include/coroute/core/app.hpp:334`). |
| HTTP/3 / QUIC | **Absent entirely.** No UDP path anywhere. |
| Client-side deferred invocation | **Absent.** No futures or promises in the tree. |

Two of your five stated requirements are already met. Two are the real work. One is the contribution.

The blocking architectural fact: `App::run()` (`src/core/app.cpp:12`) picks exactly one of three mutually
exclusive paths, so **TLS and cleartext cannot share an instance**, and `enable_multi_accept` is overridden
only by the io_uring backend, so Windows and macOS silently ignore `threads(N)` when accepting.

## The research claim, stated honestly

The loose version, "2 sockets instead of 3+", does not survive scrutiny. The defensible version:

> One TCP descriptor per TCP endpoint regardless of protocol mix, plus one UDP descriptor for QUIC.
> Conventional servers need one descriptor per (port x protocol-class) pair, because nginx cannot put
> `ssl` and non-`ssl` on the same `listen`.

State the limits up front: browsers will not speak cleartext to `:443`, so the user-visible payoff is
single-exposed-port ingress and a smaller socket table, not halved ports. Descriptor count is a proxy
metric. The engineering contributions that actually measure well are the unified demultiplexer and the
QUIC connection-ID router.

### What "mutually exclusive" costs today

Yes, serving HTTP and HTTPS from this library right now requires **two `App` instances**, and that is
worse than two listeners. `App::run(port)` (`src/core/app.cpp:12`) takes one port, branches hard on
`if (tls_enabled_ && tls_ctx_)` at `:17`, and **blocks** on `io_ctx_->run()` at `:122`, so it cannot be
called twice on one instance. `run_async` (`:125`) does not block but has no TLS branch at all.

Each `App` unconditionally creates its own `IoContext` at `:14`, so two instances means two thread pools
of `thread_count_` threads each, two event loops, two `Router`s with the routes registered twice, two
middleware chains and two template caches. Sessions, metrics and connection pools are not shared. A
plaintext-to-TLS redirect on `:80` therefore costs a second full server.

That is the motivating problem, and it is worth stating plainly in the thesis: the descriptor count is
the visible symptom, but the structural cost is a duplicated runtime.

Two sub-problems carry the novelty:

1. **First-octet classification on one TCP descriptor.** TLS records start with ContentType `0x16`; every
   cleartext HTTP method token and the HTTP/2 preface start with an ASCII uppercase letter. One byte
   decides. `is_http2_preface()` already exists (`src/http2/connection.cpp:910`) but is referenced only by
   tests, so prior-knowledge h2c is currently unreachable in production.
2. **QUIC connection-ID-aware sharding.** `SO_REUSEPORT` on UDP hashes by 4-tuple, but QUIC connections
   migrate, so packets land on the wrong worker. nginx and Angie solve this with an eBPF program attached
   via `SO_ATTACH_REUSEPORT_EBPF` that reads the DCID. A portable userspace alternative that encodes the
   worker index into the connection ID is buildable and measurable on macOS and Windows too, where eBPF
   does not exist.

## Decisions taken

| Question | Answer |
| --- | --- |
| Thesis language | **English for now**, with a two-line language switch so Bulgarian is one flag away. |
| QUIC stack | **ngtcp2 + nghttp3.** Pure state machines that do not own the socket or the loop, which is the only reason the single-descriptor design survives. |
| Classification mechanism | **Read-and-pushback, not `MSG_PEEK`.** See below. |
| Start point | Phase 0 foundation, then thesis relocation. |
| Автореферат | Scaffold now, write last. |
| Cleanup | Approved: `external/ImGui`, `external/Valdi`, the four stub sources, the dead `IViewRenderer`/`WebViewRenderer`, the vestigial `.gitmodules`. |
| Protobuf | Out of scope. Stays as future work and publication topic 6. |
| Appendices | None. Code listings go wherever they explain something, in any chapter, and may be implementation detail rather than example snippets. The example app becomes chapter VII rather than an appendix. |
| I/O backends | **Add an epoll backend.** Makes backend a real independent variable within Linux. See below. |

### Why pushback and not `MSG_PEEK`

`MSG_PEEK` needs three separate backend implementations and buys nothing, because **the bytes read to
classify are exactly the bytes the next layer wants anyway**: the request line goes to
`App::parse_request`, the ClientHello to the TLS handshake, the preface to `receive_preface`. Peeking
reads them twice.

A `PrefaceConnection` decorator that replays buffered bytes needs **zero backend edits**, and
`TlsConnection` (`include/coroute/net/tls.hpp:107`) already establishes the wrapping-`Connection` pattern
in this codebase, so this is reuse rather than a new abstraction. It also composes: TLS can wrap the
pushback connection and read the replayed ClientHello with no special case anywhere.

Rejected optimisation worth documenting: on Windows, `AcceptEx` with `dwReceiveDataLength > 0` makes
classification cost zero extra syscalls, but it holds the accept until data arrives, which is a
slow-connect DoS. Keep `0` so accept semantics match across platforms, and note it as future work.

## Phases

Sequenced so the tree builds at every step. Caveat: **there is no test for `app.cpp` or any I/O backend**,
so "the 117 tests pass" is a weak signal for most of this. Phases 1 and 3 below add the only real
regression net, which is why both are pure-function tests with no sockets.

### Phase 0: Foundation

- `CMakePresets.json`: per-OS backend presets and gate presets (clang-tidy, asan+ubsan, tsan, msan,
  coverage, release-bench). Sanitizers currently exist only as CI-side `CMAKE_CXX_FLAGS` strings.
- **C++23 in two commits, not one.** `0a`: set `CMAKE_CXX_STANDARD 23` with `COROUTE_HAS_STD_EXPECTED`
  forced OFF, verify green. `0b`: allow it on. This second step silently swaps `coroute::expected` from
  the custom class in `include/coroute/util/expected.hpp` to `std::expected` across roughly 200 call
  sites, and the custom class is more permissive about `expected<void,Error>{}`, implicit conversions and
  `unexpected` deduction. Splitting them keeps the bisect clean.
- Cleanup as authorised above.
- Fix two live TLS bugs together, since one fix covers both: `src/net/tls/tls_context.cpp:236` leaks a
  `new std::vector<std::string>`, and `:242` registers `&result` as the SNI callback argument while
  `result` is returned by value and moved, so **the SNI argument dangles on every context ever created**.
  A `shared_ptr<State>` member holding both the ALPN list and the SNI callback fixes both at once.
- Fix the per-request `string_view` to `std::string` copies at `src/core/router.cpp:156` and `:226`.
- Make the gates gate: `cpp-linter` is `continue-on-error: true` and cppcheck ends in `|| true`, so
  `WarningsAsErrors: "*"` in `.clang-tidy` currently blocks nothing.

### Phase 1: Protocol classification

New `include/coroute/net/protocol_detect.hpp` + `src/net/protocol_detect.cpp` (~210 lines):
`classify()`, an incremental tri-state `preface_match()` that returns `No` after 2 to 4 bytes for real
traffic (so it is slowloris-safe and never waits for all 24), `PrefaceConnection`, and `read_prefix()`.
Pure addition, nothing calls it yet. `tests/test_protocol_detect.cpp` covers it over byte arrays, no
sockets.

### Phase 2: Unified TCP listener

Delete the three-way fork in `App::run()`. Add `App::serve_connection`, roughly 60 lines, doing
classify then TLS-or-cleartext then ALPN-or-preface then the existing `handle_connection`, which is
reused unchanged and is already ~90% of what is needed. Extract the request-handler lambda duplicated at
`app.cpp:56-64` and `:808-816` into `App::make_request_handler()`; HTTP/3 becomes its third caller. Net
deletion of code.

`net::TlsListener` then has zero users, so delete it from `tls.hpp` and `tls_context.cpp` too. Leaving it
would repeat the `IViewRenderer` mistake.

### Phase 3: Cross-platform multi-accept

Two defaulted virtuals on `IoContext`: `worker_count()` and `run_on_worker(index, fn)`.
`enable_multi_accept` keeps its signature and stops being Linux-only. The platform semantics genuinely
differ and the difference is a good thesis table, not an embarrassment:

| Platform | Accept model | TCP descriptors, N workers | Assignment |
| --- | --- | --- | --- |
| Linux | `SO_REUSEPORT`, N listeners | N | kernel 4-tuple hash, pinned |
| macOS | 1 listener, N accept ops | 1 | whichever worker's kevent fires |
| Windows | 1 listener, `AcceptEx` pool | 1 | IOCP LIFO wakeup, not pinned |

**macOS `SO_REUSEPORT` does not load-balance** the way Linux's does, so kqueue must use the shared-socket
model. FreeBSD's `SO_REUSEPORT_LB` does, under `#ifdef`.

### Phase 3b: epoll backend

`src/net/epoll/epoll_context.cpp`, selectable on Linux by a CMake option rather than by platform, so
io_uring and epoll are both buildable on the same machine. Model it on the kqueue backend, which is the
closest existing readiness-interface implementation. Implements the same `IoContext`, `Listener`,
`Connection` and (in Phase 4) `DatagramSocket` interfaces, plus `enable_multi_accept` via `SO_REUSEPORT`
and `EPOLLEXCLUSIVE`.

This is what turns backend into a genuine independent variable, and it is the basis of publication topic 3.
Do it before Phase 4 so the datagram layer is written once against both Linux backends.

### Phase 4: UDP datagram layer

New `include/coroute/net/datagram.hpp`: `Datagram` and `DatagramSocket`, with `create()` at the tail of
each existing backend file next to `Listener::create`, matching the existing convention. `IP_PKTINFO` is
mandatory, not optional: without the local address a wildcard-bound QUIC server replies from the wrong
source IP on a multi-homed host. Backends normalise GRO-coalesced segments into separate datagrams so the
QUIC layer never sees GRO.

io_uring uses **one-shot `RECVMSG` first**, because it fits the existing `UringOperation`/`UringAwaiter`
model exactly; multishot fires many CQEs per SQE and needs heap-owned op lifetime plus a provided-buffer
ring, which the current model cannot express. Add multishot only if a benchmark justifies it.

### Phase 5: HTTP/3

`src/http3/{endpoint,connection,cid,tls_ossl}.cpp`, roughly 1300 lines. Single worker first. Requests go
through the **existing** `RequestHandler`, `Router` and middleware chain with zero new types. Dispatch
order mirrors `ngtcp2/examples/server.cc`: version negotiation, cross-worker forward, existing
connection, new connection with optional Retry, rate-limited stateless reset.

ALPN split matters: the TCP `SSL_CTX` advertises `{"h2","http/1.1"}` and **must not** advertise `h3`;
discovery over TCP is via `Alt-Svc`. Alt-Svc is emitted by one middleware registered through the existing
`App::use()`, covering all three protocols in one line, rather than hand-editing the two response
finalisation blocks. Known gap: the view-route branch at `app.cpp:342-398` bypasses the middleware chain.

### Phase 6: CID-aware sharding

`cid_fill` writes the worker index into byte 0 of the connection ID and CSPRNG into the rest;
`cid_worker` reads it back. Pure functions, so `tests/test_http3_cid.cpp` is the runnable check that
fails if routing breaks. Userspace forwarding via `run_on_worker`, with `Stats` counters.

**Measure before building eBPF.** If `fwd_in / rx` is a fraction of a percent under realistic migration,
eBPF buys nothing measurable, and *that is itself the publishable result*. eBPF needs libbpf, clang in the
build, `BPF_MAP_TYPE_REUSEPORT_SOCKARRAY`, `CAP_BPF` and CI that can load BPF: weeks, for the least novel
item on the list. Default OFF, stretch goal only.

Windows has no `SO_REUSEPORT` for UDP, so **100% of packets take the forwarding path** there. Bad for
Windows throughput, excellent for exercising the fallback. Report Windows QUIC separately.

### Phase 7: Deferred UI data resolution

`App::fetch()` already covers the awaited case completely. The deferred case is new.

- `Deferred<T>` in a new `include/coroute/view/deferred.hpp`. ViewModels may hold `Deferred<T>` fields.
- Deferred fetches start **eagerly** via the existing `start_detached()` (`coro/task.hpp:301`) so they
  overlap each other and the initial render instead of serialising.
- `ViewResultAny` (`view/view_types.hpp:52`) already type-erases the model with a captured `to_json_fn`.
  Extend it to carry pending deferreds, each with a slot id and its own `to_json`.
- Render pass one emits resolved fields inline and a placeholder per unresolved slot, then flushes. Each
  deferred flushes its JSON plus a resolve instruction as it resolves.
- **`Deferred<T>` surfaces in the browser as a real JS `Promise`.** Do this, it is not overkill. It is
  maybe fifteen lines of inline JS instead of five: the initial flush creates a pending `Promise` per slot
  in a registry, and each later chunk calls its stored `resolve`. No framework, no bundle.

  Three reasons it is worth the extra ten lines. It is literally what you asked for at the outset, that
  client-side calls are "sent like futures/Promises to the client browser". It makes the client contract
  a real language-level primitive rather than a DOM-swap convention, so page code can `await` a slot,
  compose slots with `Promise.all`, and attach error handling, which a placeholder swap cannot express.
  And it gives the C++ `Deferred<T>` a precise counterpart on the other side of the wire, which makes the
  awaited-versus-deferred distinction a **typed contract spanning both languages**. That symmetry is the
  interesting part of publication topic 5; without it the contribution is just early flushing, which
  BigPipe did in 2010.

  Keep the DOM-swap path too, as the no-JS fallback, so the deferred mode degrades rather than breaking.
- Reuse `ChunkedResponse` (`core/chunked.hpp`) for HTTP/1.1. HTTP/2 and HTTP/3 need DATA frames, so one
  small flush interface over chunked-versus-frames is the only new abstraction warranted.

### Phase 8: Measurement

See the measurement section below. It is large enough to need its own treatment.

### Phase 9: Thesis

### Phase 10: Publications list

## A constraint that lands on the implementation, not the measurement

**Every A/B arm must be a runtime flag, not a compile-time option, so both arms are served by the
identical binary.** This applies to demultiplexing on/off, awaited/deferred, and the CID-routing paths.

The reason is that build-to-build variation from code layout, link order and environment size is
documented at 5 to 10% (Mytkowicz et al., *Producing Wrong Data Without Doing Anything Obviously Wrong*,
ASPLOS 2009), while run-to-run variation on a controlled machine is 1 to 2%. A 3% difference between two
separately compiled binaries is indistinguishable from having linked the object files in a different
order. Same binary, runtime flag, and the problem disappears entirely for far less effort than
randomising link order.

Also in Phase 0: `examples/Samples/benchmark_server/main.cpp` hardcodes 12 threads unless `argv[1]` is
given. Worker count, protocol, listen backlog and the A/B flags all need to be runtime arguments, or they
cannot be controlled variables.

## Measurement

### The existing harness has more wrong with it than the two known defects

Beyond the unparsed percentiles and the post-run CPU sample, the audit found seven more. There is no prior
published methodology to correct here, so these are not framed as corrections to anything. They are the
reason the harness is rebuilt rather than patched, and several of them are worth stating in the
Methodology chapter as **failure modes any server benchmark must rule out**, since each one silently
produces plausible numbers rather than an error.

The Methodology chapter's job is to lay out the candidate approaches, compare them on their merits, and
conclude with the single one that is used, matching what the codebase actually does. Not a retrospective.

| # | Defect | Consequence |
| --- | --- | --- |
| 1 | `netem` applied to the `lo` root qdisc (`run-single-benchmark.sh:249`) | On loopback both request and response are egress on `lo`, so a configured 50 ms delay delivers ~100 ms RTT and 1% loss becomes ~2% per round trip. Silently measures a different network than the one configured. |
| 2 | `netem` with no `limit` | Default is 1000 packets. At loopback rates the qdisc backlog overflows, so "1% loss" is actually an unmeasured tail-drop condition. |
| 3 | Error counts never parsed | `parse_wrk_output` takes only `Latency` and `Requests/sec`. Non-2xx responses and socket errors are discarded, so **a server that fails fast wins on throughput.** The most dangerous omission of the set. |
| 4 | `cpu_percent` is a different physical quantity per OS | Linux `ps -o %cpu` is lifetime-average percent; PowerShell `$proc.CPU` is cumulative CPU *seconds*. Both are plotted into one column on one axis. |
| 5 | Windows silently uses a different generator | `Find-BenchmarkTool` prefers `hey`, whose branch caps total requests at `connections * duration * 100`. A "30-second" Windows run finishes in about a second on a fast server. |
| 6 | `mem_idle` is in the README spec and collected nowhere | |
| 7 | `strace -c -p $PID` with no `-f` | Attaches to one thread of a twelve-thread server, after load starts, and `-c` overhead distorts what it measures. For an io_uring server, syscall count is the wrong metric anyway: it is precisely what io_uring eliminates. |
| 8 | `benchmark_server` hardcodes 12 threads | Worker count is not a controlled variable today. |
| 9 | All five runs share one server process | The script restarts per cell, not per run, so it measures one process's page placement and allocator luck five times. **n=5 is really n=1.** |

Correction 9 is the one that matters most for statistics: a *run* must mean a fresh server process and a
fresh generator process.

### The epoll backend makes "I/O backend" a real variable

As the tree stands, `CMakeLists.txt:20-30` selects io_uring, IOCP or kqueue by platform, so backend is
perfectly confounded with OS and cannot be swept. Adding **epoll** fixes that, and it is worth doing for
the thesis rather than just for coverage.

epoll and io_uring then run on the **same OS, same kernel, same hardware, same codebase, same routes, same
TLS stack**, differing only in the completion mechanism. Almost every published readiness-versus-completion
comparison compares two different programs and therefore measures the programs. This measures the
mechanism. That is a stronger result than the io_uring mode sweep would have been, and the mode sweep
survives as a subsection inside it (default, `SQPOLL`, `DEFER_TASKRUN|COOP_TASKRUN`, registered buffers,
multishot recv).

Cost is moderate and bounded: epoll is the simplest of the four backends, the `IoContext`/`Listener`/
`Connection` seam already exists and is only 158 lines, and kqueue is the closest existing model to copy
from since both are readiness interfaces. The `DatagramSocket` addition applies to it too.

It also removes a portability footgun: io_uring is unavailable or blocked in enough container and hardened
environments that a Linux fallback has independent value.

### Tools

Standardise on **h2load built against ngtcp2 + nghttp3** for every cross-protocol comparison. It is the
only mature generator that speaks HTTP/1.1, HTTP/2 and HTTP/3 with one output format and one latency
model, and any cross-protocol claim made with three different clients is confounded by three different
client implementations. It also exposes `-m` for streams per connection, which wrk cannot express at all
and which is the central variable in an h2-versus-h3 comparison. Its `--log-file` gives raw per-request
microsecond samples, and it reports status-code counts natively, so defects 2 and 3 disappear for free.

Two honest costs to declare: h2load's HTTP/1.1 path is less optimised than wrk's, so absolute h1
throughput under h2load will read lower than under wrk. That is acceptable because every comparison is
between systems under one client, but it must be stated, and one wrk cross-check cell should quantify the
client delta so the number is known rather than assumed. Also, **h2load's `-r` is a connection arrival
rate, not a request rate**, so it is not a wrk2 substitute.

Three tools, non-overlapping jobs: h2load for saturation throughput and service-time distributions across
all three protocols; **wrk2** for HTTP/1.1 open-loop constant request rate; **k6** `constant-arrival-rate`
for latency-versus-offered-load curves and WebSocket load. Drop `hey`, `ab` and `bombardier`.

CPU and memory come from **cgroup v2 accounting**, not from `ps`: `systemd-run --scope` with
`CPUAccounting` and `MemoryAccounting`, then read `cpu.stat` and `memory.peak`. Both are kernel-tracked
and immune to the sampling race.

### Coordinated omission

With a closed loop of C connections, a stall stops the client issuing requests, so the requests that
*would* have been delayed are never sent and never measured. Closed-loop p99 is a p99 of **service time**,
not response time.

Protocol: closed-loop ramp to find each system's `T_max`; set `T_ref` to the minimum `T_max` across the
compared systems for that cell; then run open-loop at fixed offered loads of 25/50/75/90/95% of `T_ref`,
so every system receives identical offered load. Report closed-loop numbers explicitly labelled as service
time and open-loop as response time.

**A gap to declare rather than paper over: there is no mature open-loop constant-request-rate generator
for HTTP/3.** wrk2 is h1-only, k6 and vegeta do not speak h3, and h2load's rate control is connection
arrival. Report h3 tail latency from closed-loop runs only, labelled as service time, and state the
limitation. Patching h2load is about 50 lines and is optional.

### Localhost is disqualifying for the central claim

Loopback-only measurement is not merely a limitation to declare here, it would invalidate the central
claim, because **loopback is not protocol-neutral**. `lo` has MTU 65536, so TCP-based protocols get 64 KB
segments with no segmentation, no checksums and no interrupt path, while QUIC still emits 1200 to 1450
byte datagrams by design and pays full userspace crypto per datagram. Comparing h1 and h2 against h3 on
loopback systematically penalises h3 for reasons that have nothing to do with either implementation.
Loopback also makes RTT ~30 microseconds, so every latency-hiding property of multiplexing is invisible,
and it makes **connection migration untestable**, since there is only one address.

Loopback stays useful for router microbenchmarks, serialisation cost, and an explicitly labelled
"protocol overhead floor". Nothing else.

The fix is a **netns pair** joined by veth, with netem applied per direction and an explicit
`limit 100000`. That gives real MTU, a real qdisc, a real IP path, and two client addresses, which is what
makes QUIC migration testable at all. It is the hard requirement that justifies building the harness.

### Staging to the hardware that actually exists

| Phase | Platform | Produces |
| --- | --- | --- |
| A, now | Windows + Docker + MacBook | Harness, schema, baseline images, **all** conformance, fuzzing, sanitizer and coverage work, the Windows/IOCP secondary dataset, all microbenchmarks |
| B, once Linux is rebuilt | Desktop, netns pair, isolated cores | **The primary thesis dataset.** |
| C, if a link exists | Desktop to MacBook over 2.5 GbE | Repeat six to eight headline cells; report the netns-versus-wire delta as a validity check |

Phase C buys the single-host-versus-two-host validity check without a full two-machine matrix: "netns
results were validated against a two-host configuration on N cells; relative ordering was preserved and
absolute throughput differed by X%." Native macOS on the M4 is **not** a VM and is legitimate for kqueue
data; only Linux-in-Docker-on-macOS is disqualified. Note that 1 GbE caps at about 118 MB/s, which turns
every large-file test into a link-bandwidth measurement, so wire validation needs 2.5 GbE or small
payloads only.

### Experiment budget

The full cross-product is roughly 1.4 million cells, about 13 machine-years. Use one-factor-at-a-time
sweeps around a fixed baseline configuration. Seven essential experiment groups, roughly **50 machine
hours at n=5 and 30 s runs, or 110 hours at n=10 and 60 s runs**: two to three unattended overnight
campaigns. Optional groups each stand alone as a subsection.

Statistics: pilot one cell 20 times to measure run-level variation and choose n from it rather than
asserting one. Randomise run order by round-robining systems within each repetition, so thermal drift does
not correlate with system identity. **Never trim latency samples** since the tail is the phenomenon;
reject whole runs only on pre-declared mechanical faults (non-2xx above 0.1%, generator CPU above 85%,
non-zero `UdpRcvbufErrors` or `TcpExtListenOverflows`, CPU frequency drift above 2%, virtualisation
detected) and report the rejection count. Report medians with BCa bootstrap CIs, and treat a cross-system
difference as reportable only when the CI excludes zero **and** the relative difference is at least 5%.
Headline figure: a throughput-versus-p99 Pareto plot.

Add a **generator saturation check** to every run, since the most common way framework benchmarks are
wrong is that the client is the bottleneck. And add a **virtualisation guard** that refuses to write
performance records when `systemd-detect-virt` fires, so publishing VM numbers by accident becomes
mechanically impossible.

### The socket-count study

Seven artefacts: a descriptor census table (`ls -l /proc/$PID/task/*/fd` plus `ss -lntupe`, classified);
a kernel memory-per-listener table from `/proc/net/sockstat` and `/proc/slabinfo` deltas; a
connect-to-first-byte CDF comparing demux-on, demux-bypassed and nginx **from the same binary**; a
handshake-rate bar chart with `nstat` drop counters alongside; a `SO_REUSEPORT` shard-balance figure
reporting coefficient of variation; a QUIC migration time-series; and a migration correctness table.

Be honest about the weak part: **memory per listener will be single-digit KB**, so the memory argument is
weak. The descriptor argument is operational (one port to firewall, one cert path, one config surface) and
the interesting measurable is the classification cost.

Note `listen(backlog)` defaults to `128` in `include/coroute/net/io_context.hpp:96`, which is far too
small at C=4096. Make backlog an explicitly swept variable.

Migration is forced externally with `nft`/`iptables` SNAT port remapping in the client namespace rather
than requiring client cooperation, so it works identically against all four systems. ngtcp2 emits **qlog**;
parse `PATH_CHALLENGE` to `PATH_RESPONSE` timing out of it programmatically rather than eyeballing qvis.

### Correctness, not just speed

- **h2spec** for HTTP/2, run against nginx, h2o and Caddy too so the pass table has context.
- **autobahn-testsuite** for WebSocket. 500+ cases, cheap, high credibility.
- **quic-interop-runner** for HTTP/3, including its `connectionmigration` case. Budget 2 to 4 days for
  Docker packaging, and do it once the implementation stabilises, not during development. Cheaper first
  step: drive the server with ngtcp2's, quiche's and quic-go's clients and inspect qlog.
- **Direct tests of the classifier**, since it is the new code: ClientHello split across segments and
  delivered one byte at a time (the classic first-octet-classifier bug), first octet `0x16` but not
  actually TLS, a valid HTTP/1.1 request beginning with a TLS-looking byte, and a client that connects and
  sends nothing.
- **Nine libFuzzer harnesses** on the pure parsing surfaces, HPACK first (decompression-bomb and
  table-index surface). "We fuzzed N CPU-hours and found M defects" is a legitimate thesis result and the
  table belongs in Results. Plus differential request-smuggling testing against nginx.

Sanitizer judgment, stated in Methodology rather than left as an omission: **ASan+UBSan is mandatory and
highest yield here**, because `start_detached()` is called on every accepted connection so coroutine frame
lifetime is entirely by convention. **TSan is mandatory for the CID-routing work**, with a suppressions
file, since it does not model io_uring completion ordering. **MSan is skipped on the full server** and run
only on the fuzz harnesses, because instrumenting OpenSSL, nghttp2, ngtcp2, nghttp3, zlib, simdjson and
liburing is days of work for low yield.

**The existing sanitizer CI job cannot catch the bugs it exists for.** `.github/workflows/ci.yml:132-177`
builds with `COROUTE_BUILD_EXAMPLES=OFF` and runs only ctest, so the server itself is never sanitized. Fix
it to drive a running sanitized server with h2spec, autobahn and a short h2load.

Coverage tooling is clang source-based (`-fprofile-instr-generate -fcoverage-mapping`), not gcov, which
mangles line mapping for coroutines and templates. Two tiers: 90% line and 80% branch on parsers and
protocol state machines, 60% line on backends and wiring, 75% overall. **The high-yield move is merging
unit-test coverage with conformance-run coverage** so `app.cpp` and the backends get covered by h2spec,
autobahn and interop without writing tests for them.

Schedule risk to flag now: even with that trick, reaching the parser tier is **3 to 4 weeks of test
writing**, and it is the largest single item in the correctness workstream.

### Data schema

Long format, one row per run, in Parquet. Nothing encoded in filenames: `parse_artifact_name` in
`aggregate-results.py` is exactly why the current schema cannot express protocol, TLS or offered rate.
Columns cover keys, factors, outcomes, QUIC-specific counters, validity flags and provenance
(`git_commit`, `build_id`, compiler flags, dependency versions, generator argv).

Store the full HdrHistogram for every run (lossless to three significant digits, a few KB) and raw
per-request samples only for the headline cells and every migration run, since 60 s at 300k rps is 18M
rows per run and the full set would be terabytes.

An `env/<hash>.json` manifest freezes the environment programmatically, and **the driver refuses to append
to an existing plan if the hash changes** without an explicit flag. That single mechanism is what keeps a
multi-week campaign honest.

Figures and tables are generated into `doc/thesis/`, never hand-copied, per the `\R{}` scheme above.

### CI

Bare metal for every number that enters a figure; containers only for baseline build environments (with
recorded image digests), the interop runner, and macOS functional testing. **Containerise everything or
nothing** in a comparison. Rewrite `benchmark.yml` as a regression smoke gate explicitly marked
non-citable, drop the gh-pages publish of raw wrk output, and remove `-march=native`, which is
non-reproducible across runner microarchitectures.

## Thesis

### Structure

Target roughly 175 pages, 146 in the numbered chapters. Bulgarian dissertation convention differs from the
diploma convention in one way that matters: the goal, tasks and hypotheses are stated at the **end of the
analytical review**, not in the introduction, and every numbered chapter ends with an "Изводи" section.

| Part | Pages | Source A material |
| --- | --- | --- |
| Front matter: title, declaration, contents, abbreviations | 7 | new |
| Introduction (unnumbered) | 5 | `01_introduction` relevance + structure, `00_abstract` |
| I. State of the art | 26 | all of `02_existing_solutions` (expanded ~8x), protocol description parts of `03_theoretical_background`, plus goals/tasks from `01_introduction` |
| II. Theoretical foundations | 16 | `03_theoretical_background` coroutines + DFA, promoted from description to formalism |
| III. Architecture | 24 | all of `04_architecture`; the demux, QUIC, sharding and UI/API sections are new |
| IV. Implementation | 26 | all of `05_protocols` + all of `06_type_safe_params`; absorbs what would have been appendix listings |
| V. Methodology | 14 | `08_testing` lines 11-30, expanded to a chapter |
| VI. Results and analysis | 28 | bulk of `08_testing` (177-877) |
| VII. Application and validation | 12 | all of `07_example_app`; exists so the example app needs no appendix |
| Conclusion (unnumbered) | 5 | `09_conclusion` minus the contributions section |
| Contributions statement (unnumbered) | 4 | new, mandatory, absent from both source trees |
| Publications (unnumbered) | 2 | new |
| Bibliography | 8 | 90 to 140 entries, up from 32 |

Chapter I section 1.2 is load-bearing: it establishes the "3+ descriptors" baseline from nginx, Angie,
H2O, Caddy and IIS documentation as a **documented fact**, not an assertion. Section 2.2 states descriptor
minimisation formally, so the central claim is a proposition rather than an engineering anecdote.

### File tree

```
doc/thesis/
  Main.tex  lang.tex  preamble.tex  latexmkrc  .gitignore  references.bib
  chapters/  front_abbreviations, front_introduction, 01..07, back_conclusion,
             back_contributions, back_publications
  images/    tu-logo.png (from the maui tree, not Source A's ту.png)
  data/      committed CSV snapshots from the harness
  tools/     results2tex.py
  build/     gitignored entirely
```

First action: **delete `doc/thesis/build/Main.pdf`**. It is 14.7 MB and it is the maui master's thesis,
not this document. The whole `doc/thesis/` tree is untracked, so nothing enters history.

### The language switch

`lang.tex` is the entire switch: `\newif\ifbg` plus `\bgfalse`. `preamble.tex` branches once at load time
to pick `main=english` or `main=bulgarian` and to define `\bgen{bg}{en}`.

`\bgen` must be defined by **branching at load time**, not as `\newcommand{\bgen}[2]{\ifbg#1\else#2\fi}`.
The second form leaves a live conditional in the macro body, which breaks when the macro is expanded
during a write into `.toc` or `.aux`. The load-time form is fully expandable and safe inside
`\MakeUppercase` and `\addcontentsline`.

Both languages stay loaded, only `main=` moves, so T2A and `tempora` remain live in the English build and
`\foreignlanguage{bulgarian}{...}` works for the title page and quoted terminology. Only captions, the
four reference macros, the listing names, and six unnumbered headings are language-dependent. Geometry,
fonts, spacing, `titlesec`, `tocloft`, `fancyhdr` and the whole `\lstset` are untouched, so a page count
agreed in one language holds in the other.

Chapters go through `\newcommand{\chapdir}{chapters}` from day one, so a second language later is one line
plus a `git mv`, with no duplicated preamble.

### Preamble

Copy from the maui `preamble.tex` verbatim: fonts and encoding, geometry, the hyphenation block **with its
comment** (it is the institutional memory of 1227 overfull boxes), `fancyhdr`, footnotes, the whole
`titlesec` block, the `tocloft` block **with `\patchcmd{\l@section}` placed after tocloft** (reversing that
order fails silently), graphics and table packages, and the `\lst@AddToHook{PreInit}` hyphenation
relaxation that must land together with the global penalties.

Strip three things that abort a fresh build: `\input{generated/board_macros}`,
`\input{generated/gif_frames}`, and the `\capture`/`\evidencepair` macros pointing at a nonexistent
`evidence/` tree.

Change: the `emph=` list from maui vocabulary to `Request, Response, Handler, Middleware, Router, App,
Connection, WebSocketConnection, IoContext, Task, RegexMatcher`; captions and reference macros to `\bgen`;
`hyperref` loaded with `[hidelinks]` since a bound dissertation should not have coloured link boxes.

Keep the Cyrillic `literate` table **even in the English build**: source comments in this repo can contain
Cyrillic, and `\lstinputlisting` hard-fails on invalid UTF-8 without it.

Add `siunitx` (one number format for a 28-page results chapter), `pgfplots` and `pgfplotstable`.

Isolate the section-numbering choice in one commented block. The ФКСТ diploma sheet mandates Roman
numerals, but Bulgarian dissertations conventionally use "ГЛАВА 1" with Arabic. That is open question 9
and switching is three lines.

### No hand-copied numbers

The principle from the maui thesis is right and should be kept, but its 517-line `gen_tex.py` is a bespoke
model of a screenshot comparison board and none of it transfers to req/s and descriptor counts. It costs
far less here:

- `benchmark/aggregate-results.py` already reduces to `combined_results.csv` and `raw_results.json`.
- `pgfplots` reads CSV directly, so charts need zero Python.
- `pgfplotstable` typesets CSV directly, so full tables need zero Python.

That leaves only scalars quoted inside sentences, which is exactly where hand-copying rots. A ~40-line
`tools/results2tex.py` emits `\csname res@coroute.h1.rps\endcsname` style definitions, and prose reads
`\R{coroute.h1.rps}`. Three properties follow: a missing measurement prints a red `[?key]` in the PDF and
is greppable like `\TODO`, keys are names rather than row offsets so re-running the harness cannot corrupt
a sentence, and the document still builds on a machine that has never run a benchmark because the input is
guarded.

### Build order

Scaffolding first, in one sitting: delete `build/`, write `lang.tex`, `preamble.tex`, `latexmkrc`,
`.gitignore`, and a `Main.tex` whose chapter calls point at files containing only their heading and a
`\TODO`. Build it. A 15-page skeleton that compiles clean with a correct TOC proves the preamble carried
over, and that is cheap now and expensive to debug at 150 pages.

## TU-Sofia PhD requirements

Source: `konkursi-as.tu-sofia.bg/rabotnidoc/Proceduri-za-NS.pdf`. A newer ПУРПНСТУС was adopted
20.11.2025 and should be checked for deltas.

The maui tree's `REQUIREMENTS.md` is the ФКСТ **diploma-thesis** spec. Its typography carries over
unchanged and applies at any degree level. Its structure does not: the 40-50/50-60 page limits, the
eight-item diploma structure, the дипломно задание, comb binding and the 7-8 minute defence are all
degree-specific.

Section 9 д) lists what is submitted for defence: the dissertation in four copies; the **автореферат**,
four copies, A4, **up to 32 pages**, with an **English annotation on the last page**; a **справка за
приносите**; a list of published and accepted works meeting приложение 1 на ПУРПНСТУС; copies of those
works; a list of citations; a declaration of originality under чл. 27, ал. 2 от ППЗРАСРБ; and a European
CV.

Two consequences:

1. **The publications list is not optional.** Bulgarian law requires published papers on the dissertation
   topic before defence, so the file you asked for is a regulatory work plan. Papers need lead time.
2. **A jury member may not be a co-author** of the dissertant in publications included in the dissertation
   (section 9 з), except the научен ръководител. Relevant since `stankov2024regex` is co-authored with
   И. Станков.

### Open questions to close before submission

Sixteen were identified; none block writing, all are a few lines of preamble or front matter. The ones
that matter most:

1. **Is an English dissertation permitted at all?** If yes, is the title page still Bulgarian, is a
   Bulgarian автореферат still mandatory, is a bilingual abstract required? This decides whether the
   `chapdir` split gets used, so ask it first.
2. **Section numbering**: Roman per the ФКСТ sheet, or Arabic "ГЛАВА 1" per dissertation convention. Also
   whether the no-hyphenation rule applies to a dissertation, and to an English body.
3. **Volume**: minimum and maximum pages, and whether a "page" means 1800 characters. The 146-page plan is
   an assumption.
4. **Mandated section order**, in particular whether приноси come before or after the bibliography.
5. **Title page fields**, exactly. Also **publications**: minimum count and type, whether one must be
   Scopus or WoS indexed, whether co-authorship declarations are required.

The rest are recorded in the plan's working notes: governing document revision, автореферат structure,
contributions split, whether the ФКСТ sheet applies verbatim, binding and copies, declaration form,
anti-plagiarism system, mandatory lists, citation style, deadlines.

## Publications

`PUBLICATIONS.md` at the repo root. Every topic is derived from work the thesis has to do anyway.

| # | Working title | Hypothesis or goal |
| --- | --- | --- |
| 1 | Socket-minimal multi-protocol demultiplexing | First-octet classification on one TCP descriptor serves TLS and cleartext h1/h2/WebSocket at no measurable throughput or latency cost versus dedicated listeners. The core claim. |
| 2 | Portable QUIC connection-ID routing without eBPF | A userspace DCID-to-worker map with the worker index in the connection ID matches `SO_ATTACH_REUSEPORT_EBPF` steering within a bounded margin, and works where eBPF does not exist. |
| 3 | Readiness versus completion in one codebase: epoll vs io_uring | Same OS, same kernel, same hardware, same server, differing only in completion mechanism. Published comparisons compare different programs and therefore measure the programs. Extends across kqueue and IOCP as a second paper if the material justifies splitting. **If you already have a paper on epoll vs io_uring, send me the citation and this becomes an extension of it rather than a new topic.** |
| 4 | DFA routing at server scale | Extends `stankov2024regex` from a matching benchmark to end-to-end throughput, with route-count scaling against radix trees and `std::regex`. |
| 5 | Awaited versus deferred data resolution in server-rendered UIs | Deferred streaming reduces time-to-first-byte by a measurable margin at equal load, in a compiled coroutine runtime rather than a JS one. |
| 6 | Compile-time `.proto` parsing with C++26 static reflection | Whether P2996 can replace `protoc` as a build step. Needs no server work, so it can be written first. |
| 7 | Zero-copy response paths across three OS APIs | `sendfile` vs `TransmitFile` vs io_uring `SEND_ZC`, one codebase, across file sizes. |
| 8 | Coroutine-per-connection memory and tail latency | Frame size versus thread stack, and the effect on p99.9 under connection scaling. |
| 9 | A reproducible benchmarking methodology for multi-protocol servers | Protocol-fair comparison and coordinated-omission-free measurement. Justifies the thesis' own method chapter. |
| 10 | Compile-time route table verification | Extends the type-safe parameter work to catching route and handler signature mismatches at compile time. |

Papers 1, 2 and 4 are the strongest and map onto the contributions statement.

## On the Linux distribution

**The crash is probably not Arch's fault.** A machine that fully powers off under combined CPU and network
load is the signature of a hardware limit: PSU sag, VRM thermals, or an unstable EXPO/XMP profile. A panic
or an OOM kill leaves evidence; a hard power-off leaves a log that stops mid-sentence. Two cheap decisive
checks before repartitioning:

- After a crash, `journalctl -b -1 -e`. A log that ends with no panic trace means a power event.
- Run the same sustained load under **Windows**, which you already have. If it also resets, the OS is
  exonerated and the fix is hardware.

This matters beyond convenience: a machine that resets mid-run silently truncates benchmark data.

**The scientific objection to Arch is reproducibility, not stability**, and that has a clean fix. Pinning
every mirror to a dated Arch Linux Archive snapshot makes an Arch install *more* exactly reproducible than
a typical Ubuntu one:

```bash
echo 'Server=https://archive.archlinux.org/repos/2026/08/01/$repo/os/$arch' | sudo tee /etc/pacman.d/mirrorlist
```

| Option | Verdict |
| --- | --- |
| **Arch + `linux-lts` + ALA-pinned mirrors + btrfs/snapper** | Best fit if you want pacman. Fixes reproducibility exactly, LTS removes the rolling-kernel variable, snapshots make a bad update recoverable. |
| **openSUSE Tumbleweed** | The rolling distro that does not break: openQA-gated snapshots, btrfs and snapper rollback by default. Not pacman, but closest in spirit while staying defensible. |
| **CachyOS** | Arch-based and tempting, but patched kernels (BORE/sched-ext) and `x86-64-v3/v4` packages are a confound. You would be measuring a non-stock kernel. |
| **EndeavourOS / Manjaro** | Arch with a nicer installer or a two-week delay. Neither improves reproducibility. |
| **Ubuntu 24.04 LTS + HWE** | Safest default, best `perf` and eBPF packaging, zero argument from a reviewer. |

**Recommendation:** Arch + `linux-lts` + ALA pinning to keep pacman, or Ubuntu 24.04 LTS for zero
argument. Diagnose the shutdown first either way.

## Baselines

| Tier | Systems | Protocols | Purpose |
| --- | --- | --- | --- |
| 1 | nginx (quic build), h2o, Caddy, **ASP.NET Core Kestrel** | h1 + h2 + h3 | The real comparison. nginx is the system whose 3-descriptor plus eBPF model the claim argues against. |
| 2 | actix-web, axum/hyper, quinn+h3 (Rust); fasthttp, Gin, bare quic-go (Go); Drogon, Crow, Oat++ (C++) | mostly h1, some h2 | Framework and runtime comparison. |
| 3 | Express (Node), Flask (Python) | h1 only, one config, one figure | Order-of-magnitude context only. |

**Kestrel is the important addition.** It serves h1, h2 and h3 from one unified configuration, which makes
it the closest existing system to this thesis' own claim. A reviewer will ask about it, so omitting it is
a hole. It is cheap to run.

Two pairings give controlled comparisons almost for free: `quinn+h3` against Caddy separates quic-go's
protocol cost from Caddy's framework cost, and `actix-web` against `axum/hyper` separates raw performance
from architectural comparison (tower's service model against the view/middleware split here). Note as a
finding in its own right that **no mainstream Rust framework ships HTTP/3 in its default path**, which is
direct motivation for the thesis.

Crow and Oat++ are HTTP/1.1 only, so restrict them to h1 cells and say so. Gin is a router over
`net/http`, so be explicit that what is being measured is `net/http`.

`benchmark/` already wires up the C++ three plus Express and Flask, but its `competitors/` tree does not
exist and nothing in CI ever runs the harness. Do not resurrect `competitors/`; regenerate baselines from
pinned Dockerfiles with recorded digests.

## Top risks

**1. OpenSSL 3.5 is a hard floor for HTTP/3.** `libngtcp2_crypto_ossl` works with vanilla OpenSSL only
from **3.5.0**, and upstream still marks it experimental. Your machine has 3.5.6, so local work is fine,
but Ubuntu 24.04 ships 3.0.x. Plan: pin 3.5+, vendor it where the system copy is too old, `FATAL_ERROR`
when HTTP/3 was explicitly requested and the version is too low, and keep aws-lc as the documented
fallback. When HTTP/3 is disabled, everything else is byte-identical to today.

**2. ngtcp2 under MSVC via FetchContent.** This repo's CMake already `FORCE`s `ENABLE_LIB_ONLY`,
`ENABLE_STATIC_LIB` and `ENABLE_SHARED_LIB` into the global cache for nghttp2, and those leak into ngtcp2
and nghttp3. Re-set them immediately before each `MakeAvailable`, land Linux first with HTTP/3 OFF on
Windows, consider vcpkg for the Windows leg. Your primary dev machine is Windows, so budget for this.

**3. HTTP/3 is the largest new surface.** Everything else is modification of working code. QUIC is
net-new across three backends. Phases 1 to 3 are deliberately independently valuable so the thesis
survives if Phase 5 slips.

**4. `set_timeout()` is stored and never enforced by any backend** (`uring_context.cpp:553`,
`kqueue_context.cpp:404`, `iocp_context.cpp:463`). The demux adds a read *before* any protocol handler, so
a slowloris now stalls in classification. Pre-existing, newly prominent. `IoContext::schedule()` already
exists, so a watchdog is ~15 lines. Worth doing before anyone benchmarks this adversarially.

**5. The benchmark machine resets under load.** Until diagnosed, no measurement from it is trustworthy.

**6. Nothing currently gates.** `cpp-linter` is `continue-on-error`, cppcheck ends in `|| true`, there is
no coverage tooling, and the sanitizer job builds with examples OFF so the server is never sanitized.
Until Phase 0 closes that, "the tests pass" means less than it sounds.

**7. Test coverage is the largest single time sink.** 117 Catch2 cases exist and **zero** cover `app`,
`request`, `json`, `form`, `cookie`, `session`, `logging`, `metrics`, `socket`, `event_loop`, `websocket`,
any I/O backend, or TLS. Even with the merged conformance-coverage trick, reaching the parser tier is 3 to
4 weeks. Plan it explicitly rather than discovering it.

**8. Windows limits two whole experiment classes.** There is no scriptable `tc` equivalent, so degraded
network on Windows is out (`clumsy` is GUI-driven; QoS policies cannot express loss plus jitter plus
reorder). And UDP GSO/GRO and batched receive differ fundamentally, so **HTTP/3 performance claims are
Linux-only**; Windows h3 is functional testing. Say both in the limitations section rather than being
caught.

## Deliberately not built

- No `Socket`/`EventLoop` abstraction. `DatagramSocket` is the only new interface and it has three
  implementations by construction, unlike `IViewRenderer`.
- **No HTTP/2 rewrite onto nghttp2's session layer.** Large, risky, zero thesis value. Pre-empt the
  reviewer question instead: ngtcp2 and nghttp3 were chosen *because* they do not own the socket or the
  loop, which is the whole architectural argument.
- No WebSocket route patterns. Orthogonal to the listener work.
- No server push.
- No new buffer pool. `BufferPool` in `util/object_pool.hpp` already exists; reuse it for datagrams.
- No appendices, no `evidence/` tree, no port of the 517-line `gen_tex.py`.

## Verification

Each phase gates on `ctest` green under the asan+ubsan and tsan presets, `h2spec` for HTTP/2 conformance,
the QUIC interop runner for HTTP/3, `curl --http3` for a first end-to-end check, and a Release-only
benchmark run showing no regression. Benchmarks never run against Debug builds, and never inside a VM or
container on macOS, which is valid for functional testing only.
