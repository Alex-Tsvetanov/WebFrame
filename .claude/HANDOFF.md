# Coordinator handoff

Written 2026-09-02 by the coordinating session (Fable); last refreshed 04:55 EEST. A new session takes over
by reading this file first, then the memory notes `benchmark-machines`, `coordinator-role`
and `handoff-before-limit`, then `ListAgents` to find the remote sessions named below.
Run with ultracode on. Update this file at every milestone; it is the only place the
cross-machine state is written down.

## The job

Coordinate the PhD thesis in this repo (`doc/thesis/`, builds with `latexmk -pdf Main.tex`
in that directory, 148 pages) and the four papers in
`~/Documents/GitHub/Publications/paper-*` (each its own git repo; `INDEX.md` is shared
byte-for-byte across the four, `BLOCKERS.md` is per paper). Remote sessions do machine
work; this session writes, reviews, merges. Method rules that are never relaxed: one
binary with runtime flags, never two builds; not loopback for anything load-bearing;
pre-declared validity gates, unknown is not clean; no paper drafts ahead of data
(the papers were reset to zero measurements on 2026-09-01 for exactly that reason).

## Machines and sessions

| Session name | Host | What it is for | State |
| --- | --- | --- | --- |
| `Windows machine` | the desktop: Ryzen 5 3600, Windows 11, WSL2 Ubuntu-24.04 hosts the generator (its user is not the Windows user; read `$HOME` from the distribution); repo path and addresses are in the local memory note `benchmark-machines` | IOCP measurements, the campaign nights | released for tonight; running steps 0-6 below |
| `ArchLinux` | the laptop: Ryzen 7 5800H, Arch, **since 2 Sep ~13:00 on the stock kernel 7.2.2 with `tsc=nowatchdog`**, so clocksource tsc, io_uring unprivileged, perf_event_paranoid 2, governor performance (set by hand, not persistent); perf, strace, bpftrace, passwordless sudo, wired link; repo `~/GitHub/PhD-WebFrame` | Linux measurements proper, for the first time: the daemon survived the reboot under the same name | brief 5 running: clock gate, ff, mechanism run unprivileged, then the io_uring timeout experiment |
| `Dispatch background conversation` | unknown | never answered a probe | ignore |

Both hosts are Alex's daily machines. No timed run without a window Alex names.

**Machine sessions must never block on a question.** Alex is often unavailable, and a
session that calls AskUserQuestion stops processing everything, including messages from
here. Every brief repeats: never ask Alex or wait on him; send the question to the
coordinator with the default you will take and continue with it after 10 minutes; push only
to the branches the brief names; never rebase, force-push or modify `phase0-foundation`;
never change the system or enter a sudo password (skip and report); a blocked permission
prompt is reported and skipped; when unsure, stop that step and report. Both sessions were told; the desktop session keeps its own
channel to Alex on principle, which is fine because Alex pre-authorised its whole sequence.

**This repository is public.** (Enforced the hard way twice: the runbook I wrote named the
desktop by hostname, on both this branch and the harness branch, and a review lens caught it.
Grep every branch before pushing, not only the diff.) Nothing that identifies a machine beyond its hardware class
belongs here or in any commit, branch or inbox file on it: no hostnames, addresses, SSIDs,
login state or security posture. Those live in the local memory notes. Measurement result
branches (whose environment records carry the host name) go to the private paper
repositories, never here.

## Repository state

`phase0-foundation` = `8d5b2d6d4`, pushed. **No branch is in flight; everything is merged and
every branch deleted.** Tonight added, in order: the runtime backend selection (`--io-backend`,
verified on Linux, macOS and Windows/MinGW before merging); the harness's 21 Linux-readiness
gaps; a build-staleness preflight and the matcher commit in the environment; two ladder designs
rebuilt from the tables they validate; the clock sampled inside the measured window and read
from the fastest core; the kernel clock source recorded and fingerprinted; and a network
namespace pair, a perf syscall counter and the eleven gate defects that came with them.
Verified at the merge: 294 selfchecks, 178/178 tests, the census, and a loopback smoke campaign
whose records carry schema 6.

Still to do, needing a Linux host and now unblocked by the merge:

- **The io_uring timeout experiment**, authorised: raise `uring_context.cpp:492` from 1 us to
  about 1 ms on its own branch off this HEAD, measure before and after identically, and measure
  the cost the comment at `:489` names (added latency on the first request after an idle gap)
  as well as the saving. A blocking wait when nothing is in flight is the better design and is
  for after the one-line version has shown the size of the effect.
