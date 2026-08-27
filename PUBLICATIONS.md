# Publication plan

Candidate papers arising from this dissertation. Every topic here comes out of work
the thesis has to do anyway, and every one is citable inside it.

## Why this file exists

Two reasons, and the first is not optional.

Bulgarian law requires published work on the dissertation topic before the defence.
Section 9 (д) of the TU-Sofia procedures lists, among the documents submitted for
defence, a list of published and accepted works meeting приложение 1 на ПУРПНСТУС,
copies of those works, and a list of citations. Papers need lead time: submission,
review, revision, and a conference or issue date. So this is a work plan with
deadlines attached to it, not a wish list.

The second reason is that a dissertation reads better when its contributions have
already survived review somewhere. A claim that has been through a programme committee
is a different kind of claim from one that has only been through its own author.

## Constraint worth knowing before choosing venues

A jury member may not be a co-author of the dissertant in publications included in the
dissertation (section 9 (з)), with the научен ръководител as the exception. The
existing paper below is co-authored, which is worth remembering when the jury is
formed.

## Already published

**Stankov, I. and Tsvetanov, A.**, "Matching Text from Start to Finish Against Multiple
Regular Expressions", 32nd National Conference with International Participation
"Telecom 2024", Sofia, Bulgaria, November 2024, pp. 21-22, IEEE.

The DFA matching algorithm the router is built on. Cited throughout the thesis, and
the basis of topic 4 below, which extends it from a matching benchmark to end-to-end
server throughput.

## Proposed topics

Ordered by how strongly each maps onto a contribution the thesis will claim. Papers 1,
2 and 4 are the strongest and correspond most directly to the contributions statement.

| # | Working title | Depends on |
| --- | --- | --- |
| 1 | Socket-minimal multi-protocol demultiplexing | Implemented, needs measurement |
| 2 | Portable QUIC connection-ID routing without eBPF | Implemented, needs migration measurement |
| 3 | Readiness versus completion in one codebase | Both backends build, needs measurement |
| 4 | DFA routing at server scale | Router integrated, needs route-count sweep |
| 5 | Awaited versus deferred data resolution | Implemented and demonstrated |
| 6 | Compile-time `.proto` parsing with static reflection | Nothing built, and needs nothing built |
| 7 | Zero-copy response paths across three OS APIs | Partly present, needs the Windows and Linux paths measured |
| 8 | Coroutine-per-connection memory and tail latency | Needs measurement only |
| 9 | Reproducible benchmarking for multi-protocol servers | Needs the harness itself |
| 10 | Compile-time route table verification | Needs design work |

---

### 1. Socket-minimal multi-protocol demultiplexing

**Hypothesis.** A single TCP listening descriptor can serve TLS and cleartext, and
within those HTTP/1.1, HTTP/2 and WebSocket, at no measurable cost in throughput or
latency compared to one dedicated listener per protocol class.

**Why it is worth asking.** Conventional servers need one listening descriptor per
(port x protocol-class) pair, because nginx cannot put `ssl` and non-`ssl` on the same
`listen` directive. The cost of that is usually described as operational rather than
measured. This makes it measurable: the same binary serves both arrangements behind a
runtime flag, so the comparison is not between two builds.

**What has to be shown.** That first-octet classification costs nothing detectable.
TLS records begin with ContentType `0x16`; every cleartext HTTP method token and the
HTTP/2 connection preface begin with an ASCII uppercase letter. One byte decides. The
paper stands or falls on the connect-to-first-byte distribution, not on the throughput
number.

**State.** Implemented and demonstrated. One `App` serves HTTP/1.1, HTTP/2 and HTTP/3
from one port number, holding one TCP and one UDP listening descriptor per worker.
Measurement is Phase 8.

**Honest limit to state in the paper.** Browsers will not speak cleartext to `:443`, so
the user-visible payoff is single-port ingress and a smaller socket table, not halved
ports. The descriptor count is a proxy metric. The classification cost is the real
measurable.

---

