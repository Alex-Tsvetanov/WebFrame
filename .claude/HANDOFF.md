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
| `Windows machine` | the desktop: Ryzen 5 3600, Windows 11, WSL2 Ubuntu-24.04 hosts the generator (its user is not the Windows user; read `$HOME` from the distribution); repo paths and addresses are in the local memory note `benchmark-machines`, never here | IOCP measurements, the campaign nights | released for tonight; running steps 0-6 below |
| `ArchLinux` | the laptop: Ryzen 7 5800H, Arch, **since 2 Sep ~13:00 on the stock kernel 7.2.2 with `tsc=nowatchdog`**, so clocksource tsc, io_uring unprivileged, perf_event_paranoid 2, governor performance (set by hand, not persistent); perf, strace, bpftrace, wired link; repo `~/GitHub/PhD-WebFrame` | Linux measurements proper, for the first time: the daemon survived the reboot under the same name | brief 5 running: clock gate, ff, mechanism run unprivileged, then the io_uring timeout experiment |
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
- **`churn-net` finished on the desktop: 400 runs, 356 accepted, 44 refused (11.0%), 13:39 to
  16:22 EEST, at b4a01e8c7.** Per rate: 50/s and 150/s clean (0 of 100 each, median pacing 59 and
  51 us); 400/s 3 refused (median 48 us, max 665); 800/s 41 refused (median 387 us), one of them a
  total failure delivering 0.0% with 128 socket errors. The design stopped itself on two
  pre-declared rules (pooled refusals over 10%, and any refusal at 400/s).
  **Rulings.** (1) The pooled 11% is not a meaningful number: it mixes three clean rates with one
  known-bad one. The stop rule should be per rate, not pooled, and that is a defect in the rule,
  recorded in chapter V. (2) 800/s is beyond this arrangement's reach and is reported as a measured
  ceiling, not as a server result; the earlier ladder's 468 us at that rate had already said so.
  (3) **The three refusals at 400/s are all in the TLS arm and none in cleartext**, so they are not
  obviously instrument-side: refusal there may be correlated with the arm being measured, which is
  the one case where discarding refused runs biases a comparison. Sharper, from the successor's reading of
  the same file and accepted: the accepted runs at 400/s reach 665 us and the three refusals sit at
  2030, 2110 and 2170, three points within 140 us of each other after a 1365 us gap. **That is a
  second mode, not the tail of the distribution that produced the 665.** All three delivered the
  full 9200 whole-run count, so it is a pacing failure and not a delivery failure, and all three
  carry gen_cpu 0.881 to 0.948. **Open diagnostic, asked of the desktop:** the median gen_cpu of the
  97 accepted 400/s runs, beside those three. A materially lower accepted median puts the second
  mode in the generator (the campaign's case 2) and the cell is citable with a stated limitation; a
  similar median leaves the mode unexplained and the TLS-versus-cleartext comparison at 400/s is
  **The desktop ran it and the obvious mechanism fails.** Generator CPU share: TLS accepted median
  0.9506 (n=47), cleartext accepted median 0.9524 (n=50), the three refused 0.8806, 0.9466, 0.9478.
  Two of the three fell 2 ms behind while holding an ordinary share, so descheduling explains at most
  one. The bimodality and the arm asymmetry are established; the cause is not, and that is how it is
  written. Arm split at 400/s: TLS 50 cells, 3 refused (6.0%), accepted pacing 45/49/555 min-median-max;
  cleartext 50 cells, 0 refused, 34/41.5/665.
  **RULED.** Within an arm the 97 accepted cells are citable, with the per-arm refusal rate in the
  table or its caption. Across arms the difference at 400/s is reported as a LOWER BOUND with the
  direction named, because the three runs the TLS arm lost are its slowest and cleartext lost none;
  not a headline number. The second mode is itself a result and is described, not smoothed. No
  re-laddering, no re-running of refused runs. The pooled 10% stop rule becomes per-rate; 800/s is a
  measured ceiling of the arrangement, not a property of the server.
  **Free check outstanding, asked of the desktop:** the three refused runs still carry their own
  server-side latency records. Ordinary-looking records put the slip on the generator or OS side and
  make the discard harmless; a matching ~2 ms excursion means the generator fell behind because the
  server stalled, and the discarded runs are the measurement. No machine time.
  Both rules are now written into chapter V as new subsections (asymmetric refusals name the
  direction of the bound; the refusal threshold applies per offered rate, not pooled). 3 of ~50 against 0 of ~50 is
  suggestive, not established (Fisher exact about 0.24). The cell is kept, with n=97 accepted, and
  every TLS-versus-cleartext statement at 400/s carries the asymmetry in its text. It is not used
  for a headline claim. (4) **The refused runs are NOT re-run.** Re-running only the runs that
  failed is sampling on the outcome. (5) The network arm is published at 50, 150 and 400/s.
  The desktop stays at b4a01e8c7 and does NOT rebuild onto the shared socket policy mid-campaign;
  a rebuild is a campaign boundary and the later designs would then need their own directory and
  fingerprint, as the schema split was handled.
- **The laptop's own test caught kqueue on macOS, unprompted, and it is the before number.** Test 42,
  "posted work reaches an idle worker without waiting out the poll timeout", written for epoll and
  io_uring, fails on macOS at HEAD: median delivery **98480 us**, first 49699, worst 98658, against
  its threshold of 200. Worse than epoll's 50 ms for an arithmetic reason: a burst of posts each waits
  out its own 100 ms `kevent` cycle rather than sharing one. **The whole laptop stack is merged and
  built on macOS in a scratch worktree (241 targets, self-checks 301 here against 304 on Linux, the
  difference being the Linux-only checks) but is DELIBERATELY NOT PUSHED until the kqueue fix lands,
  because mainline must not carry a failing test.** Merged tip is `0894c13b7`, one past the
  `e6c507b9f` the laptop reported.
  **CORRECTION, from the laptop:** that merge was TWO COMMITS SHORT. `0894c13b7` is behind
  `e6c507b9f`, not past it; the coordinator read a push range backwards and then explained away the
  contradicting evidence (301 self-checks rather than 304) with a platform-gating story instead of
  checking it. The laptop's checks all use `FakeServer` and are platform-independent, so 301 was
  exactly the pre-change count and was the tell. Re-merged: worktree HEAD `fdb517207`, **304 checks**,
  build clean, and test 42 is now the ONLY failure, awaiting the kqueue fix. Lesson worth keeping:
  a number that contradicts the story is evidence, not noise to be explained.
  **For paper 3:** a portable test written for one backend found the same defect on three operating
  systems, and every one of those backends passed a suite that looked complete. The suite could not
  have caught it because nothing measured delivery latency on an idle loop. The finding is not that
  the code had a bug; it is that a class of defect was invisible until someone measured the mechanism
  rather than the outcome.
- **THE FREE CHECK ANSWERED: THE SERVER STALLED. The pacing gate is a server-stall detector.**
  Desktop compared the three refused TLS runs at 400/s against the 47 accepted ones of the same cell:

  | field | accepted median | accepted max | the three refused |
  |---|---|---|---|
  | latency p50 (ms) | 0.339 | 0.375 | 0.377, 0.444, 0.449 |
  | latency p99 (ms) | 0.947 | 1.038 | 2.933, 2.828, 4.699 |
  | connect p99 (ms) | 1.932 | 8.722 | 10.237, 10.215, 10.408 |

  All three sit at or above the maximum median of every accepted run, with a p99 three to five times
  the worst accepted and an establishment excursion of about 8 ms, while establishing all 9264
  connections with zero handshake failures and ordinary CPU. The server did not fail; it went slow,
  in a bounded way, in exactly the runs the gate refused. **So the discarded runs ARE the measurement**
  and the coordinator's precautionary condition is now demonstrated. The TLS arm's accepted set at
  400/s is biased fast by construction; a TLS-minus-cleartext difference there is a lower bound and
  we can now say why in terms of the server's own latency rather than the gate. The second mode has a
  mechanism: in about 6% of TLS establishment runs at that rate the server's p99 rises three to
  fivefold. **Why the server does it is unanswerable from these records; it goes to the laptop, which
  can attach a profiler. Not the desktop's to chase mid-campaign.**
  **For the methodology chapter: an open-loop generator's pacing gate is a detector of SERVER stalls,
  not merely a check on the generator.** It was built against coordinated omission and has caught a
  server-side event nothing else in the harness watches for.
- **THE IDLE LADDER IS MEASURED, and it opens a gap that may be the biggest methodological finding
  of the two days.** Blocking arm, 200 posts per gap, delivery cost against the interval between
  posts: 1 ms -> 9 us, 2 ms -> 26, 5 ms -> 63, 10 ms -> 62, 20 ms -> 63, 50 ms -> 65. It climbs and
  **plateaus flat from 5 ms onward**, so parking depth grows with the idle interval and saturates.
  An idle-cost figure without its interval is meaningless, now measured rather than asserted.
  **But it does not reconcile with the load cell and about 350 us is unaccounted for.** At 10 ms the
  ladder says a wake costs 62 us; the load cell at 100/s, same interval, same machine, same arm, shows
  an idle-versus-awake difference of 413 us, nearly seven times larger.
  **REFUTED, by a field that was already in the records.** The coordinator hypothesised that the open
  loop charges the GENERATOR'S OWN WAKE to the server: latency is measured from the assigned due
  instant, so a generator whose core had parked for 10 ms would have its exit cost land inside the
  measured latency and be attributed to the server. **`pacing_us` already records exactly that
  quantity, the lateness of the send against the due instant, and in the 469 us cell the generator was
  5 us late at the median, not 350.** The reason is that our generator paces by SPINNING --
  `generator_cpu_fraction` is 0.996 in every one of those cells -- and a core that never sleeps cannot
  pay a wake cost. (Third time today a CPU fraction near 0.996 has been the key to something after
  nearly being reported as saturation.)
  **THE FINDING SURVIVES RESHAPED AND IS STRONGER, and this is the version for the methodology
  chapter: an open-loop generator that SLEEPS between sends charges its wake-up cost to the system
  under test, and the error grows as the offered rate falls; a generator that paces by SPINNING is
  immune, at the cost of a fully occupied core. Ours spins.** That names the condition and exhibits a
  design that avoids it, rather than warning that a whole class of measurement is suspect, and it
  tells a reader with a sleeping generator what they are measuring. The immunity was not designed for:
  the spinning was chosen for pacing accuracy, and saying so is worth more than claiming foresight.
  **It sharpens the other direction too.** In the AWAKE cells the generator IS late, pacing p50 rising
  5 -> 52 us because the spinners compete with it, so the awake cell's 56 us of latency is mostly
  generator lateness. Server-and-path is then about 464 idle against about 4 awake, making the idle
  penalty LARGER than the 413 first reported. One more reason the spinner control applies only to arms
  that would otherwise idle: it costs the generator accuracy while buying the server wakefulness.
  **Confirmed from the code, and the subtraction IS principled:** both quantities are measured from
  the same origin, `c.issued = next_due`, with pacing pushed at send completion and latency at
  response arrival, so `latency - pacing` is exactly response-arrival minus send-completion, with no
  double counting. **The per-request pairing does not exist, but NOT for the reason first
  given.** The laptop first said the vectors differ in length and then corrected itself: they are the
  same length in every record (checked across nine records at both rates), because the generator hands
  new work only to connections that are not already awaiting a response, so exactly one request is
  outstanding per connection and the batch loop always pushes one. **The real reason is that the two
  vectors are sorted INDEPENDENTLY before percentiles are taken, so a difference of percentiles is not
  a percentile of differences.** That reason stands alone and is the one for the text. So 464 against 4 is a difference of
  percentiles, approximate, and the text must say so with that reason.
  **SCHEMA-9 CANDIDATE, and it is bigger than a convenience: the queueing decomposition.** A third
  vector recorded per request at response arrival, when both timestamps are in hand, gives the paired
  quantity with no identity plumbing. Response time equals wait plus service: we measure response time
  from the due instant, which is what makes it proof against coordinated omission, and the wait is the
  generator's own lateness. A run reporting both, with their per-request difference, says what the
  server did AND what the client experienced from one measurement, and shows the gap rather than
  asking a reader to trust it is small. `latency_kind` already distinguishes the two and an open loop
  currently reports only the first. Two conditions before it becomes a field:
  (i) **Do not call it service time.** It measures send-completion to response-arrival, which includes
  the path, the softirqs and the veth pair, and over the two-host arm will conspicuously not be the
  server's own occupancy. Name it for what it measures and let the text say it bounds service time
  from above; a field named for a quantity it does not contain will be quoted as that quantity.
  (ii) **Pairing is well defined: pipelining cannot happen.** `loadgen.cpp:1061` skips any connection
  with `awaiting` set, and that clears only on response completion, so one request is outstanding per
  connection always, structurally rather than by design choice. At response arrival there is exactly
  one send timestamp to subtract. So this really is one vector computed where both timestamps are
  already in hand, not the larger change. Name agreed: `time_after_send_us`.
  **Caveat on that subtraction:** a p50 minus a p50 has no interpretation unless the distributions
  align request by request. Do it PER REQUEST and take the percentile of the differences, and confirm
  from the code that latency includes pacing by construction rather than from the field names.
  **THE ARITHMETIC STILL DOES NOT CLOSE, which is now the open question.** One in-process wake at a
  10 ms interval costs 62 us; server-and-path is about 464. Several wakes at 62 each would need about
  seven, and a request over a veth pair plausibly involves two to four. So the several-wakes
  explanation does not reach it either. **Third candidate, named but untested: frequency ramp.** The
  package sits at 1921 MHz in the blocking idle arm, and a core that wakes at a low clock must do the
  request's real work while ramping, whereas the delivery test's callback does almost nothing and
  never needs the clock to rise. Consistent with C-state exit and frequency ramp never having been
  separated. **Discriminator needing no privileged write, in its sharpened form:
  parameterise the callback's arithmetic and SWEEP it, in TWO arms, idle and cores-held-awake.** Flat
  in the work parameter means a pure wake cost; rising then flattening means a ramp, and the knee
  locates where the clock catches up. The awake arm isolates the ramp by control rather than by
  inference, and doubles as the baseline for how much of the curve is simply the work getting longer;
  one arm alone leaves the rising curve arguable. The frequency story also explains the SHAPE and not
  just the magnitude: the delivery callback sets a promise and returns, so it can finish inside the
  wake latency and never needs the clock to rise, while a request must parse, route and write while
  the core ramps from 1921 MHz. Those are not the same measurement and no count of wakes reconciles
  them. The one-sided spinner run and this are two
  candidates for one question, not a sequence, and neither is for tonight.
  The laptop's competing hypothesis stays alive and is not exclusive: a request path wakes several
  components (generator core, softirq, server core, and the return), while the delivery test wakes
  one. The residual after subtracting the generator's exit is what that accounts for, and the way to
  split it is spinners on ONE side only. That is the second experiment, not the first.
  Either way, keep the laptop's general statement: a wake cost is a property of the interval AND of
  how many components must leave idle on the path, so an end-to-end figure cannot be built by
  multiplying a single-wake cost. The three appearances are then not one curve: 62 us is one wake at
  10 ms, 413 us is a whole request path at 10 ms, 220 us is a kernel-to-kernel path at 200 ms.
- **THE REFUSAL-BIAS CHECK IS IN. The coordinator's prediction FAILED as stated: five of seven
  outside, not seven.** Reported by the desktop before anything else about the table, as agreed, and
  not rescued by reinterpretation. Procedure exactly as pre-declared at `1b756e905`; transcription
  verified faithful by the desktop; **no weak-bound rows existed at all** (every cell had 23 or 24
  accepted peers), so rule 6 is satisfied trivially.
  **But the direction is unanimous and the magnitudes are not marginal.** All seven refusals rank at
  or above the top of their cell's latency distribution; five exceed every accepted peer and two fall
  just short of the single worst one (second of 23 and second of 24, within 8% of the maximum). **Not
  one refusal in the informative population sits in the body of its cell.** Where outside, the
  magnitudes are 150x to 6000x the cell maximum, not runs that drifted over a line.
  **THE COORDINATOR COMPUTED A RATIO OVER THESE ROWS AND IT WAS CIRCULAR. RETRACTED IN FULL; the
  table is deleted rather than annotated, per the rule that a number in a table gets used whatever
  sentence sits beside it.** The ratio was the REFUSED run's own latency p99 x rate / connections,
  and the verdict OUTSIDE is also a function of the refused run's latency p99. Both columns were
  monotone in the same variable, so the sort could not fail: it said "runs with high latency have
  high latency", in two columns. The transport row singled out as "the coincidence that proves the
  formula" was the tell -- a coincidence that cannot fail to occur is not evidence. The desktop caught
  it and verified the arithmetic was correct before pointing out that correct arithmetic was the
  problem.
  **THE NON-CIRCULAR VERSION SAYS THE OPPOSITE.** Using the ACCEPTED runs' latency (exogenous to the
  run being judged), no cell has the pool binding under normal operation: the largest is 0.32, a third
  of the way to the threshold, computed from the WORST accepted run rather than the median, so already
  generous. **The pool was binding in none of the seven**, and whatever slowed those runs did so with
  ample idle pool capacity -- consistent with rate 400, where the pool is ~1% utilised and the effect
  was real anyway.
  **The desktop's causal reading, adopted: the pool does not bind FIRST.** The server slows for its
  own reasons, latency rises, and only then does the pool arithmetic become large. Pool starvation is
  a CONSEQUENCE of the slowdown, not its cause, which is why the circular ratio tracked severity so
  well: it was measuring how far the consequence had run.
  **And its explanation of the two INSIDE rows is better** than the coordinator's "weakest coupling",
  which was itself reasoning from the circular quantity. Exogenously, churn at 25 is not qualitatively
  different -- every cell has an idle pool. What differs is that its accepted latency is already 4 to
  6 ms, forty times h1-deep's, so its accepted maximum is a HIGH BAR and a refusal must travel further
  to clear it. They are inside because the bar is high, and they still rank second of 23 and second of
  24.
  **`connections / rate` was offered as a clean replacement and was SHOT DOWN too, on a second
  ground.** It contains no measurement -- it is how long a total stall must last before the pool empties. 0.91 ms at
  h1-deep's top rate; 12.8 ms at transport 5000; 2560 ms at churn 25. It predicts NOTHING about which
  runs were refused and must not be used for that. It is not circular, and the desktop
  confirmed the derivation against the source. **But it is DEGENERATE: `connections` is 64 in ALL 1184
  filed records** (verified by the coordinator across every design: churn 400, h1-deep 250, transport
  500, tls-ladder 12, both churn ladders 8 each, smoke 2, tls-smoke 4), so the quantity is 1/rate
  rescaled, and any ordering it produces IS the rate ordering. As a column headed "coupling" it would
  read as an independent measurement of a mechanism while being the rate column divided into a
  constant. Same failure as the ratio in different clothes: the coordinator had checked for
  circularity and not for degeneracy.
  **And the sharper objection, which is the coordinator's own argument turned around: it measures
  sensitivity to ROUTE 1, which we have evidence was not what happened.** It describes the coupling we
  ruled out and is silent on the one that operated. Honest label, with `total` load-bearing: "how
  quickly a TOTAL server stall would starve the generator in each cell", because a partial slowdown
  never exhausts the pool and a partial slowdown is what was observed. **Keep it as ONE SENTENCE of
  illustration, never a column** -- a number in a table gets used whatever sentence sits beside it.
  **THE HONEST ESTIMATOR ALREADY EXISTS and is the desktop's own non-circular table** (which it had
  computed without recognising it was the answer to its own question). Accepted-run
  latency x rate / connections is the pool's utilisation under normal operation, computed from runs
  exogenous to any refused run being judged; it ranged 0.3% to 32% as first computed --
  **but that used latency p99, and Little's law wants the MEAN, so those were a loose upper bound.**
  Recomputed by the coordinator with the median, which brackets from below where p99 brackets from
  above (the mean lies between and nearer the median in a right-skewed distribution):

  | design | rate | n | median p50 | max p99 | util(p50) | util(p99) |
  |---|---|---|---|---|---|---|
  | h1-deep | 70000 | 48 | 0.066 ms | 0.293 ms | 0.072 | 0.320 |
  | h1-deep | 55000 | 49 | 0.070 | 0.288 | 0.060 | 0.248 |
  | h1-deep | 40000 | 48 | 0.057 | 0.144 | 0.036 | 0.090 |
  | transport | 5000 | 49 | 0.060 | 0.085 | 0.005 | 0.007 |
  | churn | 25 | 47 | 0.134 | 6.130 | 0.0001 | 0.0024 |

  **The busiest cell ran the pool at about 7% of capacity, not a third**: fourteen connections free for
  every one in use. The conclusion strengthens rather than shifts. churn at 25 is one part in ten
  thousand, so whatever caused its refusals, the pool had essentially all of itself idle.
  **Label it "utilisation under ACCEPTED operation", never "the cell's utilisation":** it is computed
  from runs that passed the gate while the hypothesis under test is that the gate removes slow runs.
  For "was the pool near binding in normal operation" that is the right quantity and arguably the only
  sensible one, since refused runs are precisely not normal operation. Turned around to characterise
  the cell including its stalls it would understate, and it inherits the selection being studied. The independence needed is from
  the RUN, not from the cell, and this has it; it also varies between cells at the same rate whenever
  the arms differ, which `connections/rate` cannot. Caveat that keeps it honest: it is not independent
  of the cell's own server behaviour, so it cannot compare a slow server's cell against a fast one's
  as though the difference were about the pool.
  **General rule worth keeping on its own account (the desktop's): not every quantity worth knowing
  has an exogenous estimator, and the text should say so rather than reach for one that is clean by
  being uninformative.**
- **ROUTE 3 IS NOT AN INDEPENDENT CAUSE and the two machines were never in conflict; the coordinator
  created the disagreement.** The laptop showed the socket buffer cannot fill (one outstanding request
  per connection, a ~100-byte GET against tens of KB). The desktop confirmed pacing is taken at SEND
  COMPLETION. Both are right: **route 3 is the point of OBSERVATION through which routes 1 and 2
  become visible**, not a third path. A thread blocked in connect, or a due slot with no free
  connection, delays the send, and pacing charges the whole delay. Describe it as the measurement
  path.
  **Route 2 is confirmed and is worse than withholding a connection.** On TLS, `c.ready` stays false
  until `SSL_do_handshake` returns 1, so one connection is withheld. On CLEARTEXT there is no such
  window because `connect_to` waits inside itself for POLLOUT, so a slow-accepting server **blocks the
  entire worker thread**, and no connection on that thread gets a due slot. In an establishment design
  `reopen()` fires on every response, so every response is followed by a blocking connect on the same
  thread. Time-after-send does not contain the handshake, so the threshold is blind to this route in
  exactly the designs where establishment is the measurement.
  **The coordinator's 14 ms citation was WRONG:** both comments describe defects that were FIXED, not
  live behaviour. What they do show cuts the same way by another route: pacing has twice been high for
  reasons wholly exogenous to the server, a second and independent argument that it is a poor
  admission statistic.
  **Caution to keep when this is written up (the laptop's):** the conservative-not-invalid argument
  holds where the lateness is real waiting already charged from the due instant. It is WEAKER on the
  cleartext establishment arm, because a thread blocked in connect is not issuing anyone's slots, so
  the load genuinely was not offered as a steady process even if all of it eventually arrives.
  Achieved share should catch that, which argues FOR the split, but the two cases must not be merged.
- **UNEXAMINED, in the desktop's own filed records: establishment time is 35x higher at low rates.**
  churn connect p50 is 9.3 ms at 25/s cleartext and 0.26 ms at 100/s, same shape on the TLS arm.
  Slower with less work is not contention; it is what an idle machine looks like, and it would be the
  **fourth appearance of the idle mediator, on the one platform that has not yet shown it** (Windows,
  IOCP). Not asserted: TIME_WAIT pressure and accept-backlog behaviour are alternatives. Worth one
  look when the queue is clear.
- **Dispatch block clean: 6 designs, 292 runs, 292 accepted, 0 failed** (main 90, scaling 60, depth 45,
  static 45, large-cheap 50, large-dfa 2), 19:27 to 19:58.
- **THE GATE MAY BE ENDOGENOUS, WHICH IS WORSE THAN BIAS, AND IT MAY BE REFUSING THE HONEST RUNS.**
  Raised by the desktop, sharper than the coordinator's "partly a server-health signal", and adopted:
  if pacing lateness is partly determined by the server, then a THRESHOLD on pacing is selection on a
  function of the dependent variable. Tail bias says the numbers are optimistic by an unbounded
  amount; endogenous admission says the admission rule is not independent of the outcome, which is the
  assumption under every percentile reported, and it applies to EVERY accepted run rather than to the
  ones near the threshold. It holds or fails whether or not the seven come out outside, because it is
  a property of the instrument.
  **THREE COUPLING ROUTES, and the laptop's threshold covers only the first.** Its condition (pool
  binds when time-after-send x rate > connections) is a real contribution and explains why the low-rate
  cells are clean, but it would exempt the rate-400 refusals we are already certain about: at 400/s
  over our pool it binds only above tens of ms, and those runs sat around ten. A discriminator that
  exempts the one established case is incomplete, not right.
  (1) A connection held by an outstanding response. The laptop's route, threshold models it exactly.
  (2) `loadgen.cpp:1061` skips a connection that is not `ready`, one clause earlier, which its own
  comment says covers a handshake still in flight, so a due slot never carries a handshake's latency.
  In an ESTABLISHMENT design every request needs a fresh handshake, so a server slow to handshake
  withholds connections by a different door -- and time-after-send does not contain the handshake, so
  the threshold is blind to this route in exactly the designs where establishment is the measurement.
  (3) Pacing is `now() - c.issued` taken at SEND COMPLETION, not at the decision to send. A server slow
  to read fills the socket, the send does not complete, and lateness accrues while the pool sits idle.
  At rate 400 the pool is about 1% utilised, so route 1 cannot have been live and route 3 does not
  care. Coordinator inferred route 3 from where the measurement is taken and has NOT traced the send
  path; both machines asked to check all three from the source and to report disagreement rather than
  resolve it.
  **AND THE EVIDENCE THAT WE MAY BE DISCARDING THE WRONG RUNS is already in the file.** The comment at
  the warmup boundary (loadgen.cpp:1042-1052) records a TLS run refused for 14 ms of pacing lag at p99
  **while the server delivered 99.7% of the offered rate**. Ask what a pacing-refused run's latencies
  are: measured from the due instant, so the lateness is already inside them. That is the accounting
  coordinated omission gets wrong and ours gets right. A run with large pacing lateness is not one
  whose numbers are invalid, it is one whose numbers are CONSERVATIVE, where the method did the most
  work it ever does. We may have been refusing the honest runs and keeping the easy ones, which is the
  opposite of what the methodology chapter says the gate does.
  **CANDIDATE REMEDY, not ruled, not for tonight, and it changes what is admissible across every
  campaign we hold:** the gate currently conflates two questions. Could the generator offer the load it
  claimed? That is achieved share, measured directly, much closer to exogenous. Was the generator
  late? That is pacing, partly downstream of the server. The first is what the gate was built for; the
  second is a diagnostic promoted into an admission rule. So: admission rests on achieved share and
  the error and status rules, pacing becomes a recorded COVARIATE reported with every cell rather than
  a threshold. Needs both machines' source readings to agree first, and it is a thesis decision.
- **A MECHANISM FOR THE STALL FINDING, and a PREDICTION put on the record before the desktop's check
  runs.** The two machines found halves of one thing without noticing. The generator hands new
  requests only to connections not already awaiting a response (`loadgen.cpp:1061`), and that flag
  clears only when the response completes. So a slow server holds connections in `awaiting`, held
  connections cannot take new work, due instants keep arriving with nothing to issue them on, and the
  generator falls behind schedule. **Server slowness propagates into pacing lateness BY CONSTRUCTION,
  through the connection pool.** That is why the desktop's three refused runs were the runs where the
  server's own distribution had moved: not a coincidence in ten runs but a path in the generator.
  **PREDICTION, recorded 2026-09-02 evening, before the refusal-bias table exists: the seven
  informative refusals will come out OUTSIDE.** If they come out inside, either the pool was not the
  binding constraint in those cells or the code reading is wrong; both would be worth knowing. The
  laptop is confirming the reading from the source with line numbers rather than from the
  coordinator's paraphrase. **The procedure at `1b756e905` is NOT to be adjusted for this** -- a
  prediction made in advance tests the mechanism, a procedure adjusted to meet one tests nothing.
  What it changes is the meaning of a positive result: not "refusal appears to remove slow runs and we
  do not know why" but "refusal removes slow runs and here is the path by which it must", which is a
  different class of claim and answers the reviewer who asks whether ten runs found a coincidence.
  **And it sharpens what the thesis owes: the pacing gate is described as a check on the GENERATOR,
  and if this holds it is partly a SERVER-health signal by construction. That is a correction to the
  methodology chapter, not an addition -- the guard built against coordinated omission is coupled to
  the thing it was guarding.**
- **THE SYSTEMIC QUESTION THIS OPENS, and the check that settles it.** If pacing refusals correlate
  with the server being slow, that is a property of pacing refusals, not of rate 400 -- and we discard
  them everywhere. h1-deep refused 4 at 40 000 and above, transport 1 at 5000, churn 2 at 25/s,
  churn-net 44. Every one was removed on the assumption of instrument noise. If they were server
  stalls, **every accepted set we hold is biased fast at the tail and every p99 and p999 in the thesis
  is optimistic by an unknown amount.** Desktop asked to run the same comparison across every pacing
  refusal in every filed design, as one table, saying for each whether its distribution sits inside or
  outside the accepted spread of its own cell. Costs no machine time. Mostly inside means the discard
  is harmless and we will have shown it rather than assumed it; mostly outside means the thesis needs
  a statement that pre-declared refusal is not neutral, that it preferentially removes the runs where
  the server behaved worst, and that reported tails are optimistic. Either answer is publishable.
  **PRE-DECLARED IN FULL BEFORE THE DESKTOP LOOKS AT ANY OF THE FIFTY** (this is the point of the
  exercise; none of it may be fitted afterwards):
  (a) A refusal is OUTSIDE if its latency p99 exceeds the maximum latency p99 of the accepted runs in
  the same cell. One statistic, one direction.
  (b) A cell is the FULL factor combination the design varies, and a refusal is compared only against
  accepted runs identical on every factor, factors read from the records. Never pool to enlarge the
  comparison set: pooling the arms at rate 400 would have compared TLS refusals against faster
  cleartext runs and systematically weakened the test towards "inside". A test that can be weakened by
  choosing a larger comparison set is not a test.
  (c) Ranks on latency p50/p99 and establishment p50/p99 reported beside the binary, plus the accepted
  count in every row: "above all 47" and "twelfth of 47" are different facts.
  (d) Fewer than five accepted runs in a cell is a WEAK BOUND and marked; zero accepted is UNDEFINED
  and counts for neither side. The test is not equally powerful across rows, and it is biased TOWARD
  the finding in thin cells, since exceeding the maximum of four runs is easy.
  (e) **The systemic conclusion is drawn only if the outside verdicts survive removing every
  weak-bound row.** Report both counts.
  (f) Rate 800's forty are reported but marked UNINFORMATIVE, because at a rate past the arrangement's
  ceiling the server is expected to be slow, so the test is undefined there rather than merely untidy:
  those runs cannot distinguish a stall from a ceiling behaving like one. Including them would inflate
  the count while weakening the finding.
  So the finding rests on **ten** runs: 2 in churn at 25/s, 1 in transport at 5000, 4 in h1-deep at
  40 000 and above, 3 in churn-net at 400. **Three of those ten are the rate-400 cases already known
  to be outside and used to define the pattern, so the interesting number is the other seven.**
  Claim wording fixed in advance both ways. Outside: refusal is not demonstrably neutral and the
  thesis must say reported tails are optimistic by an unbounded amount -- a flag on the method, NOT a
  measurement of the bias. Inside: on the refusals we have, at the rates we can test, the discard
  shows no sign of removing slow runs -- not that it is harmless in general, and the rate-400 case
  stands on its own evidence regardless.
- **Routing e2e ran, and `main` lost its first nine runs to a firewall warm-up race.** 90 runs, 81
  accepted; the nine refusals are "delivered 0.0% of the offered rate" with socket errors, at the low
  rates 1000 and 4000, and they are **positions 1 through 9 consecutively**, followed by 81 consecutive
  successes. `build\windows-routing` had been rebuilt at a path with no firewall rule; Windows created
  the Allow pair when the binary first listened, and the runs before it existed were blocked. Ruled:
  **re-run the whole design (about 42 min), keep the original file as the evidence.** Re-running is
  right here and wrong at rate 400, and the distinction is the point: there the refusals were caused
  by the system under test, so re-running would be sampling on the outcome; here an external gate
  stopped nine runs from happening at all, and nine cells that never ran are not a measurement. Whole
  design rather than the nine, so balance across rates is restored by construction and all ninety
  share one machine state. Inside the programme Alex approved, so no new authorisation needed.
  **Systemic, and it goes in the runbook:** any campaign whose binary is rebuilt at a path the firewall
  has not seen loses its first runs, silently, from whichever cells the scheduler ordered first.
  Precondition: start the server once, make one request, stop it, then begin timing. Control that
  confirms it is not the host: churn-net's single zero-delivery run is at position 292 of 400, not
  early, so it is not the same cause and remains a rate past the ceiling.
  Other blocks clean: bracket 60/60, bracket-low 30/30, large 14/15 (one CPU-frequency drift).
- **CROSS-ARM RE-RUNS ARE RUNNING on `a4519ada2`, through the driver, about six hours.** Preflight
  passed on both arms (smoke 2/2 each) and confirms schema 8 live end to end: `server_euid` 1000 and
  `generator_euid` 1000 attested, `local_interface` recorded as `veth-gen` from the socket with speed
  10000 Mbit, duplex full, MTU 1500, and `--expect-interface` enforcing rather than assuming. Set:
  transport (20 cells, 280 runs, 3.0 h), churn (16 cells, 224 runs, 2.4 h) and mechanism with
  `--count-syscalls` (2 cells, 28 runs). churn is included because the registry's own comment names
  transport and churn as the pair carrying the claim and `churn_transport.csv` is named in the same
  sentence as confounded. Results to `benchmark/results/crossarm/{design}_{arm}.jsonl` so nothing
  overwrites the committed campaigns and the arms stay separable; no cross-arm comparison drawn until
  both arms of a design are in.
  **A POWER CALCULATION FROM EXISTING DATA STOPPED AN UNNECESSARY RESTART.** The coordinator was about
  to order 25 repetitions, since chapter VI records that seven resolved the difference at only four of
  five loads in an earlier campaign. Computed instead from the desktop's filed transport records
  (50 accepted runs per cell, so a real estimate of run-to-run spread): the half-width of the interval
  on a difference, as a share of the cell median, is 1.2% to 4.7% at n=7 in nine of ten cells, and only
  25000 with TLS fails, at 6.7% against the 5% floor (its run-to-run cv is 6.4% against 1-4%
  elsewhere; at n=25 it would reach 3.6%). **So seven repetitions resolves nine of ten cells, and
  tripling six hours into twenty-one to rescue one cell is the wrong trade.** Report the achieved
  resolution per cell and say which resolved. Caveats stated to the laptop: the estimate is from
  Windows records for a Linux campaign, so it is a prior and not a prediction, and it approximates the
  harness's bootstrapped medians with a normal interval on a difference of means -- close enough to
  decide a restart, not close enough to quote.
  **Method note worth generalising: a power calculation from data already held costs nothing and can
  decide whether machine time is worth spending. Do it before long campaigns, not after.**
  The 25000-TLS cell's threefold cv deserves one sentence in the report whichever way it falls: it
  appeared on a different platform at a different commit, which makes it more interesting, not less.
  **GAP, to follow without restarting: paper 2's syscall count is NOT in this set.** The mechanism
  design here varies the BACKEND; paper 2 needs classification ON and OFF, a different axis, and its
  text says the syscall mechanism awaits the Linux run. **It cannot be appended:**
  `design_mechanism` is deliberately fixed at `protocol_detection=True` (crossing detection there
  would put another factor under the quantity it measures), so paper 2's axis needs a NEW design,
  which means editing `run_campaign.py` -- and each campaign captures `git_dirty` at its own start,
  so an edit mid-sequence would leave five of the six running campaigns against a dirty tree. Same
  class as the shared build directory: a change made while a measurement is in flight, where the
  measurement's own provenance is what gets damaged. **After the three finish: a branch off
  `a4519ada2` with the design committed, and the fourth campaign run from THAT commit.** Paper 2's
  numbers then trace to a child of mainline, which is correct provenance rather than a compromise,
  since this is a new measurement and not a re-run to be pooled.
  **RULED, overriding the laptop's io_uring-only default: BOTH arms, 40 minutes, EPOLL primary.**
  Classification reads the first octets rather than peeking and replays them, so the expected cost is
  one extra read per connection: ~1 per request under churn, ~0 under keep-alive. Under epoll that
  read IS a syscall and shows up as one more `recvfrom` against a base of about eight, so the count
  answers the question by construction. **Under io_uring it may cost NOTHING**, because the read
  becomes a submission queue entry that may batch into an enter which was happening anyway -- a null
  there would be an artefact of batching, not a finding about the classifier. The laptop's own
  earlier result is the reason: epoll's syscall count tracks WORK, io_uring's tracks TIME, and "what
  does this feature cost in syscalls" needs an instrument that tracks work. **Paper 2's sentence
  rests on epoll.** io_uring runs anyway because the CONTRAST is a result, not a control: epoll
  showing one syscall per connection while io_uring shows none is a real statement about the two
  models, the same work costing a kernel crossing under readiness and absorbed into an existing
  submission under completion, and it belongs in paper 3 beside the wait-policy decomposition. The
  same increment in both kills a plausible story before anyone writes it down. Report separately,
  never averaged. **CODE READ DONE BEFORE ANY NUMBERS EXIST, and it REFUTES the
  coordinator's batching hypothesis.** `App::detect_protocol` (app.cpp:95) calls
  `net::read_prefix(conn, 1)`, a real one-byte read, which on io_uring goes through
  `UringConnection::async_read` to `submit_sqe`, whose LAST LINE is an unconditional
  `io_uring_submit` (uring_context.cpp:471). Nothing accumulates SQEs across operations and SQPOLL is
  explicitly off (:203), so **every submitted operation is a real kernel entry**. The classification
  read therefore costs one extra enter per connection by the same construction that makes it one extra
  `recvfrom` on epoll, and **a null from io_uring would be an anomaly needing explanation, not a
  batching artefact.** Epoll stays primary for the reason that still holds: a count that tracks work
  is the right instrument for "what does this feature cost".
  **PRE-REGISTERED before the run: epoll +1 `recvfrom` per request under churn; io_uring +1
  `io_uring_enter` per request under churn; both approximately nothing under keep-alive** (one extra
  per CONNECTION against thousands of requests, so invisible there by arithmetic rather than by
  finding). Quantified so the second case cannot be misread: at the 1 ms timeout the churn arm ran at
  about 2.5 enters per request, so +1 takes it to ~3.5, a 40% rise, comfortably resolvable.
- **A LARGER CONSEQUENCE OF THE SAME LINE: this backend never batches anything, and unbatched submits
  may be most of io_uring's remaining kernel entries.** If every operation submits by itself, a
  keep-alive request needs a read and a write, so two submissions, so two enters before any wait --
  and the measured figure at the 1 ms timeout is **2.876 enters per request**, leaving 0.876 for
  waits. If that decomposition holds, roughly two thirds of io_uring's remaining entries come from
  submitting each operation separately rather than from anything intrinsic to the completion model,
  whose principal advantage is batched submission.
  **Two consequences for paper 3, both to be stated rather than left to a reviewer.** (1) The central
  comparison is epoll against an UNBATCHED io_uring, and the paper must say so in those words. (2) It
  reframes the wait-timeout result: 46.6 enters per request down to 2.876 reads as having solved the
  problem, but if two of the remaining 2.876 are unbatched submits then the timeout was the larger of
  TWO costs and the second is untouched and is a property of how the framework uses the interface
  rather than of the interface.
  **CHECK BEFORE ANYONE WRITES IT DOWN, from records already held:** attribute the 2.876 in the
  mechanism runs' syscall-table-by-type into submission enters against wait enters, and see whether
  submissions come out near two per request under keep-alive and near three under churn where accept
  and close add operations. No machine time. If it does not come out near two, the coordinator's
  reading of the submit path is wrong in a way the laptop's code read did not cover.
  **Do NOT change the backend.** A batching change would invalidate six hours of running campaigns and
  is a performance change to the arm under test. Record it; the thesis decides whether an optimisation
  is in scope, and it may well not be, since the framework's job in this work is to be the constant
  rather than to be fast.
  Preflight also caught that `benchmark/certs/bench.crt` did not exist, which would have failed the
  TLS half an hour in. Third time tonight that checking a precondition beat discovering it.
- **THE DECOMPOSITION IS DONE AND THE MECHANISMS GENUINELY DIFFER.** Three arms, 25 rotations,
  interleaved, one session, on `a389023ba`; all 75 runs admissible, worst pacing p99 21 us, so the pool
  never came near binding and none of tonight's coupling touches these numbers.

  | | wait policy | mechanism | residual 95% CI | resolution |
  |---|---|---|---|---|
  | mean, all 25 | 430.24 us (93.0%) | 32.32 us (7.0%) | [+9.56, +68.56] | 6.00% |
  | **median of medians (QUOTED)** | **449.00 us (95.9%)** | **19.00 us (4.1%)** | **[+14.00, +27.00]** | **1.31%** |
  | mean, outlier removed | 446.12 us (96.4%) | 16.44 us (3.6%) | [+6.74, +23.72] | 1.73% |

  **The mechanism residual excludes zero in all three, so this is NOT a null result: the mechanisms
  differ by about 4% of the gap.** The coordinator's reframing from equivalence to decomposition was
  right for a reason neither party anticipated -- there was something there to measure, and an
  equivalence claim would have discarded it.
  **Sentence for the paper:** of the 468 us gap between a readiness backend and a completion backend at
  low load, the wait policy accounts for 449 us and the mechanism for 19 us, residual resolved to 1.3%.
  **The contaminated-run adjudication, done properly and settled AGAINST the laptop's story.** One
  blocking repetition returned 78 us against 454-485 for the other 24. The laptop attributed it to its
  own `procwait` commit; the coordinator refused to let an estimator be chosen to tolerate it and
  required external evidence, since noticing a condition BECAUSE of a value is the shape of post-hoc
  rationalisation. **The git timestamp refutes the story:** the commit lands 48 s AFTER that cell
  finished, inside rotation 10, whose values are entirely normal. So the run STAYS and the anomaly is
  recorded as unexplained. The laptop also flagged, unprompted, that the honest path happens to give
  the TIGHTEST resolution (1.31% against 1.73%) and said so while it ran in its favour -- that sentence
  belongs in the paper, because a reader cannot otherwise tell a forced choice from a chosen one.
  **Judgement so nobody spends machine time: the conclusion is robust to the anomaly** (all three
  versions exclude zero and agree on the wait policy's share within three points), so it needs no
  resolving; 100 rotations to find three more instances would not move the answer.
- **THE UPSTREAM LESSON FROM BOTH OF TONIGHT'S ADJUDICATIONS: experiments that bypass the driver lose
  the harness's witnesses.** The laptop's script drove `loadgen` directly, so those records carry no
  `cpu_mhz_start`/`end` and the second witness did not exist when it was needed. Tonight's two most
  argued-over numbers, this one and the parking row, both came from outside the harness. **Rule:
  exploratory experiments go through the driver too unless there is a stated reason they cannot.** The
  harness is not only a runner; it is what records what else was true while a number was taken.
- **SCHEMA-9 CANDIDATE 2, ruled in: sample the clock DURING the run, not only at its ends.** The drift
  gate compares start against end, so a disturbance in the middle passes both gates -- and the middle
  is most of the run, so that is not bad luck. The driver already samples from a thread inside the
  measured window, so extending it to a cadence and recording the spread is small, and it would have
  turned both of tonight's adjudications into a lookup rather than an argument. **Note the shape the
  two schema-9 candidates share: both record what ACTUALLY HAPPENED during a run rather than what was
  configured or inferred** -- which wire carried it, what the clock did, when the request really left.
  The interface field was the first of them. Worth its own paragraph in the methodology chapter.
- **THE BLOCKING-WAIT EXPERIMENT, and the finding that carries paper 3.** Three binaries from
  `a389023ba` differing only in the wait, 300 000 completions per loaded cell.

  | wait | enter/req | p50 at 100/s, cores idle | p50 awake | parked cores | pkg MHz |
  |---|---|---|---|---|---|
  | 1 us | 46.6 | 33 us | (arm is its own control) | 6.73 | 2661 |
  | 1 ms | 2.88 | 33 us | 52 us | 8.90 | 2107 |
  | blocking | 2.50 | 469 us | 56 us | 10.92 | 1921 |

  **THE UNIFICATION: io_uring made to wait is 469 us; epoll is 495.** One binary, one machine, only
  the wait policy changed. This turns the morning's between-mechanism correlation into a
  within-mechanism controlled comparison: at low load the MECHANISM does not matter and the WAIT
  POLICY is the whole difference. **But it must be measured as an equivalence claim, not asserted:**
  the two numbers came from different binaries and 5.5% apart is on the wrong side of our own 5%
  reportability floor, so as it stands it says "might differ reportably, cannot tell". Laptop is
  **RE-FRAMED as a DECOMPOSITION rather than an equivalence, which is a stronger claim from the same
  hour.** An equivalence asks a reviewer to accept a null at 5.5% against a 5% threshold, and its best
  outcome is "we could not distinguish them". The decomposition uses three points instead: io_uring at
  1 us is 33, io_uring blocking is 469, epoll is 495. The gap being explained is 462 us; changing only
  the wait within io_uring moves 436 of it; the residual 26 us is what the MECHANISM accounts for.
  **The sentence becomes: of the 462 us gap between a readiness backend and a completion backend at
  low load, the wait policy accounts for 436 and the mechanism for 26.** Positive and quantitative,
  the near-threshold number becomes the answer rather than an embarrassment, and it satisfies the
  project's own rule that a decomposition needs one more point than components. **Run: three arms,
  n=25 each at 100/s, all interleaved in rotation so drift cannot land preferentially on one arm, ALL
  IN ONE SESSION (the rule that caught the 4.176 ratio: a decomposition may not subtract points from
  different runs).** About 90 minutes. If the resolution is too wide to say anything about the 26 us
  residual, report the resolution and not a verdict; a paired analysis over adjacent triples is the
  fallback and is not to be written up front.
- **Under load the three wait policies are indistinguishable, and the 36-against-30 must not be read
  as a cost.** Five loaded repetitions per arm at 10 000/s: per-run medians 26.6, 26.4 and 28.0 us,
  run-to-run sd about 2. blocking against 1 ms is +1.60 us (+6.1%) with interval [-1.20, +4.20], so it
  clears the 5% floor on the point estimate and fails the interval: NOT reportable. The single-run
  36 was one draw from a distribution with sd 2. Expected, since at 10 000/s completions arrive far
  faster than any of the three timeouts, so the wait never engages. Caveat: these used
  `--warmup 3 --duration 20` against the single runs' `--warmup 0 --duration 30`, so absolute medians
  are 26-28 rather than 30-36; the two tables are internally valid and must not be read against each
  other.
- **Method rule both sides earned today: a number in a table gets used regardless of the sentence
  beside it.** The laptop printed the parking row it had said it did not stand behind, and the
  coordinator reasoned from it within minutes. A row that is not stood behind does not go in the
  table.
- **The recommendation is a TRADE, not a verdict, and the coordinator had to revise itself.** On the
  six-second parking samples (which the laptop had already said it did not stand behind) blocking
  appeared to buy no extra parking, and the coordinator pushed for a sharp verdict. The thirty-second
  samples show parking is monotonic, 6.73 to 8.90 to 10.92 cores, so blocking buys about two cores'
  worth for its 470 us. Neither six-second row may be quoted; both moved, in both directions. So: 1 ms
  takes 94% of the syscall reduction at no measurable latency cost either way; blocking adds 13% more
  entries saved and two cores of parking and pays 470 us at low load whenever the machine may idle.
  A latency-sensitive service takes the millisecond, a mostly-idle background one might rationally
  take blocking. Give both numbers.
- **OPEN AND PROBABLY A RESULT: the idle cost depends on the idle INTERVAL, and our numbers disagree
  by 12x because of it.** Delivery of posted work from idle costs 46 us against 11 awake, a difference
  of 35, at a 2 ms cadence. A request at 100/s from idle costs 469 against 56, a difference of 413, at
  a 10 ms cadence. Same machine, same arm, same governor. If parking depth grows with idle time then
  neither figure is "the cost of idling" without its interval attached. Laptop asked to sweep the gap
  (1, 2, 5, 10, 20, 50 ms) and plot the cost against it. If it climbs and plateaus, the governor's
  ladder is measured directly, and the mediator's three appearances so far (470 us at 10 ms, 220 us in
  the network baseline, 35 us at 2 ms) become ONE CURVE sampled at three points rather than three
  separate observations.
- **The epoll before-number is settled and the instrument question is closed.** epoll pre-fix measured
  with the CURRENT test: p50 97956 us against kqueue's 98486, so the platforms are identical and there
  is no per-machine cadence difference. Better, one dataset holds both figures: the run's MINIMUM is
  50325/50088/49939, the random-phase first post, which is the old single-post instrument's 50138; the
  median is the phase-locked rest. One defect, two phases, one table. Method note worth keeping: the
  laptop used a build flag removing only the `wake()` call rather than building the old commit,
  because the 21-post test does not exist there and building it would have restored the old
  instrument too. io_uring's 962 is confirmed from the current test, so every before-number now comes
  from one instrument.
- **The blocking arm's delivery distribution is unimodal**, p10 42, p50 46, p75 48, max 96, against
  awake at 11 with the same tightness: the core reaches ONE idle depth consistently and exiting costs
  a consistent ~35 us. Not bimodal, so a single number is right. Qualified by the interval finding
  above: one depth AT A 2 ms CADENCE.
- **The awake control is not universally applicable and the missing cell is a tautology.** The 1 us
  arm never lets its cores sleep, so for it "idle" and "awake" are the same condition by construction;
  adding spinners does not create the condition, it oversubscribes the machine until the generator
  cannot pace, and that cell was refused by the pacing gate at p99 2239 us. The 1 ms and blocking arms
  passed the same gate at 75 and 77 us, which is what proves the refusal was about the spinning server
  rather than the spinners. Also confirmed: `generator_cpu_fraction` near 0.99 in every open-loop run
  including those with no spinners is how an open loop paces, NOT a saturation signal.
- **ALL THREE WAKES ARE FIXED AND MAINLINE IS GREEN EVERYWHERE.** macOS: 179 of 179 including the
  laptop's test 42, 310 self-checks. Delivery of a posted callback on an idle context, one table for
  the whole finding:

  | backend | before | after |
  |---|---|---|
  | epoll | 50138 us | 34 us (worst 74) |
  | io_uring (1 ms wait) | 962 us | 9 us (worst 39) |
  | kqueue | 98486 us | 6 to 12 us (worst 32) |
  | IOCP | correct already | unchanged |

  kqueue's fix is `EVFILT_USER` armed with `EV_ADD | EV_CLEAR` at init and triggered by
  `NOTE_TRIGGER` from `wake()`, which takes no lock because `kevent()` is safe from another thread on
  the same descriptor; null `udata` is what the dispatch loop already used to skip it. `stop()` wakes
  too, so teardown no longer waits out a worker's current `kevent()`. 21 of 21 posts delivered before
  AND after, so the old failure was latency and not loss.
  **CAUTION ON THAT TABLE, from the laptop: epoll's 50138 and kqueue's 98486 were produced by two
  DIFFERENT versions of the test and are not comparable.** epoll's came from the first version, which
  slept 50 ms and posted ONCE: a single post at a random phase into a 100 ms park averages 50 ms.
  kqueue's came from the current version, 21 posts each going out 2 ms after the previous one was
  delivered by a timeout, so every post lands 2 ms into a fresh park and waits out nearly all of it.
  One defect measured two ways, random phase against worst case. The coordinator first explained the
  gap as a per-machine cadence difference, which blamed the platform for a change of instrument, and
  the laptop caught it. **Being settled:** epoll rebuilt at `9d85e2f00` (pre-wake-fix) and run against
  the CURRENT test. About 98 ms means the platforms are identical and one instrument covers the table;
  about 50 ms means there is a real cadence difference worth a sentence. io_uring's 962 is believed to
  come from the current test but is to be confirmed rather than asserted.
  **Rule for this table: every before-number names the instrument that produced it, or they all come
  from one. Prefer the second.**
  Note the 100 ms timeout is deliberately KEPT in all three as a backstop: a missed wake now costs
  100 ms instead of parking a worker forever. It stopped being the mechanism and became the fallback.
- **THE WAKE DEFECT IS IN THREE BACKENDS OF FOUR, and that is the result rather than three bugs.**
  Confirmed by the coordinator on macOS: `kqueue` is epoll's case exactly. `post()`
  (src/net/kqueue/kqueue_context.cpp:245) pushes onto the callback queue and nothing wakes the loop;
  `process_events()` blocks in `kevent()` with a 100 ms timeout (:279) and `process_callbacks()` runs
  only after it returns; `EVFILT_USER` and `NOTE_TRIGGER` appear nowhere in `src/` or `include/`
  (zero hits); and the timer queue is a lambda calling `post()` (:261), so every idle timeout and
  handshake deadline on macOS is late by the same amount. Being fixed on `macos/kqueue-wake` with an
  `EVFILT_USER`/`NOTE_TRIGGER` wake, which like the eventfd needs no lock because `kevent()` is safe
  from another thread on the same descriptor. **IOCP is correct and needs nothing:** its `post()`
  calls `PostQueuedCompletionStatus`, which wakes a thread parked in `GetQueuedCompletionStatus`.
  **The pattern, for paper 3:** the one backend whose wake was right is the one whose platform gives a
  wake primitive that cannot be forgotten; the three that were wrong all needed a separate mechanism
  to be remembered and wired up. And all four passed a suite that never measured delivery latency on
  an idle loop, so the suite could not have caught it because nothing asked.
- **The wake defect is fixed and measured, on `linux/review-fixes` (tip 79e0015d8).** A single-shot
  POLL_ADD stands on the eventfd, `wake()` is a plain 8-byte write taking no lock and no submission
  queue entry, the counter is cleared with an ordinary read in the drain and re-armed under `sq_mutex`
  after it. Median delivery of a posted callback on an idle two-worker context: **epoll 50138 us to
  41-55 us, io_uring 962 us to 9 us**, all 21 posts delivering on every run, which is what proves the
  re-arm rather than merely suggesting it. The failing NOP intermediate was pushed first (758d8864d)
  so that measurement survives in history.
- **The NOP prescription was inert and the reason generalises:** any wake that reaches the ring
  through a submission queue entry must take the lock the parked worker holds, so it can never arrive
  before the timeout it is trying to pre-empt (measured: 962 us median against a 1 ms timeout).
- **Correction to the coordinator's own claim, from the laptop's reading of the manual page and
  liburing 2.15.** The implied submit in `io_uring_wait_cqe_timeout` is on OLD kernels, those WITHOUT
  `IORING_FEAT_EXT_ARG`. With the feature there is no implied submit, so the wait is not a producer,
  and holding `sq_mutex` across it protects nothing while blocking every other submitter. The
  coordinator had this backwards and told the laptop to verify rather than trust it, which is the only
  reason it did not become a latent bug. **`linux/uring-wait-lock-ext-arg` therefore joins the merge
  path:** feature-gated, old behaviour intact where the flag is absent, 186/186 on three runs. It also
  makes an untimed wait safe, since with the lock held across a blocking wait a foreign submitter
  blocks on the mutex and cannot submit the thing that would end the wait, which is a deadlock on an
  idle ring rather than a slow path.
- **The memlock failure has TWO mechanisms and needs both remedies; this revises the earlier ruling.**
  Asynchronous release, 7-14 ms after a process exits, makes consecutive tests overlap in accounting
  though never in time, and only the bounded retry helps (retry without lock passes 5 of 5 at -j1).
  Simultaneous demand, where two retrying tests each hold what they have taken while waiting for the
  rest, starves both, and only `RESOURCE_LOCK` helps (retry without lock failed 3 of 5 at -j16). Both
  together: 186 of 186 over five runs. It looked settled twice because the suite then had one fewer
  io_uring context. Neither remedy may be deleted without reproducing the other's failure.
- **Order corrected for the laptop.** The remaining review findings (HIGH 2, HIGH 3, seven mediums,
  three lows) come BEFORE the blocking-wait experiment, because the experiment is paper 3's third data
  point and therefore data, and the standing rule is that data traces to a mainline commit rather than
  to a branch tip. So: finish the findings, push, coordinator merges the stack, then the blocking-wait
  experiment AND the cross-arm re-runs both run on the merged HEAD.
- **`coord/inbox` is on the PUBLIC repository and carries the laptop's hostname.** NOT an
  oversight, and the coordinator first framed it wrongly to Alex as one: when the laptop first wrote
  to that branch it spelled out that the repository is public and that the reports would expose the
  hostname, addresses, MAC addresses, SSID and the machine's hardening profile, and asked Alex
  directly; he chose "Full text, unredacted". He is therefore revisiting his own informed decision,
  not repairing an accident. The tightened rule stands regardless. Extent:** in ten filenames and twelve body lines (`gh repo view` confirms
  `Alex-Tsvetanov/WebFrame` is `isPrivate: false`). No IP or MAC has leaked yet, but the laptop was
  about to write link facts containing two addresses and three MACs into an inbox file. It has been
  told: nothing identifying a machine goes on that repository, ever, and network identity comes to the
  coordinator by message only. Machine identity belongs in the private paper repositories, which is
  what `env.json` and `measurements/` are for. The existing hostname history needs a rewrite and a
  force push, which is Alex's call and has been put to him; a backup ref `backup/coord-inbox-pre-redaction`
  holds the current tip locally either way.
- **Link findings so far, from the laptop.** Topology settled: the desktop resolves by ARP as a
  direct neighbour on the same segment, so same switch and no layer-3 hop. **The desktop does not
  answer ICMP** (Windows Firewall default on Private and Public profiles), so the ping baseline as
  specified cannot be taken. **RULED: no firewall rule is to be changed on either machine.** The
  baseline becomes a TCP handshake round trip to a listening socket bound to the physical interface,
  which is better anyway: a SYN and SYN-ACK exercise the stack path a connection uses, which ICMP does
  not, and it answers the kernel-versus-userspace objection. It waits for the desktop's campaign to
  end, because it needs that listening socket, and it is the two-host arm's own first step.
  **The C-state effect survives into network measurement and is large.** 300 pings to the router
  (same wire, same switch, one hop): median 722 microseconds plain against 502 with the laptop's own
  cores held awake, on a measurement where the remote end and the wire are identical and only the
  receiving core's idle state changed. The minimum barely moved (385 to 339), so the floor is the wire
  and the wake-up sits on top of it. Same order as the morning's 495 against 74. **For paper 3:** a
  two-host latency comparison that does not control for the receiving core's idle state measures the
  idle governor as well as the network, and the error is hundreds of microseconds. Caveat kept: a
  router answers ICMP on a low-priority path, so 300 microseconds of spread is an upper bound on the
  segment, not an estimate of it.
- **A STATISTICAL CORRECTION the coordinator made to the laptop, which the papers depend on.** The
  laptop concluded that a per-packet spread near 300 microseconds means the path cannot resolve the
  60 to 85 microsecond demux difference. That does not follow. We do not compare packets; we compare
  per-run medians, and the standard error of a median falls as the square root of the sample count, so
  at about 100 000 requests a run a 300 microsecond per-packet spread gives a per-run median stable to
  roughly one microsecond IF the noise is independent. What enters the bootstrap is the RUN-TO-RUN
  spread of those medians. The quantity that settles it is therefore the one the harness already
  reports, the half-width of the difference interval as a share of the classification-off median
  (2.47 to 5.63 percent on loopback), measured over the wire. Two caveats that could still vindicate
  the laptop, and both are empirical: network noise is bursty and autocorrelated, and if the
  correlation time approaches a run's length the effective sample count collapses toward the number of
  runs; and any variation correlated with the ARM rather than added independently biases the
  comparison at any sample size. Do not write "loopback was the right medium" until the run-to-run
  number exists.
- **ORDER RULED: the `getsockname` interface recording lands BEFORE the blocking-wait experiment and
  before the cross-arm re-runs.** Not urgency but schema stability: it changes the environment record,
  and records from before and after cannot be pooled, so everything measured from the merged mainline
  onward must share one schema.
- **NEW CAPABILITY (Alex, 18:45): the two machines share an Ethernet segment, so a genuine two-host
  benchmark is possible** -- server on one machine, generator on the other, across a real interface
  with a driver, interrupts and queueing. This is the first arrangement that can answer the loopback
  limitation both papers currently state as a caveat, and it is stronger than the WSL arm, whose
  "network" is a virtual switch inside the Windows host.
  **The number that decides the design is the path's own jitter, not its throughput.** Paper 2's
  demultiplexing difference is 60 to 85 microseconds. If the round-trip standard deviation over this
  link is in the hundreds of microseconds, no number of repetitions recovers that difference, and the
  honest conclusion is that loopback was the RIGHT medium for that claim while a real path is the
  right medium for others. If the jitter is tens of microseconds, the caveat is replaced by a
  measurement. Both machines have been asked for link facts (interface, negotiated speed, route,
  same-switch or routed) and a 300-ping baseline reporting min, median, mean, max and standard
  deviation. The laptop reports now (it is doing code work, not measurement); the desktop reports
  after routing, dispatch and the sweep, because its host must stay quiet.
  Note also that the arrangement recorded until now had the laptop on WiFi and the desktop on
  Ethernet; the laptop has been asked to confirm which it is on, since a WiFi end would make the
  jitter question answer itself.
  **Declared in advance, so it is not discovered as a wall of refusals:** at 1 gigabit and roughly
  1100 bytes on the wire per request and response, the link saturates near 70 000 requests per second,
  which is the top of the existing ladder. A real network arm therefore has a ceiling set by the LINK
  rather than by the server, and that ceiling belongs in the design. Discovering it instead is the
  mistake the 800-per-second establishment rate already made once.
  **Which direction serves which paper:** a Windows server with a Linux generator extends paper 2's
  existing IOCP dataset with a real path; a Linux server with a Windows generator gives paper 3 its
  epoll and io_uring arms over one. Neither licenses a cross-platform comparison, which stays
  shape-only, because the two ends are different hardware.
- **MAINLINE IS `a389023ba`, pushed. Schema 8. Everything measured from now on comes from it.**
  Contents: the eventfd wake for io_uring and epoll, the EXT_ARG lock change with its pre-5.11
  caveat, all thirteen review findings, and the per-run interface recording. macOS: 310 self-checks,
  241 targets, and **one known failing test, number 42, which is the kqueue wake defect**; the fix is
  on `macos/kqueue-wake` and merges next. The coordinator reversed its own "no push until it passes"
  on the grounds that the failure is macOS-only, cannot touch any Linux number, and holding two
  machines idle for a platform neither measures on is the wrong trade. **If test 42 ever fails on
  LINUX that is a regression of the eventfd wake and must be reported at once.**
  The interface field is per-run on `RunRecord`, not per-campaign, at the laptop's insistence and it
  was right: the failure guarded against is the medium changing PARTWAY THROUGH a campaign, which a
  campaign-level field cannot express, and per-run also refuses only the affected runs instead of
  invalidating a campaign retroactively. Read from `getsockname` after a connection is established,
  matched to an interface, and the address then DISCARDED; records name, speed, duplex and MTU, never
  an address or a MAC. Verified end to end: a loopback run reports `lo`, a namespaced run reports
  `veth-gen` from inside the namespace rather than the host's Ethernet, so the reading follows the
  socket rather than the machine. `--expect-interface` refuses a run over anything else, and refuses
  an unknown interface too when an expectation was stated.
  Consequence: the desktop's campaign is at its own commit and produces schema 7, so its routing and
  sweep results are a separate population from everything after this merge, and the refusal to pool
  them is correct rather than an inconvenience.
- **The laptop withdrew the resolution conclusion** after the coordinator's correction, and restated
  the open questions itself: autocorrelated network noise collapsing the effective sample count toward
  the number of runs, and any variation correlated with the arm. Neither is settled by a ping
  statistic. **Nobody writes a sentence about which medium was right for the demux claim until the
  run-to-run spread of per-run medians exists for the wire.**
- **PAPER 2 HAS ITS WORD STEP: `6d1ebfc94` on `draft/v1`, pushed.** `build_docx.py`,
  `template.docx`, `Tsvetanov-socket-demux.docx`, plus a CMake `docx` target and a README section.
  **It is a real converter, which the template it was modelled on is not.** The finished sibling's
  script reads only `template.docx`: every sentence, table cell, citation number and reference entry
  is a Python string literal typed in by hand, so "reuse the mechanism" there would have meant
  transcribing this paper's prose into Python. Ours reads `main.tex`, `sections/*.tex`, the CSVs the
  tables typeset, `main.aux` for cite numbers and `\ref` values, and `main.bbl`, all at run time.
  Contents: 10 first-level and 16 second-level headings, 5 tables, 18 references, 1 equation, and
  **247 distinct numbers each checked back to a source**; the 190 data cells of tables II to V were
  re-derived independently from the CSVs by reimplementing the pgfplotstable formatting, with zero
  mismatches, and corroborated against `pdftotext main.pdf`. Opened independently with `textutil`
  rather than by python-docx re-reading its own output. Scanned part by part for machine identity:
  clean.
  Documented deviations, written into the README rather than hidden: the `\thanks` footnote becomes
  affiliation lines because python-docx writes no footnotes; `\paragraph`'s automatic a)/b) letters
  are lost; tables sit where the source puts them rather than floating; `\href` keeps its text and
  drops the target.
  Notable fix during review: the affiliation block had been hand-transcribed with a one-way `assert`,
  so new `\thanks` content vanished silently and `python -O` removed the guard entirely; it is now
  fully derived by regular expression with `sys.exit` on a shape change. Run it with
  `cmake --build build --target docx` after the `paper` target, or `python3 build_docx.py`.
- **In flight at 18:20 EEST.** Two local workflows: `wja27ah8t` writes and verifies the Word
  (DOCX) step for paper 2, modelled on Compile-time-Protobuf's but driven from this paper's own
  sources (the sibling's script carries the other paper's text and must not be copied);
  `wkuxtjk3o` audits every thesis chapter except VI against today's findings and the code at HEAD,
  five chapter groups, each audit adversarially verified before anything is applied. The laptop is
  building the eventfd wake for io_uring and epoll on `linux/review-fixes`. The desktop is running
  the routing designs at b4a01e8c7 into `benchmark/results/2026-09-02-desktop-routing/`, then
  dispatch, then the h1 sweep, about four hours in total.
- **For Alex, when he is next at the machines:** the desktop cannot push, so
  `measure/desktop-2026-09-02-net` (9b3ba079a, seven files, the network campaign) needs pushing to
  the private `paper-socket-demux` repository, as `measure/desktop-2026-09-02` was this morning.
  The routing results will need the same treatment into `paper-dfa-routing` when they finish.
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
    written fresh, the template's carries the other paper's text; an agent is on it), venue
    running head, Alex's read. Abstract is 254 words, fine.
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
  macOS: the tip 9d85e2f00 builds and passes 178/178 here. Methodology paragraph on the shared
  socket policy added to chapter V ("Еднакви опции на сокетите за всяко рамо").
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

