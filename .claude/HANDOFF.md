# Coordinator handoff

Written 2026-09-02 by the coordinating session (Fable); last refreshed 14:05 EEST. A new session takes over
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

## In flight at 14:05, 2 September (a successor picks these up)

- **Desktop, sequence sent 13:55 (message 0a356a7a), Alex approved the full ~8 h program
  directly.** Steps 0-2 done by 14:40: pulled to `b4a01e8c7`, both trees and the WSL generator
  rebuilt from it (the environment record now carries the matcher commit beside the dependency
  versions, closing the 03:00 gap in the record itself), smoke 2/2 at 53 and 51 us, and the
  network ladder **11 of 11 accepted** where last night it was 0 of 8: all four network rates
  admissible (50 at 64 us, 150 at 56, 400 at 54, 800 at 468). 800 is marginal, five times the
  next worst cell and 6.4% under the proceed threshold; **ruled in advance that refusals
  confined to 800 are the boundary, not the host, and are not a stop reason**; refusals at 50,
  150 or 400 are. `churn-net` at n=25 running from 14:40, ~2.9 h. Original sequence follows:
  pull to HEAD and rebuild both trees and the WSL generator; `smoke`; `churn-ladder-net` with the
  four-rate decision rule; `churn-net` n=25 (paper 2's network arm, ~2.9 h); routing e2e
  `main`/`bracket`/`bracket-low`/`large` then dispatch `main`/`scaling`/`depth`/`static`/
  `large-cheap`/`large-dfa` (paper 1's entire dataset, ~3 h); the `h1` sweep design at n=7
  (the thesis's three red sweep tables, ~1 h). Three results directories
  `2026-09-02-desktop-{net,routing,sweeps}`, each on its own local `measure/` branch, pushed by
  nobody but Alex. File each into the paper repos the way `2026-09-02-desktop/` was filed.
- **Laptop, brief 5 (message d19d2f15) on the repaired host:** clock gate passed (20.7 ns, zero
  syscalls); tree forwarded; two io_uring tests fail on `RLIMIT_MEMLOCK` (8 MiB admits two
  four-worker contexts, ctest ran them in parallel; fix is a ctest `RESOURCE_LOCK`, queued, not a
  code change); pair recreated; D (smoke over the pair, record the repaired environment), E
  (four-cell mechanism comparison, both arms unprivileged), F (the io_uring timeout
  experiment, one line at `uring_context.cpp:492`, before/after with the idle-gap latency cost
  measured) are running in that order.
- **Status at 17:30 (all three local workflows landed):**
  - Chapter VI rewritten against the real campaign (baf85cec5, 1f5fd1740: 118 sentences; the
    two-campaign/seven-repetition story was the deleted campaign's and is gone); the check's three
    residuals applied, the abstract (`abstract/02_chapters.tex`), `back_conclusion.tex` and the
    English annotation rewritten to match (5b4a4b505 and after). `data/transport.csv` had been
    clobbered by the h1-deep emission (results2csv shares file names); regenerated from
    transport.jsonl alone, now both arms. Thesis builds: 157 pages, 0 errors, 15 red markers
    (sweeps table on purpose, macOS/Linux/h3/ttfb keys pending data).
  - `thesis/figure-overlaps` merged (1f75fac90); branch can be deleted.
  - Three-branch review: 3 high, 7 medium, 3 low, nothing refuted; macOS verification of the tip
    passed (241 targets, 178/178, selfcheck 301, smoke 2/2 schema 7). Headline: **io_uring's
    wake() is dead** (eventfd written, never read through the ring), so posted work, timers and
    cross-worker handoffs wait the full timeout, now 1 ms; fix = NOP SQE under sq_mutex. Whole
    list sent to the laptop for `linux/review-fixes` stacked on `linux/socket-opts-shared`; the
    wake fix precedes the blocking-wait experiment (a blocking wait with a dead wake hangs).
  - Paper 2 (`paper-socket-demux` draft/v1, d3932ab4): drafted, reviewed (17 findings applied),
    8 pages, 0 errors, no TikZ (tables only, pages checked visually); missing citations added,
    Nagle limitation added, README errors fixed (three socket errors not six; one fingerprint for
    every design; census commit f52c49521). Still to do: Word step (`build_docx.py` must be
    written fresh, the template's carries the other paper's text), trim the abstract (~330 words),
    venue running head, Alex's read.
  - **Alex (17:40): PDFs must be committed, not gitignored**, in the paper repos (and thesis). DONE: `main.pdf` versioned in the three
    scaffolded paper repos (draft/v1) and `doc/thesis/main.pdf` (latexmkrc copies it after every
    build). `paper-quic-cid-routing` scaffolded too (3a1c9c1 on draft/v1, PDF committed; 16 bib
    entries; sections hold TODO notes only, nothing measured). All four paper repos now carry a
    built `main.pdf` on `draft/v1`.
- **Local workflow `waafab14v`: paper 2 draft** in `paper-socket-demux` on branch `draft/v1`,
  scaffolded from the Compile-time-Protobuf IEEEtran template, eight sections drafted from the
  filed evidence with every number traced to a file, assembled, reviewed by three lenses,
  fixed, built, pushed. Read its report, then read the PDF yourself before showing Alex.
- **Local workflow `wici08xnv`: chapter VI against the data.** Audits every sentence of
  `06_results.tex` against the regenerated keys and CSVs, rewrites contradictions, commits and
  pushes `06_results.tex` only. One false sentence is already known (the worst pacing "about half
  the limit" is now 979 809 us from a refused run).
- **Figure overlaps fixed and pushed: branch `thesis/figure-overlaps` at `ecde5e66d`, eight
  commits, one per figure**, each re-rendered at 220 to 440 dpi and re-inspected after the
  fix; two needed a second pass and are now clean; the coordinator spot-checked `cid_routing`
  and `closed_vs_open_loop` by eye and agrees. Common root cause in three figures was the
  global `note` style (filled dashed frame, 6 pt padding) applied to labels beside lines; fixed
  by local overrides, preamble untouched. **Merge into `phase0-foundation` once `wici08xnv` has
  pushed its chapter VI rewrite** (it builds in the shared tree; the figure branch built in its
  own worktree), then rebuild and glance at all 15 figure pages once more. Alex's standing rule
  (memory `tikz-visual-check`) applies to the papers too.
- **Alex's stated deliverable:** ready papers in all four paper repositories, following the
  Compile-time-Protobuf `paper/` template (IEEEtran conference, sections as files, bibliography,
  a Word step for submission). Paper 2 drafting now; paper 3 drafts when the laptop's E and F
  land; paper 1 drafts when the routing night lands; paper 4 stays future work.
- **Paper 1 and paper 3 scaffolds** exist on `draft/v1` in `paper-dfa-routing` (fa524bc) and
  `paper-io-portability` (b77e0d4): IEEEtran template, eight empty section files carrying `% TODO`
  notes, both build clean. The scaffold slip that wrote notes into filenames is fixed; nothing
  is drafted in either yet, by the no-drafts-ahead-of-data rule.

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

- **RESOLVED: the epoll ~1 ms structure is the cost of cores going idle, not a line of code.**
  Laptop control: same epoll netns cell, 100 rps, 4 connections, with 16 niced spinners keeping
  the cores awake (server still preempts them instantly): p50 495 µs idle → 74 µs awake; io_uring
  25 µs (its 1 µs loop never lets the core sleep, the parking finding seen from the other side).
  Corroboration: strace erases it (p50 182, the tracer keeps the worker busy); in the trace the
  server spends 119 µs per request then sits in a 9.79 ms epoll_wait; worker count irrelevant
  (1 worker 490, 4 workers 497); gone at 10 000 rps (p50 42 µs). C-state exit vs frequency ramp
  not separated (needs cpuidle/cpufreq writes we do not make); the paper claims "the core was
  idle" and no more. **Paper 3's central result:** the low-load epoll-vs-io_uring latency gap is
  mediated by CPU idle state, not by the I/O mechanism; a busy loop buys ~470 µs of wake-up
  latency at the price of a worker's worth of cores kept awake. The blocking-wait experiment is
  to be measured as latency-from-idle (three wait variants × idle/awake at 100 rps, plus 10k).
- **CONFOUND: the backends configure accepted sockets differently.** io_uring sets TCP_NODELAY,
  TCP_QUICKACK and SO_SNDBUF 256 KB (uring_context.cpp:979-989); epoll sets TCP_NODELAY only
  (epoll_context.cpp:507-508). Every cross-arm cell so far partly compares socket options. Not
  the cause of the 500 µs (the spinner control settles that) but it violates "vary only the
  mechanism". Laptop is inventorying all four backends (accepted and listening options) on
  `linux/socket-opts-shared` and finding the single call site for a shared helper; the policy
  direction RULED: io_uring's set (NODELAY, QUICKACK where present, SO_SNDBUF 256 KB) becomes
  the shared policy for every backend, so io_uring's within-arm history stays continuous; the
  methodology must say that an explicit SO_SNDBUF disables send-buffer autotuning and that
  QUICKACK is a transient hint. **Re-measurement: no single arm is re-run.** Every cross-arm
  design re-runs in full, both arms, one binary, on the merged phase0-foundation HEAD after the
  three laptop branches plus `linux/socket-opts-shared` land: the transport design (both
  `transport.csv` and `churn_transport.csv` are confounded and PROVISIONAL, and so is any
  chapter VI text wici08xnv writes against them) and the cross-arm syscall comparison (under
  churn io_uring's two extra setsockopt per accepted connection sit inside its numbers; the
  "tracks work vs tracks time" finding is re-stated after the re-run). Within-arm results (demux
  on/off CSVs, X1, the wait-timeout before/after, epoll's own per-request syscall list) stand.
  Windows has no within-platform cross-arm comparison, so the desktop's running campaign stands;
  IOCP joins the shared helper and the desktop rebuilds only between campaigns.
  **Inventory (laptop, `design/socket-options-inventory.md` on `linux/socket-opts-shared`):**
  three problems, not one. (a) epoll vs io_uring as above. (b) io_uring vs itself: only the
  multi-accept loop calls `set_tcp_opts` (uring_context.cpp:1023); `async_accept` (:792) builds an
  unconfigured connection, so unit tests and any TLS listener on that path differ from the
  benchmark's cleartext path (laptop is checking whether the filed TLS cells went through it;
  if so, "what TLS costs" is confounded, tls demux on/off still stands). (c) **kqueue and IOCP set
  no TCP_NODELAY at all: macOS and Windows ran with Nagle on, Linux with it off.** Listening
  sockets: kqueue has no SO_REUSEPORT (recorded, out of scope). Ruled: helper
  `configure_accepted_socket(NativeSocket)` in `include/coroute/net/socket_options.hpp` +
  `src/net/socket_options.cpp`, called from every connection constructor (epoll's shape, no
  accept path can bypass it); policy NODELAY everywhere, QUICKACK Linux one-shot at accept
  (stated as such), SO_SNDBUF 256 KB everywhere; SO_NOSIGPIPE and SO_UPDATE_ACCEPT_CONTEXT stay
  outside the helper as platform obligations. Laptop building it now, with before/after on the
  epoll cell (100 idle, 100 awake, 500). The awake control is 16 `nice -n 19` bash spinners,
  one per logical core, started after listen and before the generator, killed by PID.
  **DONE (laptop, `linux/socket-opts-shared` pushed):** `configure_accepted_socket()` in
  `src/net/socket_options.cpp`, called from the four connection constructors (TlsConnection and
  PrefaceConnection wrap an existing Connection and inherit it); `set_tcp_opts` deleted; verified
  under strace that both Linux backends set all three. **TLS was never confounded:** App::run has
  one accept path (multi-accept, banner records it), TLS is a per-connection wrapper. **The
  policy's own effect on epoll is nil** at 100 idle/awake and 500 rps (p50 502→502, 77→79,
  487→494 µs): the confound was real and its size at these cells is below noise; say so in the
  method section, and it does not excuse the cross-arm re-runs (throughput, churn, syscall
  tables untested). The keep-alive syscall finding stands (2 setsockopt per connection =
  0.000427/request); churn cells carry ~2/request and re-run anyway. Incidental: io_uring under
  strace did not exit on SIGTERM (epoll did), the shape a dead wake() predicts for stop().
- **The epoll ~1 ms latency structure predates the coarse clock and is not ours.** The laptop
  bisected 560532e3d (parent) against 30ad04716 (coarse clock, touches only idle_timeout.hpp) on
  the same epoll netns cell with the generator held fixed: p50 495/462 µs before, 496/490 after,
  500/490 at HEAD, at 500 and 100 rps; min 28-36 µs, ceiling ~1000 µs, uniform spread in
  between; io_uring on the same cell is 28/44 µs. Shape says requests wait for the next edge of a
  1 ms periodic wake, not a per-request cost. No 1 ms constant exists in src/net/epoll or
  include/coroute/net (the epoll_wait timeout is 100 ms). Laptop authorised, bounded to about an
  hour: repo-wide grep for waits and handoff queues, then strace the epoll server at 100 rps and
  read back from the response write to the wake; one-line fix on `linux/epoll-tick` with
  before/after if a line is named, else report and move to the blocking-wait experiment. Paper 3
  needs this settled (epoll vs io_uring at low load would otherwise measure the tick); paper 2's
  demux on/off run is admissible either way (equal rate, equal tick cost).
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
- **RETIRED: "io_uring holds the package clock about 145 MHz lower".** That run had io_uring
  as root and epoll unprivileged, so the arms differed in scheduling class and in
  RLIMIT_MEMLOCK exemption as well as in backend, and it isolated nothing. Do not quote it and
  do not reconcile it. Replaced by the idle measurement on the repaired host, both arms as the
  same unprivileged user: with zero clients connected, epoll parks eight cores at the 1103 MHz
  floor and io_uring parks about four, and the four that never park are the four workers
  spinning on the one-microsecond timeout at `uring_context.cpp:492`. Package mean at idle 2283
  against 3053 MHz (+770, +33.7%); under load 3347 against 3458 (+111, +3.3%). **The completion
  model as implemented does not slow the package; it keeps a worker's worth of cores awake**,
  a power and thermal cost rather than a clock cost, and under load the io_uring package runs
  slightly faster, which if anything favours it in a latency comparison. Still a mediator (the
  treatment causes it), still part of what io_uring costs, opposite direction. The enter rate
  on the stock kernel is about 417 000 a second, up from 230 000 on the hardened one because an
  enter is cheaper there: one more sign the rate is set by the cost of an enter, not the load.
- **RLIMIT_MEMLOCK is charged per user, not per process, and released 7 to 14 ms after the
  holding process exits.** Found while a ctest `RESOURCE_LOCK` fix was refuted: failures
  persisted at `-j1`, so they never needed parallelism; consecutive io_uring tests overlap in
  accounting while never overlapping in time. Gap ladder, 12 runs each: 0 ms gives 8 failures,
  50 ms gives 1, 200 ms gives 0. A green run that preceded this would have shipped a false fix;
  repeated five times it was luck. **Ruled: a test-side fixture that waits, bounded, for the
  per-user budget before creating a context; not a retry in ring init (changes the binary under
  study mid-campaign; recorded as a candidate production improvement), not a smaller ring
  (changes what is measured), not a larger ulimit (hides the constraint).** Portability chapter,
  beside the EINVAL result. Schema 7 adds `server_euid`: root is exempt from the limit, and that
  exemption is exactly what hid it.
- **Syscalls per request on the repaired host, both arms unprivileged, over the pair**
  (`clock_gettime` 0.0000 in every cell now, vDSO plus the coarse clock): epoll keep-alive
  5.167, io_uring keep-alive 42.230, epoll churn 19.380, io_uring churn 1050.756 (that cell
  still refused by the drift rule, a rise, expected). epoll churn over keep-alive is 3.751 on
  this kernel against 4.116 predicted for the old host: a prediction that did not survive a
  change of kernel.
- **The cause is one line, found by reading and not yet changed.**
  `src/net/io_uring/uring_context.cpp:492` sets `ts.tv_nsec = 1000`, a one-microsecond
  timeout, and `:473` runs `poll_and_resume` / `process_callbacks` in an unconditional loop
  with nothing blocking it, so every iteration issues an `io_uring_enter` with
  `IORING_ENTER_GETEVENTS` and a deadline that has usually already passed. Four workers at
  about 58 000 iterations a second is the measured 230 000. The rate is set by how long an
  enter takes on the host, not by the workload, which is exactly the time-proportional
  signature. The comment at `:489` names the trade deliberately ("balance between latency and
  CPU usage"), so this is a design choice to re-price, not a bug. Incidental correction: the
  comment at `:145` describes a kernel without `IORING_FEAT_EXT_ARG`; this one has it
  (features 0x3ffff read from a live ring), so no timeout SQE is consumed here.
  **Complete chain, all measured:** 1 us timeout to about 230 000 enters a second, to a
  syscalls-per-request figure that measures the poll loop rather than the work, to a package
  clock held about 145 MHz lower at zero load, to every latency comparison between the
  backends carrying that as part of io_uring's cost. That chain is the spine of the
  I/O-portability paper's first sub-study.
  **Authorised next, on its own branch off the merged HEAD:** raise the timeout to about 1 ms,
  one line, and measure before and after identically, including the cost the comment names,
  which is added latency on the first request after an idle gap. A blocking wait when nothing
  is in flight is the better design and should be attempted only after the one-line version
  has shown the size of the effect.
- **Nine `clock_gettime` per established connection is real per-connection work.** Stated,
  retracted, and reinstated in one night, and the third version is the measured one. A rate
  ladder at three points on a fixed shape (2000, 5000, 10000 requests a second, epoll, all
  accepted) gives 2.005, 2.004, 2.002 clock reads per request: flat across a fivefold change,
  with a least-squares floor of 10 a second, which is zero to within measurement. A fixed
  reader of the size the earlier fit claimed would have made the per-request figure fall from
  about 3.5 to 2.3 across that ladder, so there is no such reader and `timer_queue.hpp:107` is
  not the explanation. **So about 9.3 reads per established connection against 2.00 per
  request on an established one means roughly 7.3 genuine reads per connection**, and the
  deadline machinery arming and disarming more than once per connection is back to being the
  live hypothesis. Keep-alive is measured; churn is still inferred from a single rate and
  wants its own ladder.
  The retraction that failed was a two-point fit in which rate and shape moved together, so it
  could not separate them, and two backends agreeing on it corroborated only that both mixed
  the same two shapes the same way. **A decomposition that separates two effects needs at
  least one more point than it has unknowns, and a fit whose variables moved together is not
  evidence about either of them separately.** The io_uring conclusion is unaffected and the
  reason is worth keeping: its two cells differ 25-fold in rate while the enter rate differs
  1.04-fold, and no shape difference makes a per-request cost fall 25-fold.
- **(Repaired 2 Sep ~13:00: stock kernel 7.2.2 with `tsc=nowatchdog`, clocksource tsc, 20.7 ns and zero syscalls per read verified.) The Linux host's kernel had disqualified its TSC, so every clock read there was a syscall.**
  `current_clocksource` is `hpet`, and `tsc` is not even in the available list: the kernel
  marked it unstable at boot on a watchdog timeout, which on some AMD parts is a known false
  positive. Consequence measured on that host: `clock_gettime(CLOCK_MONOTONIC)` costs 1931 ns
  and one syscall per call, against 5 ns and none for the coarse clock, roughly a hundredfold.
  At about 1.7 clock reads per request that is ~3.3 microseconds per request of overhead
  belonging to the firmware rather than the code, and it lands in every latency figure and
  every syscall count: the epoll keep-alive figure is 7.09 syscalls per request there and
  about 5.09 on a TSC host. Now recorded as `tuning.clocksource` and **fingerprinted**
  (`5a8359d97`), so two hosts differing only in it cannot pool. Deliberately NOT a refusal:
  refusing would leave the project with no Linux timing data at all, and the honest
  arrangement is to record it, prevent the merge, and state the limitation. **For Alex:
  `tsc=nowatchdog` as a boot parameter is the remedy; a host reporting hpet is a host to fix,
  not a number to adjust.**
- **The drift rule stays two-sided and unchanged. Decided, with evidence.** A warmup ladder
  (5, 30, 60 s, offered rate met in all six runs) showed the io_uring keep-alive residual
  plateaus at 4.7, 3.8, 3.7 percent, with start and end reproducible to about 20 MHz. Twelve
  times the warmup buys one point and stops, so it is not ramp from idle, which sixty seconds
  would have absorbed. It is systematic, workload-specific and repeatable, which is exactly
  what a one-sided rule would stop reporting; and a rise inside the measured window still
  means early requests ran on a slower clock than late ones, so the distribution is assembled
  across a moving clock either way. The churn cell oscillates (2.8, 0.4, 6.1) rather than
  converging, so **its one accepted record at 30 s warmup is a coin landing well and is
  discarded, not used.** Consequence accepted deliberately: **the io_uring arm produces no
  admissible timing record on that host until the cause is understood.**
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

## For Alex: what is done and what remains

**Item 0 is done; the campaign is safe and filed.** Item 1 is the one that unblocks the most now.

0. (DONE 2 Sep ~13:10.) Alex pushed `measure/desktop-2026-09-02` to the private
   `paper-socket-demux` repository, and the coordinator filed the twenty files into
   `measurements/2026-09-02-desktop/` on its `main` at `e2b16448`, byte-identical to the
   branch blobs, with a README entry stating provenance and what the data does not show.
   That branch in the paper repo carries the whole framework history and can be deleted at
   leisure; the files on `main` are the record.

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