### 2. Portable QUIC connection-ID routing without eBPF

**Hypothesis.** Encoding the owning worker into the connection ID and forwarding
misdirected packets in userspace matches kernel-side `SO_ATTACH_REUSEPORT_EBPF`
steering within a bounded margin, and works where eBPF does not exist.

**Why it is worth asking.** `SO_REUSEPORT` spreads UDP datagrams by hashing the
4-tuple. QUIC connections survive their client changing address, by design, so after a
migration the hash sends packets to a worker that knows nothing about the connection.
nginx and Angie solve this with an eBPF program that reads the destination connection
ID in the kernel. That is faster where it is available and unavailable everywhere else:
no eBPF on macOS or Windows, and loading a program needs privileges a server often does
not have.

**What has to be shown.** The forwarding rate under realistic migration, and the cost
per forwarded packet. If the fraction of packets needing a handoff is small, then
kernel-side steering is optimising something that barely happens, and that is the
result rather than a gap in it.

**State.** Implemented, with counters in place: `received`, `forwarded_out`,
`forwarded_in`. Forwarding has never actually fired, because nothing in the test suite
migrates. Forcing migration needs `nft` SNAT port remapping in a network namespace,
which is Phase 8 work.

---

### 3. Readiness versus completion in one codebase: epoll against io_uring

**Hypothesis.** Comparing readiness and completion interfaces fairly requires holding
everything else constant, and doing so changes the conclusion relative to published
comparisons.

**Why it is worth asking.** Almost every published epoll-versus-io_uring comparison
compares two different programs and therefore measures the programs. Here both
backends sit behind one `IoContext` seam, serving the same routes through the same
parser on the same kernel and hardware, selectable at configure time rather than by
platform. The completion mechanism is the only variable.

**What has to be shown.** Throughput and tail latency across connection counts and
payload sizes, with the io_uring mode sweep (default, `SQPOLL`,
`DEFER_TASKRUN|COOP_TASKRUN`, registered buffers, multishot receive) as a subsection
rather than a separate result.

**State.** Both backends build and pass the full suite on the same machine, which is
what makes backend an independent variable rather than something confounded with the
operating system. No measurements yet.

**Note.** If a paper on this comparison already exists under the author's name, this
becomes an extension of it rather than a new topic, and the citation should be added
here.

---

### 4. DFA routing at server scale

**Goal.** Extend `stankov2024regex` from a matching benchmark to end-to-end server
throughput, with route-count scaling measured against radix trees and `std::regex`.

**Why it is worth asking.** The published result is about the matcher. A matcher that
is faster in isolation may or may not move a server, because routing is one step among
parsing, dispatch, handler execution and serialisation. Showing where the matching cost
sits in a full request is a different claim from showing the matcher is fast.

**What has to be shown.** Request throughput as the route table grows from a handful to
several hundred routes, against alternatives, with the share of request time spent
matching reported directly.

**State.** The matcher is integrated and in production use in the router. The sweep is
not built.

---

### 5. Awaited versus deferred data resolution in server-rendered UIs

**Hypothesis.** Streaming a page before its slowest field reduces time to first byte by
a margin worth having, and expressing the pending value as a type on both sides of the
wire is worth more than the streaming itself.

**Why it is worth asking.** Early flushing is not new: BigPipe did it in 2010. What is
new here is that the server-side `Deferred<T>` has a counterpart the page can `await`,
compose with `Promise.all`, and attach error handling to. A convention where the server
swaps some innerHTML can express none of those. The contribution is a typed contract
spanning a compiled language and a scripted one, in a coroutine runtime rather than a
JavaScript one.

**What has to be shown.** Time to first byte and time to complete render, awaited
against deferred, at equal offered load, plus the cost of the extra chunks.

**State.** Implemented and demonstrated end to end. A page whose slow field takes 800 ms
delivers its shell 716 ms before the value arrives. Both client paths ship on the same
page: `data-coroute-slot` for a reader without JavaScript, and `coroute.deferred(n)` for
the typed one.