## Who is the coordinator right now

> **What may not appear in this repository, which is PUBLIC** (`Alex-Tsvetanov/WebFrame`,
> `isPrivate: false`): hostnames, IP or MAC addresses, network names, router details, the shape of
> the local network, login state, or a machine's security posture (that last one had leaked here as
> a note about how one machine's sudo is configured, and is now gone). **Hardware models stay.** A
> measurement paper that does not name its processor is unreproducible, so the CPU, core count and
> operating system are apparatus, not identity, and they belong in the record. The line is between
> what a reader needs to repeat the experiment and what a reader would need to find or enter the
> machine. Machine identity goes to the private paper repositories, which is what `env.json` and
> the `measurements/` directories are for.


**`phd-webframe-93`, running on `claude-opus-5`.** This line is the tie-breaker: if two sessions
ever claim coordination, the machines follow whichever name this file names on `origin/phase0-foundation`,
not whichever session messaged them last. A session that takes over must change this line and push
before giving any ruling, and must tell both machines to re-read it.

What happened on 2 September, so it is not repeated. The successor task fired around 16:30 EEST
on a stale heartbeat, concluded this session had died on its usage limit, announced itself to the
Windows machine as `phd-webframe-04`, and asked for reports. The conclusion was wrong: this session
was rate-limited on Fable 5.1, not dead, and Alex moved it to Opus, so it kept its whole context.
The Windows machine noticed the contradiction itself, refused to act on rulings from either session
until it was settled, and was right to. `phd-webframe-04` has been told to stand down.