- A churn rate ladder to confirm the ~8.7 clock reads per established connection, and the
  deadline machinery as the suspect for them.
- The QUIC UDP-seam reconciliation between `feature/http3-quic` and HEAD (paper 4, the cheapest
  next step per its BLOCKERS).

## Desktop sequence sent 02:20 (message id 5461cf03)

0 pull to `aee6cea19`; 1 MSVC compile check of `linux/io-backend-runtime` in `..\wt-iob`;
2 rebuild `build\windows-tls` and `build\windows-routing`, check WSL loadgen, resolve the
vEthernet gateway, certs, idleness; 3 `smoke` quiet-host gate (pacing p99 tens of µs or
stop); 4 four ladders with a decision rule (all table rates accepted below 500 µs pacing,
else stop and report); 5 `churn` → `churn-net` → `transport` → `h1-deep` at n=25, results
in `benchmark\results\2026-09-02-desktop\`, one message per design, stop at a design
boundary if Alex wants the machine or if a design rejects >10 %; 6 commit results LOCALLY on
`measure/desktop-2026-09-02` with `git add -f`; the desktop session pushes nothing (its own policy, and Alex's
wording was that the coordinator places records in the paper repos). **Morning step for Alex: push that
local branch to the private `paper-socket-demux` repository**, then the coordinator copies the files into
`measurements/`. About 13 hours. The desktop pulled `4645e5e03` (one docs commit past `aee6cea19`,
code identical), so its records carry that hash. It reports by message only and declined the
`coord/inbox` practice; the laptop follows it.

When results arrive: copy each `.jsonl` + `.env.json` into
`Publications/paper-socket-demux/measurements/` with a README entry (machine, commit,
command, what it does not show), following the existing entries there. Chapter VI of the
thesis regenerates from them: `python -m benchmark.harness.results2csv <runs.jsonl>
doc/thesis/data` and `results2tex <runs.jsonl> doc/thesis/generated/results.tex`; the
`\R{}` keys the chapter expects are `campaign.sweeps.*`, `campaign.x1.*`,
`h1detect.*`, `tail.x1.*` (grep the chapter). Routing (night 2 in the runbook) has not
been scheduled yet.

## Findings that came out of tonight, worth keeping

- **A committed record is not the same bytes as the record on disk, and it mattered.** The
  repository normalises line endings (`* text=auto`), so records written on Windows with CRLF
  were stored as LF and restored as CRLF. The twelve data and environment files round-tripped
  byte-identically because fixed-width JSONL had nothing to convert; **four notes files did
  not**, so `git show <commit>:<path> | sha256sum` and `sha256sum <path>` disagree on them.
  With provenance by hash that is a paper citing something a reader cannot reproduce. Fixed at
  `cb56895da`: `benchmark/results/** -text`, so the bytes the harness wrote are the bytes
  stored. The corrected hashes for those four files are in the desktop's report; the sixteen
  others are unchanged.
  The same episode produced a second lesson worth keeping: after committing on a results branch
  and checking out the working branch, the force-added files vanished from the working tree,
  because they are tracked on one branch and ignored on the other. Nothing was lost, but **"the
  files are on disk" and "the files are in the commit" had stopped being the same statement**
  while a report asserted the first.

- **The coarse-clock change is merged and its before-and-after table accounts for itself.**
  Churn, epoll, 400/s, 10 000 connections each side, with futex counted (`c836cd9b4` adds that
  tracepoint permanently, lifting named coverage on this shape from 77% to 96.5%). Five
  tracepoints are identical to three decimals before and after (accept, close, epoll_ctl,
  recvfrom, sendto), which is the evidence the change altered only how often the clock is read.
  `clock_gettime` falls 4.252 per connection; the non-clock delta of 1.464 is now **fully
  named**, futex -1.073 and epoll_wait -0.392 summing to 1.465, and **the unnamed remainder is
  1.065 before and 1.065 after, unmoved to three decimals**. Removing four kernel entries per
  connection removed four preemption points, so the server contends and wakes measurably less.
  Keep-alive: clock reads 2.002 to 0.001, total 7.083 to 5.046, futex 0.002 throughout, which
  is consistent since a held-open connection gives the timer thread almost nothing to wake
  anyone about. The two before/after pairs (this one and the earlier one without futex) are
  fresh runs and **must not be mixed**: -4.252 against -4.308, same direction and size.
- **A usability trap in the staleness gate, now in the runbook.** Its whole-tree fallback
  compares every binary against every compiled source, so editing a header that only one binary
  includes refuses the other, and touching that source to clear it moves the refusal to the
  first. Delete and rebuild both after the last edit, or use a Ninja tree, where the gate asks
  the build system per target instead of comparing timestamps. It fails closed, which is right,
  but the obvious remedy makes it worse.

- **The ratio has four defensible values and the spread is wider than the effect.**
  On the one run that produced it: 4.264 raw; 4.116 with the clock rows subtracted (5.132
  against 21.125); and after the coarse-clock change, 4.821 raw and 3.843 with clock rows
  subtracted. **Quote 4.116**, because on a healthy host those reads never enter the count at
  all, and say why rather than just printing the digit. Do not compute it the way the
  coordinator first did, by taking totals from one run and clock counts from another; that
  gave 4.176 and mixed two populations, which is the failure this project keeps finding.
  **The subtraction is not exact and that is the point**: removing 4.308 clock reads per
  connection removed 5.945 syscalls, so about 1.64 non-clock syscalls went with them, and an
  arithmetic subtraction of one row is therefore not the same measurement as deleting the
  calls and re-running. Attribution of those 1.64 is queued.

- **The reboot is not optional, and the clock is not why.** The hardened kernel confines
  io_uring to root and the governor question is unsettled, so every timing number that host
  produces is already pipeline validation rather than data, and no admissible Linux figure can
  exist until it boots a different kernel. `tsc=nowatchdog` is one extra word on a boot line
  that has to be typed anyway. So the axis is not reboot-versus-no-reboot but **not-today
  versus never**, and nothing in the plan needs that machine before the stock kernel lands.
  There is also an internal-consistency argument: the methodology chapter now states in
  writing that a host reporting hpet is a host to fix rather than a number to correct, so
  publishing timing from an unfixed one would contradict the method stated in the same
  document.
- **Considered and rejected: removing the idle-timeout clock read entirely.** The timeout only
  answers whether any byte moved during the last period, which looks like an atomic flag rather
  than a timestamp, and that would be portable with no conditional and a smaller diff. It does
  not survive the re-arm path at `idle_timeout.hpp:151-185`, which computes
  `remaining = period - quiet` and re-arms for exactly that, so the deadline fires close to one
  period after the last activity. A flag cannot know how long ago the activity was, so it fires
  between one and two periods, and the same path carries the handshake deadline, which is a
  slowloris guard. Keeping the bound tight means ticking at half the period, which doubles calls
  to `TimerQueue::schedule` at 2.01 clock reads per connection, giving about half the saving
  back. The coarse clock keeps the deadline within a millisecond and saves the whole 4.00.

- **The clock reads are attributed to three sites, none needing nanoseconds.** Call-graph
  profile of the server process only (the generator is a separate process in the other
  namespace, so the separation is structural), epoll churn, 8000 connections: `IdleTimeout`'s
  `now_ns()` 4.00 per connection (41%, inlined at three call sites), the `TimerQueue` run loop
  at `timer_queue.hpp:107` 3.68 (38%), and `TimerQueue::schedule` at `:55` 2.01 (21%). All
  three are deadline arithmetic at millisecond-to-second scale against a 30 000 ms timeout.
  Nothing measurement-grade appears at all, so every read on that path is bookkeeping.
  **Authorised: change `idle_timeout.hpp:126` `now_ns()` itself**, not just `touch()`, since
  the expiry check at `:169` reads the same function and both sides of the subtraction must
  share a clock. Worth 4.00 of 9.69 per connection.
  **The `TimerQueue` pair is closed, by measurement, and not in the way either of us
  expected.** `cv_.wait_until` makes exactly one clock read per wait whether the deadline is
  future or already past (2000 waits, 2001 reads, and 2001 futex calls, so it enters the kernel
  even for an expired deadline), so waiting unconditionally and popping on the timeout return
  does not remove that read. And more decisively, **`pthread_cond_clockwait` accepts
  `CLOCK_MONOTONIC` and `CLOCK_REALTIME` and rejects both coarse clocks with EINVAL**, so a
  deadline a condition variable will wait on cannot live in the cheap clock at either site.
  The 59% is therefore not reachable by substitution under any arrangement that keeps
  `condition_variable`; it is a hard API constraint rather than a design trade. The restructure
  would recover only the explicit read, estimated at about 1.84 per connection, and that figure
  is an inference from the arithmetic (three buckets summing to 100%, one read per wait) rather
  than a measurement, resting on an assumption a pop-without-waiting iteration would break.
  **Future work, noted and explicitly not authorised:** an absolute-deadline timerfd in the
  epoll set removes the timer thread, its wait and both its reads, since the event loop is
  already blocked in `epoll_wait` and the kernel would do the waiting for free. That is a
  redesign with no counterpart on kqueue or IOCP.
  **Thesis note:** the EINVAL constraint belongs in the implementation chapter. The obvious
  optimisation is unavailable because the waiting primitive constrains which clock a deadline
  may live in, so the cost is structural for any timer built on a condition variable, and that
  is measured on the platform rather than asserted from documentation.
- **A ladder separated two explanations a single point could not, for the third time.** The
  same line was first guessed to be a fixed-rate reader, withdrawn when the keep-alive ladder
  showed no floor, and is now confirmed as the largest site: the loop is event-driven, so its
  wakeups scale with timer operations and therefore with connections, giving per-connection
  cost and no per-second floor. Right line, wrong mechanism, and only the ladder could tell.

- **First campaign design complete: `churn`, 400 runs, 394 accepted, 6 rejected (1.5%),**
  2 h 45 m on the desktop at `4645e5e03`. Rejections, all individually explicable and none
  showing the host degrading: three single socket errors in otherwise healthy runs (pacing 94,
  90, 62 us), one frequency-drift refusal at 5.3% doing exactly its job, and two pacing
  overruns both at the lowest offered rate. **The 150-rate fear did not materialise**, 99 of
  100 accepted with the single failure a socket error rather than an admissibility one, so the
  ladder gap mattered less than expected; worth a sentence in the results.
- **CAMPAIGN COMPLETE: three loopback designs, 1150 runs, 13 rejected (1.1%).** `churn`
  400/394, `transport` 500/499, `h1-deep` 250/244, committed locally on the desktop as
  `71baff233` on branch `measure/desktop-2026-09-02`, 20 files, 1184 records, **pushed
  nowhere**. Every loopback `.env.json` shares one hash and only the network ladder's differs,
  which is the transport-path separation working.
- **The "lowest rate is most fragile" generalisation is dead, and the analysis that killed it
  produced something better.** It was drawn from one design plus one run; `h1-deep` refutes it,
  since all four of its pacing failures are at 40 000 requests a second and above and its
  median pacing rises monotonically with rate (47.5, 47.0, 51.5, 60.0, 80.0), the opposite
  direction from churn. Comparing the whole distribution rather than the tail, with
  `generator_cpu_fraction` alongside, separates **three phenomena that had been conflated**:
  (1) an unsaturated generator, which is churn at 25/s and the only such cell in the campaign
  at 0.257 CPU share, where the entire distribution moves (median 94.5 against 71.0 at rate
  150) rather than only the tail; (2) a generator near its ceiling, `h1-deep` at 40 000 and
  above, degrading with load in the ordinary way; and (3) a rare isolated event needing no
  mechanism at all, `transport` at 5000, which is saturated at 0.996 with median, p90 and p99
  indistinguishable from every other cell and only one run's maximum standing out. Both earlier
  explanations, the coordinator's sampling story and the desktop's saturation story, were
  reaching for case 1 and were wrong about the other two.
  Still open and stated as such: within case 1, sleep granularity and descheduling both predict
  a low CPU share at a low rate, and separating them needs per-sample wake latency, which is not
  recorded. One suggestive record in case 2: the worst run in 1150 lost about 16% of its CPU
  share and fell nearly a second behind, which looks like descheduling rather than slowness.
- **A sentence already in chapter VI was false in general and is corrected** (`db66ac674`): the
  generator's CPU share is unity *while saturated*, not at every offered load. It stays refused
  as a rejection criterion for the reason it always gave, and becomes the diagnostic that
  separates a saturated arrangement from an unsaturated one. A rule and an observation are not
  read the same way.
- **Second design complete: `transport`, 500 runs, 499 accepted, 1 rejected (0.2%)**, 3 h 27 m.
  Third, `h1-deep`, started 09:52 local.
- **A half-specified off-host arrangement runs silently as an on-host one.**
  `run_campaign.py` reads `--wsl-loadgen` only inside `if args.wsl_distro:`, so passing it
  alone is ignored without a word, while `--host` is read unconditionally. The opposite
  direction errors out. So a generator flag that was meant to move the load off the host is
  dropped, the host-side generator runs against a foreign gateway address, and the result is a
  plausible number rather than a refusal. **The harness branch's `--generator-command` /
  `--generator-location` pair must refuse the half-specified case in both directions.** Found
  when a coordinator instruction wrongly added those flags to the loopback smoke gate and the
  desktop read the code instead of obeying: the quiet-host gate measures whether the host can
  pace a generator, so it is loopback by design, and only `churn-net` and `churn-ladder-net`
  take the off-host flags.

- **The harness records no binary provenance.** `_FINGERPRINTED` carries `build.git_commit`,
  `build.type`, `build.io_backend`, `toolchain.compiler` and four dependency versions, and
  nothing about the executable: no hash, no mtime, no size. `git_commit` describes the source
  the tree is checked out at, never the source the binary was built from. The desktop found
  this the hard way: every build tree there was pinned to the repository's old path
  (`D:\GitHub\...` before it moved), so no tree could be incrementally rebuilt and the
  binaries were frozen at 30 August while HEAD is 35 commits later. A campaign run without
  noticing would have stamped records with a commit whose validity gates the binary predates.
  **To add: a preflight that refuses when an executable is older than the commit the record
  will claim OR older than its own `CMakeCache.txt`.** Both halves are needed, and the desktop
  proved why: that routing tree's binary is dated 30 Aug 11:43, its cache holds a value that
  only became the default at 15:17, and the variable holding it did not exist until 14:19. The
  binary predates its own cache by three and a half hours, so the tree was reconfigured and
  never rebuilt, and anyone reading that cache to learn what the binary was built from would
  have been wrong. A check against the commit alone catches the frozen-tree case; only the
  cache comparison catches reconfigured-but-not-rebuilt, which is the one that actually
  happened. **The check covers the generator too**: three stale generators were found inside
  the desktop's WSL, the newest two days older than the commit it would have driven, one named
  for the TLS arm and older than the commit that added it. Note also that FetchContent bakes
  the absolute source path into every `_deps/*-subbuild/CMakeCache.txt`, so a repository that
  has moved leaves one stale cache per tree plus one per vendored dependency, and clearing the
  top-level cache alone is not enough.
- **The matcher commit is not recorded either.** `COROUTE_URL_MATCHER_TAG` decides the DFA
  router's performance and is the dependency the routing paper's claim turns on, yet the
  environment captures `openssl`, `ngtcp2`, `nghttp3`, `liburing` and not it. **To add: read it
  from `CMakeCache.txt` beside the others** (same shape as `resolve_io_backend`).
  Consequence found tonight: the desktop's routing tree still had `2220b61b`, which looked like
  a deliberate pin and is a fossil, the repository's own default until `48ec1c81a` on 30 Aug;
  three pins have landed since. The 30 August routing records were produced with it, and
  nothing in them says so.
- **Syscalls per request is not a valid normaliser for io_uring as implemented, and that
  is the mechanism finding.** Measured on the laptop over the network-namespace pair, four
  cells, every one meeting its offered rate (keep-alive 10000/s, churn 400/s):
  `io_uring_enter` fires about 230 000 times a second across four workers whatever the load,
  so between two cells whose request rate differs 25-fold its rate differs 1.04-fold. Dividing
  it by requests yields 25.7 and 586.5 "syscalls per request", which are arithmetically correct
  and measure the poll loop rather than the work. **Do not quote those ratios, or the earlier
  loopback figures, as per-request comparisons.** The earlier probe's "io_uring_enter at 4.752
  per request" was the same misreading: 950 000 enters over 13 seconds with one worker is about
  73 000 a second, and it only resembled a ratio because the load happened to be 20 000 requests
  a second. Retracted by its author before anyone quoted it.
  What survives is stronger and belongs in the paper: **epoll's syscall count tracks work and
  io_uring's tracks time.** epoll costs 1 `recvfrom`, 1 `sendto`, 2 `epoll_ctl`, 2 `clock_gettime`
  per request under keep-alive, plus 1 `accept4` and 1 `close` under churn, with churn costing
  4.264x keep-alive; those figures are stable and interpretable. **Quote the ratio with the
  host's own overhead removed**: `clock_gettime` is 2.002 of the keep-alive syscalls and 9.293
  of the churn ones, so excluding the clock rows it is about 5.09 against 21.24, a ratio of
  roughly 4.18 rather than 4.26. The conclusion survives, which is why it was worth checking,
  but both figures belong in the text with the clock source named. The completion model as
  implemented pays a per-second price, not a per-request one, so any syscall comparison between
  the two models must normalise by time, or by time and worker count, never by requests. It also
  points somewhere concrete: about 58 000 enters per worker per second while 400 requests a
  second arrive is a loop that is almost entirely empty, which suggests a short-timeout spin
  rather than a blocking wait. Measure before tuning; nothing in the backend has been touched.
  One epoll detail worth its own look: `clock_gettime` rises from 2.002 per request under
  keep-alive to 9.293 under churn, and unlike the io_uring number it does scale with work.
- **A half-specified off-host arrangement runs silently as an on-host one.**
  `run_campaign.py` reads `--wsl-loadgen` only inside `if args.wsl_distro:`, so passing it
  alone is ignored without a word, while `--host` is read unconditionally. The opposite
  direction errors out. So a generator flag that was meant to move the load off the host is
  dropped, the host-side generator runs against a foreign gateway address, and the result is a
  plausible number rather than a refusal. **The harness branch's `--generator-command` /
  `--generator-location` pair must refuse the half-specified case in both directions.** Found
  when a coordinator instruction wrongly added those flags to the loopback smoke gate and the
  desktop read the code instead of obeying: the quiet-host gate measures whether the host can
  pace a generator, so it is loopback by design, and only `churn-net` and `churn-ladder-net`
  take the off-host flags.

- **The harness records no binary provenance.** `_FINGERPRINTED` carries `build.git_commit`,
  `build.type`, `build.io_backend`, `toolchain.compiler` and four dependency versions, and
  nothing about the executable: no hash, no mtime, no size. `git_commit` describes the source
  the tree is checked out at, never the source the binary was built from. The desktop found
  this the hard way: every build tree there was pinned to the repository's old path
  (`D:\GitHub\...` before it moved), so no tree could be incrementally rebuilt and the
  binaries were frozen at 30 August while HEAD is 35 commits later. A campaign run without
  noticing would have stamped records with a commit whose validity gates the binary predates.
  **To add: a preflight that refuses when an executable is older than the commit the record
  will claim OR older than its own `CMakeCache.txt`.** Both halves are needed, and the desktop
  proved why: that routing tree's binary is dated 30 Aug 11:43, its cache holds a value that
  only became the default at 15:17, and the variable holding it did not exist until 14:19. The
  binary predates its own cache by three and a half hours, so the tree was reconfigured and
  never rebuilt, and anyone reading that cache to learn what the binary was built from would
  have been wrong. A check against the commit alone catches the frozen-tree case; only the
  cache comparison catches reconfigured-but-not-rebuilt, which is the one that actually
  happened. **The check covers the generator too**: three stale generators were found inside
  the desktop's WSL, the newest two days older than the commit it would have driven, one named
  for the TLS arm and older than the commit that added it. Note also that FetchContent bakes
  the absolute source path into every `_deps/*-subbuild/CMakeCache.txt`, so a repository that
  has moved leaves one stale cache per tree plus one per vendored dependency, and clearing the
  top-level cache alone is not enough.
- **The matcher commit is not recorded either.** `COROUTE_URL_MATCHER_TAG` decides the DFA
  router's performance and is the dependency the routing paper's claim turns on, yet the
  environment captures `openssl`, `ngtcp2`, `nghttp3`, `liburing` and not it. **To add: read it
  from `CMakeCache.txt` beside the others** (same shape as `resolve_io_backend`).
  Consequence found tonight: the desktop's routing tree still had `2220b61b`, which looked like
  a deliberate pin and is a fossil, the repository's own default until `48ec1c81a` on 30 Aug;
  three pins have landed since. The 30 August routing records were produced with it, and
  nothing in them says so.
- **Syscalls per request is measurable and has been measured** (laptop, loopback, one worker,
  pipeline validation only, corrected once by its author): epoll keep-alive 6.514, io_uring
  keep-alive 6.759, epoll churn 29.713, io_uring churn 27.097 syscalls per request. So churn
  costs 4.562x keep-alive on epoll and 4.009x on io_uring, and the backends differ by +3.76%
  (io_uring worse) in keep-alive and -8.80% (io_uring better) in churn. **Shape dominates
  backend, because 4x dwarfs 9%, and that is the part to carry forward.** Do not repeat the
  first readback, "under 4% within a shape" and "about 4.5x on both": the spread is
  shape-dependent and the sign flips between shapes. The churn difference is **confounded and
  must not be quoted as a controlled comparison**: both churn cells were throughput-limited
  below the offered rate and achieved different rates (10512 against 11005), and a server
  batches differently at different load. A rate both arms sustain, on the desktop, is what
  would settle it. The direction, if it survives, is a hypothesis for the portability paper:
  the ring's fixed per-enter cost may amortise badly across a keep-alive request needing one
  receive and one send, and well across a churn request that also accepts and closes. `io_uring_enter` at 4.75 per
  request in keep-alive is a finding about this implementation, not about io_uring.
  Instrument: `perf stat` on the server pid, which needs root because tracefs permissions
  block it whatever `perf_event_paranoid` says; `strace -c -f` costs 82% of throughput and is
  a discovery tool only. Two quiet failures to guard against: a pid that is the `sudo`
  wrapper, and `/proc/PID/comm` truncated to 15 characters; both give a confident wrong
  answer, so assert `raw_syscalls` is non-zero before believing a cell, and make the perf
  window match the measured window rather than overhang it.

- **Every Windows binary in this project is MinGW-w64 g++, not MSVC**, including all
  committed records (`toolchain.compiler` in the environment files says so, and it is
  fingerprinted). Chapter V now states it, since a reader meeting "Windows" assumes MSVC.
- **The load generator could never be compiled by MSVC**: it is deliberately unlinked from
  the library, so it inherited no `NOMINMAX`, and `windows.h` turned `std::min(` into
  C2589. Fixed in the source at `40b5dd21e`. It failed on every branch and was invisible
  because nobody had pointed MSVC at the tree. The runtime-backend seam itself compiled
  clean under MSVC; 270 of 273 targets built.
- The MSVC compile check is therefore informational. **The gate that matters is the MinGW
  build matching `build/windows-tls`**, because that is the toolchain every record comes
  from.

## Decisions taken today, with reasons

- Campaign tree is the desktop's tracking checkout at HEAD, not its `-measure` checkout
  (detached at `b14002aab`, which predates the three gate fixes of 1 Sep).
- Laptop timing numbers are pipeline validation only until Alex boots stock `linux`
  7.2.2 with the `performance` governor; the harness will get a run-level governor gate.
- Same-hardware Linux vs Windows does not exist (two machines); cross-platform is shape
  only. Recorded in all four `INDEX.md` ("Update, 2 September 2026").
- The stale wrk-era `benchmark/README.md` was replaced by the runbook; the old text is in
  history at `068431b37`.
- `Compile-time-Protobuf` has uncommitted rebuilt PDF/DOCX/presentation; not touched.

## For Alex, in the morning: one urgent action, then four decisions

**Do item 0 first. Everything else can wait; that one cannot.**

0. **PUSH THE RESULTS BRANCH. The campaign exists in one place.** `71baff233` on
   `measure/desktop-2026-09-02` on the desktop, 1184 records from three designs and eight
   hours of machine time, is a local unpushed commit plus a copy on the same disk in the same
   tree. No remote, no second machine. One disk failure loses the night. Push it to the
   private `paper-socket-demux` repository (add it as a second remote; it must not go to the
   public framework repo, because the environment records name the host). The desktop pushes
   nothing by its own policy, so this is the one step nobody else can take.

1. **The Windows firewall is blocking the off-host arm.** Two auto-created inbound block
   rules for the freshly built server binary, which is why every loopback design ran and
   `churn-net` delivered 0.0%. Several matching rules still name the repository's old path, so
   they have accumulated since the move. One rule scoped to the WSL subnet, or deleting the two
   blocks, unblocks it. No session touches a firewall, so this waits for you.
   **When it is cleared, the next run is the ladder, not the design.** The network ladder
   accepted zero of eight runs, every one for delivering 0.0% of its offered rate with 64
   socket errors, so it measured nothing and **validated nothing**: the four network rates are
   exactly as untested as before it ran. Its zeros are evidence about a firewall, not about
   rates. The loopback rates that did pass, 25 to 100 establishments a second, say nothing
   about the off-host arrangement, whose whole purpose is to reach rates loopback cannot and
   which the design's own docstring says is not comparable for magnitude. So: clear the
   firewall, **pull first** (the ladder on this HEAD is built from the tables it validates, so
   it now covers all four network rates where the version that ran covered three), rebuild,
   re-run `churn-ladder-net`, and only then `churn-net` at n=25. Running the design on
   unvalidated rates would spend eight hours being refused one run at a time, or worse produce
   a table whose rates were never shown admissible.

2. (DONE 2 Sep ~13:00: stock kernel, `tsc=nowatchdog`, clocksource tsc verified) **The Linux laptop's TSC was disqualified at boot**, so its clock source is HPET and every
   `clock_gettime` costs 1931 ns and a syscall instead of about 20 ns in userspace. That
   inflates every syscall count and every latency that host produces, by roughly 3.3
   microseconds a request. `tsc=nowatchdog` on the kernel command line is the usual remedy for
   this AMD false positive; the dmesg evidence is in
   `coordination/inbox/alex-laptop-2026-09-02T0122Z.md` on `coord/inbox`. A host reporting
   hpet is a host to fix rather than a number to correct afterwards, and fixing it removes a
   stated limitation from every Linux measurement.
   **Established since:** it cannot be done without a reboot. Writing `tsc` to
   `current_clocksource` makes `tee` exit 0 and changes nothing, and the kernel logs
   "Override clocksource tsc is unstable and not HRT compatible - cannot switch while in
   HRT/NOHZ mode". **The kernel's own advice in the boot log, `tsc=unstable`, is the opposite
   of what is wanted**: it asks for the state the machine is already in, so following it would
   reboot and fix nothing. The parameter is `tsc=nowatchdog`. The bootloader is GRUB, so it
   goes in `GRUB_CMDLINE_LINUX_DEFAULT` in `/etc/default/grub` followed by `grub-mkconfig`;
   there is no systemd-boot loader entry.
   **A reboot does not strand the machine**, which was the worry: `claude-daemon.service` is a
   lingering user unit with `Restart=always`, `Linger=yes` is set, and there is no disk
   encryption, so the user manager and the daemon start at boot with nobody logged in. It has
   survived one restart already. But the current session runs from a terminal window rather
   than the daemon and does die, and a cold boot has not been proven end to end, so **the
   first reboot happens while Alex is at the keyboard, not while he is away.**
   **Post-reboot gate, not optional:** `current_clocksource` must read `tsc`, and the clock
   ladder must show about 20 ns and zero syscalls per read instead of 1931 ns and one. A
   genuinely skewing TSC produces wrong timing rather than an obvious failure. Evidence that
   this is the known false positive: the watchdog objected to 494302697 against 496024343 ns,
   0.35%, on a single reading immediately after a remote-CPU read timeout.
3. (Alex set it to `performance` by hand after the reboot; not persistent, and whether a campaign may set it itself is still his call) **May a campaign set the governor to `performance` for its duration and restore it after?**
   The laptop is already there; this is about making it explicit and repeatable rather than
   incidental.
4. (now item 0, above; done first) The desktop pushes nothing, by its own policy and your
   wording that the coordinator places records in the paper repositories. Push that branch to
   the private `paper-socket-demux` repository and the coordinator will file the records with
   their provenance.
5. **`Compile-time-Protobuf` has uncommitted rebuilt PDF, DOCX and presentation files**,
   untouched all night. Commit or discard.

Worth reading whatever you decide: the six reports on `coord/inbox`, which carry the night's
findings in the laptop's own words, unredacted as you authorised.

## Next steps, in order

1. (done: merged as `a9c964cf6`, branch deleted)
2. (done: merged as `904778889`, branch deleted; macOS 227 selfchecks, 178/178, census, smoke)
3. Desktop reports per design → ingest into paper-socket-demux, regenerate chapter VI,
   check the red `[?key]` markers shrink, commit thesis data snapshots (`doc/thesis/data`
   CSVs are meant to be committed; `generated/` is not).
4. Schedule routing night (runbook night 2) with Alex; dispatch-only can also run on the
   laptop once the governor is `performance`.
5. With Alex's root decision: laptop netns pair + first admissible Linux run of `churn`
   and `h1-deep` on epoll (and io_uring once permitted); perf syscall counter (the
   "mechanism" both papers 2 and 3 rest on).
6. Then paper 4's UDP-seam reconciliation on a scratch branch (design work, Linux).

## Succession, automated (set up 02:30 EEST)

The switch is mechanical, so it needs nobody awake:

- The primary session refreshes
  `~/.claude/projects/-Users-Alex-Tsvetanov-Documents-GitHub-PhD-WebFrame/coordinator-alive`
  every 20 minutes (a session cron). Its first line is a UTC timestamp.
- A scheduled task `coordinator-successor` (`~/.claude/scheduled-tasks/coordinator-successor/SKILL.md`,
  `model: opus`, every 30 minutes while the desktop app is open) reads that file. Younger
  than 75 minutes: it stops. Older: it takes over — writes `coordinator-successor-lock`
  (2 h TTL) beside it, reads this file and the memory notes, reads new reports on
  `origin/coord/inbox`, tells `ArchLinux` and `Windows machine` its session name, works
  the next steps for at most about an hour, refreshes this file, pushes, deletes the lock.
  Each run is stateless; this file is the state.
- Machine sessions write every report as a file on the orphan branch `coord/inbox`
  (`coordination/inbox/<host>-<UTC>.md`) in addition to messaging, so a report that
  arrives between coordinator sessions is not lost. The laptop does this (full text, at Alex's choice); the desktop reports by message only, so a successor must ask it to re-send.
- A probe on 02:21 confirmed a scheduled session runs as `claude-opus-5` with that
  frontmatter and can message peers.

If a human takes over instead: open a session on Opus with ultracode on, say "read
.claude/HANDOFF.md and continue", and disable the scheduled task in the sidebar.
