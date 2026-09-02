# Coordinator handoff

Written 2026-09-02 by the coordinating session (Fable); last refreshed 03:55 EEST. A new session takes over
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
| `ArchLinux` | the laptop: Ryzen 7 5800H, Arch on a hardened kernel, repo `~/GitHub/PhD-WebFrame`, presets `linux-dual` (validated) and `linux-epoll`; since late 2 Sep it has perf, strace, ninja, git-lfs, passwordless sudo and a wired link | Linux code work and, once the harness branch lands, netns runs and the perf syscall counter; io_uring runs only as root on this kernel (restricted, not off); governor still powersave | brief 3 (preset check + perf feasibility probe) running |
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

`phase0-foundation` = `904778889`, pushed. **Both branches are merged and deleted; no branch is
in flight.** The harness's 21 Linux-readiness gaps are closed (off-host generator launcher for a
netns pair, port preflight, governor gate, `git_dirty` unknown refused, UDP drop counter under its
real name, server-death diagnosis, dispatch clock readings, whole-run response count, schema v4),
and `--io-backend` now exists on `run_campaign` and `run_routing_e2e`, so a campaign can select the
arm; without it a `linux-epoll` build died at every server start. **The runtime-backend work is merged** (`--io-backend`,
virtual listener/datagram creation, banner cross-check, `linux-epoll`/`linux-dual` presets),
verified on Linux, macOS and Windows/MinGW before merging, and its branch is deleted. Also:
the generator's MSVC `NOMINMAX` fix, the Linux memory peak in `route_bench.cpp`, campaign runbook
(`benchmark/README.md`), thesis gate table (chapter V) and a build that survives missing
`data/` (chapter VI guards), Linux memory peak in `route_bench.cpp` (merged from the
laptop's `linux/route-bench-memory`, branch deleted).

Branches in flight:

- Excluded from both branches, still to do on the laptop (root is now available via passwordless sudo): perf-based syscalls-per-request counter (audit items B2-B4: `perf stat
  -e raw_syscalls:sys_enter -p <pid>` around a non-timed "mechanism" run, opt-in flag,
  schema field), netns pair creation and a real off-loopback Linux run, the QUIC UDP-seam
  reconciliation between `feature/http3-quic` and HEAD (paper 4, cheapest next step per
  its BLOCKERS).

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

- **The Linux host's kernel disqualified its TSC, so every clock read there is a syscall.**
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
- **The likely cause is a finding, not a nuisance, and it is being tested.** The io_uring
  cells start systematically lower than the epoll ones at the same point after the same
  warmup, 3872-3893 MHz against 4042-4067. A server issuing ~230 000 `io_uring_enter` a
  second across four workers may simply not let the package boost the way a lighter one does.
  If so **the two backends are not being compared at the same clock**, which confounds every
  latency comparison between them on that host, and it sits alongside the syscall result: the
  completion model pays a per-second price, and part of what that price buys is a slower
  package. Per-core clocks sampled through each arm, rather than the package figure at the
  ends, will settle it.
- **Nine `clock_gettime` per established connection was a normalisation artifact too**, the
  same shape as the io_uring one but mixed rather than pure. Two-point fits from two
  independent backends agree at ~1.70 per request plus a fixed ~2900/s floor, which they must,
  since the clock code is shared and neither backend has its own. So 82% of the churn figure
  is a fixed rate divided by a low request rate. The floor is most likely the timer thread's
  wait loop (`timer_queue.hpp:107`), worth fixing on this host where each read costs 1.9 us.
  A third rate is queued to test linearity, since a two-point fit has no residual.

- **Two ladders could not validate what the campaign offers** (fixed, `2bbc55353`). The churn
  ladder stepped 100 to 200 while its table asks for 150, so the rate most in need of checking
  was the one never visited, and it cannot be interpolated: 100 paced at 83 microseconds and
  200 at 3012. And `tls-smoke` offered 1000 to both shapes, which is nothing for a reused
  connection and about seven times the establishment boundary, so its two churn cells were
  refused on every host and always would have been; read without the shape in view that looks
  like a machine pacing at 57 microseconds in one run and eight seconds behind in the next at
  the same rate, a fault nobody can find because it is not there. Both ladders are now built
  from the tables they validate and a selfcheck fails if either property is reverted.
- **The desktop's WSL arm is blocked by Windows Firewall**, not by the harness: two auto-created
  inbound block rules for the freshly rebuilt `benchmark_server.exe`, which is why every
  loopback design works and only the off-host arm sees 0.0% delivered. Several matching rules
  still name the repository's old path, so they have accumulated since the move. **Alex's
  call in the morning**, one rule scoped to the WSL subnet or delete the two blocks; no
  session touches a firewall. Until then the desktop runs loopback designs only.

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
  4.264x keep-alive; those figures are stable and interpretable. The completion model as
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

## Open questions for Alex

1. How long is tonight's desktop window; morning stop is fine at a design boundary.
2. Laptop: may a campaign set the governor to `performance` for its duration
   (`cpupower frequency-set -g performance`, restored afterwards)? Everything else on the
   old list is done: tools installed, passwordless sudo, Ethernet up; Alex declined a reboot
   and the io_uring sysctl, so io_uring arms run as root and the record must say so.
3. (done: the laptop is wired) A two-host validity check desktop<->laptop over the LAN is now possible; worth a night later.
4. Protobuf rebuilds: commit or discard.

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