---

### 6. Compile-time `.proto` parsing with C++26 static reflection

**Question.** Can P2996 static reflection replace `protoc` as a build step, parsing
`.proto` definitions at compile time into the same generated interfaces?

**Why it is worth asking.** Protobuf's code generation step is a build dependency, a
version-skew risk and a barrier to header-only distribution. If reflection can do the
same work in the compiler, the generator disappears. If it cannot, the reason why is
itself worth writing down, because it bounds what static reflection is for.

**State.** Nothing built, and deliberately so. This needs no server work at all, which
makes it the one topic that can be written first, in parallel with everything else. It
stays future work in the thesis rather than an implemented feature.

---

### 7. Zero-copy response paths across three operating system APIs

**Goal.** Compare `sendfile`, `TransmitFile` and io_uring `SEND_ZC` in one codebase,
across file sizes, and identify where each stops paying.

**Why it is worth asking.** Zero-copy is usually presented as strictly better. It has a
crossover: below some size the setup cost exceeds the copy it avoids, and that
crossover differs by API and by kernel. Naming it across three platforms in one server
is a practical result.

**State.** `util/zero_copy.hpp` exists. The comparison does not.

---

### 8. Coroutine-per-connection memory and tail latency

**Hypothesis.** A coroutine frame per connection changes the memory and tail-latency
profile relative to a thread stack per connection in ways that show up at connection
scale rather than at request rate.

**Why it is worth asking.** The usual argument for coroutines is stack size. The
interesting number is what happens to p99.9 under connection scaling, where allocator
behaviour and frame locality start to matter and a mean throughput figure hides
everything.

**State.** Needs measurement only. No implementation work.

---

### 9. A reproducible benchmarking methodology for multi-protocol servers

**Goal.** State a method for comparing servers that speak different protocols without
the comparison measuring the client, and without closed-loop measurement hiding the
tail.

**Why it is worth asking.** Two failure modes are common enough to be worth a paper.
Cross-protocol comparisons made with three different load generators measure three
different client implementations. And closed-loop measurement suffers coordinated
omission: when the server stalls, the client stops issuing requests, so the requests
that would have been delayed are never sent and never measured, and the reported p99 is
a p99 of service time rather than response time.

**What has to be shown.** That the method produces stable, reproducible numbers, and
that naive alternatives produce different ones. Several specific traps belong in it:
loopback is not protocol-neutral, because `lo` has an MTU of 65536 and gives TCP-based
protocols large segments with no segmentation and no checksums, while QUIC still emits
1200 to 1450 byte datagrams by design and pays full userspace crypto per datagram.

**State.** Not built. This is Phase 8, and it justifies the thesis' own methodology
chapter.

---

### 10. Compile-time route table verification

**Goal.** Extend the existing type-safe parameter work to catch route pattern and
handler signature mismatches at compile time rather than at first request.

**Why it is worth asking.** A route declaring two parameters and a handler taking one
is a defect that most frameworks discover in production. The type-safe parameter
machinery already in the router is most of what is needed to reject it at build time.

**State.** Type-safe parameters exist. The verification does not. This is the least
developed topic here and may not survive to submission.

---

## Sequencing

Topic 6 needs no server work and no measurements, so it can be written first and in
parallel with everything else.

Topics 1, 2, 3, 4, 5, 7 and 8 all need the measurement harness, so none of them can be
written before Phase 8 exists. That makes the harness the critical path for the
publication plan as much as for the results chapter, which is an argument for building
it before the remaining optional implementation work.

Topic 9 can be written once the harness exists, and is worth writing early because the
other measurement papers will cite its method.

## Open questions

None of these block writing, but all of them affect where things are submitted.

1. Minimum number of publications required, and whether any must be Scopus or Web of
   Science indexed.
2. Whether accepted-but-not-yet-published counts toward the requirement.
3. Whether co-authorship declarations are required, and in what form.
4. Whether the ПУРПНСТУС revision adopted on 20 November 2025 changes any of the above.