**Standing rule for the machines: never act on a design ruling while two sessions are claiming
coordination. Report to both, act for neither, and say so, exactly as the Windows machine did.**

**The ten-minute clause is withdrawn.** Every brief used to tell the machines to send a question
with the default they would take and act on it after ten minutes. The Windows machine refused it
three times today and is right: a coordinator's silence is not Alex's approval, and it caught a
real error that way (a relayed go for a window described as three hours carried a sequence of about
eight, and it asked Alex directly instead of taking the relay as consent). The clause is removed
from `~/.claude/scheduled-tasks/coordinator-successor/SKILL.md`, which would otherwise have re-sent
it on every future run. The replacement: ask, wait, and if the question blocks one step, do other
authorised work meanwhile and keep the question open. Non-blocking must never mean self-authorising.

## Succession, automated (set up 02:30 EEST)

> **RETIRED 17:55 EEST on 2 September.** The handoff this mechanism existed for happened
> inside the live session instead: Fable 5.1 hit its session limit (a subagent took a 429 at
> 17:47) and Alex switched this same session to `claude-opus-5`, so the coordinator never died
> and its context was never lost. The heartbeat cron (`f9755231`) was then cancelled, which
> would have left the `coordinator-alive` file ageing while the coordinator was still working;
> at 75 minutes the successor would have woken a **second** coordinator alongside this one, and
> two of them messaging the laptop and the desktop and pushing to `phase0-foundation` is worse
> than no safety net. The `coordinator-successor` task is therefore **paused, not deleted** --
> its SKILL.md is untouched and one `update_scheduled_task(enabled: true)` brings it back. Do
> that only together with a heartbeat writer; the two are one mechanism. The description below
> is kept because it documents how to restart it.


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
